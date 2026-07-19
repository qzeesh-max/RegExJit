#pragma once

#include <vector>
#include <memory>
#include <string>
#include <variant>

namespace regexjit {
namespace ast {

enum class NodeType {
    Literal,
    Any,
    CharClass,
    Quantifier,
    Alternative,
    Sequence,
    Group,
    Anchor,
    BackReference
};

class Node {
public:
    virtual ~Node() = default;
    virtual NodeType type() const = 0;
};

using NodePtr = std::unique_ptr<Node>;

class Literal : public Node {
public:
    std::string text;
    Literal(std::string text) : text(std::move(text)) {}
    NodeType type() const override { return NodeType::Literal; }
};

class Any : public Node {
public:
    NodeType type() const override { return NodeType::Any; }
};

struct CharRange {
    char start;
    char end;
};

class CharClass : public Node {
public:
    std::vector<CharRange> ranges;
    std::vector<char> single_chars;
    bool negated = false;
    NodeType type() const override { return NodeType::CharClass; }
};

class Quantifier : public Node {
public:
    NodePtr child;
    int min_match = 0;
    int max_match = -1; // -1 means infinity
    bool greedy = true;

    Quantifier(NodePtr child, int min, int max, bool greedy = true)
        : child(std::move(child)), min_match(min), max_match(max), greedy(greedy) {}

    NodeType type() const override { return NodeType::Quantifier; }
};

class Alternative : public Node {
public:
    std::vector<NodePtr> choices;
    NodeType type() const override { return NodeType::Alternative; }
};

class Sequence : public Node {
public:
    std::vector<NodePtr> elements;
    NodeType type() const override { return NodeType::Sequence; }
};

class Group : public Node {
public:
    NodePtr child;
    int index = -1; // -1 for non-capturing group, >0 for capturing
    Group(NodePtr child, int index) : child(std::move(child)), index(index) {}
    NodeType type() const override { return NodeType::Group; }
};

enum class AnchorType {
    StartOfLine, // ^
    EndOfLine    // $
};

class Anchor : public Node {
public:
    AnchorType anchor_type;
    Anchor(AnchorType t) : anchor_type(t) {}
    NodeType type() const override { return NodeType::Anchor; }
};

class BackReference : public Node {
public:
    int index;
    BackReference(int index) : index(index) {}
    NodeType type() const override { return NodeType::BackReference; }
};

} // namespace ast
} // namespace regexjit
