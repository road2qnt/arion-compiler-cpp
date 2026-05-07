#include "parser.hpp"

// Parse variable utama beserta akses array/field opsional.
ParseNode Parser::parseVariable(){
    // ident + (component-variable)*
    ParseNode node = ParseNode("<variable>");

    node.children.push_back(expect(TokenType::IDENT));
    while (match(TokenType::LBRACK) || match(TokenType::PERIOD)){
        node.children.push_back(parseComponentVariable());
    }

    return node;
}

// Parse komponen variable berupa index array atau field record.
ParseNode Parser::parseComponentVariable(){
    // (lbrack + index-list + rbrack) | (period + ident) 
    ParseNode node = ParseNode("<component-variable>");

    if (match(TokenType::LBRACK)){
        node.children.push_back(expect(TokenType::LBRACK));
        node.children.push_back(parseIndexList());
        node.children.push_back(expect(TokenType::RBRACK));
    } else if (match(TokenType::PERIOD)){
        node.children.push_back(expect(TokenType::PERIOD));
        node.children.push_back(expect(TokenType::IDENT));
    } else {
        node.children.push_back(error("unexpected token " + tokenToString(peek().type) + " in component-variable"));
    }

    return node;
}
