#ifndef PARSETREENODE_HPP
#define PARSETREENODE_HPP

#include <string>
#include <vector>

struct ParseNode {
    std::string label;
    std::vector<ParseNode> children;
    bool isTerminal;
    int line;
    int column;

    ParseNode(const std::string& lbl, bool terminal = false)
        : label(lbl), isTerminal(terminal), line(0), column(0) {}
};

#endif
