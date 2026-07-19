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

#include "regexjit/Regex.h"
#include "regexjit/Parser.h"
#include "regexjit/Compiler.h"
#include "regexjit/Error.h"
#include <vector>

namespace regexjit {

struct Regex::Impl {
    Compiler compiler; // Hold the compiler which holds JitRuntime. Must be declared first to be destroyed last.
    std::shared_ptr<CompiledRegex> compiled;
};

Regex::Regex() = default;
Regex::Regex(std::unique_ptr<Impl> impl) : pimpl(std::move(impl)) {}
Regex::~Regex() = default;

Regex::Regex(Regex&&) noexcept = default;
Regex& Regex::operator=(Regex&&) noexcept = default;

Regex Regex::compile(std::string_view pattern, bool disassemble) {
    auto ast = Parser::parse(pattern);
    auto impl = std::make_unique<Impl>();
    impl->compiled = impl->compiler.compile(ast, disassemble);
    return Regex(std::move(impl));
}

MatchResult Regex::match(std::string_view subject) const {
    MatchResult result;
    if (!pimpl || !pimpl->compiled || !pimpl->compiled->get_func()) {
        return result;
    }

    int num_captures = pimpl->compiled->get_max_capture_groups();
    std::vector<const char*> capture_ptrs((num_captures + 1) * 2, nullptr);

    const char* str_start = subject.data();
    const char* str_end = subject.data() + subject.size();
    
    // For a strict match, we might need to anchor it or check if it consumes everything.
    // Our compiler currently doesn't add implicit anchors, so `match` will just run from the start.
    // Let's run it from start, and see if it consumed the whole string.
    bool matched = pimpl->compiled->get_func()(str_start, str_end, capture_ptrs.data());
    
    // match() means it matches the prefix or the whole thing? The requirement is usually 'matches from start'.
    // PCRE match checks from the start. We will check if it matched at all from the start.
    if (matched) {
        result.matched = true;
        if (capture_ptrs[0] != nullptr && capture_ptrs[1] != nullptr) {
            result.match_str = std::string_view(capture_ptrs[0], capture_ptrs[1] - capture_ptrs[0]);
        }
        
        result.captures.push_back(result.match_str);
        
        for (int i = 1; i <= num_captures; ++i) {
            const char* c_start = capture_ptrs[i * 2];
            const char* c_end = capture_ptrs[i * 2 + 1];
            if (c_start && c_end) {
                result.captures.push_back(std::string_view(c_start, c_end - c_start));
            } else {
                result.captures.push_back(std::string_view());
            }
        }
    }
    
    return result;
}

MatchResult Regex::find(std::string_view subject) const {
    MatchResult result;
    if (!pimpl || !pimpl->compiled || !pimpl->compiled->get_func()) {
        return result;
    }

    int num_captures = pimpl->compiled->get_max_capture_groups();
    std::vector<const char*> capture_ptrs((num_captures + 1) * 2, nullptr);

    const char* str_start = subject.data();
    const char* str_end = subject.data() + subject.size();
    
    // Find loop
    for (const char* current = str_start; current <= str_end; ++current) {
        bool matched = pimpl->compiled->get_func()(current, str_end, capture_ptrs.data());
        if (matched) {
            result.matched = true;
            if (capture_ptrs[0] != nullptr && capture_ptrs[1] != nullptr) {
                result.match_str = std::string_view(capture_ptrs[0], capture_ptrs[1] - capture_ptrs[0]);
            }
            
            result.captures.push_back(result.match_str);
            
            for (int i = 1; i <= num_captures; ++i) {
                const char* c_start = capture_ptrs[i * 2];
                const char* c_end = capture_ptrs[i * 2 + 1];
                if (c_start && c_end) {
                    result.captures.push_back(std::string_view(c_start, c_end - c_start));
                } else {
                    result.captures.push_back(std::string_view());
                }
            }
            break; // found the first match
        }
    }
    
    return result;
}

std::string Regex::substitute(std::string_view subject, std::string_view replacement) const {
    if (!pimpl || !pimpl->compiled || !pimpl->compiled->get_func()) {
        return std::string(subject);
    }

    std::string result;
    int num_captures = pimpl->compiled->get_max_capture_groups();
    std::vector<const char*> capture_ptrs((num_captures + 1) * 2, nullptr);

    const char* current = subject.data();
    const char* str_end = subject.data() + subject.size();
    
    while (current <= str_end) {
        bool matched = pimpl->compiled->get_func()(current, str_end, capture_ptrs.data());
        if (matched) {
            // Append replacement
            for (size_t i = 0; i < replacement.size(); ++i) {
                if (replacement[i] == '\\' && i + 1 < replacement.size() && std::isdigit(replacement[i+1])) {
                    int group_idx = replacement[i+1] - '0';
                    if (group_idx > 0 && group_idx <= num_captures) {
                        const char* c_start = capture_ptrs[group_idx * 2];
                        const char* c_end = capture_ptrs[group_idx * 2 + 1];
                        if (c_start && c_end) {
                            result.append(c_start, c_end - c_start);
                        }
                    }
                    i++; // skip digit
                } else if (replacement[i] == '&') { // & represents the whole match (0th capture group) in many substitutions
                    const char* c_start = capture_ptrs[0];
                    const char* c_end = capture_ptrs[1];
                    if (c_start && c_end) {
                        result.append(c_start, c_end - c_start);
                    }
                } else {
                    result.push_back(replacement[i]);
                }
            }
            const char* match_end = capture_ptrs[1];
            if (match_end == current) {
                // To avoid infinite loop on empty match
                if (current < str_end) {
                    result.push_back(*current);
                }
                current++;
            } else {
                current = match_end;
            }
        } else {
            if (current < str_end) {
                result.push_back(*current);
            }
            current++;
        }
    }
    
    return result;
}

} // namespace regexjit
