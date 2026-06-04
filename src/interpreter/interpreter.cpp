#include "interpreter.hpp"
#include <iostream>
#include <sstream>

// Forward declaration of helper function
static double runtimeToDouble(const RuntimeValue& v) {
    if (v.type == RuntimeType::INTEGER) return (double)std::get<int>(v.value);
    if (v.type == RuntimeType::REAL) return std::get<double>(v.value);
    return 0.0;
}

InterpreterResult Interpreter::run(const std::vector<Instruction>& instructions) {
    stack.reset();
    result = InterpreterResult();
    instructionPointer = 0;

    try {
        while (instructionPointer >= 0 && instructionPointer < (int)instructions.size()) {
            const Instruction& inst = instructions[instructionPointer];
            execute(inst);
            result.executedInstructionCount++;
        }
    } catch (const std::exception& e) {
        reportRuntimeError(e.what());
    }

    return result;
}

void Interpreter::execute(const Instruction& instruction) {
    switch (instruction.code) {
        case InstructionCode::LIT:
            executeLiteral(instruction);
            break;
        case InstructionCode::LOD:
            executeLoad(instruction);
            break;
        case InstructionCode::STO:
            executeStore(instruction);
            break;
        case InstructionCode::CAL:
            executeCall(instruction);
            break;
        case InstructionCode::INT:
            stack.allocateFrame(requireIntegerArgument(instruction));
            instructionPointer++;
            break;
        case InstructionCode::JMP:
            executeJump(instruction);
            break;
        case InstructionCode::JPC:
            executeConditionalJump(instruction);
            break;
        case InstructionCode::OPR:
            executeOperation(instruction);
            instructionPointer++;
            break;
        case InstructionCode::RET:
            executeReturn(instruction);
            break;
        case InstructionCode::LDA:
            executeLoadAddress(instruction);
            break;
        case InstructionCode::LDI:
            executeLoadIndirect(instruction);
            break;
        case InstructionCode::STI:
            executeStoreIndirect(instruction);
            break;
    }
}

void Interpreter::executeLiteral(const Instruction& instruction) {
    RuntimeValue val = makeRuntimeValue(instruction.argument);
    stack.push(val);
    instructionPointer++;
}

void Interpreter::executeLoad(const Instruction& instruction) {
    int offset = requireIntegerArgument(instruction);
    int addr = stack.resolveAddress(instruction.level, offset);
    RuntimeValue val = stack.load(addr);
    stack.push(val);
    instructionPointer++;
}

void Interpreter::executeStore(const Instruction& instruction) {
    int offset = requireIntegerArgument(instruction);
    RuntimeValue val = stack.pop();
    int addr = stack.resolveAddress(instruction.level, offset);
    stack.store(addr, val);
    instructionPointer++;
}

void Interpreter::executeCall(const Instruction& instruction) {
    int entryPoint = requireIntegerArgument(instruction);
    stack.pushFrame(instructionPointer + 1, instruction.level);
    instructionPointer = entryPoint;
}

void Interpreter::executeJump(const Instruction& instruction) {
    instructionPointer = requireIntegerArgument(instruction);
}

void Interpreter::executeConditionalJump(const Instruction& instruction) {
    RuntimeValue cond = stack.pop();
    if (runtimeValueIsFalse(cond)) {
        instructionPointer = requireIntegerArgument(instruction);
    } else {
        instructionPointer++;
    }
}

