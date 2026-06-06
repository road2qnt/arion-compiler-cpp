#include "codeGenerator.hpp"
#include <algorithm>

CodeGenerationResult CodeGenerator::generate(ProgramNode* root, const SymbolTable& syms) {
    this->symbols = &syms;
    result = CodeGenerationResult();
    subprogramEntries.clear();
    currentCaseTempAddress = -1;

    if (root) {
        // Phase 1: Collect all subprogram entry points (traverse AST to find all SubprogramDeclNode)
        collectSubprogramEntries(root);

        // Phase 2: Generate code
        generateProgram(root);
    }

    return result;
}

// Utility

void CodeGenerator::collectSubprogramEntries(ProgramNode* node) {
    if (node && node->declarations) {
        for (auto* decl : node->declarations->decls) {
            if (auto* sub = dynamic_cast<SubprogramDeclNode*>(decl)) {
                subprogramEntries[sub->tabIndex] = -1;  // placeholder
                collectSubprogramEntriesFromSub(sub);
            }
        }
    }
}

void CodeGenerator::collectSubprogramEntriesFromSub(SubprogramDeclNode* node) {
    if (node && node->localDecls) {
        for (auto* decl : node->localDecls->decls) {
            if (auto* sub = dynamic_cast<SubprogramDeclNode*>(decl)) {
                subprogramEntries[sub->tabIndex] = -1;
                collectSubprogramEntriesFromSub(sub);
            }
        }
    }
}

int CodeGenerator::add(InstructionCode code, int level, InstructionArgument argument, const std::string& comment) {
    result.instructions.push_back(Instruction(code, level, argument, comment));
    return (int)result.instructions.size() - 1;
}

void CodeGenerator::editInstruction(int instructionIndex, int value) {
    if (instructionIndex >= 0 && instructionIndex < (int)result.instructions.size()) {
        result.instructions[instructionIndex].argument = value;
    }
}

int CodeGenerator::currentLine() const {
    return (int)result.instructions.size();
}

void CodeGenerator::reportError(const std::string& message, const ASTNode* node) {
    int line = node ? node->line : 0;
    result.errors.push_back(CodeGenerationError(message, line));
}

int CodeGenerator::programFrameSize(ProgramNode* node) const {
    int progTabIdx = (node && node->tabIndex >= 0) ? node->tabIndex : -1;

    // Use the program's tab entry ref to find the correct block (same as subprogramFrameSize)
    if (progTabIdx >= 0 && progTabIdx < (int)symbols->getTabSize()) {
        int progBtab = symbols->getTab(progTabIdx).ref;
        if (progBtab >= 0 && progBtab < (int)symbols->getBtabSize()) {
            return FRAME_HEADER_SIZE + symbols->getBtab(progBtab).vsze;
        }
    }

    // Fallback: scan blocks
    for (int i = (int)symbols->getBtabSize() - 1; i >= 1; --i) {
        const auto& b = symbols->getBtab(i);
        if (progTabIdx >= 0 && b.last >= progTabIdx) {
            return FRAME_HEADER_SIZE + b.vsze;
        }
    }
    return FRAME_HEADER_SIZE;
}

int CodeGenerator::subprogramFrameSize(const SubprogramDeclNode* node) const {
    int subBtab = -1;
    if (node->tabIndex >= 0 && node->tabIndex < (int)symbols->getTabSize()) {
        subBtab = symbols->getTab(node->tabIndex).ref;
        if (subBtab >= 0 && subBtab < (int)symbols->getBtabSize()) {
            return FRAME_HEADER_SIZE + symbols->getBtab(subBtab).vsze;
        }
        for (int i = (int)symbols->getBtabSize() - 1; i >= 1; --i) {
            const auto& b = symbols->getBtab(i);
            if (b.last >= node->tabIndex) {
                return FRAME_HEADER_SIZE + b.vsze;
            }
        }
    }
    return FRAME_HEADER_SIZE;
}

