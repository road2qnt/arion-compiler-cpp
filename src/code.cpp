#include "code.hpp"
#include <iostream>

codeOutput runCode(SemanticAnalyzer& semanticAnalyzer) {
    codeOutput output;

    // Get decorated AST and symbol table
    ProgramNode* ast = semanticAnalyzer.getAST();
    SymbolTable& symTab = semanticAnalyzer.getSymbolTable();

    if (!ast) {
        CodeGenerationError err("No AST available - semantic analysis may have failed", 0);
        output.codegen.errors.push_back(err);
        return output;
    }

    // Generate intermediate code
    CodeGenerator codegen;
    output.codegen = codegen.generate(ast, symTab);

    if (!output.codegen.ok()) {
        // Code generation had errors - return early
        return output;
    }

    // Execute the generated code
    Interpreter interpreter;
    output.runtime = interpreter.run(output.codegen.instructions);

    return output;
}


