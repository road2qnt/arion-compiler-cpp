#include "semanticAnalyzer.hpp"

// === Print untuk Semantic Analysis ===

static void printLocation(std::ostream& os, const ASTNode* node) {
    if (node && node->line > 0) {
        os << " line=" << node->line;
        if (node->column > 0) os << ":" << node->column;
    }
}

static void printTypeShape(std::ostream& os, const TypeNode* node) {
    if (!node) return;
    if (auto* simple = dynamic_cast<const SimpleTypeNode*>(node)) {
        os << " <type simple=" << simple->name << ">";
    } else if (auto* range = dynamic_cast<const RangeTypeNode*>(node)) {
        os << " <type range>";
        if (auto* lo = dynamic_cast<NumberNode*>(range->low)) os << " low=" << lo->value;
        if (auto* hi = dynamic_cast<NumberNode*>(range->high)) os << " high=" << hi->value;
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
        os << ">";
    } else if (auto* rec = dynamic_cast<const RecordTypeNode*>(node)) {
        os << " <type record fields=" << rec->fields.size();
        if (rec->btabIndex >= 0) os << " btab=" << rec->btabIndex;
        os << ">";
    }
}

void SemanticAnalyzer::printDecoratedAST(std::ostream& os, ASTNode* node, int depth) const {
    if (!node) return;

    std::string indent(depth * 2, ' ');

    if (auto* prog = dynamic_cast<ProgramNode*>(node)) {
        os << indent << "Program: " << prog->name;
        if (prog->tabIndex >= 0) os << " [tab=" << prog->tabIndex << "]";
        printLocation(os, prog);
        os << "\n";
        if (prog->declarations) printDecoratedAST(os, prog->declarations, depth + 1);
        if (prog->body) printDecoratedAST(os, prog->body, depth + 1);
    } else if (auto* decls = dynamic_cast<DeclarationsNode*>(node)) {
        os << indent << "Declarations:\n";
        for (auto* decl : decls->decls) {
            printDecoratedAST(os, decl, depth + 1);
        }
    } else if (auto* vd = dynamic_cast<VarDeclNode*>(node)) {
        os << indent << "VarDecl: " << vd->name;
        if (vd->tabIndex >= 0) os << " [tab=" << vd->tabIndex << "]";
        std::string typeStr;
        switch (vd->type) {
            case TYPE_INTEGER: typeStr = "integer"; break;
            case TYPE_REAL:    typeStr = "real"; break;
            case TYPE_BOOLEAN: typeStr = "boolean"; break;
            case TYPE_CHAR:    typeStr = "char"; break;
            case TYPE_STRING:  typeStr = "string"; break;
            case TYPE_ARRAY:   typeStr = "array"; break;
            case TYPE_RECORD:  typeStr = "record"; break;
            case TYPE_ENUM:    typeStr = "enum"; break;
            case TYPE_SUBRANGE:typeStr = "subrange"; break;
            case TYPE_VOID:    typeStr = "void"; break;
            default:           typeStr = std::to_string(vd->type); break;
        }
        os << " type=" << typeStr;
        printLocation(os, vd);
        printTypeShape(os, vd->varType);
        os << "\n";
    } else if (auto* cd = dynamic_cast<ConstDeclNode*>(node)) {
        os << indent << "ConstDecl: " << cd->name;
        if (cd->tabIndex >= 0) os << " [tab=" << cd->tabIndex << "]";
        std::string typeStr;
        switch (cd->type) {
            case TYPE_INTEGER: typeStr = "integer"; break;
            case TYPE_REAL:    typeStr = "real"; break;
            case TYPE_BOOLEAN: typeStr = "boolean"; break;
            case TYPE_CHAR:    typeStr = "char"; break;
            case TYPE_STRING:  typeStr = "string"; break;
            default:           typeStr = std::to_string(cd->type); break;
        }
        os << " type=" << typeStr;
        printLocation(os, cd);
        if (cd->value) {
            if (auto* num = dynamic_cast<NumberNode*>(cd->value)) os << " value=" << num->value;
            else if (auto* re = dynamic_cast<RealNode*>(cd->value)) os << " value=" << re->value;
            else if (auto* ch = dynamic_cast<CharNode*>(cd->value)) os << " value='" << ch->value << "'";
            else if (auto* st = dynamic_cast<StringNode*>(cd->value)) os << " value=\"" << st->value << "\"";
            else if (auto* b = dynamic_cast<BoolNode*>(cd->value)) os << " value=" << (b->value ? "true" : "false");
            else if (auto* u = dynamic_cast<UnaryOpNode*>(cd->value)) os << " value=<unary " << u->op << ">";
            else if (auto* vn = dynamic_cast<VarNode*>(cd->value)) os << " value=" << vn->name;
        }
        os << "\n";
    } else if (auto* td = dynamic_cast<TypeDeclNode*>(node)) {
        os << indent << "TypeDecl: " << td->name;
        if (td->tabIndex >= 0) os << " [tab=" << td->tabIndex << "]";
        std::string typeStr;
        switch (td->type) {
            case TYPE_INTEGER: typeStr = "integer"; break;
            case TYPE_REAL:    typeStr = "real"; break;
            case TYPE_BOOLEAN: typeStr = "boolean"; break;
            case TYPE_CHAR:    typeStr = "char"; break;
            case TYPE_STRING:  typeStr = "string"; break;
            case TYPE_ARRAY:   typeStr = "array"; break;
            case TYPE_RECORD:  typeStr = "record"; break;
            case TYPE_ENUM:    typeStr = "enum"; break;
            case TYPE_SUBRANGE:typeStr = "subrange"; break;
            default:           typeStr = std::to_string(td->type); break;
        }
        os << " type=" << typeStr;
        printLocation(os, td);
        printTypeShape(os, td->typeDef);
        os << "\n";
    } else if (auto* sd = dynamic_cast<SubprogramDeclNode*>(node)) {
        os << indent << (sd->isFunction ? "Function: " : "Procedure: ") << sd->name;
        if (sd->tabIndex >= 0) os << " [tab=" << sd->tabIndex << "]";
        os << " lev=" << sd->lev;
        printLocation(os, sd);
        if (sd->isFunction) {
            std::string rt;
            int t = sd->returnType ? sd->returnType->type : sd->type;
            switch (t) {
                case TYPE_INTEGER: rt = "integer"; break;
                case TYPE_REAL:    rt = "real"; break;
                case TYPE_BOOLEAN: rt = "boolean"; break;
                case TYPE_CHAR:    rt = "char"; break;
                case TYPE_STRING:  rt = "string"; break;
                default:           rt = std::to_string(t); break;
            }
            os << " returns=" << rt;
        }
        os << "\n";
        if (!sd->params.empty()) {
            os << indent << "  Params:\n";
            for (auto* p : sd->params) printDecoratedAST(os, p, depth + 2);
        }
        if (sd->localDecls) {
            os << indent << "  Locals:\n";
            for (auto* d : sd->localDecls->decls) printDecoratedAST(os, d, depth + 2);
        }
        if (sd->body) printDecoratedAST(os, sd->body, depth + 1);
    } else if (auto* assign = dynamic_cast<AssignNode*>(node)) {
        os << indent << "Assign:\n";
        if (assign->target) printDecoratedAST(os, assign->target, depth + 1);
        if (assign->value) printDecoratedAST(os, assign->value, depth + 1);
    } else if (auto* binOp = dynamic_cast<BinOpNode*>(node)) {
        os << indent << "BinOp: " << binOp->op;
        std::string typeStr;
        switch (binOp->type) {
            case TYPE_INTEGER: typeStr = "integer"; break;
            case TYPE_REAL:    typeStr = "real"; break;
            case TYPE_BOOLEAN: typeStr = "boolean"; break;
            default:           typeStr = std::to_string(binOp->type); break;
        }
        os << " type=" << typeStr << "\n";
        if (binOp->left) printDecoratedAST(os, binOp->left, depth + 1);
        if (binOp->right) printDecoratedAST(os, binOp->right, depth + 1);
    } else if (auto* unary = dynamic_cast<UnaryOpNode*>(node)) {
        os << indent << "UnaryOp: " << unary->op;
        printLocation(os, unary);
        os << "\n";
        if (unary->operand) printDecoratedAST(os, unary->operand, depth + 1);
    } else if (auto* num = dynamic_cast<NumberNode*>(node)) {
        os << indent << "Number: " << num->value << "\n";
    } else if (auto* real = dynamic_cast<RealNode*>(node)) {
        os << indent << "Real: " << real->value << "\n";
    } else if (auto* ch = dynamic_cast<CharNode*>(node)) {
        os << indent << "Char: '" << ch->value << "'\n";
    } else if (auto* str = dynamic_cast<StringNode*>(node)) {
        os << indent << "String: \"" << str->value << "\"\n";
    } else if (auto* b = dynamic_cast<BoolNode*>(node)) {
        os << indent << "Bool: " << (b->value ? "true" : "false") << "\n";
    } else if (auto* var = dynamic_cast<VarNode*>(node)) {
        os << indent << "Var: " << var->name;
        for (auto* component : var->components) {
            if (!component) continue;
            if (component->kind == VarComponentNode::ARRAY_ACCESS) {
                os << "[";
                for (size_t i = 0; i < component->indices.size(); i++) {
                    if (i) os << ",";
                    if (auto* n = dynamic_cast<NumberNode*>(component->indices[i])) os << n->value;
                    else if (auto* ch = dynamic_cast<CharNode*>(component->indices[i])) os << "'" << ch->value << "'";
                    else if (auto* v = dynamic_cast<VarNode*>(component->indices[i])) os << v->name;
                    else os << "?";
                }
                os << "]";
            } else if (component->kind == VarComponentNode::FIELD_ACCESS) {
                os << "." << component->fieldName;
            }
        }
        if (var->tabIndex >= 0) os << " [tab=" << var->tabIndex << "]";
        std::string typeStr;
        switch (var->type) {
            case TYPE_INTEGER: typeStr = "integer"; break;
            case TYPE_REAL:    typeStr = "real"; break;
            case TYPE_BOOLEAN: typeStr = "boolean"; break;
            case TYPE_CHAR:    typeStr = "char"; break;
            case TYPE_STRING:  typeStr = "string"; break;
            case TYPE_VOID:    typeStr = "void"; break;
            case TYPE_ERROR:   typeStr = "error"; break;
            default:           typeStr = std::to_string(var->type); break;
        }
        os << " type=" << typeStr << " lev=" << var->lev;
        os << " lval=" << (var->isLValue ? "true" : "false");
        printLocation(os, var);
        os << "\n";
    } else if (auto* empty = dynamic_cast<EmptyStmtNode*>(node)) {
        os << indent << "EmptyStmt";
        printLocation(os, empty);
        os << "\n";
    } else if (auto* call = dynamic_cast<ProcCallNode*>(node)) {
        os << indent << "Call: " << call->name;
        os << " [tab=" << call->tabIndex << "]\n";
        for (auto* arg : call->args) {
            printDecoratedAST(os, arg, depth + 1);
        }
    } else if (auto* block = dynamic_cast<BlockNode*>(node)) {
        os << indent << "Block: [" << block->statements.size() << " statements]\n";
        for (auto* stmt : block->statements) {
            printDecoratedAST(os, stmt, depth + 1);
        }
    } else if (auto* ifNode = dynamic_cast<IfNode*>(node)) {
        os << indent << "If:\n";
        if (ifNode->condition) {
            os << indent << "  Condition:\n";
            printDecoratedAST(os, ifNode->condition, depth + 2);
        }
        if (ifNode->thenBranch) {
            os << indent << "  Then:\n";
            printDecoratedAST(os, ifNode->thenBranch, depth + 2);
        }
        if (ifNode->elseBranch) {
            os << indent << "  Else:\n";
            printDecoratedAST(os, ifNode->elseBranch, depth + 2);
        }
    } else if (auto* whileNode = dynamic_cast<WhileNode*>(node)) {
        os << indent << "While:\n";
        if (whileNode->condition) {
            os << indent << "  Condition:\n";
            printDecoratedAST(os, whileNode->condition, depth + 2);
        }
        if (whileNode->body) {
            os << indent << "  Body:\n";
            printDecoratedAST(os, whileNode->body, depth + 2);
        }
    } else if (auto* repeatNode = dynamic_cast<RepeatNode*>(node)) {
        os << indent << "Repeat:\n";
        if (repeatNode->body) {
            os << indent << "  Body:\n";
            printDecoratedAST(os, repeatNode->body, depth + 2);
        }
        if (repeatNode->condition) {
            os << indent << "  Until:\n";
            printDecoratedAST(os, repeatNode->condition, depth + 2);
        }
    } else if (auto* forNode = dynamic_cast<ForNode*>(node)) {
        os << indent << "For: " << forNode->varName
           << (forNode->isDownto ? " downto" : " to") << "\n";
        if (forNode->start) printDecoratedAST(os, forNode->start, depth + 1);
        if (forNode->end) printDecoratedAST(os, forNode->end, depth + 1);
        if (forNode->body) printDecoratedAST(os, forNode->body, depth + 1);
    } else if (auto* caseNode = dynamic_cast<CaseNode*>(node)) {
        os << indent << "Case:\n";
        if (caseNode->expression) {
            os << indent << "  Expression:\n";
            printDecoratedAST(os, caseNode->expression, depth + 2);
        }
        for (size_t i = 0; i < caseNode->branches.size(); i++) {
            os << indent << "  Branch " << i << ":\n";
            if (!caseNode->branches[i]->constants.empty()) {
                os << indent << "    Labels:\n";
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
    printDecoratedAST(os, astRoot);

    symTab.printSymbolTable(os);

    if (typeChecker.hasErrors()) {
        os << "\n========== Semantic Errors ==========\n";
        typeChecker.printErrors(os);
    }
}
