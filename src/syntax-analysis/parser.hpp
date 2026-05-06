#ifndef PARSER_HPP
#define PARSER_HPP

#include <vector>
#include <string>
#include "../lexical-analysis/token.hpp"
#include "parseTreeNode.hpp"

class Parser {
public:
    Parser(const std::vector<Token>& tokenList);
    ParseNode parse();

    // Debugging
    ParseNode testDummyTree();
private:
    // Atribut
    std::vector<Token> tokens;
    int pos;

    // Private Method
    Token peek() const;
    Token peekAhead(int n) const;
    void advance();
    bool match(TokenType type) const;
    ParseNode expect(TokenType type);
    void error(const std::string& msg) const;

    std::string tokenToLabel(const Token& t) const;

    // Deklarasi Fungsi Parser (Spesifik)
    // 1. Header (Program) [parserProgram.cpp]
    ParseNode parseProgramHeader();
    ParseNode parseBlock();

    // 2. Declaration [parserDeclaration.cpp]
    ParseNode parseDeclarationPart();
    ParseNode parseConstDeclaration();
    ParseNode parseTypeDeclaration();
    ParseNode parseVarDeclaration();
    ParseNode parseSubprogramDeclaration();
    ParseNode parseProcedureDeclaration();
    ParseNode parseFunctionDeclaration();
    ParseNode parseFormalParameterList();
    ParseNode parseParameterGroup();

    // 3. Types [parserType.cpp]
    ParseNode parseType();
    ParseNode parseArrayType();
    ParseNode parseRange();
    ParseNode parseEnumerated();
    ParseNode parseRecordType();
    ParseNode parseFieldList();
    ParseNode parseFieldPart();

    // 4. Helper [parserHelper.cpp]
    ParseNode parseIdentifierList();
    ParseNode parseConstant();
    ParseNode parseIndexList();
    ParseNode parseRelationalOperator();
    ParseNode parseAdditiveOperator();
    ParseNode parseMultiplicativeOperator();

    // 5. Statements [parserStatement.cpp]
    ParseNode parseCompoundStatement();
    ParseNode parseStatementList();
    ParseNode parseStatement();
    ParseNode parseAssignmentStatement();
    ParseNode parseIfStatement();
    ParseNode parseCaseStatement();
    ParseNode parseCaseBlock();
    ParseNode parseWhileStatement();
    ParseNode parseRepeatStatement();
    ParseNode parseForStatement();

    // 6. Variable [parserVariable.cpp]
    ParseNode parseVariable();
    ParseNode parseComponentVariable();

    // 7. Expression [parserExpression.cpp]
    ParseNode parseExpression();
    ParseNode parseSimpleExpression();
    ParseNode parseTerm();
    ParseNode parseFactor();
    ParseNode parseProcedureFunctionCall();
    ParseNode parseParameterList();
};

#endif
