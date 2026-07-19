#include "regexjit/Parser.h"
#include "regexjit/Error.h"
#include <cctype>
#include <stdexcept>

using namespace regexjit;
using namespace regexjit::ast;

namespace {

class ParserImpl {
    std::string_view pattern;
    size_t pos = 0;
    int group_count = 0;

    bool match(char c) {
        if (pos < pattern.size() && pattern[pos] == c) {
            pos++;
            return true;
        }
        return false;
    }

    char peek() const {
        return pos < pattern.size() ? pattern[pos] : '\0';
    }

    char consume() {
        if (pos >= pattern.size()) {
            throw ParseError("Unexpected end of pattern");
        }
        return pattern[pos++];
    }

    NodePtr parseAlternative() {
        auto seq = parseSequence();
        if (!seq) return nullptr;

        if (match('|')) {
            auto alt = std::make_unique<Alternative>();
            alt->choices.push_back(std::move(seq));
            do {
                auto next_seq = parseSequence();
                if (!next_seq) {
                    throw ParseError("Expected sequence after '|'");
                }
                alt->choices.push_back(std::move(next_seq));
            } while (match('|'));
            return alt;
        }
        return seq;
    }

    NodePtr parseSequence() {
        auto seq = std::make_unique<Sequence>();
        while (pos < pattern.size() && peek() != '|' && peek() != ')') {
            auto item = parseItem();
            if (!item) break;
            seq->elements.push_back(std::move(item));
        }
        if (seq->elements.empty()) {
            return std::make_unique<Literal>(""); // Empty sequence
        }
        if (seq->elements.size() == 1) {
            return std::move(seq->elements[0]);
        }
        return seq;
    }

