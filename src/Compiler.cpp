// RegExJit - A fast JIT compiled regex engine
// Copyright (C) 2026 Zeeshan Qazi
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "regexjit/Compiler.h"
#include "regexjit/Error.h"

#include <iostream>

using namespace asmjit;
using namespace regexjit;
using namespace regexjit::ast;

namespace {

int count_groups(const Node* node) {
    if (!node) return 0;
    int max_group = 0;
    
    switch (node->type()) {
        case regexjit::ast::NodeType::Group: {
            auto g = static_cast<const Group*>(node);
            max_group = std::max(max_group, g->index);
            max_group = std::max(max_group, count_groups(g->child.get()));
            break;
        }
        case regexjit::ast::NodeType::Quantifier: {
            auto q = static_cast<const Quantifier*>(node);
            max_group = std::max(max_group, count_groups(q->child.get()));
            break;
        }
        case regexjit::ast::NodeType::Alternative: {
            auto a = static_cast<const Alternative*>(node);
            for (auto& c : a->choices) {
                max_group = std::max(max_group, count_groups(c.get()));
            }
            break;
        }
        case regexjit::ast::NodeType::Sequence: {
            auto s = static_cast<const Sequence*>(node);
            for (auto& e : s->elements) {
                max_group = std::max(max_group, count_groups(e.get()));
            }
            break;
        }
        default:
            break;
    }
    return max_group;
}

#if ASMJIT_ARCH_X86
class X86RegexCompiler {
    x86::Compiler& cc;
    
public:
    X86RegexCompiler(x86::Compiler& cc) : cc(cc) {}
    
    // We will implement a simplified JIT compiling fallback for basic nodes.
    // For a fully featured PCRE engine, a backtracking stack is required.
    // Here we implement a simplified matching strategy.
    
    void compile(const NodePtr& ast, int max_groups) {
        // Function signature: bool match_func(const char* str, const char* end, const char** captures)
        FuncNode* func = cc.add_func(FuncSignature::build<bool, const char*, const char*, const char**>(CallConvId::kCDecl));
        
        x86::Gp str = cc.new_gp_ptr("str");
        x86::Gp end = cc.new_gp_ptr("end");
        x86::Gp captures = cc.new_gp_ptr("captures");
        
        func->set_arg(0, str);
        func->set_arg(1, end);
        func->set_arg(2, captures);
        
        // Initialize captures to 0
        if (max_groups >= 0) {
            x86::Gp zero = cc.new_gp_ptr("zero");
            cc.xor_(zero, zero);
            for (int i = 0; i <= max_groups * 2 + 1; ++i) {
                cc.mov(x86::ptr(captures, i * 8), zero);
            }
            
            // captures[0] = str
            cc.mov(x86::ptr(captures, 0), str);
        }
        
        Label L_Success = cc.new_label();
        Label L_Fail = cc.new_label();
        
        generate_node(ast.get(), str, end, captures, L_Success, L_Fail);
        
        cc.bind(L_Success);
        if (max_groups >= 0) {
            // captures[1] = str (end of match)
            cc.mov(x86::ptr(captures, 8), str);
        }
        x86::Gp ret1 = cc.new_gp32("ret1");
        cc.mov(ret1, 1);
        cc.ret(ret1);
        
        cc.bind(L_Fail);
        x86::Gp ret0 = cc.new_gp32("ret0");
        cc.mov(ret0, 0);
        cc.ret(ret0);
        
        cc.end_func();
    }
    
