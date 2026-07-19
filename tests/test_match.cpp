#include <boost/test/unit_test.hpp>
#include "regexjit/Regex.h"

using namespace regexjit;

BOOST_AUTO_TEST_CASE(TestRegexMatchBasic) {
    auto re = Regex::compile("abc");
    BOOST_CHECK(re.match("abc").matched);
    BOOST_CHECK(!re.match("ab").matched);
    // Our simplified matching matches the prefix if it's not anchored at the end.
    // Actually, since we didn't add an implicit $ at the end, it matches if the prefix matches.
    BOOST_CHECK(re.match("abcd").matched);
}

BOOST_AUTO_TEST_CASE(TestRegexMatchCharClass) {
    auto re = Regex::compile("[a-z]123");
    BOOST_CHECK(re.match("b123").matched);
    BOOST_CHECK(!re.match("A123").matched);
}

BOOST_AUTO_TEST_CASE(TestRegexMatchQuantifier) {
    auto re = Regex::compile("a*b");
    BOOST_CHECK(re.match("b").matched);
    BOOST_CHECK(re.match("ab").matched);
    BOOST_CHECK(re.match("aab").matched);
    BOOST_CHECK(!re.match("c").matched);
}

BOOST_AUTO_TEST_CASE(TestRegexMatchAlternative) {
    auto re = Regex::compile("cat|dog");
    BOOST_CHECK(re.match("cat").matched);
    BOOST_CHECK(re.match("dog").matched);
    BOOST_CHECK(!re.match("bird").matched);
}

BOOST_AUTO_TEST_CASE(TestRegexMatchGroup) {
    auto re = Regex::compile("a(b)c");
    auto res = re.match("abc");
    BOOST_CHECK(res.matched);
    BOOST_REQUIRE_EQUAL(res.captures.size(), 2);
    BOOST_CHECK_EQUAL(res.captures[1], "b");
}

BOOST_AUTO_TEST_CASE(TestRegexMatchBackReference) {
    auto re = Regex::compile("([a-c])\\1");
    BOOST_CHECK(re.match("aa").matched);
    BOOST_CHECK(re.match("bb").matched);
    BOOST_CHECK(re.match("cc").matched);
    BOOST_CHECK(!re.match("ab").matched);
}

BOOST_AUTO_TEST_CASE(TestRegexMatchNegative) {
    auto re = Regex::compile("^[a-z]+@[0-9]+\\.com$");
    BOOST_CHECK(!re.match("user@domain.com").matched); // domain not numbers
    BOOST_CHECK(!re.match("123@123.com").matched); // user not letters
    BOOST_CHECK(!re.match("user@123.net").matched); // wrong extension
    
    auto re2 = Regex::compile("(cat|dog|fish) \\1");
    BOOST_CHECK(!re2.match("cat dog").matched);
    BOOST_CHECK(!re2.match("fish bird").matched);
    
    auto re3 = Regex::compile("A{3,5}");
    BOOST_CHECK(!re3.match("AA").matched);
    BOOST_CHECK(re3.match("AAA").matched);
}
