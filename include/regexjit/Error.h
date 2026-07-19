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
#include <stdexcept>
#include <string>

namespace regexjit {

class RegexError : public std::runtime_error {
public:
    explicit RegexError(const std::string& message) : std::runtime_error(message) {}
};

class ParseError : public RegexError {
public:
    explicit ParseError(const std::string& message) : RegexError("Regex Parse Error: " + message) {}
};

class CompileError : public RegexError {
public:
    explicit CompileError(const std::string& message) : RegexError("Regex Compile Error: " + message) {}
};

class RuntimeError : public RegexError {
public:
    explicit RuntimeError(const std::string& message) : RegexError("Regex Runtime Error: " + message) {}
};

} // namespace regexjit
