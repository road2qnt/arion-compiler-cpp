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
    while (match(TokenType::PROGRAMSY)){
        node.children.push_back(parseSubprogramDeclaration());
    }

    return node;
}


ParseNode Parser::parseConstDeclaration(){
    // constsy + (ident + eql + constant + semicolon)+
    ParseNode node = ParseNode("const-declaration");
    // Expect contsy
    node.children.push_back(expect(TokenType::CONSTSY));
    while(match(TokenType::IDENT)){
        // Expect ident
        node.children.push_back(expect(TokenType::IDENT));
        // Expect eql
        node.children.push_back(expect(TokenType::EQL));
        // [!] Parseconstant
        node.children.push_back(parseConstant());
        // Expect semicolon
        node.children.push_back(expect(TokenType::SEMICOLON));
    }
    return node;
}

ParseNode Parser::parseTypeDeclaration(){
    // typesy + (ident + eql + type + semicolon)+
    ParseNode node = ParseNode("type-declaration");

    // Expect typesy
    node.children.push_back(expect(TokenType::TYPESY));
    while (match(TokenType::IDENT)){
        // Expect ident
        node.children.push_back(expect(TokenType::IDENT));
        // Expect eql
        node.children.push_back(expect(TokenType::EQL));
        // [!] parseType
        node.children.push_back(parseType());
        // Expect semicolon
        node.children.push_back(expect(TokenType::SEMICOLON));
    }
}
ParseNode Parser::parseVarDeclaration(){
    // varsy + (identifier-list + colon + type + semicolon)+
    ParseNode node = ParseNode("var-declaration");

    // Expect varsy
    node.children.push_back(expect(TokenType::VARSY));
    // While masih deklarasi identifier list
    while (true){
        // [!] parseIdentifierList();
        node.children.push_back(parseIdentifierList());
        // Expect colon
        node.children.push_back(expect(TokenType::COLON));
        // [!] parseType()
        node.children.push_back(parseType());
        // Expect semicolon
        node.children.push_back(expect(TokenType::SEMICOLON));
    }
}
ParseNode Parser::parseSubprogramDeclaration(){
    // procedure-declaration | function-declaration
    // While masih deklarasi procedure or function
    while (true){
        
    }
}
ParseNode Parser::parseProcedureDeclaration(){
    // proceduresy + ident + (formal-parameter-list)? + semicolon + block + semicolon
    ParseNode node = ParseNode("<procedure-declaration>");

    node.children.push_back(expect(TokenType::PROCEDURESY));
    node.children.push_back(expect(TokenType::IDENT));
    // Formal Parameter List    
    node.children.push_back(expect(TokenType::SEMICOLON));
    node.children.push_back(parseBlock());
    node.children.push_back(expect(TokenType::SEMICOLON));
}
ParseNode Parser::parseFunctionDeclaration(){
    // functionsy + ident + (formal-parameter-list)? + colon + ident + semicolon+ block + semicolon
    ParseNode node = ParseNode("<function-declaration>");

    node.children.push_back(expect(TokenType::FUNCTIONSY));
    node.children.push_back(expect(TokenType::IDENT));
    // Formal Parameter List    
    node.children.push_back(expect(TokenType::COLON));
    node.children.push_back(expect(TokenType::IDENT));
    node.children.push_back(expect(TokenType::SEMICOLON));
    node.children.push_back(parseBlock());
    node.children.push_back(expect(TokenType::SEMICOLON));
}
ParseNode Parser::parseFormalParameterList(){
    // lparent + parameter-group + (semicolon + parameter-group)* + rparent
}
ParseNode Parser::parseParameterGroup(){
    // identifier-list + colon + (ident | array-type)
}