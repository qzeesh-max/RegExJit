#include <boost/test/unit_test.hpp>
#include "regexjit/Compiler.h"
#include "regexjit/Parser.h"

using namespace regexjit;

BOOST_AUTO_TEST_CASE(TestCompilerBasic) {
    Compiler compiler;
    auto ast = Parser::parse("a");
    auto compiled = compiler.compile(ast);
    BOOST_CHECK(compiled != nullptr);
    BOOST_CHECK(compiled->get_func() != nullptr);
}
