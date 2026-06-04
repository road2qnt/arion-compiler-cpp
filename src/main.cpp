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
#include "code.hpp"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc == 1) {
        return runInteractiveMode();
    }

    if (argc > 6) {
        cout << "Usage: " << argv[0] << " [source_file] [lexical_output] [syntax_output] [semantic_output] [codegen_output]" << endl;
        return 1;
    }

    return runCommandMode(argc, argv);

}
