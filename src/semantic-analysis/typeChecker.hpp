#ifndef TYPECHECKER_HPP
#define TYPECHECKER_HPP

#include "astNode.hpp"
#include "symbolTable.hpp"

// Error structure for semantic errors
struct SemanticError {
    std::string message;
    int line;

    SemanticError(const std::string& msg, int line = 0) : message(msg), line(line) {}
};

class TypeChecker {
public:
    TypeChecker(SymbolTable& symTab);

    bool isCompatible(int type1, int type2) const;
    bool isAssignmentCompatible(int targetType, int valueType) const;
    bool isNumeric(int type) const;
    bool isOrdinal(int type) const;
    bool isBoolean(int type) const;
    bool isInteger(int type) const;
    bool isReal(int type) const;
    bool isString(int type) const;

    bool isValidRelationalOperator(int leftType, int rightType) const;
    bool isValidArithmeticOperator(const std::string& op, int leftType, int rightType) const;
    bool isValidLogicalOperator(const std::string& op, int leftType, int rightType) const;
    bool isValidUnaryOperator(const std::string& op, int operandType) const;

    int getResultType(const std::string& op, int leftType, int rightType) const;
    int getUnaryResultType(const std::string& op, int operandType) const;

    void reportError(const std::string& msg, int line = 0);
    bool hasErrors() const { return !errors.empty(); }
    void printErrors(std::ostream& os) const;
    void clearErrors() { errors.clear(); }

private:
    SymbolTable& symTab;
    std::vector<SemanticError> errors;
};

#endif // TYPECHECKER_HPP
