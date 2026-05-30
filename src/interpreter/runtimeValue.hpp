#ifndef RUNTIME_VALUE_HPP
#define RUNTIME_VALUE_HPP

#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>

enum class RuntimeType {
    VOID,
    INTEGER,
    REAL,
    BOOLEAN,
    CHAR,
    STRING
};

struct RuntimeValue {
    RuntimeType type;
    std::variant<int, double, bool, char, std::string> value;

    RuntimeValue() : type(RuntimeType::VOID), value(0) {}
    explicit RuntimeValue(int value) : type(RuntimeType::INTEGER), value(value) {}
    explicit RuntimeValue(double value) : type(RuntimeType::REAL), value(value) {}
    explicit RuntimeValue(bool value) : type(RuntimeType::BOOLEAN), value(value) {}
    explicit RuntimeValue(char value) : type(RuntimeType::CHAR), value(value) {}
    explicit RuntimeValue(const std::string& value) : type(RuntimeType::STRING), value(value) {}
};

inline std::string runtimeTypeToString(RuntimeType type) {
    switch (type) {
        case RuntimeType::VOID: return "void";
        case RuntimeType::INTEGER: return "integer";
        case RuntimeType::REAL: return "real";
        case RuntimeType::BOOLEAN: return "boolean";
        case RuntimeType::CHAR: return "char";
        case RuntimeType::STRING: return "string";
    }
    return "unknown";
}

inline bool runtimeValueIsFalse(const RuntimeValue& value) {
    if (value.type != RuntimeType::BOOLEAN) {
        throw std::runtime_error("JPC expects boolean value");
    }
    return !std::get<bool>(value.value);
}

inline std::string runtimeValueToString(const RuntimeValue& value) {
    std::ostringstream out;
    switch (value.type) {
        case RuntimeType::VOID: return "";
        case RuntimeType::INTEGER: out << std::get<int>(value.value); break;
        case RuntimeType::REAL: out << std::get<double>(value.value); break;
        case RuntimeType::BOOLEAN: out << (std::get<bool>(value.value) ? "true" : "false"); break;
        case RuntimeType::CHAR: out << std::get<char>(value.value); break;
        case RuntimeType::STRING: out << std::get<std::string>(value.value); break;
    }
    return out.str();
}

#endif
