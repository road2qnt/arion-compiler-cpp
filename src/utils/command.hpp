#ifndef COMMAND_HPP
#define COMMAND_HPP

#include "../lexical-analysis/lexer.hpp"
#include "../lexical-analysis/token.hpp"
#include "../syntax-analysis/parser.hpp"
#include "../semantic-analysis/semanticAnalyzer.hpp"
#include "../code.hpp"
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

void runSemanticAnalysis(SemanticAnalyzer& analyzer, const ParseNode& root, const vector<Token>& tokens) {
    analyzer.analyze(root, tokens);
}

bool outputLexicalAnalysis(const vector<Token>& tokens, const string& outputPath = "") {
    printLexicalAnalysis(cout, tokens);
    if (!outputPath.empty()) {
        return printLexicalAnalysisToFile("test/milestone-1/" + outputPath, tokens);
    }
    return true;
}

bool outputSyntaxAnalysis(const ParseNode& root, const string& outputPath = "") {
    printTree(cout, root);
    if (!outputPath.empty()) {
        return printSyntaxAnalysisToFile("test/milestone-2/" + outputPath, root);
    }
    return true;
}

bool outputSemanticAnalysis(const SemanticAnalyzer& analyzer, const string& outputPath = "") {
    printSemanticAnalysis(cout, analyzer);
    if (!outputPath.empty()) {
        return printSemanticAnalysisToFile("test/milestone-3/" + outputPath, analyzer);
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

bool isSemanticChoice(const string& choice) {
    return choice == "4" || choice == "semantic" || choice == "Semantic" || choice == "SEMANTIC";
}

bool isCodegenChoice(const string& choice) {
    return choice == "5" || choice == "codegen" || choice == "Codegen" || choice == "CODEGEN" ||
           choice == "milestone4" || choice == "Milestone4" || choice == "MILESTONE4";
}

bool isAllChoice(const string& choice) {
    return choice == "6" || choice == "semua" || choice == "Semua" || choice == "SEMUA" ||
           choice == "all" || choice == "All" || choice == "ALL";
}

int runCodegenFlow(const string& inputPrefix) {
    vector<Token> tokens = runLexicalAnalysis("test/milestone-1/" + inputPrefix);
    ParseNode root = runSyntaxAnalysis(tokens);
    SemanticAnalyzer analyzer;
    runSemanticAnalysis(analyzer, root, tokens);

    if (analyzer.hasErrors()) {
        cout << "\n[!] Semantic analysis failed. Cannot proceed to code generation." << endl;
        analyzer.printResults(cout);
        return 0;
    }

    codeOutput output = runCode(analyzer);

    cout << "\n========== Intermediate Code ==========" << endl;
    printIntermediateCode(cout, output.codegen.instructions);

    if (!output.codegen.ok()) {
        cout << "\n[!] Code generation errors:" << endl;
        for (const auto& err : output.codegen.errors) {
            cout << "  Line " << err.line << ": " << err.message << endl;
        }
        return 0;
    }

    cout << "\n========== Runtime Output ==========" << endl;
    cout << output.runtime.output;

    if (!output.runtime.ok()) {
        cout << "\n[!] Runtime errors:" << endl;
        for (const auto& err : output.runtime.errors) {
            cout << "  IP=" << err.instructionPointer << ": " << err.message << endl;
        }
    }

    return 0;
}

int runCodegenOutput(const string& inputPrefix, const string& outputPath) {
    vector<Token> tokens = runLexicalAnalysis("test/milestone-1/" + inputPrefix);
    ParseNode root = runSyntaxAnalysis(tokens);
    SemanticAnalyzer analyzer;
    runSemanticAnalysis(analyzer, root, tokens);

    if (analyzer.hasErrors()) {
        cout << "\n[!] Semantic analysis failed." << endl;
        printSemanticAnalysis(cout, analyzer);
        return 0;
    }

    codeOutput output = runCode(analyzer);

    if (!output.codegen.ok()) {
        cout << "\n[!] Code generation errors:" << endl;
        for (const auto& err : output.codegen.errors) {
            cout << "  Line " << err.line << ": " << err.message << endl;
        }
        return 0;
    }

    if (!outputPath.empty()) {
        if (!printCodegenOutputToFile("test/milestone-4/" + outputPath, output.codegen.instructions)) {
            return 1;
        }
    }

    cout << "\n========== Intermediate Code ==========" << endl;
    printIntermediateCode(cout, output.codegen.instructions);

    cout << "\n========== Runtime Output ==========" << endl;
    cout << output.runtime.output;

    if (!output.runtime.ok()) {
        cout << "\n[!] Runtime errors:" << endl;
        for (const auto& err : output.runtime.errors) {
            cout << "  IP=" << err.instructionPointer << ": " << err.message << endl;
        }
    }

    return 0;
}

int runInteractiveMode() {
    cout << "Pilih mode:" << endl;
    cout << "1. Lexical ONLY" << endl;
    cout << "2. Syntax ONLY" << endl;
    cout << "3. Lexical + Syntax Analysis" << endl;
    cout << "4. Semantic" << endl;
    cout << "5. Codegen + Interpreter (Milestone 4)" << endl;
    cout << "6. Semua" << endl;

    string choice = askLine("Pilihan: ");
    if (isLexicalChoice(choice)) {
        std::cout << "=== [!] Input: Source Code ===" << endl;
        std::cout << "=== [!] Output: File Token ===" << endl;
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
        std::cout << "=== [!] Output: Parse Tree ===" << endl;
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

    if (isSemanticChoice(choice)) {
        std::cout << "=== [!] Input: Source Code ===" << endl;
        std::cout << "=== [!] Output: Decorated AST, Symbol Table, Semantic Errors ===" << endl;
        std::cout << "=== Input file berada di folder milestone-1 [test/milestone-1] ===" << endl;
        std::cout << "=== Output file berada di folder milestone-3 [test/milestone-3] ===" << endl;
        string inputFile = askLine("File input source: ");
        string outputFile = askLine("File output semantic (kosongkan kalau tidak mau print ke file): ");

        vector<Token> tokens = runLexicalAnalysis("test/milestone-1/" + inputFile);
        ParseNode root = runSyntaxAnalysis(tokens);
        SemanticAnalyzer analyzer;
        runSemanticAnalysis(analyzer, root, tokens);
        if (!outputSemanticAnalysis(analyzer, outputFile)) {
            return 1;
        }
        return 0;
    }

    if (isCodegenChoice(choice)) {
        std::cout << "=== [!] Input: Source Code ===" << endl;
        std::cout << "=== [!] Output: Intermediate Code + Runtime Output ===" << endl;
        std::cout << "=== Input file berada di folder milestone-1 [test/milestone-1] ===" << endl;
        std::cout << "=== Output file berada di folder milestone-4 [test/milestone-4] ===" << endl;
        string inputFile = askLine("File input source: ");
        string outputFile = askLine("File output intermediate code (kosongkan kalau tidak mau print ke file): ");

        return runCodegenOutput(inputFile, outputFile);
    }

    if (isAllChoice(choice)) {
        std::cout << "=== [!] Input: Source Code (belum menjadi token) -> FILE Harus berada di [test/milestone-1/] ===" << endl;
        std::cout << "=== [!] Output: Token -> FILE akan berada di [test/milestone-1/] ===" << endl;
        std::cout << "=== [!] Output: Parse Tree -> FILE akan berada di [test/milestone-2/] ===" << endl;
        std::cout << "=== [!] Output: Decorated AST -> FILE akan berada di [test/milestone-3/] ===" << endl;
        std::cout << "=== [!] Output: Intermediate Code + Runtime -> akan di print ke console ===" << endl;
        string inputFile = askLine("File input source: ");
        string lexicalOutput = askLine("File output lexical (kosongkan kalau tidak mau print ke file): ");
        string syntaxOutput = askLine("File output syntax (kosongkan kalau tidak mau print ke file): ");
        string semanticOutput = askLine("File output semantic (kosongkan kalau tidak mau print ke file): ");

        vector<Token> tokens = runLexicalAnalysis("test/milestone-1/" + inputFile);
        if (!outputLexicalAnalysis(tokens, lexicalOutput)) {
            return 1;
        }

        ParseNode root = runSyntaxAnalysis(tokens);
        if (!outputSyntaxAnalysis(root, syntaxOutput)) {
            return 1;
        }

        SemanticAnalyzer analyzer;
        runSemanticAnalysis(analyzer, root, tokens);
        if (!outputSemanticAnalysis(analyzer, semanticOutput)) {
            return 1;
        }

        if (!analyzer.hasErrors()) {
            cout << "\n========== Intermediate Code ==========" << endl;
            codeOutput co = runCode(analyzer);
            printIntermediateCode(cout, co.codegen.instructions);
            cout << "\n========== Runtime Output ==========" << endl;
            cout << co.runtime.output;
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
    string semanticOutput = argc >= 5 ? argv[4] : "";
    string codegenOutput = argc >= 6 ? argv[5] : "";

    vector<Token> tokens = runLexicalAnalysis(inputFile);
    if (!outputLexicalAnalysis(tokens, lexicalOutput)) {
        return 1;
    }

    ParseNode root = runSyntaxAnalysis(tokens);
    if (!outputSyntaxAnalysis(root, syntaxOutput)) {
        return 1;
    }

    SemanticAnalyzer analyzer;
    runSemanticAnalysis(analyzer, root, tokens);
    if (!outputSemanticAnalysis(analyzer, semanticOutput)) {
        return 1;
    }

    if (!analyzer.hasErrors() && !codegenOutput.empty()) {
        codeOutput co = runCode(analyzer);
        printCodegenOutputToFile("test/milestone-4/" + codegenOutput, co.codegen.instructions);
        cout << "\n========== Runtime Output ==========" << endl;
        cout << co.runtime.output;
    } else if (!analyzer.hasErrors()) {
        codeOutput co = runCode(analyzer);
        cout << "\n========== Intermediate Code ==========" << endl;
        printIntermediateCode(cout, co.codegen.instructions);
        cout << "\n========== Runtime Output ==========" << endl;
        cout << co.runtime.output;
    }

    return 0;
}

#endif
