#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP

#include <ostream>
#include <sstream>
#include <string>
#include <variant>

// <line> <opcode> <level> <argument>
enum class InstructionCode {
    LIT,  // push literal to stack
    LOD,  // load value from runtime address
    STO,  // store top stack value to runtime address
    CAL,  // call procedure/function entry point
    INT,  // allocate frame/memory slots
    JMP,  // unconditional jump
    JPC,  // jump when popped condition is false
    OPR,  // built-in operation by operation code
    RET   // return/finish current frame
};

// Operation Number (OPR)
enum class OperationCode {
    NEG = 1,
    ADD = 2,
    SUB = 3,
    MUL = 4,
    DIV = 5,
    MOD = 6,
    EQL = 7,
    NEQ = 8,
    LSS = 9,
    GEQ = 10,
    GTR = 11,
    LEQ = 12,
    WRT = 13,
    WRTLN = 14
};

using InstructionArgument = std::variant<int, double, bool, char, std::string>;

struct Instruction {
    InstructionCode code;
    int level;
    InstructionArgument argument;
    std::string comment;

    Instruction(InstructionCode code, int level, InstructionArgument argument, const std::string& comment = "") 
        : code(code), level(level), argument(argument), comment(comment) {}
};

inline std::string instructionCodeToString(InstructionCode code) {
    switch (code) {
        case InstructionCode::LIT: return "LIT";
        case InstructionCode::LOD: return "LOD";
        case InstructionCode::STO: return "STO";
        case InstructionCode::CAL: return "CAL";
        case InstructionCode::INT: return "INT";
        case InstructionCode::JMP: return "JMP";
        case InstructionCode::JPC: return "JPC";
        case InstructionCode::OPR: return "OPR";
        case InstructionCode::RET: return "RET";
    }
    return "UNKNOWN";
}

inline int operationNumber(OperationCode code) {
    return static_cast<int>(code);
}

inline std::string instructionArgumentToString(const InstructionArgument& arg) {
    std::ostringstream out;
    std::visit([&out](const auto& value) {
        out << value;
    }, arg);
    return out.str();
}

inline std::string instructionToString(int line, const Instruction& instruction) {
    std::ostringstream out;
    out << line << " " << instructionCodeToString(instruction.code)
        << " " << instruction.level << " "
        << instructionArgumentToString(instruction.argument);
    if (!instruction.comment.empty()) {
        out << " ; " << instruction.comment;
    }
    return out.str();
}

inline std::ostream& operator<<(std::ostream& os, const Instruction& instruction) {
    os << instructionCodeToString(instruction.code) << " "
       << instruction.level << " "
       << instructionArgumentToString(instruction.argument);
    if (!instruction.comment.empty()) {
        os << " ; " << instruction.comment;
    }
    return os;
}

#endif
