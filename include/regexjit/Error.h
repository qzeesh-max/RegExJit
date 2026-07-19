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