    // Generates code that matches `node` starting at `str`.
    // If successful, advances `str` and jumps to / falls through to success path.
    // If it fails, jumps to `L_Fail`.
    void generate_node(const Node* node, x86::Gp& str, x86::Gp& end, x86::Gp& captures, Label L_Next, Label L_Fail) {
        if (!node) {
            cc.jmp(L_Next);
            return;
        }
        
        switch (node->type()) {
            case regexjit::ast::NodeType::Literal: {
                auto lit = static_cast<const Literal*>(node);
                for (char c : lit->text) {
                    cc.cmp(str, end);
                    cc.je(L_Fail);
                    
                    x86::Gp ch = cc.new_gp32("ch");
                    cc.movzx(ch, x86::byte_ptr(str));
                    cc.cmp(ch, c);
                    cc.jne(L_Fail);
                    
                    cc.add(str, 1);
                }
                cc.jmp(L_Next);
                break;
            }
            case regexjit::ast::NodeType::Any: {
                cc.cmp(str, end);
                cc.je(L_Fail);
                
                x86::Gp ch = cc.new_gp32("ch");
                cc.movzx(ch, x86::byte_ptr(str));
                cc.cmp(ch, '\n');
                cc.je(L_Fail); // '.' usually does not match newline
                
                cc.add(str, 1);
                cc.jmp(L_Next);
                break;
            }
            case regexjit::ast::NodeType::CharClass: {
                auto cc_node = static_cast<const CharClass*>(node);
                cc.cmp(str, end);
                cc.je(L_Fail);
                
                x86::Gp ch = cc.new_gp32("ch");
                cc.movzx(ch, x86::byte_ptr(str));
                
                Label L_MatchChar = cc.new_label();
                Label L_EndClass = cc.new_label();
                
                for (char c : cc_node->single_chars) {
                    cc.cmp(ch, c);
                    cc.je(L_MatchChar);
                }
                for (auto& r : cc_node->ranges) {
                    Label L_NextRange = cc.new_label();
                    cc.cmp(ch, r.start);
                    cc.jl(L_NextRange);
                    cc.cmp(ch, r.end);
                    cc.jg(L_NextRange);
                    cc.jmp(L_MatchChar);
                    cc.bind(L_NextRange);
                }
                
                if (cc_node->negated) {
                    // It didn't match any, which means it matches the negated class
                    cc.add(str, 1);
                    cc.jmp(L_Next);
                } else {
                    cc.jmp(L_Fail);
                }
                
                cc.bind(L_MatchChar);
                if (cc_node->negated) {
                    cc.jmp(L_Fail);
                } else {
                    cc.add(str, 1);
                    cc.jmp(L_Next);
                }
                break;
            }
            case regexjit::ast::NodeType::Sequence: {
                auto seq = static_cast<const Sequence*>(node);
                if (seq->elements.empty()) {
                    cc.jmp(L_Next);
                    return;
                }
                
                // Chain them
                std::vector<Label> labels;
                for (size_t i = 0; i < seq->elements.size() - 1; ++i) {
                    labels.push_back(cc.new_label());
                }
                labels.push_back(L_Next);
                
                for (size_t i = 0; i < seq->elements.size(); ++i) {
                    generate_node(seq->elements[i].get(), str, end, captures, labels[i], L_Fail);
                    if (i < seq->elements.size() - 1) {
                        cc.bind(labels[i]);
                    }
                }
                break;
            }
            case regexjit::ast::NodeType::Alternative: {
                auto alt = static_cast<const Alternative*>(node);
                if (alt->choices.empty()) {
                    cc.jmp(L_Fail);
                    return;
                }
                
                // Backtracking point!
                // We need to save `str` before trying each alternative.
                x86::Gp saved_str = cc.new_gp_ptr("saved_str");
                
                for (size_t i = 0; i < alt->choices.size(); ++i) {
                    cc.mov(saved_str, str);
                    Label L_NextAlt = cc.new_label();
                    Label L_AltSuccess = cc.new_label();
                    
                    x86::Gp cur_str = cc.new_gp_ptr("cur_str");
                    cc.mov(cur_str, saved_str);
                    
                    generate_node(alt->choices[i].get(), cur_str, end, captures, L_AltSuccess, L_NextAlt);
                    
                    cc.bind(L_AltSuccess);
                    cc.mov(str, cur_str);
                    cc.jmp(L_Next);
                    
                    cc.bind(L_NextAlt);
                }
                cc.jmp(L_Fail);
                break;
            }
            case regexjit::ast::NodeType::Quantifier: {
                // Highly simplified greedy matcher for * and + for single characters
                auto quant = static_cast<const Quantifier*>(node);
                
                // Save current string pointer
                x86::Gp saved_str = cc.new_gp_ptr("saved_str");
                cc.mov(saved_str, str);
                
                x86::Gp loop_count = cc.new_gp32("loop_count");
                cc.xor_(loop_count, loop_count);
                
                Label L_LoopStart = cc.new_label();
                Label L_LoopExit = cc.new_label();
                Label L_LoopFail = cc.new_label();
                
                cc.bind(L_LoopStart);
                
                // Check max match
                if (quant->max_match >= 0) {
                    cc.cmp(loop_count, quant->max_match);
                    cc.jge(L_LoopExit);
                }
                
                Label L_LoopSuccess = cc.new_label();
                generate_node(quant->child.get(), str, end, captures, L_LoopSuccess, L_LoopExit);
                
                cc.bind(L_LoopSuccess);
                cc.add(loop_count, 1);
                cc.jmp(L_LoopStart);
                
                cc.bind(L_LoopExit);
                
                // Check min match
                cc.cmp(loop_count, quant->min_match);
                cc.jl(L_Fail);
                
                // Real backtracking requires saving all positions. 
                // For a proper PCRE engine, we'd need a backtracking stack.
                // This is a naive greedy match without backtracking.
                cc.jmp(L_Next);
                break;
            }
            case regexjit::ast::NodeType::Group: {
                auto group = static_cast<const Group*>(node);
                if (group->index > 0) {
                    // captures[group->index * 2] = str;
                    cc.mov(x86::ptr(captures, group->index * 16), str); // 16 = 2 * 8 bytes (start pointer)
                }
                
                Label L_GroupSuccess = cc.new_label();
                generate_node(group->child.get(), str, end, captures, L_GroupSuccess, L_Fail);
                
                cc.bind(L_GroupSuccess);
                if (group->index > 0) {
                    // captures[group->index * 2 + 1] = str;
                    cc.mov(x86::ptr(captures, group->index * 16 + 8), str); // end pointer
                }
                cc.jmp(L_Next);
                break;
            }
            case regexjit::ast::NodeType::Anchor: {
                auto anchor = static_cast<const Anchor*>(node);
                if (anchor->anchor_type == AnchorType::EndOfLine) {
                    cc.cmp(str, end);
                    cc.jne(L_Fail);
                    cc.jmp(L_Next);
                } else {
                    // Start of line is implicit at the beginning for match, but we can't easily check it here without passing start_str
                    // Simplification: assume always true if it's start
                    cc.jmp(L_Next);
                }
                break;
            }
            case regexjit::ast::NodeType::BackReference: {
                auto bref = static_cast<const BackReference*>(node);
                
                x86::Gp ref_start = cc.new_gp_ptr("ref_start");
                x86::Gp ref_end = cc.new_gp_ptr("ref_end");
                
                cc.mov(ref_start, x86::ptr(captures, bref->index * 16));
                cc.mov(ref_end, x86::ptr(captures, bref->index * 16 + 8));
                
                // if ref_start == 0 (not captured), fail
                cc.test(ref_start, ref_start);
                cc.jz(L_Fail);
                
                x86::Gp len = cc.new_gp_ptr("len");
                cc.mov(len, ref_end);
                cc.sub(len, ref_start);
                
                // check if str + len <= end
                x86::Gp str_plus_len = cc.new_gp_ptr("str_plus_len");
                cc.mov(str_plus_len, str);
                cc.add(str_plus_len, len);
                cc.cmp(str_plus_len, end);
                cc.ja(L_Fail);
                
                // Loop to compare strings
                Label L_CmpStart = cc.new_label();
                Label L_CmpEnd = cc.new_label();
                
                x86::Gp i = cc.new_gp_ptr("i");
                cc.xor_(i, i);
                
                cc.bind(L_CmpStart);
                cc.cmp(i, len);
                cc.je(L_CmpEnd);
                
                x86::Gp ch1 = cc.new_gp32("ch1");
                x86::Gp ch2 = cc.new_gp32("ch2");
                
                cc.movzx(ch1, x86::byte_ptr(str, i));
                cc.movzx(ch2, x86::byte_ptr(ref_start, i));
                
                cc.cmp(ch1, ch2);
                cc.jne(L_Fail);
                
                cc.add(i, 1);
                cc.jmp(L_CmpStart);
                
                cc.bind(L_CmpEnd);
                cc.add(str, len);
                cc.jmp(L_Next);
                break;
            }
        }
    }
};
#endif

#if ASMJIT_ARCH_ARM == 64
class AArch64RegexCompiler {
    a64::Compiler& cc;
    
public:
    AArch64RegexCompiler(a64::Compiler& cc) : cc(cc) {}
    
