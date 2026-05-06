#include "parser.hpp"

ParseNode Parser::parseDeclarationPart(){
    ParseNode node = ParseNode("<declaration-part>");

    // Tunggu Jawaban
    node.children.push_back(parseConstDeclaration());
    node.children.push_back(parseTypeDeclaration());
    node.children.push_back(parseVarDeclaration());
    node.children.push_back(parseSubprogramDeclaration());
    return node;
}


ParseNode Parser::parseConstDeclaration(){
    // constsy + (ident + eql + constant + semicolon)+
}

ParseNode Parser::parseTypeDeclaration(){
    // typesy + (ident + eql + type + semicolon)+
}
ParseNode Parser::parseVarDeclaration(){
    // varsy + (identifier-list + colon + type + semicolon)+
}
ParseNode Parser::parseSubprogramDeclaration(){
    // procedure-declaration | function-declaration
}
ParseNode Parser::parseProcedureDeclaration(){
    // proceduresy + ident + (formal-parameter-list)? + semicolon + block + semicolon
}
ParseNode Parser::parseFunctionDeclaration(){
    // functionsy + ident + (formal-parameter-list)? + colon + ident + semicolon+ block + semicolon
}
ParseNode Parser::parseFormalParameterList(){
    // lparent + parameter-group + (semicolon + parameter-group)* + rparent
}
ParseNode Parser::parseParameterGroup(){
    // identifier-list + colon + (ident | array-type)
}