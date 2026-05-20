#include "parser.hpp"
#include "../debugger/parserTrace.hpp"
#include <iostream>

using namespace std;

Parser::Parser(const vector<Token>& tokenList) : pos(0) {
    for (const Token& token : tokenList) {
        if (token.type != TokenType::COMMENT) {
            tokens.push_back(token);
        }
    }
}

Token Parser::peek() const {
    if (pos < (int)tokens.size()) {
        return tokens[pos];
    }
    return Token(TokenType::ERROR, "EOF");
}

Token Parser::peekAhead(int n) const {
    if (pos + n < (int)tokens.size()) {
        return tokens[pos + n];
    }
    return Token(TokenType::ERROR, "EOF");
}

void Parser::advance() {
    if (pos < (int)tokens.size()) {
        // TRACE_MSG("Maju 1 token dari: " + tokenToLabel(tokens[pos]));
        pos++;
    }
}

bool Parser::match(TokenType type) const {
    return peek().type == type;
}

ParseNode Parser::expect(TokenType type) {
    if (match(type)) {
        ParseNode node(tokenToLabel(peek()),true);
        node.line = peek().line;
        node.column = peek().column;
        advance();
        return node;
    } else {
        string expectedStr = tokenToString(type);
        string foundStr = tokenToString(peek().type);
        return error("unexpected token " + foundStr + ", expected " + expectedStr);
    }
}

ParseNode Parser::error(const string& msg) const {
    cout << "Syntax error: " << msg << endl;
    ParseNode node("error", true);
    node.line = peek().line;
    node.column = peek().column;
    return node;
}

string Parser::tokenToLabel(const Token& t) const {
    string typeStr = tokenToString(t.type);
    if (!t.value.empty()) {
        return typeStr + "(" + t.value + ")";
    }
    return typeStr;
}

ParseNode Parser::parse(){
    ParseNode result = ParseNode("<Program>");
    result.children.push_back(parseProgramHeader());
    result.children.push_back(parseDeclarationPart());
    result.children.push_back(parseCompoundStatement());
    result.children.push_back(expect(TokenType::PERIOD));

    return result;
}
ParseNode Parser::testDummyTree() {
    TRACE(); // CONTOH PENGGUNAAN TRACE (DEBUGGER)
    
    ParseNode root("<program>");
    
    ParseNode header("<program-header>");
    header.children.push_back(ParseNode("programsy", true));
    header.children.push_back(ParseNode("ident(Hello)", true));
    header.children.push_back(ParseNode("semicolon", true));
    
    root.children.push_back(header);
    
    // Add other placeholders to match spec output structure
    ParseNode declPart("<declaration-part>");
    root.children.push_back(declPart);
    
    ParseNode compStmt("<compound-statement>");
    root.children.push_back(compStmt);
    
    root.children.push_back(ParseNode("period", true));

    return root;
}