    void compile(const NodePtr& ast, int max_groups) {
        // Function signature: bool match_func(const char* str, const char* end, const char** captures)
        FuncNode* func = cc.add_func(FuncSignature::build<bool, const char*, const char*, const char**>(CallConvId::kCDecl));
        
        a64::Gp str = cc.new_gp_ptr("str");
        a64::Gp end = cc.new_gp_ptr("end");
        a64::Gp captures = cc.new_gp_ptr("captures");
        
        func->set_arg(0, str);
        func->set_arg(1, end);
        func->set_arg(2, captures);
        
        // Initialize captures to 0
        if (max_groups >= 0) {
            a64::Gp zero = cc.new_gp_ptr("zero");
            cc.mov(zero, 0);
            for (int i = 0; i <= max_groups * 2 + 1; ++i) {
                cc.str(zero, a64::ptr(captures, i * 8));
            }
            cc.str(str, a64::ptr(captures, 0));
        }
        
        Label L_Success = cc.new_label();
        Label L_Fail = cc.new_label();
        
        generate_node(ast.get(), str, end, captures, L_Success, L_Fail);
        
        cc.bind(L_Success);
        if (max_groups >= 0) {
            cc.str(str, a64::ptr(captures, 8)); // captures[1] = str (end of match)
        }
        a64::Gp ret1 = cc.new_gp32("ret1");
        cc.mov(ret1, 1);
        cc.ret(ret1);
        
        cc.bind(L_Fail);
        a64::Gp ret0 = cc.new_gp32("ret0");
        cc.mov(ret0, 0);
        cc.ret(ret0);
        
        cc.end_func();
    }

