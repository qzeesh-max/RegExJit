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
