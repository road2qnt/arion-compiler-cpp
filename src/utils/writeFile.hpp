#ifndef WRITEFILE_HPP
#define WRITEFILE_HPP

#include <iostream>
#include <fstream>
#include "../lexical-analysis/lexer.hpp"
#include "printTree.hpp"

// Mencetak daftar token (hasil Lexical Analysis)
void printLexicalAnalysis(ostream& out, const vector<Token>& tokens) {
    for (const Token& token : tokens) {
        out << tokenToString(token.type);
        if (!token.value.empty()) {
            out << "(" << token.value << ")";
        }
        out << endl;
    }
}

// Mencetak daftar token (hasil Lexical Analysis) ke file (tetap memanggil fungsi printLexicalAnalysis())
bool printLexicalAnalysisToFile(const string& outputPath, const vector<Token>& tokens) {
    ofstream outFile(outputPath);
    if (!outFile.is_open()) {
        cout << "[!] File output lexical tidak bisa dibuka." << endl;
        return false;
    }

    printLexicalAnalysis(outFile, tokens);
    return true;
}

// Mencetak Parse Tree (hasil Syntax Analysis) ke file (memanggil printTree() yang ada di file printTree.hpp)
bool printSyntaxAnalysisToFile(const string& outputPath, const ParseNode& root) {
    ofstream outFile(outputPath);
    if (!outFile.is_open()) {
        cout << "[!] File output syntax tidak bisa dibuka." << endl;
        return false;
    }

    printTree(outFile, root);
    return true;
}

#endif