    void generate_node(const Node* node, a64::Gp& str, a64::Gp& end, a64::Gp& captures, Label L_Next, Label L_Fail) {
        if (!node) {
            cc.b(L_Next);
            return;
        }

        switch (node->type()) {
            case regexjit::ast::NodeType::Literal: {
                auto lit = static_cast<const Literal*>(node);
                for (char c : lit->text) {
                    cc.cmp(str, end);
                    cc.b_eq(L_Fail);
                    
                    a64::Gp ch = cc.new_gp32("ch");
                    cc.ldrb(ch, a64::ptr(str));
                    
                    a64::Gp exp_ch = cc.new_gp32("exp_ch");
                    cc.mov(exp_ch, c);
                    cc.cmp(ch, exp_ch);
                    cc.b_ne(L_Fail);
                    
                    cc.add(str, str, 1);
                }
                cc.b(L_Next);
                break;
            }
            case regexjit::ast::NodeType::Any: {
                cc.cmp(str, end);
                cc.b_eq(L_Fail);
                
                a64::Gp ch = cc.new_gp32("ch");
                cc.ldrb(ch, a64::ptr(str));
                
                a64::Gp exp_nl = cc.new_gp32("exp_nl");
                cc.mov(exp_nl, '\n');
                cc.cmp(ch, exp_nl);
                cc.b_eq(L_Fail);
                
                cc.add(str, str, 1);
                cc.b(L_Next);
                break;
            }
            case regexjit::ast::NodeType::Sequence: {
                auto seq = static_cast<const Sequence*>(node);
                if (seq->elements.empty()) {
                    cc.b(L_Next);
                    return;
                }
                
                std::vector<Label> labels;
                for (size_t i = 0; i < seq->elements.size() - 1; ++i) {
                    labels.push_back(cc.new_label());
                }
                labels.push_back(L_Next);
                
                for (size_t i = 0; i < seq->elements.size(); ++i) {
                    generate_node(seq->elements[i].get(), str, end, captures, labels[i], L_Fail);
                    if (i < seq->elements.size() - 1) {
                        cc.bind(labels[i]);
                    }
                }
                break;
            }
            case regexjit::ast::NodeType::Anchor: {
                auto anchor = static_cast<const Anchor*>(node);
                if (anchor->anchor_type == AnchorType::EndOfLine) {
                    cc.cmp(str, end);
                    cc.b_ne(L_Fail);
                    cc.b(L_Next);
                } else {
                    cc.b(L_Next);
                }
                break;
            }
            case regexjit::ast::NodeType::CharClass: {
                auto cc_node = static_cast<const CharClass*>(node);
                cc.cmp(str, end);
                cc.b_eq(L_Fail);
                
                a64::Gp ch = cc.new_gp32("ch");
                cc.ldrb(ch, a64::ptr(str));
                
                Label L_MatchChar = cc.new_label();
                Label L_EndClass = cc.new_label();
                
                for (char c : cc_node->single_chars) {
                    a64::Gp exp_c = cc.new_gp32("exp_c");
                    cc.mov(exp_c, c);
                    cc.cmp(ch, exp_c);
                    cc.b_eq(L_MatchChar);
                }
                for (auto& r : cc_node->ranges) {
                    Label L_NextRange = cc.new_label();
                    a64::Gp exp_start = cc.new_gp32("exp_start");
                    cc.mov(exp_start, r.start);
                    cc.cmp(ch, exp_start);
                    cc.b_lt(L_NextRange);
                    
                    a64::Gp exp_end = cc.new_gp32("exp_end");
                    cc.mov(exp_end, r.end);
                    cc.cmp(ch, exp_end);
                    cc.b_gt(L_NextRange);
                    
                    cc.b(L_MatchChar);
                    cc.bind(L_NextRange);
                }
                
                if (cc_node->negated) {
                    cc.add(str, str, 1);
                    cc.b(L_Next);
                } else {
                    cc.b(L_Fail);
                }
                
                cc.bind(L_MatchChar);
                if (cc_node->negated) {
                    cc.b(L_Fail);
                } else {
                    cc.add(str, str, 1);
                    cc.b(L_Next);
                }
                break;
            }
            case regexjit::ast::NodeType::Alternative: {
                auto alt = static_cast<const Alternative*>(node);
                if (alt->choices.empty()) {
                    cc.b(L_Fail);
                    return;
                }
                
                a64::Gp saved_str = cc.new_gp_ptr("saved_str");
                
                for (size_t i = 0; i < alt->choices.size(); ++i) {
                    cc.mov(saved_str, str);
                    Label L_NextAlt = cc.new_label();
                    Label L_AltSuccess = cc.new_label();
                    
                    a64::Gp cur_str = cc.new_gp_ptr("cur_str");
                    cc.mov(cur_str, saved_str);
                    
                    generate_node(alt->choices[i].get(), cur_str, end, captures, L_AltSuccess, L_NextAlt);
                    
                    cc.bind(L_AltSuccess);
                    cc.mov(str, cur_str);
                    cc.b(L_Next);
                    
                    cc.bind(L_NextAlt);
                }
                cc.b(L_Fail);
                break;
            }
            case regexjit::ast::NodeType::Group: {
                auto group = static_cast<const Group*>(node);
                if (group->index > 0) {
                    cc.str(str, a64::ptr(captures, group->index * 16));
                }
                
                Label L_GroupSuccess = cc.new_label();
                generate_node(group->child.get(), str, end, captures, L_GroupSuccess, L_Fail);
                
                cc.bind(L_GroupSuccess);
                if (group->index > 0) {
                    cc.str(str, a64::ptr(captures, group->index * 16 + 8));
                }
                cc.b(L_Next);
                break;
            }
            case regexjit::ast::NodeType::BackReference: {
                auto bref = static_cast<const BackReference*>(node);
                
                a64::Gp ref_start = cc.new_gp_ptr("ref_start");
                a64::Gp ref_end = cc.new_gp_ptr("ref_end");
                
                cc.ldr(ref_start, a64::ptr(captures, bref->index * 16));
                cc.ldr(ref_end, a64::ptr(captures, bref->index * 16 + 8));
                
                cc.cmp(ref_start, 0);
                cc.b_eq(L_Fail);
                
                a64::Gp len = cc.new_gp_ptr("len");
                cc.sub(len, ref_end, ref_start);
                
                a64::Gp str_plus_len = cc.new_gp_ptr("str_plus_len");
                cc.add(str_plus_len, str, len);
                cc.cmp(str_plus_len, end);
                cc.b_hi(L_Fail);
                
                Label L_CmpStart = cc.new_label();
                Label L_CmpEnd = cc.new_label();
                
                a64::Gp i = cc.new_gp_ptr("i");
                cc.mov(i, 0);
                
                cc.bind(L_CmpStart);
                cc.cmp(i, len);
                cc.b_eq(L_CmpEnd);
                
                a64::Gp ch1 = cc.new_gp32("ch1");
                a64::Gp ch2 = cc.new_gp32("ch2");
                
                cc.ldrb(ch1, a64::ptr(str, i));
                cc.ldrb(ch2, a64::ptr(ref_start, i));
                
                cc.cmp(ch1, ch2);
                cc.b_ne(L_Fail);
                
                cc.add(i, i, 1);
                cc.b(L_CmpStart);
                
                cc.bind(L_CmpEnd);
                cc.add(str, str, len);
                cc.b(L_Next);
                break;
            }
            case regexjit::ast::NodeType::Quantifier: {
                auto quant = static_cast<const Quantifier*>(node);
                
                a64::Gp saved_str = cc.new_gp_ptr("saved_str");
                cc.mov(saved_str, str);
                
                a64::Gp loop_count = cc.new_gp32("loop_count");
                cc.mov(loop_count, 0);
                
                Label L_LoopStart = cc.new_label();
                Label L_LoopExit = cc.new_label();
                Label L_LoopFail = cc.new_label();
                
                cc.bind(L_LoopStart);
                
                if (quant->max_match >= 0) {
                    a64::Gp max_m = cc.new_gp32("max_m");
                    cc.mov(max_m, quant->max_match);
                    cc.cmp(loop_count, max_m);
                    cc.b_ge(L_LoopExit);
                }
                
                Label L_LoopSuccess = cc.new_label();
                generate_node(quant->child.get(), str, end, captures, L_LoopSuccess, L_LoopExit);
                
                cc.bind(L_LoopSuccess);
                cc.add(loop_count, loop_count, 1);
                cc.b(L_LoopStart);
                
                cc.bind(L_LoopExit);
                
                a64::Gp min_m = cc.new_gp32("min_m");
                cc.mov(min_m, quant->min_match);
                cc.cmp(loop_count, min_m);
                cc.b_lt(L_Fail);
                
                cc.b(L_Next);
                break;
            }
        }
    }
};
#endif

} // namespace

