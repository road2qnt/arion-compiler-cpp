#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP

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

    InterpreterResult() : executedInstructionCount(0) {}
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
    void executeJump(const Instruction& instruction);
    void executeConditionalJump(const Instruction& instruction);
    void executeOperation(const Instruction& instruction);
    void executeReturn(const Instruction& instruction);

    RuntimeValue makeRuntimeValue(const InstructionArgument& argument) const;
    int requireIntegerArgument(const Instruction& instruction) const;
    OperationCode requireOperationCode(const Instruction& instruction) const;
    void reportRuntimeError(const std::string& message);
    bool isValidJumpTarget(int target, int instructionCount) const;
};

#endif