    NodePtr parseItem() {
        NodePtr node;
        char c = peek();
        
        if (c == '^') {
            consume();
            node = std::make_unique<Anchor>(AnchorType::StartOfLine);
        } else if (c == '$') {
            consume();
            node = std::make_unique<Anchor>(AnchorType::EndOfLine);
        } else if (c == '.') {
            consume();
            node = std::make_unique<Any>();
        } else if (c == '(') {
            consume();
            bool capture = true;
            if (match('?')) {
                if (match(':')) {
                    capture = false;
                } else {
                    throw ParseError("Unsupported extension '?" + std::string(1, peek()) + "'");
                }
            }
            int index = -1;
            if (capture) {
                index = ++group_count;
            }
            auto inner = parseAlternative();
            if (!match(')')) {
                throw ParseError("Unmatched '('");
            }
            node = std::make_unique<Group>(std::move(inner), index);
        } else if (c == '[') {
            consume();
            auto cc = std::make_unique<CharClass>();
            if (match('^')) {
                cc->negated = true;
            }
            while (pos < pattern.size() && peek() != ']') {
                // Check for POSIX class like [:space:]
                if (pos + 1 < pattern.size() && pattern[pos] == '[' && pattern[pos+1] == ':') {
                    size_t end_pos = pattern.find(":]", pos + 2);
                    if (end_pos != std::string_view::npos) {
                        std::string_view posix_class = pattern.substr(pos + 2, end_pos - (pos + 2));
                        pos = end_pos + 2; // Consume up to and including ':]'
                        
                        if (posix_class == "alnum") { cc->ranges.push_back({'A', 'Z'}); cc->ranges.push_back({'a', 'z'}); cc->ranges.push_back({'0', '9'}); }
                        else if (posix_class == "alpha") { cc->ranges.push_back({'A', 'Z'}); cc->ranges.push_back({'a', 'z'}); }
                        else if (posix_class == "blank") { cc->single_chars.push_back(' '); cc->single_chars.push_back('\t'); }
                        else if (posix_class == "cntrl") { cc->ranges.push_back({'\x00', '\x1F'}); cc->single_chars.push_back('\x7F'); }
                        else if (posix_class == "digit") { cc->ranges.push_back({'0', '9'}); }
                        else if (posix_class == "graph") { cc->ranges.push_back({'\x21', '\x7E'}); }
                        else if (posix_class == "lower") { cc->ranges.push_back({'a', 'z'}); }
                        else if (posix_class == "print") { cc->ranges.push_back({'\x20', '\x7E'}); }
                        else if (posix_class == "punct") { 
                            for (char pc : "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~") cc->single_chars.push_back(pc); 
                        }
                        else if (posix_class == "space") { cc->single_chars.push_back(' '); cc->single_chars.push_back('\t'); cc->single_chars.push_back('\r'); cc->single_chars.push_back('\n'); cc->single_chars.push_back('\v'); cc->single_chars.push_back('\f'); }
                        else if (posix_class == "upper") { cc->ranges.push_back({'A', 'Z'}); }
                        else if (posix_class == "word") { cc->ranges.push_back({'A', 'Z'}); cc->ranges.push_back({'a', 'z'}); cc->ranges.push_back({'0', '9'}); cc->single_chars.push_back('_'); }
                        else if (posix_class == "xdigit") { cc->ranges.push_back({'0', '9'}); cc->ranges.push_back({'A', 'F'}); cc->ranges.push_back({'a', 'f'}); }
                        else {
                            throw ParseError("Unknown POSIX class: [:" + std::string(posix_class) + ":]");
                        }
                        continue;
                    }
                }
                
                char start = consume();
                if (peek() == '-' && pos + 1 < pattern.size() && pattern[pos + 1] != ']') {
                    consume(); // consume '-'
                    char end = consume();
                    cc->ranges.push_back({start, end});
                } else {
                    cc->single_chars.push_back(start);
                }
            }
            if (!match(']')) {
                throw ParseError("Unmatched '['");
            }
            node = std::move(cc);
        } else if (c == '\\') {
            consume();
            char esc = consume();
            if (std::isdigit(esc)) {
                // Backreference
                int index = esc - '0';
                node = std::make_unique<BackReference>(index);
            } else if (esc == 'd' || esc == 'D' || esc == 'w' || esc == 'W' || esc == 's' || esc == 'S') {
                auto cc = std::make_unique<CharClass>();
                if (esc == 'D' || esc == 'W' || esc == 'S') cc->negated = true;
                char lower = std::tolower(esc);
                if (lower == 'd') { cc->ranges.push_back({'0', '9'}); }
                else if (lower == 'w') { cc->ranges.push_back({'A', 'Z'}); cc->ranges.push_back({'a', 'z'}); cc->ranges.push_back({'0', '9'}); cc->single_chars.push_back('_'); }
                else if (lower == 's') { cc->single_chars.push_back(' '); cc->single_chars.push_back('\t'); cc->single_chars.push_back('\r'); cc->single_chars.push_back('\n'); cc->single_chars.push_back('\v'); cc->single_chars.push_back('\f'); }
                node = std::move(cc);
            } else {
                // Escaped literal
                node = std::make_unique<Literal>(std::string(1, esc));
            }
        } else {
            // Literal
            std::string lit;
            while (pos < pattern.size() && std::string_view("()|*+?{.^$[\\").find(peek()) == std::string_view::npos) {
                lit += consume();
            }
            if (!lit.empty()) {
                node = std::make_unique<Literal>(lit);
            } else {
                return nullptr;
            }
        }

        // Parse quantifier
        if (node) {
            c = peek();
            if (c == '*' || c == '+' || c == '?' || c == '{') {
                int min_match = 0;
                int max_match = -1;
                if (c == '*') { consume(); min_match = 0; max_match = -1; }
                else if (c == '+') { consume(); min_match = 1; max_match = -1; }
                else if (c == '?') { consume(); min_match = 0; max_match = 1; }
                else if (c == '{') {
                    consume();
                    std::string min_str, max_str;
                    while (std::isdigit(peek())) min_str += consume();
                    if (match(',')) {
                        while (std::isdigit(peek())) max_str += consume();
                    } else {
                        max_str = min_str;
                    }
                    if (!match('}')) throw ParseError("Expected '}' in quantifier");
                    
                    min_match = min_str.empty() ? 0 : std::stoi(min_str);
                    if (!max_str.empty()) max_match = std::stoi(max_str);
                    else if (min_str.empty() && max_str.empty()) throw ParseError("Invalid quantifier {}");
                }
                
                bool greedy = true;
                if (match('?')) {
                    greedy = false;
                }
                
                // If it's a multi-character literal, we only quantify the last character
                if (node->type() == NodeType::Literal) {
                    auto* litNode = static_cast<Literal*>(node.get());
                    if (litNode->text.size() > 1) {
                        std::string prefix = litNode->text.substr(0, litNode->text.size() - 1);
                        char last_char = litNode->text.back();
                        
                        auto seq = std::make_unique<Sequence>();
                        seq->elements.push_back(std::make_unique<Literal>(prefix));
                        auto quantified = std::make_unique<Quantifier>(std::make_unique<Literal>(std::string(1, last_char)), min_match, max_match, greedy);
                        seq->elements.push_back(std::move(quantified));
                        node = std::move(seq);
                    } else {
                        node = std::make_unique<Quantifier>(std::move(node), min_match, max_match, greedy);
                    }
                } else {
                    node = std::make_unique<Quantifier>(std::move(node), min_match, max_match, greedy);
                }
            }
        }
        return node;
    }

public:
    ParserImpl(std::string_view pattern) : pattern(pattern) {}

    NodePtr parse() {
        if (pattern.empty()) {
            return std::make_unique<Literal>("");
        }
        auto result = parseAlternative();
        if (pos < pattern.size()) {
            throw ParseError("Unexpected character at end of pattern: '" + std::string(1, peek()) + "'");
        }
        return result;
    }
};

} // namespace

namespace regexjit {

ast::NodePtr Parser::parse(std::string_view pattern) {
    ParserImpl impl(pattern);
    return impl.parse();
}

} // namespace regexjit
