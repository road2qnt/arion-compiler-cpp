#include "parser.hpp"

// [TODO!] Belum ada implementasi untuk 6 fungsi di bawah!

ParseNode Parser::parseExpression(){
    // simple-expression (relational-operator + simple-expression)?
}
ParseNode Parser::parseSimpleExpression(){
    // (plus | minus)? term (additive-operator + term)*
}
ParseNode Parser::parseTerm(){
    // factor (multiplicative-operator + factor)*
}
ParseNode Parser::parseFactor(){
    // ident | intcon | realcon | charcon | string | (lparent + expression + rparent) | (notsy + factor) | procedure/function-call | variable
}
ParseNode Parser::parseProcedureFunctionCall(){
    // ident + (lparent + parameter-list + rparent)
}
ParseNode Parser::parseParameterList(){
    // expression (comma + expression)*
}