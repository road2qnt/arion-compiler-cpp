#include <iostream>
#include <fstream>
#include "lexical-analysis/lexer.hpp"
#include "lexical-analysis/token.hpp"
#include "syntax-analysis/parser.hpp"
#include "utils/printTree.hpp"
#include "utils/writeFile.hpp"
#include "utils/tokenReadWrite.hpp"
#include "utils/command.hpp"
#include "debugger/tokenDumper.hpp"
#include "debugger/astVisualizer.hpp"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc == 1) {
        return runInteractiveMode();
    }

    if (argc > 5) {
        cout << "Usage: " << argv[0] << " [source_file] [lexical_output_file] [syntax_output_file] [semantic_output_file]" << endl;
        return 1;
    }

    return runCommandMode(argc, argv);

}
