#ifndef SYNTAXERROR_HPP
#define SYNTAXERROR_HPP

#include <stdexcept>
#include <string>

class SyntaxError : public std::runtime_error {
public:
    SyntaxError(const std::string& msg) : std::runtime_error(msg) {}
};

#endif
