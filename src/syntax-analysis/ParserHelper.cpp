#include "parser.hpp"

namespace {
    // Cek apakah token dapat menjadi awal index array.
    bool isIndexStart(TokenType type){
        return type == TokenType::INTCON ||
               type == TokenType::CHARCON ||
               type == TokenType::IDENT;
    }
}

// Parse daftar identifier yang dipisahkan koma.
ParseNode Parser::parseIdentifierList(){
    // ident (comma + ident)*
    ParseNode node = ParseNode("<identifier-list>");

    node.children.push_back(expect(TokenType::IDENT));
    while(match(TokenType::COMMA)){
        node.children.push_back(expect(TokenType::COMMA));
        node.children.push_back(expect(TokenType::IDENT));
    }

    return node;
}
// Parse konstanta literal atau identifier bertanda plus/minus opsional.
ParseNode Parser::parseConstant(){
    // charcon | string | [(plus | minus)? + (ident | intcon | realcon)]
    ParseNode node = ParseNode("<constant>");

    if (match(TokenType::CHARCON)){
        node.children.push_back(expect(TokenType::CHARCON));
    } else if (match(TokenType::STRING)){
        node.children.push_back(expect(TokenType::STRING));
    } else {
        if (match(TokenType::PLUS)){
            node.children.push_back(expect(TokenType::PLUS));
        } else if (match(TokenType::MINUS)){
            node.children.push_back(expect(TokenType::MINUS));
        }

        if (match(TokenType::IDENT)){
            node.children.push_back(expect(TokenType::IDENT));
        } else if (match(TokenType::INTCON)){
            node.children.push_back(expect(TokenType::INTCON));
        } else if (match(TokenType::REALCON)){
            node.children.push_back(expect(TokenType::REALCON));
        } else {
            node.children.push_back(error("unexpected token " + tokenToString(peek().type) + " in constant"));
        }
    }

    return node;
}
// Parse satu atau lebih index array.
ParseNode Parser::parseIndexList(){
    //  (intcon | charcon | ident ) + ( comma + index-list )*
    ParseNode node = ParseNode("<index-list>");

    if (match(TokenType::INTCON)){
        node.children.push_back(expect(TokenType::INTCON));
    } else if (match(TokenType::CHARCON)){
        node.children.push_back(expect(TokenType::CHARCON));
    } else if (match(TokenType::IDENT)){
        node.children.push_back(expect(TokenType::IDENT));
    } else {
        node.children.push_back(error("unexpected token " + tokenToString(peek().type) + " in index-list"));
    }

    while (match(TokenType::COMMA)){
        node.children.push_back(expect(TokenType::COMMA));
        if (!isIndexStart(peek().type)){
            node.children.push_back(error("unexpected token " + tokenToString(peek().type) + " in index-list"));
        }

        if (match(TokenType::INTCON)){
            node.children.push_back(expect(TokenType::INTCON));
        } else if (match(TokenType::CHARCON)){
            node.children.push_back(expect(TokenType::CHARCON));
        } else {
            node.children.push_back(expect(TokenType::IDENT));
        }
    }

    return node;
}
// Parse operator perbandingan.
ParseNode Parser::parseRelationalOperator(){
    // eql | neq | gtr | geq | lss | leq
    ParseNode node = ParseNode("<relational-operator>");

    if (match(TokenType::EQL)){
        node.children.push_back(expect(TokenType::EQL));
    } else if (match(TokenType::NEQ)){
        node.children.push_back(expect(TokenType::NEQ));
    } else if (match(TokenType::GTR)){
        node.children.push_back(expect(TokenType::GTR));
    } else if (match(TokenType::GEQ)){
        node.children.push_back(expect(TokenType::GEQ));
    } else if (match(TokenType::LSS)){
        node.children.push_back(expect(TokenType::LSS));
    } else if (match(TokenType::LEQ)){
        node.children.push_back(expect(TokenType::LEQ));
    } else {
        node.children.push_back(error("unexpected token " + tokenToString(peek().type) + " in relational-operator"));
    }

    return node;
}
// Parse operator penjumlahan, pengurangan, atau or.
ParseNode Parser::parseAdditiveOperator(){
    // plus | minus | orsy
    ParseNode node = ParseNode("<additive-operator>");

    if (match(TokenType::PLUS)){
        node.children.push_back(expect(TokenType::PLUS));
    } else if (match(TokenType::MINUS)){
        node.children.push_back(expect(TokenType::MINUS));
    } else if (match(TokenType::ORSY)){
        node.children.push_back(expect(TokenType::ORSY));
    } else {
        node.children.push_back(error("unexpected token " + tokenToString(peek().type) + " in additive-operator"));
    }

    return node;
}
// Parse operator perkalian, pembagian, mod, atau and.
ParseNode Parser::parseMultiplicativeOperator(){
    // times | rdiv | idiv | imod | andsy
    ParseNode node = ParseNode("<multiplicative-operator>");

    if (match(TokenType::TIMES)){
        node.children.push_back(expect(TokenType::TIMES));
    } else if (match(TokenType::RDIV)){
        node.children.push_back(expect(TokenType::RDIV));
    } else if (match(TokenType::IDIV)){
        node.children.push_back(expect(TokenType::IDIV));
    } else if (match(TokenType::IMOD)){
        node.children.push_back(expect(TokenType::IMOD));
    } else if (match(TokenType::ANDSY)){
        node.children.push_back(expect(TokenType::ANDSY));
    } else {
        node.children.push_back(error("unexpected token " + tokenToString(peek().type) + " in multiplicative-operator"));
    }

    return node;
}
