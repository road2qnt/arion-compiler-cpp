#include "token.hpp"
Token::Token(TokenType type, const string& value) {
    this->type = type;
    this->value = value;
}
string tokenToString(TokenType type) {
    switch (type) {
        case TokenType::INTCON:       return "intcon";
        case TokenType::REALCON:      return "realcon";
        case TokenType::CHARCON:      return "charcon";
        case TokenType::STRING:       return "string";
        case TokenType::NOTSY:        return "notsy";
        case TokenType::ANDSY:        return "andsy";
        case TokenType::ORSY:         return "orsy";
        case TokenType::PLUS:         return "plus";
        case TokenType::MINUS:        return "minus";
        case TokenType::TIMES:        return "times";
        case TokenType::IDIV:         return "idiv";
        case TokenType::RDIV:         return "rdiv";
        case TokenType::IMOD:         return "imod";
        case TokenType::EQL:          return "eql";
        case TokenType::NEQ:          return "neq";
        case TokenType::GTR:          return "gtr";
        case TokenType::GEQ:          return "geq";
        case TokenType::LSS:          return "lss";
        case TokenType::LEQ:          return "leq";
        case TokenType::LPARENT:      return "lparent";
        case TokenType::RPARENT:      return "rparent";
        case TokenType::LBRACK:       return "lbrack";
        case TokenType::RBRACK:       return "rbrack";
        case TokenType::COMMA:        return "comma";
        case TokenType::SEMICOLON:    return "semicolon";
        case TokenType::PERIOD:       return "period";
        case TokenType::COLON:        return "colon";
        case TokenType::BECOMES:      return "becomes";
        case TokenType::CONSTSY:      return "constsy";
        case TokenType::TYPESY:       return "typesy";
        case TokenType::VARSY:        return "varsy";
        case TokenType::FUNCTIONSY:   return "functionsy";
        case TokenType::PROCEDURESY:  return "proceduresy";
        case TokenType::ARRAYSY:      return "arraysy";
        case TokenType::RECORDSY:     return "recordsy";
        case TokenType::PROGRAMSY:    return "programsy";
        case TokenType::IDENT:        return "ident";
        case TokenType::BEGINSY:      return "beginsy";
        case TokenType::IFSY:         return "ifsy";
        case TokenType::CASESY:       return "casesy";
        case TokenType::REPEATSY:     return "repeatsy";
        case TokenType::WHILESY:      return "whilesy";
        case TokenType::FORSY:        return "forsy";
        case TokenType::ENDSY:        return "endsy";
        case TokenType::ELSESY:       return "elsesy";
        case TokenType::UNTILSY:      return "untilsy";
        case TokenType::OFSY:         return "ofsy";
        case TokenType::DOSY:         return "dosy";
        case TokenType::TOSY:         return "tosy";
        case TokenType::DOWNTOSY:     return "downtosy";
        case TokenType::THENSY:       return "thensy";
        case TokenType::COMMENT:      return "comment";
        case TokenType::ERROR:        return "error";
        default:                      return "unknown";
    }
}
string toLower(const string& str) {
    string result = str;
    for (int i = 0; i < result.size(); i++) {
        if (result[i] >= 'A' && result[i] <= 'Z') {
            result[i] = result[i] + ('a' - 'A');
        }
    }
    return result;
}
TokenType keyword(const string& ident) {
    string lower = toLower(ident);
    if (lower == "not")       return TokenType::NOTSY;
    if (lower == "div")       return TokenType::IDIV;
    if (lower == "mod")       return TokenType::IMOD;
    if (lower == "and")       return TokenType::ANDSY;
    if (lower == "or")        return TokenType::ORSY;
    if (lower == "const")     return TokenType::CONSTSY;
    if (lower == "type")      return TokenType::TYPESY;
    if (lower == "var")       return TokenType::VARSY;
    if (lower == "function")  return TokenType::FUNCTIONSY;
    if (lower == "procedure") return TokenType::PROCEDURESY;
    if (lower == "array")     return TokenType::ARRAYSY;
    if (lower == "record")    return TokenType::RECORDSY;
    if (lower == "program")   return TokenType::PROGRAMSY;
    if (lower == "begin")     return TokenType::BEGINSY;
    if (lower == "if")        return TokenType::IFSY;
    if (lower == "case")      return TokenType::CASESY;
    if (lower == "repeat")    return TokenType::REPEATSY;
    if (lower == "while")     return TokenType::WHILESY;
    if (lower == "for")       return TokenType::FORSY;
    if (lower == "end")       return TokenType::ENDSY;
    if (lower == "else")      return TokenType::ELSESY;
    if (lower == "until")     return TokenType::UNTILSY;
    if (lower == "of")        return TokenType::OFSY;
    if (lower == "do")        return TokenType::DOSY;
    if (lower == "to")        return TokenType::TOSY;
    if (lower == "downto")    return TokenType::DOWNTOSY;
    if (lower == "then")      return TokenType::THENSY;
    return TokenType::IDENT;
}