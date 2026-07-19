#include <boost/test/unit_test.hpp>
#include "regexjit/Regex.h"

using namespace regexjit;

BOOST_AUTO_TEST_CASE(TestRegexFindBasic) {
    auto re = Regex::compile("abc");
    auto res = re.find("xxxabcyyy");
    BOOST_CHECK(res.matched);
    BOOST_CHECK_EQUAL(res.match_str, "abc");
}

BOOST_AUTO_TEST_CASE(TestRegexFindGroup) {
    auto re = Regex::compile("x(y)z");
    auto res = re.find("123xyz456");
    BOOST_CHECK(res.matched);
    BOOST_CHECK_EQUAL(res.match_str, "xyz");
    BOOST_REQUIRE_EQUAL(res.captures.size(), 2);
    BOOST_CHECK_EQUAL(res.captures[1], "y");
}

BOOST_AUTO_TEST_CASE(TestRegexFindNegative) {
    auto re = Regex::compile("([0-9]{3})-([0-9]{2})-([0-9]{4})");
    BOOST_CHECK(!re.find("SSN is 123-456-7890").matched); // middle is 3 digits instead of 2
    BOOST_CHECK(!re.find("SSN is 12-34-5678").matched); // first is 2 digits
    BOOST_CHECK(!re.find("SSN is 123-45-678").matched); // last is 3 digits
    BOOST_CHECK(re.find("SSN is 123-45-6789").matched); // valid
    
    auto re2 = Regex::compile("foo(bar|baz)qux");
    BOOST_CHECK(!re2.find("this is foobazqu").matched);
    BOOST_CHECK(!re2.find("this is foobaqux").matched);
}
