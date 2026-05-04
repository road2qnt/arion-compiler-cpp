#ifndef PARSETREENODE_HPP
#define PARSETREENODE_HPP

#include <string>
#include <vector>

struct ParseNode {
    std::string label;
    std::vector<ParseNode> children;
    bool isTerminal;

    ParseNode(const std::string& lbl, bool terminal = false)
        : label(lbl), isTerminal(terminal) {}
};

#endif
