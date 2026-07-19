#include <boost/test/unit_test.hpp>
#include "regexjit/Regex.h"

using namespace regexjit;

BOOST_AUTO_TEST_CASE(TestRegexSubstituteBasic) {
    auto re = Regex::compile("cat");
    std::string res = re.substitute("the cat sat", "dog");
    BOOST_CHECK_EQUAL(res, "the dog sat");
}

BOOST_AUTO_TEST_CASE(TestRegexSubstituteGroup) {
    auto re = Regex::compile("(a)(b)");
    std::string res = re.substitute("xyzabuvw", "\\2\\1");
    BOOST_CHECK_EQUAL(res, "xyzbauvw");
}

BOOST_AUTO_TEST_CASE(TestRegexSubstituteMultiple) {
    auto re = Regex::compile("a");
    std::string res = re.substitute("banana", "o");
    BOOST_CHECK_EQUAL(res, "bonono");
}
