#include <iostream>
#include <fstream>
#include "lexical-analysis/lexer.hpp"
#include "lexical-analysis/token.hpp"
#include "syntax-analysis/parser.hpp"
#include "utils/printTree.hpp"
#include "utils/writeFile.hpp"
#include "debugger/tokenDumper.hpp"
#include "debugger/astVisualizer.hpp"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " <source_file> [lexical_output_file] [syntax_output_file]" << endl;
        return 1;
    }
    string filename = argv[1];

    // Lexical Analysis
    Lexer lexer;
    lexer.DFA(filename);
    vector<Token> tokens = lexer.getTokens();

    // Lexical Analysis Output
    if (argc >= 3) {
        if (!printLexicalAnalysisToFile(argv[2], tokens)) {
            return 1;
        }
    } else {
        printLexicalAnalysis(cout, tokens);
    }

    // Syntax Analysis Output
    Parser par = Parser(tokens);
    ParseNode root = par.parse(); 

    if (argc >= 4) {
        if (!printSyntaxAnalysisToFile(argv[3], root)) {
            return 1;
        }
    } else {
        printTree(cout, root);
    }

    /* [debugger]
    dumpTokens(tokens);
    */

    /*
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
    */

    return 0;
}
