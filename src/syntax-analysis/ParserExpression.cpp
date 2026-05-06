#include "parser.hpp"

namespace {
    bool isRelationalOperator(TokenType type){
        return type == TokenType::EQL ||
               type == TokenType::NEQ ||
               type == TokenType::GTR ||
               type == TokenType::GEQ ||
               type == TokenType::LSS ||
               type == TokenType::LEQ;
    }

    bool isAdditiveOperator(TokenType type){
        return type == TokenType::PLUS ||
               type == TokenType::MINUS ||
               type == TokenType::ORSY;
    }

    bool isMultiplicativeOperator(TokenType type){
        return type == TokenType::TIMES ||
               type == TokenType::RDIV ||
               type == TokenType::IDIV ||
               type == TokenType::IMOD ||
               type == TokenType::ANDSY;
    }
}

ParseNode Parser::parseExpression(){
    // simple-expression (relational-operator + simple-expression)?
    ParseNode node = ParseNode("<expression>");

    node.children.push_back(parseSimpleExpression());
    if (isRelationalOperator(peek().type)){
        node.children.push_back(parseRelationalOperator());
        node.children.push_back(parseSimpleExpression());
    }

    return node;
}
ParseNode Parser::parseSimpleExpression(){
    // (plus | minus)? term (additive-operator + term)*
    ParseNode node = ParseNode("<simple-expression>");

    if (match(TokenType::PLUS)){
        node.children.push_back(expect(TokenType::PLUS));
    } else if (match(TokenType::MINUS)){
        node.children.push_back(expect(TokenType::MINUS));
    }

    node.children.push_back(parseTerm());
    while (isAdditiveOperator(peek().type)){
        node.children.push_back(parseAdditiveOperator());
        node.children.push_back(parseTerm());
    }

    return node;
}
ParseNode Parser::parseTerm(){
    // factor (multiplicative-operator + factor)*
    ParseNode node = ParseNode("<term>");

    node.children.push_back(parseFactor());
    while (isMultiplicativeOperator(peek().type)){
        node.children.push_back(parseMultiplicativeOperator());
        node.children.push_back(parseFactor());
    }

    return node;
}
ParseNode Parser::parseFactor(){
    // ident | intcon | realcon | charcon | string | (lparent + expression + rparent) | (notsy + factor) | procedure/function-call | variable
    ParseNode node = ParseNode("<factor>");

    if (match(TokenType::IDENT)){
        if (peekAhead(1).type == TokenType::LPARENT){
            node.children.push_back(parseProcedureFunctionCall());
        } else {
            node.children.push_back(parseVariable());
        }
    } else if (match(TokenType::INTCON)){
        node.children.push_back(expect(TokenType::INTCON));
    } else if (match(TokenType::REALCON)){
        node.children.push_back(expect(TokenType::REALCON));
    } else if (match(TokenType::CHARCON)){
        node.children.push_back(expect(TokenType::CHARCON));
    } else if (match(TokenType::STRING)){
        node.children.push_back(expect(TokenType::STRING));
    } else if (match(TokenType::LPARENT)){
        node.children.push_back(expect(TokenType::LPARENT));
        node.children.push_back(parseExpression());
        node.children.push_back(expect(TokenType::RPARENT));
    } else if (match(TokenType::NOTSY)){
        node.children.push_back(expect(TokenType::NOTSY));
        node.children.push_back(parseFactor());
    } else {
        error("unexpected token " + tokenToString(peek().type) + " in factor");
    }

    return node;
}
ParseNode Parser::parseProcedureFunctionCall(){
    // ident + (lparent + parameter-list + rparent)
    ParseNode node = ParseNode("<procedure-function-call>");

    node.children.push_back(expect(TokenType::IDENT));
    node.children.push_back(expect(TokenType::LPARENT));
    node.children.push_back(parseParameterList());
    node.children.push_back(expect(TokenType::RPARENT));

    return node;
}
ParseNode Parser::parseParameterList(){
    // expression (comma + expression)*
    ParseNode node = ParseNode("<parameter-list>");

    node.children.push_back(parseExpression());
    while (match(TokenType::COMMA)){
        node.children.push_back(expect(TokenType::COMMA));
        node.children.push_back(parseExpression());
    }

    return node;
}
