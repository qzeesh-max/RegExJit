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
