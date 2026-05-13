#include "typeChecker.hpp"
#include <iostream>

TypeChecker::TypeChecker(SymbolTable& symTab) : symTab(symTab) {}

bool TypeChecker::isCompatible(int type1, int type2) const {
    if (type1 == type2) return true;

    if ((type1 == TYPE_INTEGER && type2 == TYPE_REAL) ||
        (type1 == TYPE_REAL && type2 == TYPE_INTEGER)) {
        return true;
    }

    if (isNumeric(type1) && isNumeric(type2)) return true;

    return false;
}

bool TypeChecker::isAssignmentCompatible(int targetType, int valueType) const {
    if (targetType == TYPE_REAL && valueType == TYPE_INTEGER) return true;

    return isCompatible(targetType, valueType);
}

bool TypeChecker::isNumeric(int type) const {
    return type == TYPE_INTEGER || type == TYPE_REAL;
}

bool TypeChecker::isOrdinal(int type) const {
    return type == TYPE_INTEGER || type == TYPE_CHAR || type == TYPE_BOOLEAN;
}

bool TypeChecker::isBoolean(int type) const {
    return type == TYPE_BOOLEAN;
}

bool TypeChecker::isInteger(int type) const {
    return type == TYPE_INTEGER;
}

bool TypeChecker::isReal(int type) const {
    return type == TYPE_REAL;
}

bool TypeChecker::isString(int type) const {
    return type == TYPE_STRING;
}

bool TypeChecker::isValidRelationalOperator(int leftType, int rightType) const {
    return isCompatible(leftType, rightType);
}

bool TypeChecker::isValidArithmeticOperator(const std::string& op, int leftType, int rightType) const {
    if (op == "+" || op == "-" || op == "*") {
        return isNumeric(leftType) && isNumeric(rightType);
    }
    if (op == "/") {
        return isNumeric(leftType) && isNumeric(rightType);
    }
    if (op == "div" || op == "mod") {
        return isInteger(leftType) && isInteger(rightType);
    }
    return false;
}

bool TypeChecker::isValidLogicalOperator(const std::string& op, int leftType, int rightType) const {
    if (op == "and" || op == "or") {
        return isBoolean(leftType) && isBoolean(rightType);
    }
    return false;
}

bool TypeChecker::isValidUnaryOperator(const std::string& op, int operandType) const {
    if (op == "+" || op == "-") {
        return isNumeric(operandType);
    }
    if (op == "not") {
        return isBoolean(operandType);
    }
    return false;
}

int TypeChecker::getResultType(const std::string& op, int leftType, int rightType) const {
    if (op == "=" || op == "<>" || op == "<" || op == "<=" || op == ">" || op == ">=") {
        if (isValidRelationalOperator(leftType, rightType)) {
            return TYPE_BOOLEAN;
        }
        return TYPE_ERROR;
    }

    if (op == "and" || op == "or") {
        if (isValidLogicalOperator(op, leftType, rightType)) {
            return TYPE_BOOLEAN;
        }
        return TYPE_ERROR;
    }

    if (op == "/") {
        if (isValidArithmeticOperator(op, leftType, rightType)) {
            return TYPE_REAL;
        }
        return TYPE_ERROR;
    }

    if (op == "div" || op == "mod") {
        if (isValidArithmeticOperator(op, leftType, rightType)) {
            return TYPE_INTEGER;
        }
        return TYPE_ERROR;
    }

    if (op == "+" || op == "-" || op == "*") {
        if (isValidArithmeticOperator(op, leftType, rightType)) {
            if (leftType == TYPE_REAL || rightType == TYPE_REAL) {
                return TYPE_REAL;
            }
            return TYPE_INTEGER;
        }
        return TYPE_ERROR;
    }

    return TYPE_ERROR;
}

int TypeChecker::getUnaryResultType(const std::string& op, int operandType) const {
    if (op == "+" || op == "-") {
        if (isNumeric(operandType)) {
            return operandType;
        }
        return TYPE_ERROR;
    }
    if (op == "not") {
        if (isBoolean(operandType)) {
            return TYPE_BOOLEAN;
        }
        return TYPE_ERROR;
    }
    return TYPE_ERROR;
}

void TypeChecker::reportError(const std::string& msg, int line) {
    errors.push_back(SemanticError(msg, line));
}

void TypeChecker::printErrors(std::ostream& os) const {
    for (const auto& err : errors) {
        if (err.line > 0) {
            os << "Semantic error at line " << err.line << ": " << err.message << std::endl;
        } else {
            os << "Semantic error: " << err.message << std::endl;
        }
    }
}
