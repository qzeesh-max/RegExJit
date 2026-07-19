#pragma once
#include "regexjit/AST.h"
#include <string_view>

namespace regexjit {

class Parser {
public:
    static ast::NodePtr parse(std::string_view pattern);
};

} // namespace regexjit
