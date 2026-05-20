#include "semanticAnalyzer.hpp"

SemanticAnalyzer::SemanticAnalyzer()
    : typeChecker(symTab), astRoot(nullptr), tokens(nullptr) {}

SemanticAnalyzer::~SemanticAnalyzer() {
    delete astRoot;
}

ProgramNode* SemanticAnalyzer::analyze(const ParseNode& parseTree, const std::vector<Token>& tok) {
    // Inisialisasi
    tokens = &tok;
    typeChecker.clearErrors();

    // Fase 1 (semanticAnalysis_AST.cpp)
    astRoot = convertProgram(parseTree);
    if (!astRoot) return nullptr;

    // Fase 2 (semanticAnalysis_DecorAST.cpp)
    visit(astRoot);

    // Return Hasil (Hasil juga bisa diambil dengan getAST() dan getSymbolTable())
    return astRoot;
}

