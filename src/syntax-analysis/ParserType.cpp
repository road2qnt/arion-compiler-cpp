#include "parser.hpp"

ParseNode Parser::parseType(){
    /* ident |
    array-type |
    range | 
    enumerated |
    record-type
    */
}
ParseNode Parser::parseArrayType(){
    // arraysy + lbrack + (range | ident) + rbrack + ofsy + type
}
ParseNode Parser::parseRange(){
    // constant + period + period + constant
}
ParseNode Parser::parseEnumerated(){
    // lparent + ident + (comma + ident)* + rparent
}
ParseNode Parser::parseRecordType(){
    // recordsy + field-list + endsy
}
ParseNode Parser::parseFieldList(){
    // field-part + (semicolon + field-part)*
}   
ParseNode Parser::parseFieldPart(){
    // identifier-list + colon + type
}