int CodeGenerator::subprogramParamSize(const SubprogramDeclNode* node) const {
    if (node->tabIndex < 0) return 0;
    int subBtab = symbols->getTab(node->tabIndex).ref;
    if (subBtab <= 0 || subBtab >= (int)symbols->getBtabSize()) return 0;
    return symbols->getBtab(subBtab).psze;
}

OperationCode CodeGenerator::operationForBinaryOperator(const std::string& op) const {
    if (op == "+") return OperationCode::ADD;
    if (op == "-") return OperationCode::SUB;
    if (op == "*") return OperationCode::MUL;
    if (op == "/" || op == "rdiv") return OperationCode::DIV;
    if (op == "div") return OperationCode::DIV;
    if (op == "mod") return OperationCode::MOD;
    if (op == "=")   return OperationCode::EQL;
    if (op == "<>")  return OperationCode::NEQ;
    if (op == "<")   return OperationCode::LSS;
    if (op == "<=")  return OperationCode::LEQ;
    if (op == ">")   return OperationCode::GTR;
    if (op == ">=")  return OperationCode::GEQ;
    if (op == "and") return OperationCode::MUL;
    if (op == "or")  return OperationCode::ADD;
    return OperationCode::ADD;
}

bool CodeGenerator::isBuiltinWriteCall(const ProcCallNode* node) const {
    if (!node || node->tabIndex < 0 || node->tabIndex >= (int)symbols->getTabSize()) return false;
    const TabEntry& e = symbols->getTab(node->tabIndex);
    std::string lo;
    for (char c : e.id) lo += (char)std::tolower((unsigned char)c);
    return (e.lev == 0 && (lo == "write" || lo == "writeln"));
}

bool CodeGenerator::isBuiltinReadCall(const ProcCallNode* node) const {
    if (!node || node->tabIndex < 0 || node->tabIndex >= (int)symbols->getTabSize()) return false;
    const TabEntry& e = symbols->getTab(node->tabIndex);
    std::string lo;
    for (char c : e.id) lo += (char)std::tolower((unsigned char)c);
    return (e.lev == 0 && (lo == "read" || lo == "readln"));
}

bool CodeGenerator::containsCase(ASTNode* node) const {
    if (!node) return false;

    if (dynamic_cast<CaseNode*>(node)) return true;

    if (auto* block = dynamic_cast<BlockNode*>(node)) {
        for (auto* stmt : block->statements) {
            if (containsCase(stmt)) return true;
        }
    } else if (auto* ifNode = dynamic_cast<IfNode*>(node)) {
        return containsCase(ifNode->thenBranch) || containsCase(ifNode->elseBranch);
    } else if (auto* whileNode = dynamic_cast<WhileNode*>(node)) {
        return containsCase(whileNode->body);
    } else if (auto* repeatNode = dynamic_cast<RepeatNode*>(node)) {
        return containsCase(repeatNode->body);
    } else if (auto* forNode = dynamic_cast<ForNode*>(node)) {
        return containsCase(forNode->body);
    } else if (auto* sub = dynamic_cast<SubprogramDeclNode*>(node)) {
        return containsCase(sub->body);
    }

    return false;
}

// Program

void CodeGenerator::generateProgram(ProgramNode* node) {
    if (!node) return;

    currentLevel = 1;

    // JMP at start to skip past subprogram bodies
    int skipJmp = add(InstructionCode::JMP, 0, 0, "skip subprogram bodies");

    // Generate all subprogram bodies first (recording their entry points)
    generateSubprogramBodies(node);

    // Patch skip JMP to point to main program
    editInstruction(skipJmp, currentLine());

    // Generate main program body
    int baseFrameSize = programFrameSize(node);
    bool needsCaseTemp = containsCase(node->body);
    int frameSize = baseFrameSize + (needsCaseTemp ? 1 : 0);
    int savedCaseTempAddress = currentCaseTempAddress;
    currentCaseTempAddress = needsCaseTemp ? baseFrameSize : -1;
    add(InstructionCode::INT, 0, frameSize, "main program frame (" + std::to_string(frameSize) + " slots)");

    if (node->body) {
        generateStatement(node->body);
    }

    add(InstructionCode::RET, 0, 0, "program end");

    // Restore level
    currentLevel = 1;
    currentCaseTempAddress = savedCaseTempAddress;
}

