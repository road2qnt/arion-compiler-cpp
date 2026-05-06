#ifndef TOKENREADWRITE_H
#define TOKENREADWRITE_H

#include "../lexical-analysis/token.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>

// === Jika Baca dari File yang berisi TOKEN ===

class TokenReadWrite{
private:
    std::string pathFile;

    static std::string trim(const std::string& text){
        size_t start = 0;
        while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))){
            start++;
        }

        size_t end = text.size();
        while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))){
            end--;
        }

        return text.substr(start, end - start);
    }

    static std::string toLowerString(std::string text){
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c){
            return static_cast<char>(std::tolower(c));
        });
        return text;
    }

    static bool stringToTokenType(const std::string& text, TokenType& type){
        std::string lower = toLowerString(text);

        if (lower == "intcon") type = TokenType::INTCON;
        else if (lower == "realcon") type = TokenType::REALCON;
        else if (lower == "charcon") type = TokenType::CHARCON;
        else if (lower == "string") type = TokenType::STRING;
        else if (lower == "notsy") type = TokenType::NOTSY;
        else if (lower == "andsy") type = TokenType::ANDSY;
        else if (lower == "orsy") type = TokenType::ORSY;
        else if (lower == "plus") type = TokenType::PLUS;
        else if (lower == "minus") type = TokenType::MINUS;
        else if (lower == "times") type = TokenType::TIMES;
        else if (lower == "idiv") type = TokenType::IDIV;
        else if (lower == "rdiv") type = TokenType::RDIV;
        else if (lower == "imod") type = TokenType::IMOD;
        else if (lower == "eql") type = TokenType::EQL;
        else if (lower == "neq") type = TokenType::NEQ;
        else if (lower == "gtr") type = TokenType::GTR;
        else if (lower == "geq") type = TokenType::GEQ;
        else if (lower == "lss") type = TokenType::LSS;
        else if (lower == "leq") type = TokenType::LEQ;
        else if (lower == "lparent") type = TokenType::LPARENT;
        else if (lower == "rparent") type = TokenType::RPARENT;
        else if (lower == "lbrack") type = TokenType::LBRACK;
        else if (lower == "rbrack") type = TokenType::RBRACK;
        else if (lower == "comma") type = TokenType::COMMA;
        else if (lower == "semicolon") type = TokenType::SEMICOLON;
        else if (lower == "period") type = TokenType::PERIOD;
        else if (lower == "colon") type = TokenType::COLON;
        else if (lower == "becomes") type = TokenType::BECOMES;
        else if (lower == "constsy") type = TokenType::CONSTSY;
        else if (lower == "typesy") type = TokenType::TYPESY;
        else if (lower == "varsy") type = TokenType::VARSY;
        else if (lower == "functionsy") type = TokenType::FUNCTIONSY;
        else if (lower == "proceduresy") type = TokenType::PROCEDURESY;
        else if (lower == "arraysy") type = TokenType::ARRAYSY;
        else if (lower == "recordsy") type = TokenType::RECORDSY;
        else if (lower == "programsy") type = TokenType::PROGRAMSY;
        else if (lower == "ident") type = TokenType::IDENT;
        else if (lower == "beginsy") type = TokenType::BEGINSY;
        else if (lower == "ifsy") type = TokenType::IFSY;
        else if (lower == "casesy") type = TokenType::CASESY;
        else if (lower == "repeatsy") type = TokenType::REPEATSY;
        else if (lower == "whilesy") type = TokenType::WHILESY;
        else if (lower == "forsy") type = TokenType::FORSY;
        else if (lower == "endsy") type = TokenType::ENDSY;
        else if (lower == "elsesy") type = TokenType::ELSESY;
        else if (lower == "untilsy") type = TokenType::UNTILSY;
        else if (lower == "ofsy") type = TokenType::OFSY;
        else if (lower == "dosy") type = TokenType::DOSY;
        else if (lower == "tosy") type = TokenType::TOSY;
        else if (lower == "downtosy") type = TokenType::DOWNTOSY;
        else if (lower == "thensy") type = TokenType::THENSY;
        else if (lower == "comment") type = TokenType::COMMENT;
        else if (lower == "unknown") type = TokenType::UNKNOWN;
        else if (lower == "error") type = TokenType::ERROR;
        else return false;

        return true;
    }

    static Token parseLine(const std::string& line, int lineNumber){
        std::string cleaned = trim(line);
        std::string typeText;
        std::string value;

        size_t openParen = cleaned.find('(');
        if (openParen != std::string::npos){
            typeText = trim(cleaned.substr(0, openParen));
            size_t closeParen = cleaned.rfind(')');

            if (closeParen != std::string::npos && closeParen > openParen){
                value = cleaned.substr(openParen + 1, closeParen - openParen - 1);
            } else {
                value = cleaned.substr(openParen + 1);
            }
        } else {
            std::istringstream stream(cleaned);
            stream >> typeText;
        }

        TokenType type;
        if (!stringToTokenType(typeText, type)){
            return Token(TokenType::ERROR, cleaned, lineNumber);
        }

        return Token(type, value, lineNumber);
    }

public:
    TokenReadWrite() = default;
    TokenReadWrite(const std::string& pathFile) : pathFile(pathFile) {}

    void setPathFile(const std::string& pathFile){
        this->pathFile = pathFile;
    }

    std::vector<Token> parseToVector(){
        // Baca token dan parsing menjadi vector
        std::vector<Token> result;
        
        // Baca File
        std::fstream file(pathFile);

        // Masukkan ke vector 
        std::string line;
        int lineNumber = 0;
        if (!file.is_open()){
            std::cout << "gagal!";
        }

        while (std::getline(file,line)){
            lineNumber++;
            std::string cleaned = trim(line);
            if (cleaned.empty()){
                continue;
            }

            result.push_back(parseLine(cleaned, lineNumber));
        }

        return result;
    }
};



#endif
