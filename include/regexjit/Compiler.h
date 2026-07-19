#pragma once
#include "regexjit/AST.h"
#include <asmjit/core.h>
#if ASMJIT_ARCH_X86
#include <asmjit/x86.h>
#endif
#if ASMJIT_ARCH_ARM == 64
#include <asmjit/a64.h>
#endif
#include <memory>
#include <vector>

namespace regexjit {

// Function signature for JIT-compiled regex
// str: start of string
// end: end of string
// captures: array of pointers to store capture start/end. Size should be at least (max_group_index + 1) * 2
typedef bool (*RegexJitFunc)(const char* str, const char* end, const char** captures);

class CompiledRegex {
public:
    CompiledRegex(asmjit::JitRuntime& runtime, RegexJitFunc func, int max_capture_groups)
        : runtime_(runtime), func_(func), max_capture_groups_(max_capture_groups) {}
    ~CompiledRegex() {
        if (func_) {
            runtime_.release(func_);
        }
    }

    RegexJitFunc get_func() const { return func_; }
    int get_max_capture_groups() const { return max_capture_groups_; }

private:
    asmjit::JitRuntime& runtime_;
    RegexJitFunc func_;
    int max_capture_groups_;
};

class Compiler {
public:
    Compiler();
    ~Compiler();

    std::shared_ptr<CompiledRegex> compile(const ast::NodePtr& ast, bool disassemble = false);

private:
    asmjit::JitRuntime runtime_;
};

} // namespace regexjit