void CodeGenerator::generateSubprogramBodies(ProgramNode* node) {
    if (!node || !node->declarations) return;

    for (auto* decl : node->declarations->decls) {
        if (auto* sub = dynamic_cast<SubprogramDeclNode*>(decl)) {
            generateSingleSubprogram(sub);
        }
    }
}

void CodeGenerator::generateSingleSubprogram(SubprogramDeclNode* sub) {
    // Record entry point
    subprogramEntries[sub->tabIndex] = currentLine();

    // Update current lexical level (subprogram body runs at level sub->lev + 1)
    int savedLevel = currentLevel;
    if (sub->tabIndex >= 0 && sub->tabIndex < (int)symbols->getTabSize()) {
        currentLevel = symbols->getTab(sub->tabIndex).lev + 1;
    } else {
        currentLevel = savedLevel + 1;
    }

    // Allocate frame
    int baseFrameSize = subprogramFrameSize(sub);
    bool needsCaseTemp = containsCase(sub->body);
    int totalFrameSize = baseFrameSize + (needsCaseTemp ? 1 : 0);
    int savedCaseTempAddress = currentCaseTempAddress;
    currentCaseTempAddress = needsCaseTemp ? baseFrameSize : -1;
    add(InstructionCode::INT, 0, totalFrameSize, sub->name + " frame (" + std::to_string(totalFrameSize) + " slots)");

    // Copy arguments from operand stack to parameter memory slots
    for (int i = (int)sub->params.size() - 1; i >= 0; i--) {
        auto* param = sub->params[i];
        if (param && param->tabIndex >= 0) {
            int adr = FRAME_HEADER_SIZE + symbols->getTab(param->tabIndex).adr;
            add(InstructionCode::STO, 0, adr, "param " + param->name);
        }
    }

    // Process nested subprogram declarations (if any)
    if (sub->localDecls) {
        for (auto* decl : sub->localDecls->decls) {
            if (auto* nested = dynamic_cast<SubprogramDeclNode*>(decl)) {
                int skipJmp = add(InstructionCode::JMP, 0, 0, "skip nested subprograms");
                generateSingleSubprogram(nested);
                editInstruction(skipJmp, currentLine());
            }
        }
    }

    // Generate body statements
    if (sub->body) {
        generateStatement(sub->body);
    }

    // For functions, push return value onto operand stack before RET
    if (sub->isFunction) {
        int funcLev = (sub->tabIndex >= 0 && sub->tabIndex < (int)symbols->getTabSize())
            ? symbols->getTab(sub->tabIndex).lev : -1;
        for (int ti = 0; ti < (int)symbols->getTabSize(); ti++) {
            const TabEntry& e = symbols->getTab(ti);
            if (e.id == sub->name && e.obj == OBJ_VARIABLE && e.lev == funcLev + 1) {
                int adr = FRAME_HEADER_SIZE + e.adr;
                add(InstructionCode::LOD, 0, adr, "load " + sub->name + " return value");
                break;
            }
        }
    }

    // Return
    add(InstructionCode::RET, 0, sub->isFunction ? 1 : 0, "return from " + sub->name);

    // Restore level
    currentLevel = savedLevel;
    currentCaseTempAddress = savedCaseTempAddress;
}

// Statements

void CodeGenerator::generateStatement(ASTNode* node) {
    if (!node) return;

    if (auto* block = dynamic_cast<BlockNode*>(node)) {
        for (auto* stmt : block->statements) {
            generateStatement(stmt);
        }
    } else if (auto* assign = dynamic_cast<AssignNode*>(node)) {
        generateAssign(assign);
    } else if (auto* ifNode = dynamic_cast<IfNode*>(node)) {
        generateIf(ifNode);
    } else if (auto* whileNode = dynamic_cast<WhileNode*>(node)) {
        generateWhile(whileNode);
    } else if (auto* repeatNode = dynamic_cast<RepeatNode*>(node)) {
        generateRepeat(repeatNode);
    } else if (auto* forNode = dynamic_cast<ForNode*>(node)) {
        generateFor(forNode);
    } else if (auto* caseNode = dynamic_cast<CaseNode*>(node)) {
        generateCase(caseNode);
    } else if (auto* call = dynamic_cast<ProcCallNode*>(node)) {
        generateCall(call);
    } else if (auto* sub = dynamic_cast<SubprogramDeclNode*>(node)) {
        if (sub->body) generateStatement(sub->body);
    }
}

