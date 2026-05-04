#ifndef TOKEN_DUMPER_HPP
#define TOKEN_DUMPER_HPP

#include "../lexical-analysis/token.hpp"
#include <vector>
#include <iostream>

inline void dumpTokens(const std::vector<Token>& tokens) {
    std::cout << "\n--- Token Dump Debugger ---" << std::endl;
    for (size_t i = 0; i < tokens.size(); ++i) {
        std::cout << "[" << i << "] " << tokenToString(tokens[i].type);
        if (!tokens[i].value.empty()) {
            std::cout << " ('" << tokens[i].value << "')";
        }
        std::cout << " @ line " << tokens[i].line << std::endl;
    }
    std::cout << "---------------------------\n" << std::endl;
}

#endif
