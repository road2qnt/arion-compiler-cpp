#ifndef SEMANTICANALYZER_HPP
#define SEMANTICANALYZER_HPP

#include <vector>
#include <string>
#include <iostream>
#include "../lexical-analysis/token.hpp"
#include "../syntax-analysis/parseTreeNode.hpp"
#include "astNode.hpp"
#include "symbolTable.hpp"
#include "typeChecker.hpp"

class SemanticAnalyzer {
public:
    SemanticAnalyzer();
    ~SemanticAnalyzer();

    // Main entry point: convert parse tree -> AST -> decorate
    ProgramNode* analyze(const ParseNode& parseTree, const std::vector<Token>& tokens);

    // Get results
    SymbolTable& getSymbolTable() { return symTab; }
    TypeChecker& getTypeChecker() { return typeChecker; }
    ProgramNode* getAST() { return astRoot; }

    // Output
    void printDecoratedAST(std::ostream& os, ASTNode* node, int depth = 0) const;
    void printResults(std::ostream& os) const;
    bool hasErrors() const { return typeChecker.hasErrors(); }

private:
    SymbolTable symTab;
    TypeChecker typeChecker;
    ProgramNode* astRoot;
    const std::vector<Token>* tokens;

    ProgramNode* convertProgram(const ParseNode& parseTree);
    DeclarationsNode* convertDeclarationPart(const ParseNode& node);
    ASTNode* convertDeclaration(const ParseNode& node);
    std::vector<DeclNode*> convertVarDecls(const ParseNode& node);
    std::vector<DeclNode*> convertConstDecls(const ParseNode& node);
    std::vector<DeclNode*> convertTypeDecls(const ParseNode& node);
    SubprogramDeclNode* convertSubprogramDecl(const ParseNode& node);

    TypeNode* convertType(const ParseNode& node);

    ASTNode* convertCompoundStatement(const ParseNode& node);
    ASTNode* convertStatementList(const ParseNode& node);
    ASTNode* convertStatement(const ParseNode& node);
    ASTNode* convertAssignmentStatement(const ParseNode& node);
    ASTNode* convertIfStatement(const ParseNode& node);
    ASTNode* convertWhileStatement(const ParseNode& node);
    ASTNode* convertRepeatStatement(const ParseNode& node);
    ASTNode* convertForStatement(const ParseNode& node);
    ASTNode* convertCaseStatement(const ParseNode& node);
    ASTNode* convertProcedureFunctionCall(const ParseNode& node);

    ASTNode* convertExpression(const ParseNode& node);
    ASTNode* convertSimpleExpression(const ParseNode& node);
    ASTNode* convertTerm(const ParseNode& node);
    ASTNode* convertFactor(const ParseNode& node);
    ASTNode* convertVariable(const ParseNode& node);
    ASTNode* convertConstant(const ParseNode& node);

    void visit(ASTNode* node);
    void visitProgram(ProgramNode* node);
    void visitDeclarations(DeclarationsNode* node);
    void visitVarDecl(VarDeclNode* node);
    void visitConstDecl(ConstDeclNode* node);
    void visitTypeDecl(TypeDeclNode* node);
    void visitSubprogramDecl(SubprogramDeclNode* node);
    void visitAssign(AssignNode* node);
    void visitBinOp(BinOpNode* node);
    void visitUnaryOp(UnaryOpNode* node);
    void visitIf(IfNode* node);
    void visitWhile(WhileNode* node);
    void visitRepeat(RepeatNode* node);
    void visitFor(ForNode* node);
    void visitCase(CaseNode* node);
    void visitProcCall(ProcCallNode* node);
    void visitVar(VarNode* node);

    int resolveType(const std::string& typeName) const;
    std::string getTokenValue(const ParseNode& node) const;
    int findLineForNode(const ParseNode& node) const;
    bool isTokenNode(const ParseNode& node, const std::string& tokenType) const;
    ParseNode findChild(const ParseNode& node, const std::string& label) const;

    struct ResolvedType {
        int typeCode;
        int ref;
        int size;
        ResolvedType() : typeCode(TYPE_VOID), ref(0), size(0) {}
        ResolvedType(int t, int r, int s) : typeCode(t), ref(r), size(s) {}
    };
    ResolvedType resolveTypeNode(TypeNode* tn);
    ResolvedType resolveArrayType(ArrayTypeNode* arr);
    ResolvedType resolveRecordType(RecordTypeNode* rec);
    ResolvedType resolveRangeType(RangeTypeNode* rng);
    ResolvedType resolveEnumType(EnumeratedTypeNode* en);

    int constNodeOrdinalValue(ASTNode* c, int& outType) const;
    int sizeOfPrimitive(int typeCode) const;
};

#endif // SEMANTICANALYZER_HPP