void CodeGenerator::generateAssign(AssignNode* node) {
    if (!node) return;

    if (node->value) generateExpression(node->value);
    else add(InstructionCode::LIT, 0, 0, "default value");

    if (node->target) generateVariableStore(node->target);
}

void CodeGenerator::generateIf(IfNode* node) {
    if (!node) return;

    if (node->condition) generateExpression(node->condition);

    int jpcIdx = add(InstructionCode::JPC, 0, 0, "if: skip then");

    if (node->thenBranch) generateStatement(node->thenBranch);

    if (node->elseBranch) {
        int jmpIdx = add(InstructionCode::JMP, 0, 0, "if: skip else");
        editInstruction(jpcIdx, currentLine());
        generateStatement(node->elseBranch);
        editInstruction(jmpIdx, currentLine());
    } else {
        editInstruction(jpcIdx, currentLine());
    }
}

void CodeGenerator::generateWhile(WhileNode* node) {
    if (!node) return;

    int startIdx = currentLine();

    if (node->condition) generateExpression(node->condition);

    int jpcIdx = add(InstructionCode::JPC, 0, 0, "while: exit");

    if (node->body) generateStatement(node->body);

    add(InstructionCode::JMP, 0, startIdx, "while: loop");

    editInstruction(jpcIdx, currentLine());
}

void CodeGenerator::generateRepeat(RepeatNode* node) {
    if (!node) return;

    int startIdx = currentLine();

    if (node->body) generateStatement(node->body);

    if (node->condition) generateExpression(node->condition);

    // Jump back if condition is false (repeat UNTIL true)
    add(InstructionCode::JPC, 0, startIdx, "repeat: loop");
}

void CodeGenerator::generateFor(ForNode* node) {
    if (!node || node->varTabIndex < 0) return;

    int varAdr = FRAME_HEADER_SIZE + symbols->getTab(node->varTabIndex).adr;

    // Initialize loop variable
    if (node->start) generateExpression(node->start);
    else add(InstructionCode::LIT, 0, 0, "default start");
    add(InstructionCode::STO, 0, varAdr, "for: init");

    int checkIdx = currentLine();

    // Load var and end value, compare
    add(InstructionCode::LOD, 0, varAdr, "for: load var");
    if (node->end) generateExpression(node->end);
    else add(InstructionCode::LIT, 0, 0, "default end");

    if (node->isDownto) {
        // downto: continue while var >= end, exit when var < end
        add(InstructionCode::OPR, 0, operationNumber(OperationCode::GEQ), "for: check >=");
    } else {
        // to: continue while var <= end, exit when var > end
        add(InstructionCode::OPR, 0, operationNumber(OperationCode::LEQ), "for: check <=");
    }

    int jpcIdx = add(InstructionCode::JPC, 0, 0, "for: exit");

    if (node->body) generateStatement(node->body);

    // Increment/decrement
    add(InstructionCode::LOD, 0, varAdr, "for: load var for inc");
    add(InstructionCode::LIT, 0, 1, "for: 1");
    add(InstructionCode::OPR, 0,
        operationNumber(node->isDownto ? OperationCode::SUB : OperationCode::ADD),
        "for: inc/dec");
    add(InstructionCode::STO, 0, varAdr, "for: store var");

    add(InstructionCode::JMP, 0, checkIdx, "for: back to check");

    editInstruction(jpcIdx, currentLine());
}

