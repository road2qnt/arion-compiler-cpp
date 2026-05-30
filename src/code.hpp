#ifndef MILESTONE4_FLOW_HPP
#define MILESTONE4_FLOW_HPP

#include <string>
#include <vector>
#include "intermediate-code/codeGenerator.hpp"
#include "interpreter/interpreter.hpp"
#include "semantic-analysis/semanticAnalyzer.hpp"

struct codeOutput {
    CodeGenerationResult codegen;
    InterpreterResult runtime;

    bool ok() const {
        return codegen.ok() && runtime.ok();
    }
};

// Command Flow: 
// 1. Run lexical + syntax + semantic analysis.
// 2. Stop if semanticAnalyzer.hasErrors() is true.
// 3. Generate intermediate code from decorated AST + symbol table.
// 4. Execute generated code with the stack-machine interpreter.
codeOutput runCode(SemanticAnalyzer& semanticAnalyzer);

#endif
