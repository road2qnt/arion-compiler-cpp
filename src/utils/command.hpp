#ifndef COMMAND_HPP
#define COMMAND_HPP

#include "../lexical-analysis/lexer.hpp"
#include "../lexical-analysis/token.hpp"
#include "../syntax-analysis/parser.hpp"
#include "printTree.hpp"
#include "writeFile.hpp"
#include "tokenReadWrite.hpp"

vector<Token> runLexicalAnalysis(const string& filename) {
    Lexer lexer;
    lexer.DFA(filename);
    return lexer.getTokens();
}

ParseNode runSyntaxAnalysis(const vector<Token>& tokens) {
    Parser parser(tokens);
    return parser.parse();
}

bool outputLexicalAnalysis(const vector<Token>& tokens, const string& outputPath = "") {
    printLexicalAnalysis(cout, tokens);
    if (!outputPath.empty()) {
        return printLexicalAnalysisToFile(outputPath, tokens);
    }
    return true;
}

bool outputSyntaxAnalysis(const ParseNode& root, const string& outputPath = "") {
    printTree(cout, root);
    if (!outputPath.empty()) {
        return printSyntaxAnalysisToFile(outputPath, root);
    }
    return true;
}

string askLine(const string& prompt) {
    string input;
    cout << prompt;
    getline(cin, input);
    return input;
}

bool isLexicalChoice(const string& choice) {
    return choice == "1" || choice == "lexical" || choice == "Lexical" || choice == "LEXICAL";
}

bool isSyntaxChoice(const string& choice) {
    return choice == "2" || choice == "syntax" || choice == "Syntax" || choice == "SYNTAX";
}

bool isBothChoice(const string& choice) {
    return choice == "3" || choice == "keduanya" || choice == "Keduanya" ||
           choice == "both" || choice == "Both" || choice == "BOTH";
}

int runInteractiveMode() {
    cout << "Pilih mode:" << endl;
    cout << "1. lexical" << endl;
    cout << "2. syntax" << endl;
    cout << "3. keduanya" << endl;

    string choice = askLine("Pilihan: ");
    if (isLexicalChoice(choice)) {
        std::cout << "=== Input dan Output file berada di folder milestone-1 [test/milestone-1] ===" << endl;
        string inputFile = askLine("File input source: ");
        string outputFile = askLine("File output lexical (kosongkan kalau tidak mau print ke file): ");

        vector<Token> tokens = runLexicalAnalysis("test/milestone-1/" + inputFile);
        if (!outputLexicalAnalysis(tokens, outputFile)) {
            return 1;
        }
        return 0;
    } 

    if (isSyntaxChoice(choice)) {
        std::cout << "=== Input dan Output file berada di folder milestone-2 [test/milestone-2] ===" << endl;
        string inputFile = askLine("File input token hasil lexical: ");
        string outputFile = askLine("File output syntax (kosongkan kalau tidak mau print ke file): ");

        TokenReadWrite reader("test/milestone-2/" + inputFile);
        vector<Token> tokens = reader.parseToVector();
        ParseNode root = runSyntaxAnalysis(tokens);
        if (!outputSyntaxAnalysis(root, outputFile)) {
            return 1;
        }
        return 0;
    }

    if (isBothChoice(choice)) {
        string inputFile = askLine("File input source: ");
        string lexicalOutput = askLine("File output lexical (kosongkan kalau tidak mau print ke file): ");
        string syntaxOutput = askLine("File output syntax (kosongkan kalau tidak mau print ke file): ");

        vector<Token> tokens = runLexicalAnalysis(inputFile);
        if (!outputLexicalAnalysis(tokens, lexicalOutput)) {
            return 1;
        }

        ParseNode root = runSyntaxAnalysis(tokens);
        if (!outputSyntaxAnalysis(root, syntaxOutput)) {
            return 1;
        }
        return 0;
    }

    cout << "[!] Pilihan tidak dikenali." << endl;
    return 1;
}

int runCommandMode(int argc, char* argv[]) {
    string inputFile = argv[1];
    string lexicalOutput = argc >= 3 ? argv[2] : "";
    string syntaxOutput = argc >= 4 ? argv[3] : "";

    vector<Token> tokens = runLexicalAnalysis(inputFile);
    if (!outputLexicalAnalysis(tokens, lexicalOutput)) {
        return 1;
    }

    ParseNode root = runSyntaxAnalysis(tokens);
    if (!outputSyntaxAnalysis(root, syntaxOutput)) {
        return 1;
    }

    return 0;
}

#endif