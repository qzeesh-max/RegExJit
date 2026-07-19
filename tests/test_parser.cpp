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

#define BOOST_TEST_MODULE RegExJitTests
#include <boost/test/unit_test.hpp>
#include "regexjit/Parser.h"
#include "regexjit/AST.h"
#include "regexjit/Error.h"

using namespace regexjit;
using namespace regexjit::ast;

BOOST_AUTO_TEST_CASE(TestParseLiteral) {
    auto ast = Parser::parse("abc");
    BOOST_REQUIRE(ast != nullptr);
    BOOST_CHECK(ast->type() == NodeType::Literal);
    auto lit = static_cast<Literal*>(ast.get());
    BOOST_CHECK_EQUAL(lit->text, "abc");
}

BOOST_AUTO_TEST_CASE(TestParseQuantifier) {
    auto ast = Parser::parse("a*");
    BOOST_REQUIRE(ast != nullptr);
    BOOST_CHECK(ast->type() == NodeType::Quantifier);
    auto quant = static_cast<Quantifier*>(ast.get());
    BOOST_CHECK_EQUAL(quant->min_match, 0);
    BOOST_CHECK_EQUAL(quant->max_match, -1);
}

BOOST_AUTO_TEST_CASE(TestParseAlternative) {
    auto ast = Parser::parse("a|b");
    BOOST_REQUIRE(ast != nullptr);
    BOOST_CHECK(ast->type() == NodeType::Alternative);
}

BOOST_AUTO_TEST_CASE(TestParseGroup) {
    auto ast = Parser::parse("(a)");
    BOOST_REQUIRE(ast != nullptr);
    BOOST_CHECK(ast->type() == NodeType::Group);
    auto grp = static_cast<Group*>(ast.get());
    BOOST_CHECK_EQUAL(grp->index, 1);
}

BOOST_AUTO_TEST_CASE(TestParseBackReference) {
    auto ast = Parser::parse("\\1");
    BOOST_REQUIRE(ast != nullptr);
    BOOST_CHECK(ast->type() == NodeType::BackReference);
    auto bref = static_cast<BackReference*>(ast.get());
    BOOST_CHECK_EQUAL(bref->index, 1);
}

BOOST_AUTO_TEST_CASE(TestParseError) {
    BOOST_CHECK_THROW(Parser::parse("("), ParseError);
    BOOST_CHECK_THROW(Parser::parse("[a-"), ParseError);
}
