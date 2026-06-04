#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP

#include <memory>
#include <ostream>
#include <string>
#include <vector>
#include "../intermediate-code/instruction.hpp"
#include "runtimeValue.hpp"
#include "stackMachine.hpp"

struct InterpreterError {
    std::string message;
    int instructionPointer;

    InterpreterError(const std::string& message, int instructionPointer = -1)
        : message(message), instructionPointer(instructionPointer) {}
};

struct InterpreterResult {
    std::string output;
    std::vector<InterpreterError> errors;
    int executedInstructionCount;
    std::shared_ptr<std::string> inputSource;  // For read/readln operations (shared to allow sharing)
    int inputPos;

    InterpreterResult() : executedInstructionCount(0), inputPos(0) {}
    bool ok() const { return errors.empty(); }
};

// Executes Intermediate Code 
class Interpreter {
public:
    InterpreterResult run(const std::vector<Instruction>& instructions);

private:
    StackMachine stack;
    InterpreterResult result;
    int instructionPointer = 0;

    void execute(const Instruction& instruction);
    void executeLiteral(const Instruction& instruction);
    void executeLoad(const Instruction& instruction);
    void executeStore(const Instruction& instruction);
    void executeCall(const Instruction& instruction);
    void executeJump(const Instruction& instruction);
    void executeConditionalJump(const Instruction& instruction);
    void executeOperation(const Instruction& instruction);
    void executeReturn(const Instruction& instruction);
    void executeLoadAddress(const Instruction& instruction);
    void executeLoadIndirect(const Instruction& instruction);
    void executeStoreIndirect(const Instruction& instruction);

    RuntimeValue makeRuntimeValue(const InstructionArgument& argument) const;
    int requireIntegerArgument(const Instruction& instruction) const;
    int requireOperationCode(const Instruction& instruction) const;
    void reportRuntimeError(const std::string& message);
    bool isValidJumpTarget(int target, int instructionCount) const;

    // Value comparison helpers
    bool valuesEqual(const RuntimeValue& a, const RuntimeValue& b) const;
    bool valuesLess(const RuntimeValue& a, const RuntimeValue& b, bool orEqual) const;
};

#endif
