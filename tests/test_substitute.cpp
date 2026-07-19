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

BOOST_AUTO_TEST_CASE(TestRegexSubstituteBackrefSwap) {
    // Swap first and last name (single occurrence)
    auto re = Regex::compile("([a-zA-Z]+) ([a-zA-Z]+)");
    std::string res = re.substitute("John Doe", "\\2, \\1");
    BOOST_CHECK_EQUAL(res, "Doe, John");
}

BOOST_AUTO_TEST_CASE(TestRegexSubstituteNoMatch) {
    // When pattern doesn't match, subject should be returned unchanged
    auto re = Regex::compile("xyz");
    std::string res = re.substitute("hello world", "REPLACED");
    BOOST_CHECK_EQUAL(res, "hello world");
}

BOOST_AUTO_TEST_CASE(TestRegexSubstituteWholeMatchAmpersand) {
    // & represents the whole match
    auto re = Regex::compile("[0-9]+");
    std::string res = re.substitute("item 42 costs 100", "[&]");
    BOOST_CHECK_EQUAL(res, "item [42] costs [100]");
}

BOOST_AUTO_TEST_CASE(TestRegexSubstituteEmptyReplacement) {
    // Effectively deletes all matches
    auto re = Regex::compile("[0-9]");
    std::string res = re.substitute("a1b2c3", "");
    BOOST_CHECK_EQUAL(res, "abc");
}

BOOST_AUTO_TEST_CASE(TestRegexSubstituteBackrefInvalidGroup) {
    // \3 doesn't exist, should insert nothing for that group
    auto re = Regex::compile("(a)(b)");
    std::string res = re.substitute("ab", "\\3");
    // Group 3 doesn't exist, so \3 is not expanded
    BOOST_CHECK_EQUAL(res, "");
}
