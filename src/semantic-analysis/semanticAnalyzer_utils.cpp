#include "semanticAnalyzer.hpp"

// === Print untuk Semantic Analysis ===

static std::string typeToString(int type) {
    switch (type) {
        case TYPE_VOID:     return "void";
        case TYPE_INTEGER:  return "integer";
        case TYPE_REAL:     return "real";
        case TYPE_BOOLEAN:  return "boolean";
        case TYPE_CHAR:     return "char";
        case TYPE_ARRAY:    return "array";
        case TYPE_RECORD:   return "record";
        case TYPE_STRING:   return "string";
        case TYPE_ENUM:     return "enum";
        case TYPE_SUBRANGE: return "subrange";
        case TYPE_ERROR:    return "error";
        default:            return "unknown(" + std::to_string(type) + ")";
    }
}

static void printLocation(std::ostream& os, const ASTNode* node) {
    if (node && node->line > 0) {
        os << " line=" << node->line;
        if (node->column > 0) os << ":" << node->column;
    }
}

static std::string treeIndent(int depth) {
    if (depth <= 0) return "";

    std::string indent;
    for (int i = 1; i < depth; i++) {
        indent += "│   ";
    }
    indent += "├── ";
    return indent;
}

static std::string treeGroupIndent(int depth) {
    std::string indent;
    for (int i = 0; i < depth; i++) {
        indent += "│   ";
    }
    return indent;
}

static std::string exprToString(const ASTNode* node) {
    if (!node) return "<null>";
    if (auto* num = dynamic_cast<const NumberNode*>(node)) return std::to_string(num->value);
    if (auto* real = dynamic_cast<const RealNode*>(node)) return std::to_string(real->value);
    if (auto* ch = dynamic_cast<const CharNode*>(node)) return "'" + std::string(1, ch->value) + "'";
    if (auto* str = dynamic_cast<const StringNode*>(node)) return "\"" + str->value + "\"";
    if (auto* b = dynamic_cast<const BoolNode*>(node)) return b->value ? "true" : "false";
    if (auto* var = dynamic_cast<const VarNode*>(node)) {
        std::string out = var->name;
        for (auto* component : var->components) {
            if (!component) continue;
            if (component->kind == VarComponentNode::ARRAY_ACCESS) {
                out += "[";
                for (size_t i = 0; i < component->indices.size(); i++) {
                    if (i) out += ",";
                    out += exprToString(component->indices[i]);
                }
                out += "]";
            } else if (component->kind == VarComponentNode::FIELD_ACCESS) {
                out += "." + component->fieldName;
            }
        }
        return out;
    }
    if (auto* call = dynamic_cast<const ProcCallNode*>(node)) {
        std::string out = call->name + "(";
        for (size_t i = 0; i < call->args.size(); i++) {
            if (i) out += ", ";
            out += exprToString(call->args[i]);
        }
        out += ")";
        return out;
    }
    if (auto* unary = dynamic_cast<const UnaryOpNode*>(node)) {
        return unary->op + exprToString(unary->operand);
    }
    if (auto* bin = dynamic_cast<const BinOpNode*>(node)) {
        return "(" + exprToString(bin->left) + " " + bin->op + " " +
               exprToString(bin->right) + ")";
    }
    return "<expr>";
}