void CodeGenerator::generateCase(CaseNode* node) {
    if (!node || !node->expression) return;
    if (currentCaseTempAddress < 0) {
        reportError("case temporary slot was not allocated", node);
        return;
    }

    generateExpression(node->expression);
    int tempAdr = currentCaseTempAddress;
    add(InstructionCode::STO, 0, tempAdr, "case: save expr");

    std::vector<int> endJumps;

    for (auto* branch : node->branches) {
        if (!branch) continue;

        std::vector<int> bodyJumps;
        for (size_t ci = 0; ci < branch->constants.size(); ci++) {
            add(InstructionCode::LOD, 0, tempAdr, "case: load expr");
            generateExpression(branch->constants[ci]);
            add(InstructionCode::OPR, 0, operationNumber(OperationCode::EQL), "case: cmp");

            int jpc = add(InstructionCode::JPC, 0, 0, "case: next constant");
            bodyJumps.push_back(add(InstructionCode::JMP, 0, 0, "case: matched branch"));
            editInstruction(jpc, currentLine());
        }

        int nextBranchJump = add(InstructionCode::JMP, 0, 0, "case: next branch");
        int bodyStart = currentLine();
        for (int jump : bodyJumps) {
            editInstruction(jump, bodyStart);
        }

        if (branch->statement) generateStatement(branch->statement);

        endJumps.push_back(add(InstructionCode::JMP, 0, 0, "case: end of branch"));
        editInstruction(nextBranchJump, currentLine());
    }

    int endIdx = currentLine();
    for (int jump : endJumps) {
        editInstruction(jump, endIdx);
    }
}

void CodeGenerator::generateCall(ProcCallNode* node) {
    if (!node) return;

    // Handle built-in write/writeln
    if (isBuiltinWriteCall(node)) {
        bool isWriteln = false;
        if (node->tabIndex >= 0 && node->tabIndex < (int)symbols->getTabSize()) {
            std::string nm;
            for (char c : symbols->getTab(node->tabIndex).id) nm += (char)std::tolower((unsigned char)c);
            isWriteln = (nm == "writeln");
        }

        if (node->args.empty() && isWriteln) {
            add(InstructionCode::OPR, 0, operationNumber(OperationCode::WRTLN), "writeln empty");
            return;
        }

        // Push arguments RIGHT-to-LEFT so leftmost arg ends up on top
        for (int i = (int)node->args.size() - 1; i >= 0; i--) {
            if (node->args[i]) generateExpression(node->args[i]);
        }

        // Emit OPR for each arg left-to-right
        for (size_t i = 0; i < node->args.size(); i++) {
            bool lastArg = (i == node->args.size() - 1);
            if (lastArg && isWriteln) {
                add(InstructionCode::OPR, 0, operationNumber(OperationCode::WRTLN), "writeln arg");
            } else {
                add(InstructionCode::OPR, 0, operationNumber(OperationCode::WRT), "write arg");
            }
        }
        return;
    }

    // Handle built-in read/readln
    if (isBuiltinReadCall(node)) {
        for (auto* arg : node->args) {
            if (auto* var = dynamic_cast<VarNode*>(arg)) {
                add(InstructionCode::OPR, 0, 15, "read value");
                generateVariableStore(var);
            }
        }
        return;
    }

    // User-defined subprogram call
    if (node->tabIndex < 0 || node->tabIndex >= (int)symbols->getTabSize()) {
        reportError("unknown subprogram: " + node->name, node);
        return;
    }

    const TabEntry& e = symbols->getTab(node->tabIndex);
    int entryPoint = subprogramEntries[node->tabIndex];
    if (entryPoint < 0) {
        reportError("subprogram entry not found: " + node->name, node);
        return;
    }

    // Determine parameter modes (PARAM_VALUE or PARAM_REF) from symbol table
    std::vector<int> paramModes;
    int subBtab = e.ref;
    if (subBtab > 0 && subBtab < (int)symbols->getBtabSize()) {
        int subLev = e.lev + 1;
        int cur = symbols->getBtab(subBtab).lpar;
        std::vector<int> paramIndices;
        while (cur > 0 && cur < (int)symbols->getTabSize()) {
            const TabEntry& p = symbols->getTab(cur);
            if (p.lev != subLev || p.obj != OBJ_VARIABLE) break;
            if (e.obj == OBJ_FUNCTION && p.id == node->name) break;
            paramIndices.push_back(cur);
            cur = p.link;
        }
        std::reverse(paramIndices.begin(), paramIndices.end());
        for (int pi : paramIndices) {
            paramModes.push_back(symbols->getTab(pi).nrm);
        }
    }

    // Push arguments in order (left to right)
    for (size_t i = 0; i < node->args.size(); i++) {
        auto* arg = node->args[i];
        if (!arg) continue;
        bool isVar = (i < paramModes.size() && paramModes[i] == PARAM_REF);
        if (isVar) {
            // Var parameter: push address
            if (auto* var = dynamic_cast<VarNode*>(arg)) {
                // Use generateVariableAddress which handles simple vars, array elements, and record fields
                generateVariableAddress(var);
            } else {
                reportError("var parameter must be a variable: " + node->name, node);
            }
        } else {
            // Value parameter: push value
            generateExpression(arg);
        }
    }

    int levelDiff = currentLevel - e.lev;
    if (levelDiff < 0) levelDiff = 0;

    add(InstructionCode::CAL, levelDiff, entryPoint, "call " + node->name);
}

