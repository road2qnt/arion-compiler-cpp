#ifndef AST_VISUALIZER_HPP
#define AST_VISUALIZER_HPP

#include "../syntax-analysis/parseTreeNode.hpp"
#include <iostream>
#include <fstream>
#include <string>

class ASTVisualizer {
private:
    int nodeCounter;

    void generateDotRecursively(std::ostream& os, const ParseNode& node, int parentId) {
        int currentId = ++nodeCounter;
        
        std::string label = node.label;
        size_t pos = 0;
        while ((pos = label.find("\"", pos)) != std::string::npos) {
            label.replace(pos, 1, "\\\"");
            pos += 2;
        }

        os << "    node" << currentId << " [label=\"" << label << "\"];\n";
        
        if (parentId != -1) {
            os << "    node" << parentId << " -> node" << currentId << ";\n";
        }

        for (const auto& child : node.children) {
            generateDotRecursively(os, child, currentId);
        }
    }

public:
    ASTVisualizer() : nodeCounter(0) {}

    void generateDotFile(const ParseNode& root, const std::string& filename) {
        std::ofstream outFile(filename);
        if (!outFile.is_open()) {
            std::cerr << "[DEBUG] Error creating DOT file: " << filename << std::endl;
            return;
        }

        outFile << "digraph ParseTree {\n";
        outFile << "    node [shape=box, fontname=\"Courier\"];\n";
        nodeCounter = 0;
        generateDotRecursively(outFile, root, -1);
        outFile << "}\n";
        outFile.close();
        
        std::cout << "\n[DEBUG] AST exported to Graphviz format: " << filename << std::endl;
        std::cout << "[DEBUG] Use https://dreampuf.github.io/GraphvizOnline/ to render." << std::endl;
        std::cout << "[DEBUG] Or run: dot -Tpng " << filename << " -o tree.png\n" << std::endl;
    }
};

#endif