void Interpreter::executeOperation(const Instruction& instruction) {
    int opNum = requireIntegerArgument(instruction);

    switch (opNum) {
        case 1: {  // NEG
            RuntimeValue a = stack.pop();
            if (a.type == RuntimeType::INTEGER) {
                stack.push(RuntimeValue(-std::get<int>(a.value)));
            } else if (a.type == RuntimeType::REAL) {
                stack.push(RuntimeValue(-std::get<double>(a.value)));
            } else {
                reportRuntimeError("NEG: operand must be numeric");
            }
            break;
        }
        case 2: {  // ADD
            RuntimeValue b = stack.pop();
            RuntimeValue a = stack.pop();
            if (a.type == RuntimeType::INTEGER && b.type == RuntimeType::INTEGER) {
                stack.push(RuntimeValue(std::get<int>(a.value) + std::get<int>(b.value)));
            } else {
                double da = (a.type == RuntimeType::INTEGER) ? (double)std::get<int>(a.value) : std::get<double>(a.value);
                double db = (b.type == RuntimeType::INTEGER) ? (double)std::get<int>(b.value) : std::get<double>(b.value);
                stack.push(RuntimeValue(da + db));
            }
            break;
        }
        case 3: {  // SUB
            RuntimeValue b = stack.pop();
            RuntimeValue a = stack.pop();
            if (a.type == RuntimeType::INTEGER && b.type == RuntimeType::INTEGER) {
                stack.push(RuntimeValue(std::get<int>(a.value) - std::get<int>(b.value)));
            } else {
                double da = (a.type == RuntimeType::INTEGER) ? (double)std::get<int>(a.value) : std::get<double>(a.value);
                double db = (b.type == RuntimeType::INTEGER) ? (double)std::get<int>(b.value) : std::get<double>(b.value);
                stack.push(RuntimeValue(da - db));
            }
            break;
        }
        case 4: {  // MUL
            RuntimeValue b = stack.pop();
            RuntimeValue a = stack.pop();
            if (a.type == RuntimeType::INTEGER && b.type == RuntimeType::INTEGER) {
                stack.push(RuntimeValue(std::get<int>(a.value) * std::get<int>(b.value)));
            } else {
                double da = (a.type == RuntimeType::INTEGER) ? (double)std::get<int>(a.value) : std::get<double>(a.value);
                double db = (b.type == RuntimeType::INTEGER) ? (double)std::get<int>(b.value) : std::get<double>(b.value);
                stack.push(RuntimeValue(da * db));
            }
            break;
        }
        case 5: {  // DIV
            RuntimeValue b = stack.pop();
            RuntimeValue a = stack.pop();
            double db = (b.type == RuntimeType::INTEGER) ? (double)std::get<int>(b.value) : std::get<double>(b.value);
            if (db == 0.0) {
                reportRuntimeError("DIV: division by zero");
                stack.push(RuntimeValue(0));
                break;
            }
            if (a.type == RuntimeType::INTEGER && b.type == RuntimeType::INTEGER) {
                stack.push(RuntimeValue(std::get<int>(a.value) / std::get<int>(b.value)));
            } else {
                double da = (a.type == RuntimeType::INTEGER) ? (double)std::get<int>(a.value) : std::get<double>(a.value);
                stack.push(RuntimeValue(da / db));
            }
            break;
        }
        case 6: {  // MOD
            RuntimeValue b = stack.pop();
            RuntimeValue a = stack.pop();
            if (a.type == RuntimeType::INTEGER && b.type == RuntimeType::INTEGER) {
                int ib = std::get<int>(b.value);
                if (ib == 0) {
                    reportRuntimeError("MOD: division by zero");
                    stack.push(RuntimeValue(0));
                    break;
                }
                stack.push(RuntimeValue(std::get<int>(a.value) % ib));
            } else {
                reportRuntimeError("MOD: operands must be integers");
                stack.push(RuntimeValue(0));
            }
            break;
        }
        case 7: {  // EQL
            RuntimeValue b = stack.pop();
            RuntimeValue a = stack.pop();
            stack.push(RuntimeValue(valuesEqual(a, b)));
            break;
        }
        case 8: {  // NEQ
            RuntimeValue b = stack.pop();
            RuntimeValue a = stack.pop();
            stack.push(RuntimeValue(!valuesEqual(a, b)));
            break;
        }
        case 9: {  // LSS (<)
            RuntimeValue b = stack.pop();
            RuntimeValue a = stack.pop();
            stack.push(RuntimeValue(valuesLess(a, b, false)));
            break;
        }
        case 10: {  // GEQ (>=)
            RuntimeValue b = stack.pop();
            RuntimeValue a = stack.pop();
            stack.push(RuntimeValue(!valuesLess(a, b, false)));  // !(a < b) = a >= b
            break;
        }
        case 11: {  // GTR (>)
            RuntimeValue b = stack.pop();
            RuntimeValue a = stack.pop();
            stack.push(RuntimeValue(valuesLess(b, a, false)));
            break;
        }
        case 12: {  // LEQ (<=)
            RuntimeValue b = stack.pop();
            RuntimeValue a = stack.pop();
            stack.push(RuntimeValue(!valuesLess(b, a, false)));  // !(b < a) = b >= a = a <= b
            break;
        }
        case 13: {  // WRT
            RuntimeValue v = stack.pop();
            result.output += runtimeValueToString(v);
            break;
        }
        case 14: {  // WRTLN
            if (stack.emptyOperandStack()) {
                // Empty writeln: just print newline
                result.output += "\n";
            } else {
                RuntimeValue v = stack.pop();
                result.output += runtimeValueToString(v) + "\n";
            }
            break;
        }
        case 15: {  // READ (extension) — reads integer from input source
            int input;
            if (result.inputSource && !result.inputSource->empty()) {
                // Read from string source
                std::istringstream iss(*result.inputSource);
                // Find the next integer from the current position
                int pos = result.inputPos;
                iss.seekg(pos);
                if (iss >> input) {
                    result.inputPos = (int)iss.tellg();
                } else {
                    input = 0;
                }
            } else {
                std::cin >> input;
            }
            stack.push(RuntimeValue(input));
            break;
        }
        case 16: {  // READLN (extension)
            int input;
            if (result.inputSource && !result.inputSource->empty()) {
                std::istringstream iss(*result.inputSource);
                int pos = result.inputPos;
                iss.seekg(pos);
                if (iss >> input) {
                    result.inputPos = (int)iss.tellg();
                }
                // Skip to next line
                std::string dummy;
                std::getline(iss, dummy);
                result.inputPos = (int)iss.tellg();
            } else {
                std::cin >> input;
                std::cin.ignore(10000, '\n');
            }
            stack.push(RuntimeValue(input));
            break;
        }
        default:
            reportRuntimeError("Unknown OPR code: " + std::to_string(opNum));
            break;
    }
}

