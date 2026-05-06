#include "parser.hpp"

namespace {
    bool isConstantValueStart(TokenType type){
        return type == TokenType::IDENT ||
               type == TokenType::INTCON ||
               type == TokenType::REALCON;
    }

    bool isConstantStart(TokenType type){
        return type == TokenType::CHARCON ||
               type == TokenType::STRING ||
               type == TokenType::PLUS ||
               type == TokenType::MINUS ||
               isConstantValueStart(type);
    }
}

ParseNode Parser::parseType(){
    /* ident |
    array-type |
    range | 
    enumerated |
    record-type
    */
    ParseNode node = ParseNode("<type>");

    if (match(TokenType::ARRAYSY)){
        node.children.push_back(parseArrayType());
    } else if (match(TokenType::LPARENT)){
        node.children.push_back(parseEnumerated());
    } else if (match(TokenType::RECORDSY)){
        node.children.push_back(parseRecordType());
    } else if (isConstantStart(peek().type)){
        int lookahead = 0;

        if (peekAhead(lookahead).type == TokenType::CHARCON ||
            peekAhead(lookahead).type == TokenType::STRING){
            lookahead++;
        } else {
            if (peekAhead(lookahead).type == TokenType::PLUS ||
                peekAhead(lookahead).type == TokenType::MINUS){
                lookahead++;
            }

            if (isConstantValueStart(peekAhead(lookahead).type)){
                lookahead++;
            }
        }

        if (peekAhead(lookahead).type == TokenType::PERIOD &&
            peekAhead(lookahead + 1).type == TokenType::PERIOD){
            node.children.push_back(parseRange());
        } else if (match(TokenType::IDENT)){
            node.children.push_back(expect(TokenType::IDENT));
        } else {
            error("unexpected token " + tokenToString(peek().type) + " in type");
        }
    } else {
        error("unexpected token " + tokenToString(peek().type) + " in type");
    }

    return node;
}
ParseNode Parser::parseArrayType(){
    // arraysy + lbrack + (range | ident) + rbrack + ofsy + type
    ParseNode node = ParseNode("<array-type>");

    node.children.push_back(expect(TokenType::ARRAYSY));
    node.children.push_back(expect(TokenType::LBRACK));

    int lookahead = 0;
    if (peekAhead(lookahead).type == TokenType::CHARCON ||
        peekAhead(lookahead).type == TokenType::STRING){
        lookahead++;
    } else {
        if (peekAhead(lookahead).type == TokenType::PLUS ||
            peekAhead(lookahead).type == TokenType::MINUS){
            lookahead++;
        }

        if (isConstantValueStart(peekAhead(lookahead).type)){
            lookahead++;
        }
    }

    if (peekAhead(lookahead).type == TokenType::PERIOD &&
        peekAhead(lookahead + 1).type == TokenType::PERIOD){
        node.children.push_back(parseRange());
    } else if (match(TokenType::IDENT)){
        node.children.push_back(expect(TokenType::IDENT));
    } else {
        error("unexpected token " + tokenToString(peek().type) + " in array-type");
    }

    node.children.push_back(expect(TokenType::RBRACK));
    node.children.push_back(expect(TokenType::OFSY));
    node.children.push_back(parseType());

    return node;
}
ParseNode Parser::parseRange(){
    // constant + period + period + constant
    ParseNode node = ParseNode("<range>");

    node.children.push_back(parseConstant());
    node.children.push_back(expect(TokenType::PERIOD));
    node.children.push_back(expect(TokenType::PERIOD));
    node.children.push_back(parseConstant());

    return node;
}
ParseNode Parser::parseEnumerated(){
    // lparent + ident + (comma + ident)* + rparent
    ParseNode node = ParseNode("<enumerated>");

    node.children.push_back(expect(TokenType::LPARENT));
    node.children.push_back(expect(TokenType::IDENT));
    while (match(TokenType::COMMA)){
        node.children.push_back(expect(TokenType::COMMA));
        node.children.push_back(expect(TokenType::IDENT));
    }
    node.children.push_back(expect(TokenType::RPARENT));

    return node;
}
ParseNode Parser::parseRecordType(){
    // recordsy + field-list + endsy
    ParseNode node = ParseNode("<record-type>");

    node.children.push_back(expect(TokenType::RECORDSY));
    node.children.push_back(parseFieldList());
    node.children.push_back(expect(TokenType::ENDSY));

    return node;
}
ParseNode Parser::parseFieldList(){
    // field-part + (semicolon + field-part)*
    ParseNode node = ParseNode("<field-list>");

    node.children.push_back(parseFieldPart());
    while (match(TokenType::SEMICOLON)){
        node.children.push_back(expect(TokenType::SEMICOLON));
        node.children.push_back(parseFieldPart());
    }

    return node;
}   
ParseNode Parser::parseFieldPart(){
    // identifier-list + colon + type
    ParseNode node = ParseNode("<field-part>");

    node.children.push_back(parseIdentifierList());
    node.children.push_back(expect(TokenType::COLON));
    node.children.push_back(parseType());

    return node;
}
