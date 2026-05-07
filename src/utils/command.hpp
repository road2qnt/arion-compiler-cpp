#ifndef COMMAND_HPP
#define COMMAND_HPP

#include "../lexical-analysis/lexer.hpp"
#include "../lexical-analysis/token.hpp"
#include "../syntax-analysis/parser.hpp"
#include "printTree.hpp"
#include "writeFile.hpp"
#include "tokenReadWrite.hpp"

// === Fungsi Helper ===

// Menjalankan Lexical Analysis
vector<Token> runLexicalAnalysis(const string& filename) {
    Lexer lexer;
    lexer.DFA(filename);
    return lexer.getTokens();
}

// Menjalankan Syntax Analysis 
ParseNode runSyntaxAnalysis(const vector<Token>& tokens) {
    Parser parser(tokens);
    return parser.parse();
}

// Mencetak hasil Lexical Analysis ke File dan/atau terminal.
bool outputLexicalAnalysis(const vector<Token>& tokens, const string& outputPath = "") {
    printLexicalAnalysis(cout, tokens);
    if (!outputPath.empty()) {
        return printLexicalAnalysisToFile("test/milestone-1/" + outputPath, tokens);
    }
    return true;
}

// Mencetak hasil Syntax Analysis ke File dan/atau terminal.
bool outputSyntaxAnalysis(const ParseNode& root, const string& outputPath = "") {
    printTree(cout, root);
    if (!outputPath.empty()) {
        return printSyntaxAnalysisToFile("test/milestone-2/" + outputPath, root);
    }
    return true;
}

// [Helper] Fungsi untuk menerima sebuah input dibarengi dengan print pertanyaan
string askLine(const string& prompt) {
    string input;
    cout << prompt;
    getline(cin, input);
    return input;
}

// [Helper] Command yang menandakan user memilih lexical
bool isLexicalChoice(const string& choice) {
    return choice == "1" || choice == "lexical" || choice == "Lexical" || choice == "LEXICAL";
}

// [Helper] Command yang menandakan user memilih syntax
bool isSyntaxChoice(const string& choice) {
    return choice == "2" || choice == "syntax" || choice == "Syntax" || choice == "SYNTAX";
}

// [Helper] Command yang menandakan user memilih keduanya (syntax + lexical)
bool isBothChoice(const string& choice) {
    return choice == "3" || choice == "keduanya" || choice == "Keduanya" ||
           choice == "both" || choice == "Both" || choice == "BOTH";
}

// Fungsi untuk handle menu interaktif di CLI. (Aktif Jika User menjalankan program dengan "make run" saja!)
int runInteractiveMode() {
    cout << "Pilih mode:" << endl;
    cout << "1. lexical" << endl;
    cout << "2. syntax" << endl;
    cout << "3. keduanya" << endl;

    string choice = askLine("Pilihan: ");
    if (isLexicalChoice(choice)) {
        std::cout << "=== [!] Input: Source Code ===" << endl;
        std::cout << "=== [!] Ouput: File Token ===" << endl;
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
        std::cout << "=== [!] Input: File Token ===" << endl;
        std::cout << "=== [!] Ouput: Parse Tree ===" << endl;
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
        std::cout << "=== [!] Input: Source Code (belum menjadi token) -> FILE Harus berada di [test/milestone-1/] ===" << endl;
        std::cout << "=== [!] Output: Token -> FILE akan berada di [test/milestone-1/] ===" << endl;
        std::cout << "=== [!] Output: Parse Tree -> FILE akan berada di [test/milestone-2/] ===" << endl;
        string inputFile = askLine("File input source: ");
        string lexicalOutput = askLine("File output lexical (kosongkan kalau tidak mau print ke file): ");
        string syntaxOutput = askLine("File output syntax (kosongkan kalau tidak mau print ke file): ");

        vector<Token> tokens = runLexicalAnalysis("test/milestone-1/" + inputFile);
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

// Fungsi untuk handle program yang langsung berjalan tanpa input di CLI. (Aktif Jika User menjalankan program dengan "make run [format] ..." yang sudah dijelaskan di readme.md)
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