namespace regexjit {

static asmjit::JitRuntime& get_jit_runtime() {
    static asmjit::JitRuntime* runtime = new asmjit::JitRuntime();
    return *runtime;
}

CompiledRegex::CompiledRegex(RegexJitFunc func, int max_capture_groups)
    : func_(func), max_capture_groups_(max_capture_groups) {}

CompiledRegex::~CompiledRegex() {
    if (func_) {
        // get_jit_runtime().release(func_);
    }
}

Compiler::Compiler() {}
Compiler::~Compiler() {}

std::shared_ptr<CompiledRegex> Compiler::compile(const ast::NodePtr& ast, bool disassemble) {
    int max_groups = count_groups(ast.get());
    
    FileLogger logger(stdout);
    
    CodeHolder code;
    code.init(get_jit_runtime().environment());
    
    if (disassemble) {
        code.set_logger(&logger);
    }
    
#if ASMJIT_ARCH_X86
    x86::Compiler cc(&code);
    X86RegexCompiler rcc(cc);
    rcc.compile(ast, max_groups);
    cc.finalize();
#elif ASMJIT_ARCH_ARM == 64
    a64::Compiler cc(&code);
    AArch64RegexCompiler rcc(cc);
    rcc.compile(ast, max_groups);
    cc.finalize();
#else
    throw CompileError("Unsupported architecture for RegexJIT");
#endif

    RegexJitFunc func = nullptr;
    Error err = get_jit_runtime().add(&func, &code);
    if (err != asmjit::kErrorOk) {
        throw CompileError("AsmJit compilation failed");
    }

    if (disassemble) {
        std::cout << "--- RegexJit Disassembly ---\n";
        // Assembly is already printed to stdout by FileLogger
    }

    return std::make_shared<CompiledRegex>(func, max_groups);
}

} // namespace regexjit
