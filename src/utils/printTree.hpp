#ifndef PRINTTREE_HPP
#define PRINTTREE_HPP

#include "../syntax-analysis/parseTreeNode.hpp"
#include <iostream>
#include <fstream>
#include <string>

inline void printTreeHelper(std::ostream& os, const ParseNode& node, const std::string& prefix, bool isLast, bool isRoot) {
    if (isRoot) {
        os << node.label << "\n";
    } else {
        os << prefix << (isLast ? "└── " : "├── ") << node.label << "\n";
    }

    std::string nextPrefix = prefix;
    if (!isRoot) {
        nextPrefix += (isLast ? "    " : "│   ");
    }

    for (size_t i = 0; i < node.children.size(); ++i) {
        printTreeHelper(os, node.children[i], nextPrefix, i == node.children.size() - 1, false);
    }
}

inline void printTree(std::ostream& os, const ParseNode& root) {
    printTreeHelper(os, root, "", true, true);
}

#endif
