#include <boost/test/unit_test.hpp>
#include "regexjit/Regex.h"
#include "regexjit/Error.h"

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

BOOST_AUTO_TEST_CASE(TestRegexMatchComplexEdgeCases) {
    // Nested alternation and grouping
    auto re = Regex::compile("((a|b)|c)d");
    BOOST_CHECK(re.match("ad").matched);
    BOOST_CHECK(re.match("bd").matched);
    BOOST_CHECK(re.match("cd").matched);
    BOOST_CHECK(!re.match("xd").matched);

    // Quantified backreference
    auto re2 = Regex::compile("([0-9])\\1+");
    BOOST_CHECK(re2.match("55").matched);
    BOOST_CHECK(re2.match("555").matched);
    BOOST_CHECK(!re2.match("56").matched);
}

BOOST_AUTO_TEST_CASE(TestRegexParseErrors) {
    // Unmatched parenthesis
    BOOST_CHECK_THROW(Regex::compile("("), regexjit::ParseError);
    // Unmatched bracket
    BOOST_CHECK_THROW(Regex::compile("[a-z"), regexjit::ParseError);
    // Invalid quantifier
    BOOST_CHECK_THROW(Regex::compile("a{3,2}"), regexjit::ParseError);
}

BOOST_AUTO_TEST_CASE(TestRegexMatchEmptyPattern) {
    auto re = Regex::compile("");
    BOOST_CHECK(re.match("").matched);
    BOOST_CHECK(re.match("anything").matched);
}

BOOST_AUTO_TEST_CASE(TestRegexMatchSingleChar) {
    auto re = Regex::compile("x");
    BOOST_CHECK(re.match("x").matched);
    BOOST_CHECK(!re.match("y").matched);
    BOOST_CHECK(re.match("xyz").matched);  // prefix match
}

BOOST_AUTO_TEST_CASE(TestRegexMatchDot) {
    auto re = Regex::compile("a.c");
    BOOST_CHECK(re.match("abc").matched);
    BOOST_CHECK(re.match("axc").matched);
    BOOST_CHECK(re.match("a1c").matched);
    BOOST_CHECK(!re.match("ac").matched);
}

BOOST_AUTO_TEST_CASE(TestRegexMatchNestedGroups) {
    auto re = Regex::compile("((a)(b(c)))");
    auto res = re.match("abc");
    BOOST_CHECK(res.matched);
    // Group 1 = abc, Group 2 = a, Group 3 = bc, Group 4 = c
    BOOST_REQUIRE_GE(res.captures.size(), 5u);
    BOOST_CHECK_EQUAL(res.captures[1], "abc");
    BOOST_CHECK_EQUAL(res.captures[2], "a");
    BOOST_CHECK_EQUAL(res.captures[3], "bc");
    BOOST_CHECK_EQUAL(res.captures[4], "c");
}

BOOST_AUTO_TEST_CASE(TestRegexMatchQuantifierEdgeCases) {
    // a+ should not match empty string
    auto re1 = Regex::compile("a+");
    BOOST_CHECK(!re1.match("").matched);
    BOOST_CHECK(re1.match("a").matched);
    BOOST_CHECK(re1.match("aaa").matched);

    // a? should match empty string
    auto re2 = Regex::compile("a?");
    BOOST_CHECK(re2.match("").matched);
    BOOST_CHECK(re2.match("a").matched);

    // Exact count {3}
    auto re3 = Regex::compile("x{3}");
    BOOST_CHECK(!re3.match("xx").matched);
    BOOST_CHECK(re3.match("xxx").matched);
    BOOST_CHECK(re3.match("xxxx").matched); // prefix match
}

BOOST_AUTO_TEST_CASE(TestRegexMatchCharClassNegated) {
    auto re = Regex::compile("[^0-9]");
    BOOST_CHECK(re.match("a").matched);
    BOOST_CHECK(re.match("Z").matched);
    BOOST_CHECK(!re.match("5").matched);
}

BOOST_AUTO_TEST_CASE(TestRegexMatchAlternationMultiple) {
    auto re = Regex::compile("red|green|blue|yellow");
    BOOST_CHECK(re.match("red").matched);
    BOOST_CHECK(re.match("green").matched);
    BOOST_CHECK(re.match("blue").matched);
    BOOST_CHECK(re.match("yellow").matched);
    BOOST_CHECK(!re.match("orange").matched);
    BOOST_CHECK(!re.match("purple").matched);
}

BOOST_AUTO_TEST_CASE(TestRegexMatchNegativeEdgeCases) {
    // Pattern longer than subject
    auto re1 = Regex::compile("abcdef");
    BOOST_CHECK(!re1.match("abc").matched);

    // Completely different characters
    auto re2 = Regex::compile("[0-9]+");
    BOOST_CHECK(!re2.match("abcdef").matched);

    // Backreference to uncaptured group (group captured but doesn't match)
    auto re3 = Regex::compile("(a)b\\1");
    BOOST_CHECK(re3.match("aba").matched);
    BOOST_CHECK(!re3.match("abb").matched);
    BOOST_CHECK(!re3.match("abc").matched);
}

