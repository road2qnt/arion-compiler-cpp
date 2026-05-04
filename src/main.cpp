#include <iostream>
#include <fstream>
#include "lexical-analysis/lexer.hpp"
#include "lexical-analysis/token.hpp"
#include "syntax-analysis/parser.hpp"
#include "utils/printTree.hpp"
#include "debugger/tokenDumper.hpp"
#include "debugger/astVisualizer.hpp"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " <source_file> [output_file]" << endl;
        return 1;
    }
    string filename = argv[1];
    Lexer lexer;
    lexer.DFA(filename);
    vector<Token> tokens = lexer.getTokens();

    dumpTokens(tokens);

    ostream* out = &cout;
    ofstream outFile;
    if (argc >= 3) {
        outFile.open(argv[2]);
        if (!outFile.is_open()) {
            cout << "[!] Output file tidak bisa dibuka." << endl;
            return 1;
        }
        out = &outFile;
    }

    // CHECKPOINT TEST
    cout << "--- Phase 1: Dummy Tree Test ---" << endl;
    Parser parser(tokens);
    ParseNode dummyRoot = parser.testDummyTree();
    printTree(cout, dummyRoot);
    if (out != &cout) {
        printTree(*out, dummyRoot);
    }
    cout << "--------------------------------" << endl;

    ASTVisualizer visualizer;
    visualizer.generateDotFile(dummyRoot, "dummy_tree.dot");

    // TODO: Phase 2+ -> root = parser.parseProgram(); printTree(*out, root);


    return 0;
}
