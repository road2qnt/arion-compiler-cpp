#ifndef LEXER_H
#define LEXER_H
#include "token.hpp"
#include <iostream>
#include <fstream>
#include <vector>
enum state{
    STATE_START, 
    STATE_KID, 
    STATE_NUM, 
    STATE_DOT, 
    STATE_FLO, 
    STATE_CS1, 
    STATE_CS2, 
    STATE_CV, 
    STATE_STR,
    STATE_PLS, 
    STATE_MIN, 
    STATE_TMS, 
    STATE_DIV, 
    STATE_EQ1, 
    STATE_EQ2, 
    STATE_LSS, 
    STATE_NEQ, 
    STATE_LEQ, 
    STATE_GTR, 
    STATE_GEQ, 
    STATE_COM, 
    STATE_COL, 
    STATE_BEC, 
    STATE_PRD, 
    STATE_SCO, 
    STATE_LPR, 
    STATE_LKOM1, 
    STATE_LISI, 
    STATE_LKOM2, 
    STATE_RPR, 
    STATE_LBR, 
    STATE_RBR, 
    STATE_KOM,
    STATE_ISI,
    STATE_KOMF,
    STATE_LKOM,
    STATE_ERR
};
class Lexer {
private:
    vector<Token> tokens;
    int state;
    char c;
public:
    Lexer(){
        tokens = vector<Token>();
        state = STATE_START;
    }
    void DFA(const string& filename); 
    vector<Token> getTokens() const { return tokens; }
    bool transisiFirstSymbol(char c);
    bool isWord(char c);
    bool isDigit(char c);
    bool isWhitespace(char c);
    void addToken(bool finish, int state, string& value);
};
#endif