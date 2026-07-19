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
#include <regexjit/Regex.h>
#include <regexjit/Error.h>
#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>

using namespace regexjit;

const std::string text = R"(
[Server::Production]
# Primary telemetry & cluster routing
endpoint = "https://admin:p%40ssw0rd@api.cluster-01.us-east.prod.internal:8443/v2/telemetry?depth=full&retry=true"
matrix = ((A + B) * (C - (D / E)))
owner_email = "sysadmin.core+alerts-2026@sub-domain.enterprise-cloud.org"
data_packet = <[{ID: 0x7F9A, Status: "ACTIVE", Tags:[alpha, beta, gamma]}]>
)";

// Helper function to test regex using Perl as external PCRE reference
bool check_with_perl(const std::string& pattern, const std::string& input) {
    std::ofstream tmp_text("tmp_test_text.txt", std::ios::binary);
    tmp_text << input;
    tmp_text.close();

    std::ofstream tmp_pattern("tmp_test_pattern.txt", std::ios::binary);
    tmp_pattern << pattern;
    tmp_pattern.close();

    std::string script = R"(
    open(my $tf, '<', 'tmp_test_text.txt') or die;
    my $text = do { local $/; <$tf> };
    close($tf);

    open(my $pf, '<', 'tmp_test_pattern.txt') or die;
    my $pat = do { local $/; <$pf> };
    close($pf);

    my $re;
    eval {
        $re = qr/$pat/;
    };
    if ($@) { exit 2; } # Parse error

    if ($text =~ $re) { exit 0; } else { exit 1; }
    )";

    std::ofstream tmp_script("tmp_test_script.pl");
    tmp_script << script;
    tmp_script.close();

    int ret = std::system("perl tmp_test_script.pl");
    
    std::remove("tmp_test_text.txt");
    std::remove("tmp_test_pattern.txt");
    std::remove("tmp_test_script.pl");

#ifdef _WIN32
    if (ret == 0) return true;
    if (ret == 1) return false;
#else
    if (WIFEXITED(ret)) {
        int status = WEXITSTATUS(ret);
        if (status == 0) return true;
        if (status == 1) return false;
    }
#endif
    throw std::runtime_error("Perl script failed to execute properly");
}

BOOST_AUTO_TEST_CASE(test_pcre_subroutine_driven_url_validator) {
    std::string pattern = R"((?xm)
(?<=endpoint\s=\s")
(?&URI)
(?="$)
(?(DEFINE)
  (?<URI>      (?&SCHEME) :// (?:(?&USERINFO) @)? (?&HOST) (?: : (?&PORT) )? (?&PATH) (?: \? (?&QUERY) )? )
  (?<SCHEME>   https? | sftp )
  (?<USERINFO> (?> [a-zA-Z0-9._%+-]++ ) (?: : (?> [a-zA-Z0-9._%+-]++ ) )? )
  (?<HOST>     (?: (?&SUBDOMAIN) \. )++ (?&TLD) )
  (?<SUBDOMAIN>[a-zA-Z0-9](?>[a-zA-Z0-9-]*[a-zA-Z0-9])?)
  (?<TLD>      (?:[a-zA-Z]{2,}|internal|prod|local))
  (?<PORT>     \d{1,5} )
  (?<PATH>     (?: / [a-zA-Z0-9._-]+ )* )
  (?<QUERY>    (?&PARAM) (?: & (?&PARAM) )* )
  (?<PARAM>    [a-zA-Z0-9_]+ = [a-zA-Z0-9_.-]+ )
))";

    // This should succeed in perl
    BOOST_CHECK(check_with_perl(pattern, text) == true);
    
    // RegExJit doesn't support this yet, ensure it fails gracefully with ParseError
    BOOST_CHECK_THROW(Regex::compile(pattern), ParseError);
}

BOOST_AUTO_TEST_CASE(test_pcre_mutually_recursive_validator) {
    std::string pattern = R"((?x)
(?<=data_packet\s=\s)
(
  <
    (?:
      [^<>[\]{}]++
      | (?1)
      | (?2)
      | (?3)
    )*+
  >
)
|
(
  \[
    (?:
      [^<>[\]{}]++
      | (?1)
      | (?2)
      | (?3)
    )*+
  \]
)
|
(
  \{
    (?:
      [^<>[\]{}]++
      | (?1)
      | (?2)
      | (?3)
    )*+
  \}
))";

    BOOST_CHECK(check_with_perl(pattern, text) == true);
    BOOST_CHECK_THROW(Regex::compile(pattern), ParseError);
}

BOOST_AUTO_TEST_CASE(test_pcre_comment_skipping_algebraic_evaluator) {
    std::string pattern = R"((?xm)
(?:
  ^\s*\#.*$
  |
  "[^"\\]*(?:\\.[^"\\]*)*"
)(*SKIP)(*FAIL)
|
(?<=matrix\s=\s)
(
  \(
    (?:
      [^()]+
      |
      (?1)
    )*+
  \)
))";

    BOOST_CHECK(check_with_perl(pattern, text) == true);
    BOOST_CHECK_THROW(Regex::compile(pattern), ParseError);
}

BOOST_AUTO_TEST_CASE(test_pcre_possessive_quantifier_trap) {
    std::string pattern = R"((?x)
(?<=endpoint\s=\s"https://)
(?:[a-zA-Z0-9:%._+-]+@)?
(?<HOST>[a-zA-Z0-9.-]++) \. [a-zA-Z]{2,}
(?::\d+)? /)";

    // This one is designed to fail because of possessive quantifiers
    BOOST_CHECK(check_with_perl(pattern, text) == false);
    BOOST_CHECK_THROW(Regex::compile(pattern), ParseError);
}

BOOST_AUTO_TEST_CASE(test_pcre_recursive_assertion_contamination) {
    std::string pattern = R"((?x)
(
  (?<=matrix\s=\s)
  \(
    (?:
      [^()]+
      |
      (?1)
    )*
  \)
))";

    // Fails due to recursion triggering the lookbehind inappropriately
    BOOST_CHECK(check_with_perl(pattern, text) == false);
    BOOST_CHECK_THROW(Regex::compile(pattern), ParseError);
}

BOOST_AUTO_TEST_CASE(test_pcre_commit_global_abortion_verb) {
    std::string pattern = R"((?xm)
^\[(?<section>[a-zA-Z]+)
(?:
  :: (?<subsection>[a-zA-Z]+) (*COMMIT) - (?<env>\d+)
|
  \]
))";

    // Fails due to (*COMMIT) aborting alternative match branches
    BOOST_CHECK(check_with_perl(pattern, text) == false);
    BOOST_CHECK_THROW(Regex::compile(pattern), ParseError);
}
