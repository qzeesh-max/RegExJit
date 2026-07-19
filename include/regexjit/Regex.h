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

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <optional>

namespace regexjit {

struct MatchResult {
    bool matched = false;
    std::string_view match_str;
    std::vector<std::string_view> captures;
};

class Regex {
public:
    // Factory method to compile a regex
    static Regex compile(std::string_view pattern, bool disassemble = false);

    // Default constructor (invalid regex)
    Regex();
    
    // Move semantics
    Regex(Regex&&) noexcept;
    Regex& operator=(Regex&&) noexcept;

    // Delete copy semantics
    Regex(const Regex&) = delete;
    Regex& operator=(const Regex&) = delete;

    ~Regex();

    // Core operations
    MatchResult match(std::string_view subject) const;
    MatchResult find(std::string_view subject) const;
    std::string substitute(std::string_view subject, std::string_view replacement) const;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
    
    explicit Regex(std::unique_ptr<Impl> impl);
};

} // namespace regexjit
