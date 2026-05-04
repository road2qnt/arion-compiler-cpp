#ifndef PARSER_HPP
#define PARSER_HPP

#include <vector>
#include <string>
#include "../lexical-analysis/token.hpp"
#include "parseTreeNode.hpp"

class Parser {
private:
    std::vector<Token> tokens;
    int pos;

    Token peek() const;
    Token peekAhead(int n) const;
    void advance();
    bool match(TokenType type) const;
    void expect(TokenType type);
    void error(const std::string& msg) const;

    std::string tokenToLabel(const Token& t) const;

public:
    Parser(const std::vector<Token>& tokenList);
    
    ParseNode testDummyTree();
};

#endif