static void printTypeShape(std::ostream& os, const TypeNode* node) {
    if (!node) return;
    if (auto* simple = dynamic_cast<const SimpleTypeNode*>(node)) {
        os << " <type simple=" << simple->name;
        if (simple->type != TYPE_VOID) os << " resolved=" << typeToString(simple->type);
        os << ">";
    } else if (auto* range = dynamic_cast<const RangeTypeNode*>(node)) {
        os << " <type range";
        if (range->low) os << " low=" << exprToString(range->low);
        if (range->high) os << " high=" << exprToString(range->high);
        if (range->ref >= 0) os << " atab=" << range->ref;
        os << ">";
    } else if (auto* en = dynamic_cast<const EnumeratedTypeNode*>(node)) {
        os << " <type enum=(";
        for (size_t i = 0; i < en->values.size(); i++) {
            if (i) os << ",";
            os << en->values[i];
        }
        os << ")>";
    } else if (auto* arr = dynamic_cast<const ArrayTypeNode*>(node)) {
        os << " <type array";
        if (arr->atabIndex >= 0) os << " atab=" << arr->atabIndex;
        if (arr->size > 0) os << " size=" << arr->size;
        os << ">";
    } else if (auto* rec = dynamic_cast<const RecordTypeNode*>(node)) {
        os << " <type record fields=" << rec->fields.size();
        if (rec->btabIndex >= 0) os << " btab=" << rec->btabIndex;
        if (rec->size > 0) os << " size=" << rec->size;
        os << ">";
    }
}

