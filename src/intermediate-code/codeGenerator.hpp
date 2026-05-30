#ifndef CODE_GENERATOR_HPP
#define CODE_GENERATOR_HPP

#include <string>
#include <vector>
#include "../semantic-analysis/astNode.hpp"
#include "../semantic-analysis/symbolTable.hpp"
#include "instruction.hpp"

struct CodeGenerationError {
    std::string message;
    int line;

    CodeGenerationError(const std::string& message, int line = 0)
        : message(message), line(line) {}
};

struct CodeGenerationResult {
    std::vector<Instruction> instructions;
    std::vector<CodeGenerationError> errors;

    bool ok() const { return errors.empty(); }
};

// Converts decorated AST + symbol table into Intermeddiate Code
class CodeGenerator {
public:
    CodeGenerationResult generate(ProgramNode* root, const SymbolTable& symbols);

private:
    // Offset (static link, dynamic link, return address)
    static constexpr int FRAME_HEADER_SIZE = 3; 

    // Field
    const SymbolTable* symbols = nullptr;
    CodeGenerationResult result;

    int add(InstructionCode code, int level, InstructionArgument argument, const std::string& comment = "");
    void editInstruction(int instructionIndex, int value);
    void reportError(const std::string& message, const ASTNode* node = nullptr);
    
    // Code Generation Functions
    void generateProgram(ProgramNode* node);
    void generateDeclarations(DeclarationsNode* node);
    void generateStatement(ASTNode* node);
    void generateBlock(BlockNode* node);
    void generateAssign(AssignNode* node);
    void generateIf(IfNode* node);
    void generateWhile(WhileNode* node);
    void generateRepeat(RepeatNode* node);
    void generateFor(ForNode* node);
    void generateCase(CaseNode* node);
    void generateCall(ProcCallNode* node);

    void generateExpression(ASTNode* node);
    void generateVariableLoad(VarNode* node);
    void generateVariableStore(VarNode* node);
    void generateBinaryOperation(BinOpNode* node);
    void generateUnaryOperation(UnaryOpNode* node);

    // Helper Functions
    int currentLine() const;
    int runtimeAddressForTab(int tabIndex) const;
    int programFrameSize(ProgramNode* node) const;
    OperationCode operationForBinaryOperator(const std::string& op) const;
    OperationCode operationForUnaryOperator(const std::string& op) const;
    bool isBuiltinWriteCall(const ProcCallNode* node) const;
    bool isBuiltinReadCall(const ProcCallNode* node) const;
};

#endif
