#include "parser.hpp"
#include "../debugger/parserTrace.hpp"
#include <iostream>

using namespace std;

// Inisialisasi parser dan buang token comment dari input.
Parser::Parser(const vector<Token>& tokenList) : pos(0) {
    for (const Token& token : tokenList) {
        if (token.type != TokenType::COMMENT) {
            tokens.push_back(token);
        }
    }
}

// Ambil token saat ini tanpa memajukan posisi.
Token Parser::peek() const {
    if (pos < (int)tokens.size()) {
        return tokens[pos];
    }
    return Token(TokenType::ERROR, "EOF");
}

// Ambil token beberapa posisi setelah token saat ini.
Token Parser::peekAhead(int n) const {
    if (pos + n < (int)tokens.size()) {
        return tokens[pos + n];
    }
    return Token(TokenType::ERROR, "EOF");
}

// Majukan posisi parser satu token.
void Parser::advance() {
    if (pos < (int)tokens.size()) {
        // TRACE_MSG("Maju 1 token dari: " + tokenToLabel(tokens[pos]));
        pos++;
    }
}

// Cek apakah token saat ini sesuai tipe tertentu.
bool Parser::match(TokenType type) const {
    return peek().type == type;
}

// Konsumsi token yang diharapkan atau buat node error.
ParseNode Parser::expect(TokenType type) {
    if (match(type)) {
        ParseNode node(tokenToLabel(peek()),true);
        advance();
        return node;
    } else {
        string expectedStr = tokenToString(type);
        string foundStr = tokenToString(peek().type);
        return error("unexpected token " + foundStr + ", expected " + expectedStr);
    }
}

// Cetak syntax error dan kembalikan node error.
ParseNode Parser::error(const string& msg) const {
    cout << "Syntax error: " << msg << endl;
    return ParseNode("error", true);
}

// Ubah token menjadi label terminal parse tree.
string Parser::tokenToLabel(const Token& t) const {
    string typeStr = tokenToString(t.type);
    if (!t.value.empty()) {
        return typeStr + "(" + t.value + ")";
    }
    return typeStr;
}

// Fungsi yang akan dipanggil di main
// Parse keseluruhan program sebagai root parse tree.
ParseNode Parser::parse(){
    ParseNode result = ParseNode("<Program>");
    result.children.push_back(parseProgramHeader());
    result.children.push_back(parseDeclarationPart());
    result.children.push_back(parseCompoundStatement());
    result.children.push_back(expect(TokenType::PERIOD));

    return result;
}      


// === Bukan Bagian dari Program ===
// Membuat dummy tree untuk kebutuhan debugging visualizer.
ParseNode Parser::testDummyTree() {
    TRACE(); // CONTOH PENGGUNAAN TRACE (DEBUGGER)
    
    ParseNode root("<program>");
    
    ParseNode header("<program-header>");
    header.children.push_back(ParseNode("programsy", true));
    header.children.push_back(ParseNode("ident(Hello)", true));
    header.children.push_back(ParseNode("semicolon", true));
    
    root.children.push_back(header);
    
    // Add other placeholders to match spec output structure
    ParseNode declPart("<declaration-part>");
    root.children.push_back(declPart);
    
    ParseNode compStmt("<compound-statement>");
    root.children.push_back(compStmt);
    
    root.children.push_back(ParseNode("period", true));

    return root;
}
