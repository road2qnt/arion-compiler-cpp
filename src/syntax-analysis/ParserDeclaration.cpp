#include "parser.hpp"

ParseNode Parser::parseDeclarationPart(){
    ParseNode node = ParseNode("<declaration-part>");

    while (match(TokenType::CONSTSY)){
        node.children.push_back(parseConstDeclaration());
    }
    while (match(TokenType::TYPESY)){
        node.children.push_back(parseTypeDeclaration());
    }
    while (match(TokenType::VARSY)){
        node.children.push_back(parseVarDeclaration());
    }
    while (match(TokenType::PROCEDURESY) || match(TokenType::FUNCTIONSY)){
        node.children.push_back(parseSubprogramDeclaration());
    }

    return node;
}

ParseNode Parser::parseConstDeclaration(){
    // constsy + (ident + eql + constant + semicolon)+
    ParseNode node = ParseNode("<const-declaration>");

    node.children.push_back(expect(TokenType::CONSTSY));
    if (!match(TokenType::IDENT)){
        node.children.push_back(error("unexpected token " + tokenToString(peek().type) + " in const-declaration"));
    }

    while (match(TokenType::IDENT)){
        node.children.push_back(expect(TokenType::IDENT));
        node.children.push_back(expect(TokenType::EQL));
        node.children.push_back(parseConstant());
        node.children.push_back(expect(TokenType::SEMICOLON));
    }

    return node;
}

ParseNode Parser::parseTypeDeclaration(){
    // typesy + (ident + eql + type + semicolon)+
    ParseNode node = ParseNode("<type-declaration>");

    node.children.push_back(expect(TokenType::TYPESY));
    if (!match(TokenType::IDENT)){
        node.children.push_back(error("unexpected token " + tokenToString(peek().type) + " in type-declaration"));
    }

    while (match(TokenType::IDENT)){
        node.children.push_back(expect(TokenType::IDENT));
        node.children.push_back(expect(TokenType::EQL));
        node.children.push_back(parseType());
        node.children.push_back(expect(TokenType::SEMICOLON));
    }

    return node;
}
ParseNode Parser::parseVarDeclaration(){
    // varsy + (identifier-list + colon + type + semicolon)+
    ParseNode node = ParseNode("<var-declaration>");

    node.children.push_back(expect(TokenType::VARSY));
    if (!match(TokenType::IDENT)){
        node.children.push_back(error("unexpected token " + tokenToString(peek().type) + " in var-declaration"));
    }

    while (match(TokenType::IDENT)){
        node.children.push_back(parseIdentifierList());
        node.children.push_back(expect(TokenType::COLON));
        node.children.push_back(parseType());
        node.children.push_back(expect(TokenType::SEMICOLON));
    }

    return node;
}
ParseNode Parser::parseSubprogramDeclaration(){
    // procedure-declaration | function-declaration
    ParseNode node = ParseNode("<subprogram-declaration>");

    if (match(TokenType::PROCEDURESY)){
        node.children.push_back(parseProcedureDeclaration());
    } else if (match(TokenType::FUNCTIONSY)){
        node.children.push_back(parseFunctionDeclaration());
    } else {
        node.children.push_back(error("unexpected token " + tokenToString(peek().type) + " in subprogram-declaration"));
    }

    return node;
}
ParseNode Parser::parseProcedureDeclaration(){
    // proceduresy + ident + (formal-parameter-list)? + semicolon + block + semicolon
    ParseNode node = ParseNode("<procedure-declaration>");

    node.children.push_back(expect(TokenType::PROCEDURESY));
    node.children.push_back(expect(TokenType::IDENT));
    if (match(TokenType::LPARENT)){
        node.children.push_back(parseFormalParameterList());
    }
    node.children.push_back(expect(TokenType::SEMICOLON));
    node.children.push_back(parseBlock());
    node.children.push_back(expect(TokenType::SEMICOLON));

    return node;
}
ParseNode Parser::parseFunctionDeclaration(){
    // functionsy + ident + (formal-parameter-list)? + colon + ident + semicolon+ block + semicolon
    ParseNode node = ParseNode("<function-declaration>");

    node.children.push_back(expect(TokenType::FUNCTIONSY));
    node.children.push_back(expect(TokenType::IDENT));
    if (match(TokenType::LPARENT)){
        node.children.push_back(parseFormalParameterList());
    }
    node.children.push_back(expect(TokenType::COLON));
    node.children.push_back(expect(TokenType::IDENT));
    node.children.push_back(expect(TokenType::SEMICOLON));
    node.children.push_back(parseBlock());
    node.children.push_back(expect(TokenType::SEMICOLON));

    return node;
}
ParseNode Parser::parseFormalParameterList(){
    // lparent + parameter-group + (semicolon + parameter-group)* + rparent
    ParseNode node = ParseNode("<formal-parameter-list>");

    node.children.push_back(expect(TokenType::LPARENT));
    node.children.push_back(parseParameterGroup());
    while (match(TokenType::SEMICOLON)){
        node.children.push_back(expect(TokenType::SEMICOLON));
        node.children.push_back(parseParameterGroup());
    }
    node.children.push_back(expect(TokenType::RPARENT));

    return node;
}
ParseNode Parser::parseParameterGroup(){
    // (varsy)? + identifier-list + colon + (ident | array-type)
    ParseNode node = ParseNode("<parameter-group>");

    if (match(TokenType::VARSY)){
        node.children.push_back(expect(TokenType::VARSY));
    }
    node.children.push_back(parseIdentifierList());
    node.children.push_back(expect(TokenType::COLON));
    if (match(TokenType::IDENT)){
        node.children.push_back(expect(TokenType::IDENT));
    } else if (match(TokenType::ARRAYSY)){
        node.children.push_back(parseArrayType());
    } else {
        node.children.push_back(error("unexpected token " + tokenToString(peek().type) + " in parameter-group"));
    }

    return node;
}