void SemanticAnalyzer::printDecoratedAST(std::ostream& os, ASTNode* node, int depth) const {
    if (!node) return;

    std::string indent = treeIndent(depth);
    std::string groupIndent = treeGroupIndent(depth);

    if (auto* prog = dynamic_cast<ProgramNode*>(node)) {
        os << indent << "Program: " << prog->name;
        if (prog->tabIndex >= 0) os << " [tab=" << prog->tabIndex << "]";
        os << " type=" << typeToString(prog->type) << " lev=" << prog->lev;
        printLocation(os, prog);
        os << "\n";
        if (prog->declarations) printDecoratedAST(os, prog->declarations, depth + 1);
        if (prog->body) printDecoratedAST(os, prog->body, depth + 1);
    } else if (auto* decls = dynamic_cast<DeclarationsNode*>(node)) {
        os << indent << "Declarations: [" << decls->decls.size() << " declarations]";
        printLocation(os, decls);
        os << "\n";
        for (auto* decl : decls->decls) {
            printDecoratedAST(os, decl, depth + 1);
        }
    } else if (auto* vd = dynamic_cast<VarDeclNode*>(node)) {
        os << indent << "VarDecl: " << vd->name;
        if (vd->tabIndex >= 0) os << " [tab=" << vd->tabIndex << "]";
        os << " type=" << typeToString(vd->type) << " lev=" << vd->lev;
        printLocation(os, vd);
        printTypeShape(os, vd->varType);
        os << "\n";
    } else if (auto* cd = dynamic_cast<ConstDeclNode*>(node)) {
        os << indent << "ConstDecl: " << cd->name;
        if (cd->tabIndex >= 0) os << " [tab=" << cd->tabIndex << "]";
        os << " type=" << typeToString(cd->type) << " lev=" << cd->lev;
        printLocation(os, cd);
        if (cd->value) {
            os << " init=" << exprToString(cd->value);
        }
        if (cd->tabIndex >= 0 && cd->tabIndex < symTab.getTabSize()) {
            const TabEntry& e = symTab.getTab(cd->tabIndex);
            if (e.type == TYPE_STRING) os << " storedLength=" << e.adr;
            else if (e.type == TYPE_CHAR) os << " storedValue='" << (char)e.adr << "'";
            else if (e.type == TYPE_BOOLEAN) os << " storedValue=" << (e.adr ? "true" : "false");
            else os << " storedValue=" << e.adr;
        }
        os << "\n";
    } else if (auto* td = dynamic_cast<TypeDeclNode*>(node)) {
        os << indent << "TypeDecl: " << td->name;
        if (td->tabIndex >= 0) os << " [tab=" << td->tabIndex << "]";
        os << " type=" << typeToString(td->type) << " lev=" << td->lev;
        printLocation(os, td);
        printTypeShape(os, td->typeDef);
        os << "\n";
    } else if (auto* sd = dynamic_cast<SubprogramDeclNode*>(node)) {
        os << indent << (sd->isFunction ? "Function: " : "Procedure: ") << sd->name;
        if (sd->tabIndex >= 0) os << " [tab=" << sd->tabIndex << "]";
        os << " type=" << typeToString(sd->type) << " lev=" << sd->lev;
        printLocation(os, sd);
        if (sd->isFunction) {
            int t = sd->returnType ? sd->returnType->type : sd->type;
            os << " returns=" << typeToString(t);
            printTypeShape(os, sd->returnType);
        }
        os << "\n";
        if (!sd->params.empty()) {
            os << groupIndent << "├── Params:\n";
            for (auto* p : sd->params) printDecoratedAST(os, p, depth + 2);
        }
        if (sd->localDecls) {
            os << groupIndent << "├── Locals:\n";
            for (auto* d : sd->localDecls->decls) printDecoratedAST(os, d, depth + 2);
        }
        if (sd->body) printDecoratedAST(os, sd->body, depth + 1);
    } else if (auto* assign = dynamic_cast<AssignNode*>(node)) {
        os << indent << "Assign: type=" << typeToString(assign->type);
        printLocation(os, assign);
        os << "\n";
        if (assign->target) printDecoratedAST(os, assign->target, depth + 1);
        if (assign->value) printDecoratedAST(os, assign->value, depth + 1);
    } else if (auto* binOp = dynamic_cast<BinOpNode*>(node)) {
        os << indent << "BinOp: " << binOp->op;
        os << " type=" << typeToString(binOp->type);
        printLocation(os, binOp);
        os << "\n";
        if (binOp->left) printDecoratedAST(os, binOp->left, depth + 1);
        if (binOp->right) printDecoratedAST(os, binOp->right, depth + 1);
    } else if (auto* unary = dynamic_cast<UnaryOpNode*>(node)) {
        os << indent << "UnaryOp: " << unary->op;
        os << " type=" << typeToString(unary->type);
        printLocation(os, unary);
        os << "\n";
        if (unary->operand) printDecoratedAST(os, unary->operand, depth + 1);
    } else if (auto* num = dynamic_cast<NumberNode*>(node)) {
        os << indent << "Number: " << num->value << " type=" << typeToString(num->type);
        printLocation(os, num);
        os << "\n";
    } else if (auto* real = dynamic_cast<RealNode*>(node)) {
        os << indent << "Real: " << real->value << " type=" << typeToString(real->type);
        printLocation(os, real);
        os << "\n";
    } else if (auto* ch = dynamic_cast<CharNode*>(node)) {
        os << indent << "Char: '" << ch->value << "' type=" << typeToString(ch->type);
        printLocation(os, ch);
        os << "\n";
    } else if (auto* str = dynamic_cast<StringNode*>(node)) {
        os << indent << "String: \"" << str->value << "\" type=" << typeToString(str->type)
           << " length=" << str->value.size();
        printLocation(os, str);
        os << "\n";
    } else if (auto* b = dynamic_cast<BoolNode*>(node)) {
        os << indent << "Bool: " << (b->value ? "true" : "false")
           << " type=" << typeToString(b->type);
        printLocation(os, b);
        os << "\n";
    } else if (auto* var = dynamic_cast<VarNode*>(node)) {
        os << indent << "Var: " << exprToString(var);
        if (var->tabIndex >= 0) os << " [tab=" << var->tabIndex << "]";
        os << " type=" << typeToString(var->type) << " lev=" << var->lev;
        os << " lval=" << (var->isLValue ? "true" : "false");
        printLocation(os, var);
        os << "\n";
    } else if (auto* empty = dynamic_cast<EmptyStmtNode*>(node)) {
        os << indent << "EmptyStmt";
        printLocation(os, empty);
        os << "\n";
    } else if (auto* call = dynamic_cast<ProcCallNode*>(node)) {
        os << indent << "Call: " << call->name;
        if (call->tabIndex >= 0) os << " [tab=" << call->tabIndex << "]";
        os << " type=" << typeToString(call->type) << " lev=" << call->lev
           << " args=" << call->args.size();
        printLocation(os, call);
        os << "\n";
        for (auto* arg : call->args) {
            printDecoratedAST(os, arg, depth + 1);
        }
    } else if (auto* block = dynamic_cast<BlockNode*>(node)) {
        os << indent << "Block: [" << block->statements.size() << " statements]";
        printLocation(os, block);
        os << "\n";
        for (auto* stmt : block->statements) {
            printDecoratedAST(os, stmt, depth + 1);
        }
    } else if (auto* ifNode = dynamic_cast<IfNode*>(node)) {
        os << indent << "If:";
        printLocation(os, ifNode);
        os << "\n";
        if (ifNode->condition) {
            os << groupIndent << "├── Condition:\n";
            printDecoratedAST(os, ifNode->condition, depth + 2);
        }
        if (ifNode->thenBranch) {
            os << groupIndent << "├── Then:\n";
            printDecoratedAST(os, ifNode->thenBranch, depth + 2);
        }
        if (ifNode->elseBranch) {
            os << groupIndent << "├── Else:\n";
            printDecoratedAST(os, ifNode->elseBranch, depth + 2);
        }
    } else if (auto* whileNode = dynamic_cast<WhileNode*>(node)) {
        os << indent << "While:";
        printLocation(os, whileNode);
        os << "\n";
        if (whileNode->condition) {
            os << groupIndent << "├── Condition:\n";
            printDecoratedAST(os, whileNode->condition, depth + 2);
        }
        if (whileNode->body) {
            os << groupIndent << "├── Body:\n";
            printDecoratedAST(os, whileNode->body, depth + 2);
        }
    } else if (auto* repeatNode = dynamic_cast<RepeatNode*>(node)) {
        os << indent << "Repeat:";
        printLocation(os, repeatNode);
        os << "\n";
        if (repeatNode->body) {
            os << groupIndent << "├── Body:\n";
            printDecoratedAST(os, repeatNode->body, depth + 2);
        }
        if (repeatNode->condition) {
            os << groupIndent << "├── Until:\n";
            printDecoratedAST(os, repeatNode->condition, depth + 2);
        }
    } else if (auto* forNode = dynamic_cast<ForNode*>(node)) {
        os << indent << "For: " << forNode->varName
           << (forNode->isDownto ? " downto" : " to");
        if (forNode->varTabIndex >= 0) os << " [varTab=" << forNode->varTabIndex << "]";
        os << " type=" << typeToString(forNode->type);
        printLocation(os, forNode);
        os << "\n";
        if (forNode->start) printDecoratedAST(os, forNode->start, depth + 1);
        if (forNode->end) printDecoratedAST(os, forNode->end, depth + 1);
        if (forNode->body) printDecoratedAST(os, forNode->body, depth + 1);
    } else if (auto* caseNode = dynamic_cast<CaseNode*>(node)) {
        os << indent << "Case:";
        printLocation(os, caseNode);
        os << "\n";
        if (caseNode->expression) {
            os << groupIndent << "├── Expression:\n";
            printDecoratedAST(os, caseNode->expression, depth + 2);
        }
        for (size_t i = 0; i < caseNode->branches.size(); i++) {
            os << groupIndent << "├── Branch " << i << ":\n";
            if (!caseNode->branches[i]->constants.empty()) {
                os << groupIndent << "│   ├── Labels:\n";
                for (auto* c : caseNode->branches[i]->constants) {
                    printDecoratedAST(os, c, depth + 3);
                }
            }
            if (caseNode->branches[i]->statement)
                printDecoratedAST(os, caseNode->branches[i]->statement, depth + 2);
        }
    }
}

void SemanticAnalyzer::printResults(std::ostream& os) const {
    os << "\n========== Decorated AST ==========\n";
    if (astRoot) {
        printDecoratedAST(os, astRoot);
    } else {
        os << "(empty AST: semantic analyzer did not produce an AST)\n";
    }

    symTab.printSymbolTable(os);

    if (typeChecker.hasErrors()) {
        os << "\n========== Semantic Errors ==========\n";
        typeChecker.printErrors(os);
    }
}