// Expressions

void CodeGenerator::generateExpression(ASTNode* node) {
    if (!node) return;

    if (auto* num = dynamic_cast<NumberNode*>(node)) {
        add(InstructionCode::LIT, 0, num->value, "int " + std::to_string(num->value));
    } else if (auto* real = dynamic_cast<RealNode*>(node)) {
        add(InstructionCode::LIT, 0, real->value, "real " + std::to_string(real->value));
    } else if (auto* ch = dynamic_cast<CharNode*>(node)) {
        add(InstructionCode::LIT, 0, ch->value, "char '" + std::string(1, ch->value) + "'");
    } else if (auto* str = dynamic_cast<StringNode*>(node)) {
        add(InstructionCode::LIT, 0, str->value, "string \"" + str->value + "\"");
    } else if (auto* b = dynamic_cast<BoolNode*>(node)) {
        add(InstructionCode::LIT, 0, b->value ? 1 : 0, "bool " + std::string(b->value ? "true" : "false"));
    } else if (auto* var = dynamic_cast<VarNode*>(node)) {
        generateVariableLoad(var);
    } else if (auto* bin = dynamic_cast<BinOpNode*>(node)) {
        generateBinaryOperation(bin);
    } else if (auto* unary = dynamic_cast<UnaryOpNode*>(node)) {
        generateUnaryOperation(unary);
    } else if (auto* call = dynamic_cast<ProcCallNode*>(node)) {
        generateCall(call);
    } else if (auto* block = dynamic_cast<BlockNode*>(node)) {
        for (auto* s : block->statements) generateStatement(s);
    }
}

void CodeGenerator::generateVariableLoad(VarNode* node) {
    if (!node || node->tabIndex < 0) {
        add(InstructionCode::LIT, 0, 0, "unknown var");
        return;
    }

    const TabEntry& e = symbols->getTab(node->tabIndex);

    // If variable has components (array access, record field), compute address and load indirect
    if (!node->components.empty() || node->isArrayAccess) {
        generateVariableAddress(node);
        add(InstructionCode::LDI, 0, 0, "load " + node->name + " val via addr");
        return;
    }

    int levelDiff = currentLevel - e.lev;
    if (levelDiff < 0) levelDiff = 0;
    int adr = FRAME_HEADER_SIZE + e.adr;

    if (e.nrm == PARAM_REF) {
        // Var parameter: load address from param slot, then indirect load
        add(InstructionCode::LOD, levelDiff, adr, "load " + node->name + " addr");
        add(InstructionCode::LDI, 0, 0, "load " + node->name + " val (ref)");
    } else {
        add(InstructionCode::LOD, levelDiff, adr, "load " + node->name);
    }
}

