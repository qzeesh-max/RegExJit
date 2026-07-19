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

BOOST_AUTO_TEST_CASE(TestRegexFindComplexEdgeCases) {
    // Find with complex backreferences and quantifiers
    auto re = Regex::compile("([a-z]+) \\1");
    auto res = re.find("hello hello world");
    BOOST_CHECK(res.matched);
    BOOST_CHECK_EQUAL(res.match_str, "hello hello");
    
    // Find multiple occurrences by calling find repeatedly? 
    // `find` just finds the first.
    auto re2 = Regex::compile("([0-9])\\1{2,}");
    BOOST_CHECK(re2.find("12 33 4444 55").matched);
    BOOST_CHECK_EQUAL(re2.find("12 33 4444 55").match_str, "4444");
}

BOOST_AUTO_TEST_CASE(TestRegexFindEmptySubject) {
    auto re = Regex::compile("abc");
    BOOST_CHECK(!re.find("").matched);
}

BOOST_AUTO_TEST_CASE(TestRegexFindAlternation) {
    auto re = Regex::compile("error|warning|fatal");
    auto res = re.find("this is a warning message");
    BOOST_CHECK(res.matched);
    BOOST_CHECK_EQUAL(res.match_str, "warning");

    auto res2 = re.find("everything is fine");
    BOOST_CHECK(!res2.matched);
}

BOOST_AUTO_TEST_CASE(TestRegexFindAtBoundaries) {
    // Pattern at the very start
    auto re = Regex::compile("[0-9]+");
    auto res = re.find("42 is the answer");
    BOOST_CHECK(res.matched);
    BOOST_CHECK_EQUAL(res.match_str, "42");

    // Pattern at the very end
    auto res2 = re.find("the answer is 42");
    BOOST_CHECK(res2.matched);
    BOOST_CHECK_EQUAL(res2.match_str, "42");
}

BOOST_AUTO_TEST_CASE(TestRegexFindNegativeMoreCases) {
    // Pattern that almost matches
    auto re = Regex::compile("abcdef");
    BOOST_CHECK(!re.find("abcde").matched);
    BOOST_CHECK(!re.find("bcdef").matched);
    BOOST_CHECK(re.find("xabcdefy").matched);

    // Character class that matches nothing in subject
    auto re2 = Regex::compile("[A-Z]{5}");
    BOOST_CHECK(!re2.find("all lowercase text here").matched);
}
