#ifndef TOKEN_H
#define TOKEN_H
#include <string>
using namespace std;
enum class TokenType {
    INTCON, REALCON, CHARCON, STRING,
    NOTSY, ANDSY, ORSY,
    PLUS, MINUS, TIMES, IDIV, RDIV, IMOD,
    EQL, NEQ, GTR, GEQ, LSS, LEQ,
    LPARENT, RPARENT, LBRACK, RBRACK,
    COMMA, SEMICOLON, PERIOD, COLON, BECOMES,
    CONSTSY, TYPESY, VARSY, FUNCTIONSY, PROCEDURESY,
    ARRAYSY, RECORDSY, PROGRAMSY,
    IDENT,
    BEGINSY, IFSY, CASESY, REPEATSY, WHILESY, FORSY,
    ENDSY, ELSESY, UNTILSY, OFSY, DOSY, TOSY, DOWNTOSY, THENSY,
    COMMENT,
    UNKNOWN,
    ERROR
};
struct Token {
    TokenType type;
    string value;
    int line;
    Token(TokenType type, const string& value = "", int line = 0);
};
string tokenToString(TokenType type);
TokenType keyword(const string& ident);
#endif
