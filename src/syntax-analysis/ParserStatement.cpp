#include "parser.hpp"

// [TODO!] Belum ada implementasi untuk 10 fungsi di bawah!

ParseNode Parser::parseCompoundStatement(){
    // beginsy + statement-list + endsy
}
ParseNode Parser::parseStatementList(){
    // statement (semicolon + statement)*
}
ParseNode Parser::parseStatement(){
    /* (assignment-statement | 
    if-statement | case-statement |
    while-statement |
    repeat-statement | for-statement | procedure/function-call )?
    */
}
ParseNode Parser::parseAssignmentStatement(){
    // variable + becomes + expressio
}
ParseNode Parser::parseIfStatement(){
    // ifsy + expression + thensy + statement + (elsy + statement)?
}
ParseNode Parser::parseCaseStatement(){
    // casesy + expression + ofsy + case-block + endsy
}
ParseNode Parser::parseCaseBlock(){
    // constant + (comma + constant)* + colon + statement +  (semicolon + case-block?)* 
}
ParseNode Parser::parseWhileStatement(){
    // whilesy + expression + dosy + statement
}
ParseNode Parser::parseRepeatStatement(){
    // repeatsy + statement-list + untilsy + expression
}
ParseNode Parser::parseForStatement(){
    // forsy + ident + becomes + expression + ( tosy | downtosy) + expression + dosy + statement
}