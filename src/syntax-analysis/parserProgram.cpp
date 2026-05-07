#include "parser.hpp"

// Parse header program: keyword program, nama, dan semicolon.
ParseNode Parser::parseProgramHeader(){
    ParseNode node = ParseNode("<Program-header>");
    node.children.push_back(expect(TokenType::PROGRAMSY));
    node.children.push_back(expect(TokenType::IDENT));
    node.children.push_back(expect(TokenType::SEMICOLON));
    return node;
}

// Parse block yang berisi declaration-part dan compound-statement.
ParseNode Parser::parseBlock(){
    // declaration-part + compound-statement
    ParseNode node = ParseNode("<Block>");
    node.children.push_back(parseDeclarationPart());
    node.children.push_back(parseCompoundStatement());

    return node;
}
