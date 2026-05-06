#include "parser.hpp"

ParseNode Parser::parseIdentifierList(){
    // ident (comma + ident)*
}
ParseNode Parser::parseConstant(){
    // charcon | string | [(plus | minus)? + (ident | intcon | realcon)]
}
ParseNode Parser::parseIndexList(){
    //  (intcon | charcon | ident ) + ( comma + index-list )*
}
ParseNode Parser::parseRelationalOperator(){
    // eql | neq | gtr | geq | lss | leq
}
ParseNode Parser::parseAdditiveOperator(){
    // plus | minus | orsy
}
ParseNode Parser::parseMultiplicativeOperator(){
    // times | rdiv | idiv | imod | andsy
}