#include "parser.hpp"

namespace {
    // Cek token yang boleh mengakhiri statement kosong.
    bool isStatementFollow(TokenType type){
        return type == TokenType::SEMICOLON ||
               type == TokenType::ENDSY ||
               type == TokenType::ELSESY ||
               type == TokenType::UNTILSY ||
               type == TokenType::PERIOD;
    }

    // Cek apakah token dapat menjadi awal constant pada case-block.
    bool isConstantStart(TokenType type){
        return type == TokenType::CHARCON ||
               type == TokenType::STRING ||
               type == TokenType::PLUS ||
               type == TokenType::MINUS ||
               type == TokenType::IDENT ||
               type == TokenType::INTCON ||
               type == TokenType::REALCON;
    }
}

// Parse block statement yang diawali begin dan diakhiri end.
ParseNode Parser::parseCompoundStatement(){
    // beginsy + statement-list + endsy
    ParseNode node = ParseNode("<compound-statement>");

    node.children.push_back(expect(TokenType::BEGINSY));
    node.children.push_back(parseStatementList());
    node.children.push_back(expect(TokenType::ENDSY));

    return node;
}
// Parse daftar statement yang dipisahkan semicolon.
ParseNode Parser::parseStatementList(){
    // statement (semicolon + statement)*
    ParseNode node = ParseNode("<statement-list>");

    node.children.push_back(parseStatement());

    while (match(TokenType::SEMICOLON)){
        node.children.push_back(expect(TokenType::SEMICOLON));
        node.children.push_back(parseStatement());
    }

    return node;
}
// Parse satu statement atau statement kosong.
ParseNode Parser::parseStatement(){
    /* (assignment-statement | if-statement | case-statement |while-statement |repeat-statement | for-statement | procedure/function-call )?*/
    ParseNode node = ParseNode("<statement>");

    if (match(TokenType::IDENT)){
        bool assignmentStatement = false;
        int lookahead = 1;
        TokenType nextType = peekAhead(lookahead).type;

        while (!isStatementFollow(nextType) && nextType != TokenType::ERROR){
            if (nextType == TokenType::BECOMES){
                assignmentStatement = true;
                break;
            }
            lookahead++;
            nextType = peekAhead(lookahead).type;
        }

        if (assignmentStatement){
            node.children.push_back(parseAssignmentStatement());
        } else {
            node.children.push_back(parseProcedureFunctionCall());
        }
    } else if (match(TokenType::IFSY)){
        node.children.push_back(parseIfStatement());
    } else if (match(TokenType::CASESY)){
        node.children.push_back(parseCaseStatement());
    } else if (match(TokenType::WHILESY)){
        node.children.push_back(parseWhileStatement());
    } else if (match(TokenType::REPEATSY)){
        node.children.push_back(parseRepeatStatement());
    } else if (match(TokenType::FORSY)){
        node.children.push_back(parseForStatement());
    } else if (!isStatementFollow(peek().type)){
        node.children.push_back(error("unexpected token " + tokenToString(peek().type) + " in statement"));
    }

    return node;
}
// Parse assignment ke variable.
ParseNode Parser::parseAssignmentStatement(){
    // variable + becomes + expression
    ParseNode node = ParseNode("<assignment-statement>");

    node.children.push_back(parseVariable());
    node.children.push_back(expect(TokenType::BECOMES));
    node.children.push_back(parseExpression());

    return node;
}
// Parse statement percabangan if-then-else.
ParseNode Parser::parseIfStatement(){
    // ifsy + expression + thensy + statement + (elsy + statement)?
    ParseNode node = ParseNode("<if-statement>");

    node.children.push_back(expect(TokenType::IFSY));
    node.children.push_back(parseExpression());
    node.children.push_back(expect(TokenType::THENSY));
    node.children.push_back(parseStatement());
    if (match(TokenType::ELSESY)){
        node.children.push_back(expect(TokenType::ELSESY));
        node.children.push_back(parseStatement());
    }

    return node;
}
// Parse statement case-of.
ParseNode Parser::parseCaseStatement(){
    // casesy + expression + ofsy + case-block + endsy
    ParseNode node = ParseNode("<case-statement>");

    node.children.push_back(expect(TokenType::CASESY));
    node.children.push_back(parseExpression());
    node.children.push_back(expect(TokenType::OFSY));
    node.children.push_back(parseCaseBlock());
    node.children.push_back(expect(TokenType::ENDSY));

    return node;
}
// Parse satu atau lebih cabang dalam case statement.
ParseNode Parser::parseCaseBlock(){
    // constant + (comma + constant)* + colon + statement +  (semicolon + case-block?)* 
    ParseNode node = ParseNode("<case-block>");

    node.children.push_back(parseConstant());
    while (match(TokenType::COMMA)){
        node.children.push_back(expect(TokenType::COMMA));
        node.children.push_back(parseConstant());
    }
    node.children.push_back(expect(TokenType::COLON));
    node.children.push_back(parseStatement());

    while (match(TokenType::SEMICOLON)){
        node.children.push_back(expect(TokenType::SEMICOLON));
        if (isConstantStart(peek().type)){
            node.children.push_back(parseCaseBlock());
        }
    }

    return node;
}
// Parse perulangan while-do.
ParseNode Parser::parseWhileStatement(){
    // whilesy + expression + dosy + statement
    ParseNode node = ParseNode("<while-statement>");

    node.children.push_back(expect(TokenType::WHILESY));
    node.children.push_back(parseExpression());
    node.children.push_back(expect(TokenType::DOSY));
    node.children.push_back(parseStatement());

    return node;
}
// Parse perulangan repeat-until.
ParseNode Parser::parseRepeatStatement(){
    // repeatsy + statement-list + untilsy + expression
    ParseNode node = ParseNode("<repeat-statement>");

    node.children.push_back(expect(TokenType::REPEATSY));
    node.children.push_back(parseStatementList());
    node.children.push_back(expect(TokenType::UNTILSY));
    node.children.push_back(parseExpression());

    return node;
}
// Parse perulangan for-to/downto-do.
ParseNode Parser::parseForStatement(){
    // forsy + ident + becomes + expression + ( tosy | downtosy) + expression + dosy + statement
    ParseNode node = ParseNode("<for-statement>");

    node.children.push_back(expect(TokenType::FORSY));
    node.children.push_back(expect(TokenType::IDENT));
    node.children.push_back(expect(TokenType::BECOMES));
    node.children.push_back(parseExpression());
    if (match(TokenType::TOSY)){
        node.children.push_back(expect(TokenType::TOSY));
    } else if (match(TokenType::DOWNTOSY)){
        node.children.push_back(expect(TokenType::DOWNTOSY));
    } else {
        node.children.push_back(error("unexpected token " + tokenToString(peek().type) + ", expected tosy or downtosy"));
    }
    node.children.push_back(parseExpression());
    node.children.push_back(expect(TokenType::DOSY));
    node.children.push_back(parseStatement());

    return node;
}