void CodeGenerator::generateVariableStore(VarNode* node) {
    if (!node || node->tabIndex < 0) return;

    const TabEntry& e = symbols->getTab(node->tabIndex);

    // If variable has components (array access, record field), compute address dynamically
    if (!node->components.empty() || node->isArrayAccess) {
        generateVariableAddress(node);
        // Stack now: [address] [value to store]
        // STI pops address (top) then value, stores value at address
        add(InstructionCode::STI, 0, 0, "store " + node->name + " via address");
        return;
    }

    int levelDiff = currentLevel - e.lev;
    if (levelDiff < 0) levelDiff = 0;
    int adr = FRAME_HEADER_SIZE + e.adr;

    if (e.nrm == PARAM_REF) {
        // Var parameter: load address from param slot first, then store indirect
        add(InstructionCode::LOD, levelDiff, adr, "load " + node->name + " addr");
        add(InstructionCode::STI, 0, 0, "store " + node->name + " (ref)");
    } else {
        add(InstructionCode::STO, levelDiff, adr, "store " + node->name);
    }
}

void CodeGenerator::generateVariableAddress(VarNode* node) {
    if (!node || node->tabIndex < 0) {
        add(InstructionCode::LIT, 0, 0, "unknown var address");
        return;
    }

    const TabEntry& baseEntry = symbols->getTab(node->tabIndex);
    int levelDiff = currentLevel - baseEntry.lev;
    if (levelDiff < 0) levelDiff = 0;
    int baseAdr = FRAME_HEADER_SIZE + baseEntry.adr;

    // If it's a simple variable (no components, no array access), push its address
    if (node->components.empty() && !node->isArrayAccess && node->fieldName.empty()) {
        if (baseEntry.nrm == PARAM_REF) {
            // Var parameter: the slot contains an address; load it with LOD
            add(InstructionCode::LOD, levelDiff, baseAdr, "load addr from var param " + node->name);
        } else {
            add(InstructionCode::LDA, levelDiff, baseAdr, "addr " + node->name);
        }
        return;
    }

    // For variables with components, start with the base variable's address
    if (baseEntry.nrm == PARAM_REF) {
        add(InstructionCode::LOD, levelDiff, baseAdr, "load base addr from var param " + node->name);
    } else {
        add(InstructionCode::LDA, levelDiff, baseAdr, "base addr " + node->name);
    }

    // Walk through components to compute dynamic offsets
    // We need to track the current type/ref to resolve array metadata
    int currentType = baseEntry.type;
    int currentRef = baseEntry.ref;

    for (auto* component : node->components) {
        if (!component) continue;

        if (component->kind == VarComponentNode::ARRAY_ACCESS) {
            // For each index in this array access
            for (auto* idxExpr : component->indices) {
                if (!idxExpr) continue;

                if (currentType == TYPE_ARRAY && currentRef >= 0 &&
                    currentRef < (int)symbols->getAtabSize()) {
                    const AtabEntry& a = symbols->getAtab(currentRef);

                    // Push index expression
                    generateExpression(idxExpr);

                    // Subtract lower bound: (index - low)
                    add(InstructionCode::LIT, 0, a.low, "array low " + std::to_string(a.low));
                    add(InstructionCode::OPR, 0, operationNumber(OperationCode::SUB), "idx - low");

                    // Multiply by element size: (index - low) * elsz
                    add(InstructionCode::LIT, 0, a.elsz, "elsz " + std::to_string(a.elsz));
                    add(InstructionCode::OPR, 0, operationNumber(OperationCode::MUL), "offset * elsz");

                    // Add to running address: base + offset
                    add(InstructionCode::OPR, 0, operationNumber(OperationCode::ADD), "addr + offset");

                    // Update current type/ref for next component
                    currentType = a.etyp;
                    currentRef = a.eref;
                }
            }
        } else if (component->kind == VarComponentNode::FIELD_ACCESS) {
            // Field access: compute field offset and add to address
            if (currentType == TYPE_RECORD && currentRef >= 0 &&
                currentRef < (int)symbols->getBtabSize()) {
                // Find the field entry in the record's block
                std::string targetName;
                for (char c : component->fieldName) targetName += (char)std::tolower((unsigned char)c);

                int cur = symbols->getBtab(currentRef).last;
                int fieldAdr = -1;
                int fieldType = TYPE_VOID;
                int fieldRef = -1;

                while (cur > 0 && cur < (int)symbols->getTabSize()) {
                    std::string curName;
                    for (char c : symbols->getTab(cur).id) curName += (char)std::tolower((unsigned char)c);
                    if (curName == targetName) {
                        fieldAdr = symbols->getTab(cur).adr;
                        fieldType = symbols->getTab(cur).type;
                        fieldRef = symbols->getTab(cur).ref;
                        break;
                    }
                    cur = symbols->getTab(cur).link;
                }

                if (fieldAdr >= 0) {
                    add(InstructionCode::LIT, 0, fieldAdr, "field " + component->fieldName + " offset");
                    add(InstructionCode::OPR, 0, operationNumber(OperationCode::ADD), "addr + field");
                    currentType = fieldType;
                    currentRef = fieldRef;
                }
            }
        }
    }

    // Also handle the legacy isArrayAccess + index pattern (no components)
    if (node->isArrayAccess && node->index) {
        if (currentType == TYPE_ARRAY && currentRef >= 0 &&
            currentRef < (int)symbols->getAtabSize()) {
            const AtabEntry& a = symbols->getAtab(currentRef);
            generateExpression(node->index);
            add(InstructionCode::LIT, 0, a.low, "array low");
            add(InstructionCode::OPR, 0, operationNumber(OperationCode::SUB), "idx - low");
            add(InstructionCode::LIT, 0, a.elsz, "elsz");
            add(InstructionCode::OPR, 0, operationNumber(OperationCode::MUL), "offset * elsz");
            add(InstructionCode::OPR, 0, operationNumber(OperationCode::ADD), "addr + offset");
        }
    }

    // Handle legacy field access
    if (!node->fieldName.empty()) {
        if (currentType == TYPE_RECORD && currentRef >= 0 &&
            currentRef < (int)symbols->getBtabSize()) {
            int cur = symbols->getBtab(currentRef).last;
            int fieldAdr = -1;
            while (cur > 0 && cur < (int)symbols->getTabSize()) {
                if (symbols->getTab(cur).id == node->fieldName) {
                    fieldAdr = symbols->getTab(cur).adr;
                    break;
                }
                cur = symbols->getTab(cur).link;
            }
            if (fieldAdr >= 0) {
                add(InstructionCode::LIT, 0, fieldAdr, "field offset");
                add(InstructionCode::OPR, 0, operationNumber(OperationCode::ADD), "addr + field");
            }
        }
    }
}

void CodeGenerator::generateBinaryOperation(BinOpNode* node) {
    if (!node) return;

    if (node->left) generateExpression(node->left);
    if (node->right) generateExpression(node->right);

    if (node->op == "and") {
        add(InstructionCode::OPR, 0, operationNumber(OperationCode::MUL), "and");
    } else if (node->op == "or") {
        add(InstructionCode::OPR, 0, operationNumber(OperationCode::ADD), "or");
        add(InstructionCode::LIT, 0, 0, "");
        add(InstructionCode::OPR, 0, operationNumber(OperationCode::GTR), "or > 0");
    } else {
        add(InstructionCode::OPR, 0, operationNumber(operationForBinaryOperator(node->op)), "op " + node->op);
    }
}

void CodeGenerator::generateUnaryOperation(UnaryOpNode* node) {
    if (!node || !node->operand) return;

    generateExpression(node->operand);

    if (node->op == "-") {
        add(InstructionCode::OPR, 0, operationNumber(OperationCode::NEG), "neg");
    } else if (node->op == "not") {
        add(InstructionCode::LIT, 0, 0, "");
        add(InstructionCode::OPR, 0, operationNumber(OperationCode::EQL), "not");
    }
}