void Interpreter::executeReturn(const Instruction& instruction) {
    bool isFunction = (requireIntegerArgument(instruction) == 1);
    RuntimeValue retVal;
    if (isFunction) {
        retVal = stack.pop();
    }

    FrameInfo info = stack.popFrame();
    instructionPointer = info.returnAddress;

    if (isFunction) {
        stack.push(retVal);
    }
}

RuntimeValue Interpreter::makeRuntimeValue(const InstructionArgument& argument) const {
    if (std::holds_alternative<int>(argument)) {
        return RuntimeValue(std::get<int>(argument));
    } else if (std::holds_alternative<double>(argument)) {
        return RuntimeValue(std::get<double>(argument));
    } else if (std::holds_alternative<bool>(argument)) {
        return RuntimeValue(std::get<bool>(argument));
    } else if (std::holds_alternative<char>(argument)) {
        return RuntimeValue(std::get<char>(argument));
    } else if (std::holds_alternative<std::string>(argument)) {
        return RuntimeValue(std::get<std::string>(argument));
    }
    return RuntimeValue(0);
}

int Interpreter::requireIntegerArgument(const Instruction& instruction) const {
    if (std::holds_alternative<int>(instruction.argument)) {
        return std::get<int>(instruction.argument);
    }
    // We can't call reportRuntimeError from const method
    // Just return 0
    return 0;
}

int Interpreter::requireOperationCode(const Instruction& instruction) const {
    if (std::holds_alternative<int>(instruction.argument)) {
        return std::get<int>(instruction.argument);
    }
    return 0;
}

void Interpreter::reportRuntimeError(const std::string& message) {
    result.errors.push_back(InterpreterError(message, instructionPointer));
}

void Interpreter::executeLoadAddress(const Instruction& instruction) {
    int offset = requireIntegerArgument(instruction);
    int addr = stack.resolveAddress(instruction.level, offset);
    // Push the absolute address as an integer onto the operand stack
    stack.push(RuntimeValue(addr));
    instructionPointer++;
}

void Interpreter::executeLoadIndirect(const Instruction& instruction) {
    RuntimeValue addrVal = stack.pop();
    if (addrVal.type != RuntimeType::INTEGER) {
        reportRuntimeError("LDI: expected integer address on stack");
        stack.push(RuntimeValue(0));
        instructionPointer++;
        return;
    }
    int addr = std::get<int>(addrVal.value);
    RuntimeValue val = stack.load(addr);
    stack.push(val);
    instructionPointer++;
}

void Interpreter::executeStoreIndirect(const Instruction& instruction) {
    RuntimeValue addrVal = stack.pop();
    if (addrVal.type != RuntimeType::INTEGER) {
        reportRuntimeError("STI: expected integer address on stack");
        instructionPointer++;
        return;
    }
    int addr = std::get<int>(addrVal.value);
    RuntimeValue val = stack.pop();
    stack.store(addr, val);
    instructionPointer++;
}

bool Interpreter::isValidJumpTarget(int target, int instructionCount) const {
    return target >= 0 && target < instructionCount;
}

bool Interpreter::valuesEqual(const RuntimeValue& a, const RuntimeValue& b) const {
    if (a.type == RuntimeType::INTEGER && b.type == RuntimeType::INTEGER) {
        return std::get<int>(a.value) == std::get<int>(b.value);
    }
    if (a.type == RuntimeType::REAL || b.type == RuntimeType::REAL) {
        return runtimeToDouble(a) == runtimeToDouble(b);
    }
    if (a.type == RuntimeType::BOOLEAN && b.type == RuntimeType::BOOLEAN) {
        return std::get<bool>(a.value) == std::get<bool>(b.value);
    }
    if (a.type == RuntimeType::CHAR && b.type == RuntimeType::CHAR) {
        return std::get<char>(a.value) == std::get<char>(b.value);
    }
    if (a.type == RuntimeType::STRING && b.type == RuntimeType::STRING) {
        return std::get<std::string>(a.value) == std::get<std::string>(b.value);
    }
    return false;
}

bool Interpreter::valuesLess(const RuntimeValue& a, const RuntimeValue& b, bool orEqual) const {
    if (a.type == RuntimeType::INTEGER && b.type == RuntimeType::INTEGER) {
        int ia = std::get<int>(a.value);
        int ib = std::get<int>(b.value);
        return orEqual ? (ia <= ib) : (ia < ib);
    }
    if (a.type == RuntimeType::REAL || b.type == RuntimeType::REAL) {
        double da = runtimeToDouble(a);
        double db = runtimeToDouble(b);
        return orEqual ? (da <= db) : (da < db);
    }
    if (a.type == RuntimeType::CHAR && b.type == RuntimeType::CHAR) {
        char ca = std::get<char>(a.value);
        char cb = std::get<char>(b.value);
        return orEqual ? (ca <= cb) : (ca < cb);
    }
    return false;
}
