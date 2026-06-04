#include "semanticAnalyzer.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <functional>

// === Fase 2: Dekorasi AST ===

static std::string toLowerCopy(const std::string& s) {
    std::string r;
    r.reserve(s.size());
    for (char c : s) r += (char)std::tolower((unsigned char)c);
    return r;
}

static bool needsTypeRef(int type) {
    return type == TYPE_ARRAY || type == TYPE_RECORD || type == TYPE_SUBRANGE;
}

static int valueTypeRef(const TabEntry& e) {
    if (e.obj == OBJ_FUNCTION && needsTypeRef(e.type)) return e.adr;
    return e.ref;
}

void SemanticAnalyzer::visit(ASTNode* node) {
    if (!node) return;

    if (auto* prog = dynamic_cast<ProgramNode*>(node)) {
        visitProgram(prog);
    } else if (auto* decls = dynamic_cast<DeclarationsNode*>(node)) {
        visitDeclarations(decls);
    } else if (auto* vd = dynamic_cast<VarDeclNode*>(node)) {
        visitVarDecl(vd);
    } else if (auto* cd = dynamic_cast<ConstDeclNode*>(node)) {
        visitConstDecl(cd);
    } else if (auto* td = dynamic_cast<TypeDeclNode*>(node)) {
        visitTypeDecl(td);
    } else if (auto* sd = dynamic_cast<SubprogramDeclNode*>(node)) {
        visitSubprogramDecl(sd);
    } else if (auto* assign = dynamic_cast<AssignNode*>(node)) {
        visitAssign(assign);
    } else if (auto* binOp = dynamic_cast<BinOpNode*>(node)) {
        visitBinOp(binOp);
    } else if (auto* unary = dynamic_cast<UnaryOpNode*>(node)) {
        visitUnaryOp(unary);
    } else if (auto* ifNode = dynamic_cast<IfNode*>(node)) {
        visitIf(ifNode);
    } else if (auto* whileNode = dynamic_cast<WhileNode*>(node)) {
        visitWhile(whileNode);
    } else if (auto* repeatNode = dynamic_cast<RepeatNode*>(node)) {
        visitRepeat(repeatNode);
    } else if (auto* forNode = dynamic_cast<ForNode*>(node)) {
        visitFor(forNode);
    } else if (auto* caseNode = dynamic_cast<CaseNode*>(node)) {
        visitCase(caseNode);
    } else if (auto* block = dynamic_cast<BlockNode*>(node)) {
        for (auto* stmt : block->statements) {
            visit(stmt);
        }
    } else if (dynamic_cast<EmptyStmtNode*>(node)) {
        node->type = TYPE_VOID;
    } else if (auto* call = dynamic_cast<ProcCallNode*>(node)) {
        visitProcCall(call);
    } else if (auto* var = dynamic_cast<VarNode*>(node)) {
        visitVar(var);
    }
}

void SemanticAnalyzer::visitProgram(ProgramNode* node) {
    int existing = symTab.lookup(node->name);
    if (existing == -1) {
        TabEntry entry(node->name, OBJ_PROGRAM, TYPE_VOID, 0, 0);
        entry.nrm = PARAM_VALUE;
        entry.ref = 0;
        entry.link = symTab.getBtab(0).last;
        int idx = symTab.addToTab(entry);
        symTab.getBtab(0).last = idx;
        node->tabIndex = idx;
        node->lev = 0;
    } else {
        node->tabIndex = existing;
    }

    // Visit Declaration and Body --- Scope (masuk ke program)
    symTab.pushScope();
    if (node->declarations) visit(node->declarations);

    int bodyBtab = symTab.getCurrentBtabIndex();
    if (node->tabIndex >= 0) {
        symTab.getTab(node->tabIndex).ref = bodyBtab;
    }

    if (node->body) visit(node->body);

    symTab.popScope();
}

void SemanticAnalyzer::visitDeclarations(DeclarationsNode* node) {
    for (auto* decl : node->decls) {
        visit(decl);
    }
}

void SemanticAnalyzer::visitVarDecl(VarDeclNode* node) {
    int existing = symTab.lookupInCurrentScope(node->name);
    if (existing != -1) {
        typeChecker.reportError("duplicate identifier: " + node->name, node->line);
        return;
    }

    ResolvedType rt;
    if (node->varType) {
        rt = resolveTypeNode(node->varType);
    } else {
        rt = ResolvedType(TYPE_INTEGER, 0, INT_SIZE);
    }

    int btabIdx = symTab.getCurrentBtabIndex();
    int adr = symTab.getBtab(btabIdx).psze + symTab.getBtab(btabIdx).vsze;
    int sz = (rt.size > 0) ? rt.size : 1;
    symTab.getBtab(btabIdx).vsze += sz;

    TabEntry entry(node->name, OBJ_VARIABLE, rt.typeCode, symTab.getCurrentLevel(), adr);
    entry.ref = rt.ref;
    entry.nrm = PARAM_VALUE;
    entry.link = symTab.getBtab(btabIdx).last;
    int idx = symTab.addToTab(entry);
    symTab.getBtab(btabIdx).last = idx;

    node->tabIndex = idx;
    node->type = rt.typeCode;
    node->lev = symTab.getCurrentLevel();
}

void SemanticAnalyzer::visitConstDecl(ConstDeclNode* node) {
    int existing = symTab.lookupInCurrentScope(node->name);
    if (existing != -1) {
        typeChecker.reportError("duplicate identifier: " + node->name, node->line);
        return;
    }

    int constType = TYPE_INTEGER;
    int constValue = 0;

    std::function<bool(ASTNode*, int&, int&)> evalConst = [&](ASTNode* valueNode,int& outType,  int& outValue) -> bool {
        if (!valueNode) return false;

        if (auto* num = dynamic_cast<NumberNode*>(valueNode)) {
            outType = TYPE_INTEGER;
            outValue = num->value;
            num->type = TYPE_INTEGER;
            return true;
        }
        if (auto* real = dynamic_cast<RealNode*>(valueNode)) {
            outType = TYPE_REAL;
            outValue = (int)real->value;
            real->type = TYPE_REAL;
            return true;
        }
        if (auto* ch = dynamic_cast<CharNode*>(valueNode)) {
            outType = TYPE_CHAR;
            outValue = (int)ch->value;
            ch->type = TYPE_CHAR;
            return true;
        }
        if (auto* str = dynamic_cast<StringNode*>(valueNode)) {
            outType = TYPE_STRING;
            outValue = (int)str->value.size();
            str->type = TYPE_STRING;
            return true;
        }
        if (auto* b = dynamic_cast<BoolNode*>(valueNode)) {
            outType = TYPE_BOOLEAN;
            outValue = b->value ? 1 : 0;
            b->type = TYPE_BOOLEAN;
            return true;
        }
        if (auto* var = dynamic_cast<VarNode*>(valueNode)) {
            int idx = symTab.lookup(var->name);
            if (idx == -1) {
                typeChecker.reportError("undeclared identifier: " + var->name, var->line);
                outType = TYPE_ERROR;
                outValue = 0;
                return false;
            }

            const TabEntry& e = symTab.getTab(idx);
            var->tabIndex = idx;
            var->type = e.type;
            var->lev = e.lev;

            if (e.obj != OBJ_CONSTANT) {
                typeChecker.reportError("constant initializer must be a constant: " + var->name, var->line);
                outType = TYPE_ERROR;
                outValue = 0;
                return false;
            }

            outType = e.type;
            outValue = e.adr;
            return true;
        }
        if (auto* unary = dynamic_cast<UnaryOpNode*>(valueNode)) {
            int innerType = TYPE_ERROR;
            int innerValue = 0;
            if (!evalConst(unary->operand, innerType, innerValue)) {
                unary->type = TYPE_ERROR;
                outType = TYPE_ERROR;
                outValue = 0;
                return false;
            }

            if (unary->op != "+" && unary->op != "-") {
                typeChecker.reportError("invalid unary operator in constant initializer: " + unary->op,
                                        unary->line);
                unary->type = TYPE_ERROR;
                outType = TYPE_ERROR;
                outValue = 0;
                return false;
            }
            if (innerType != TYPE_INTEGER && innerType != TYPE_REAL) {
                typeChecker.reportError("unary sign in constant initializer requires numeric constant",
                                        unary->line);
                unary->type = TYPE_ERROR;
                outType = TYPE_ERROR;
                outValue = 0;
                return false;
            }

            unary->type = innerType;
            outType = innerType;
            outValue = (unary->op == "-") ? -innerValue : innerValue;
            return true;
        }

        outType = TYPE_ERROR;
        outValue = 0;
        return false;
    };

    if (node->value) {
        if (!evalConst(node->value, constType, constValue)) {
            constType = TYPE_ERROR;
            constValue = 0;
        }
    }

    int btabIdx = symTab.getCurrentBtabIndex();
    TabEntry entry(node->name, OBJ_CONSTANT, constType, symTab.getCurrentLevel(), constValue);
    entry.nrm = PARAM_VALUE;
    entry.ref = 0;
    entry.link = symTab.getBtab(btabIdx).last;
    int idx = symTab.addToTab(entry);
    symTab.getBtab(btabIdx).last = idx;
    node->tabIndex = idx;
    node->type = constType;
    node->lev = symTab.getCurrentLevel();
}

void SemanticAnalyzer::visitTypeDecl(TypeDeclNode* node) {
    int existing = symTab.lookupInCurrentScope(node->name);
    if (existing != -1) {
        typeChecker.reportError("duplicate identifier: " + node->name, node->line);
        return;
    }

    ResolvedType rt;
    if (node->typeDef) {
        rt = resolveTypeNode(node->typeDef);
    }

    int btabIdx = symTab.getCurrentBtabIndex();
    TabEntry entry(node->name, OBJ_TYPE, rt.typeCode, symTab.getCurrentLevel(), rt.size);
    entry.ref = rt.ref;
    entry.nrm = PARAM_VALUE;
    entry.link = symTab.getBtab(btabIdx).last;
    int idx = symTab.addToTab(entry);
    symTab.getBtab(btabIdx).last = idx;
    node->tabIndex = idx;
    node->type = rt.typeCode;
    node->lev = symTab.getCurrentLevel();
}

void SemanticAnalyzer::visitSubprogramDecl(SubprogramDeclNode* node) {
    int existing = symTab.lookupInCurrentScope(node->name);
    if (existing != -1) {
        typeChecker.reportError("duplicate identifier: " + node->name, node->line);
        return;
    }

    int returnType = TYPE_VOID;
    int returnRef = 0;
    int returnSize = 0;
    if (node->isFunction && node->returnType) {
        ResolvedType rrt = resolveTypeNode(node->returnType);
        returnType = rrt.typeCode;
        returnRef = rrt.ref;
        returnSize = rrt.size;
    }

    int parentBtab = symTab.getCurrentBtabIndex();
    int parentLev = symTab.getCurrentLevel();

    TabEntry subEntry(node->name,
                      node->isFunction ? OBJ_FUNCTION : OBJ_PROCEDURE,
                      returnType, parentLev, returnRef);
    subEntry.nrm = PARAM_VALUE;
    subEntry.link = symTab.getBtab(parentBtab).last;
    int subIdx = symTab.addToTab(subEntry);
    symTab.getBtab(parentBtab).last = subIdx;
    node->tabIndex = subIdx;
    node->lev = parentLev;

    symTab.pushScope();
    int subBtab = symTab.getCurrentBtabIndex();
    symTab.getTab(subIdx).ref = subBtab;
    int subLev = symTab.getCurrentLevel();

    int paramAdr = 0;
    for (auto* param : node->params) {
        if (!param) continue;
        int existingParam = symTab.lookupInCurrentScope(param->name);
        if (existingParam != -1) {
            typeChecker.reportError("duplicate parameter: " + param->name, param->line);
            continue;
        }

        ResolvedType pr;
        if (param->varType) {
            pr = resolveTypeNode(param->varType);
        } else {
            pr = ResolvedType(TYPE_INTEGER, 0, INT_SIZE);
        }

        TabEntry pe(param->name, OBJ_VARIABLE, pr.typeCode, subLev, paramAdr);
        pe.ref = pr.ref;
        pe.nrm = param->nrm;
        pe.link = symTab.getBtab(subBtab).last;
        int pidx = symTab.addToTab(pe);
        symTab.getBtab(subBtab).last = pidx;
        param->tabIndex = pidx;
        param->type = pr.typeCode;
        param->lev = subLev;

        int sz = (pr.size > 0) ? pr.size : 1;
        paramAdr += sz;
    }
    symTab.getBtab(subBtab).lpar = symTab.getBtab(subBtab).last;
    symTab.getBtab(subBtab).psze = paramAdr;

    if (node->isFunction) {
        int rfAdr = symTab.getBtab(subBtab).psze + symTab.getBtab(subBtab).vsze;
        int existingReturnSlot = symTab.lookupInCurrentScope(node->name);
        if (existingReturnSlot != -1) {
            typeChecker.reportError("duplicate identifier: " + node->name, node->line);
        } else {
            TabEntry rfe(node->name, OBJ_VARIABLE, returnType, subLev, rfAdr);
            rfe.ref = returnRef;
            rfe.nrm = PARAM_VALUE;
            rfe.link = symTab.getBtab(subBtab).last;
            int rfIdx = symTab.addToTab(rfe);
            symTab.getBtab(subBtab).last = rfIdx;
            int rfSize = (returnSize > 0) ? returnSize : 1;
            symTab.getBtab(subBtab).vsze += rfSize;
        }
    }

    if (node->localDecls) visit(node->localDecls);

    if (node->body) visit(node->body);

    symTab.popScope();
}

void SemanticAnalyzer::visitAssign(AssignNode* node) {
    if (node->target) visit(node->target);
    if (node->value)  visit(node->value);

    auto knownStringLength = [&](ASTNode* n) -> int {
        if (!n) return -1;
        if (auto* sn = dynamic_cast<StringNode*>(n)) return (int)sn->value.size();
        if (n->tabIndex >= 0 && n->tabIndex < symTab.getTabSize()) {
            const TabEntry& e = symTab.getTab(n->tabIndex);
            if (e.type == TYPE_STRING && e.obj == OBJ_CONSTANT) return e.adr;
        }
        return -1;
    };

    auto targetSubrangeRef = [&]() -> int {
        if (!node->target || node->target->tabIndex < 0 ||
            node->target->tabIndex >= symTab.getTabSize()) {
            return -1;
        }

        const TabEntry& base = symTab.getTab(node->target->tabIndex);
        int currentType = base.type;
        int currentRef = valueTypeRef(base);

        for (auto* component : node->target->components) {
            if (!component) continue;

            if (component->kind == VarComponentNode::ARRAY_ACCESS) {
                for (size_t i = 0; i < component->indices.size(); i++) {
                    if (currentType != TYPE_ARRAY ||
                        currentRef < 0 || currentRef >= symTab.getAtabSize()) {
                        return -1;
                    }
                    const AtabEntry& ae = symTab.getAtab(currentRef);
                    currentType = ae.etyp;
                    currentRef = ae.eref;
                }
            } else if (component->kind == VarComponentNode::FIELD_ACCESS) {
                if (currentType != TYPE_RECORD ||
                    currentRef < 0 || currentRef >= symTab.getBtabSize()) {
                    return -1;
                }

                int cur = symTab.getBtab(currentRef).last;
                int found = -1;
                std::string target = toLowerCopy(component->fieldName);
                while (cur > 0 && cur < symTab.getTabSize()) {
                    if (toLowerCopy(symTab.getTab(cur).id) == target) {
                        found = cur;
                        break;
                    }
                    cur = symTab.getTab(cur).link;
                }
                if (found == -1) return -1;

                const TabEntry& field = symTab.getTab(found);
                currentType = field.type;
                currentRef = field.ref;
            }
        }

        return (currentType == TYPE_SUBRANGE) ? currentRef : -1;
    };

    if (node->target && !node->target->isLValue && node->target->type != TYPE_ERROR) {
        typeChecker.reportError("cannot assign to non-variable: " + node->target->name, node->line);
    }

    if (node->target && node->value) {
        if (node->target->type != TYPE_ERROR && node->value->type != TYPE_ERROR) {
            if (!typeChecker.isAssignmentCompatible(node->target->type, node->value->type)) {
                typeChecker.reportError("incompatible types", node->line);
            }

            if (node->target->type == TYPE_STRING && node->value->type == TYPE_STRING) {
                int targetLen = knownStringLength(node->target);
                int valueLen  = knownStringLength(node->value);
                if (targetLen >= 0 && valueLen >= 0 &&
                    !typeChecker.isStringLengthCompatible(targetLen, valueLen)) {
                    typeChecker.reportError("string length mismatch: cannot assign string of length " +
                                            std::to_string(valueLen) + " to string of length " +
                                            std::to_string(targetLen), node->line);
                }
            }

            if (node->target->type == TYPE_SUBRANGE) {
                int tRef = targetSubrangeRef();
                if (tRef >= 0 && tRef < symTab.getAtabSize()) {
                    const AtabEntry& ae = symTab.getAtab(tRef);
                    int valueType = TYPE_ERROR;
                    int value = constNodeOrdinalValue(node->value, valueType);
                    if (valueType != TYPE_ERROR && valueType != TYPE_REAL &&
                        (value < ae.low || value > ae.high)) {
                        typeChecker.reportError("value " + std::to_string(value) +
                                                " out of subrange [" + std::to_string(ae.low) +
                                                ".." + std::to_string(ae.high) + "]", node->line);
                    }
                }
            }
        }
        node->type = node->value->type;
    } else {
        node->type = TYPE_ERROR;
    }
}

void SemanticAnalyzer::visitBinOp(BinOpNode* node) {
    if (!node->left || !node->right) {
        node->type = TYPE_ERROR;
        return;
    }
    visit(node->left);
    visit(node->right);

    auto knownStringLength = [&](ASTNode* n) -> int {
        if (!n) return -1;
        if (auto* sn = dynamic_cast<StringNode*>(n)) return (int)sn->value.size();
        if (n->tabIndex >= 0 && n->tabIndex < symTab.getTabSize()) {
            const TabEntry& e = symTab.getTab(n->tabIndex);
            if (e.type == TYPE_STRING && e.obj == OBJ_CONSTANT) return e.adr;
        }
        return -1;
    };

    auto subrangeRefOf = [&](ASTNode* n) -> int {
        if (auto* call = dynamic_cast<ProcCallNode*>(n)) {
            if (call->tabIndex >= 0 && call->tabIndex < symTab.getTabSize()) {
                const TabEntry& e = symTab.getTab(call->tabIndex);
                if (e.obj == OBJ_FUNCTION && e.type == TYPE_SUBRANGE) {
                    return valueTypeRef(e);
                }
            }
            return -1;
        }

        auto* var = dynamic_cast<VarNode*>(n);
        if (!var || var->tabIndex < 0 || var->tabIndex >= symTab.getTabSize()) {
            return -1;
        }

        const TabEntry& base = symTab.getTab(var->tabIndex);
        int currentType = base.type;
        int currentRef = valueTypeRef(base);

        for (auto* component : var->components) {
            if (!component) continue;

            if (component->kind == VarComponentNode::ARRAY_ACCESS) {
                for (size_t i = 0; i < component->indices.size(); i++) {
                    if (currentType != TYPE_ARRAY ||
                        currentRef < 0 || currentRef >= symTab.getAtabSize()) {
                        return -1;
                    }
                    const AtabEntry& ae = symTab.getAtab(currentRef);
                    currentType = ae.etyp;
                    currentRef = ae.eref;
                }
            } else if (component->kind == VarComponentNode::FIELD_ACCESS) {
                if (currentType != TYPE_RECORD ||
                    currentRef < 0 || currentRef >= symTab.getBtabSize()) {
                    return -1;
                }

                int cur = symTab.getBtab(currentRef).last;
                int found = -1;
                std::string target = toLowerCopy(component->fieldName);
                while (cur > 0 && cur < symTab.getTabSize()) {
                    if (toLowerCopy(symTab.getTab(cur).id) == target) {
                        found = cur;
                        break;
                    }
                    cur = symTab.getTab(cur).link;
                }
                if (found == -1) return -1;

                const TabEntry& field = symTab.getTab(found);
                currentType = field.type;
                currentRef = field.ref;
            }
        }

        return (currentType == TYPE_SUBRANGE) ? currentRef : -1;
    };

    auto checkSubrangeLiteral = [&](ASTNode* subrangeSide, ASTNode* valueSide) {
        int ref = subrangeRefOf(subrangeSide);
        if (ref < 0 || ref >= symTab.getAtabSize()) return;

        int valueType = TYPE_ERROR;
        int value = constNodeOrdinalValue(valueSide, valueType);
        if (valueType == TYPE_ERROR || valueType == TYPE_REAL) return;

        const AtabEntry& ae = symTab.getAtab(ref);
        if (value < ae.low || value > ae.high) {
            typeChecker.reportError("value " + std::to_string(value) +
                                    " out of subrange [" + std::to_string(ae.low) +
                                    ".." + std::to_string(ae.high) + "]", node->line);
        }
    };

    bool isRelational = (node->op == "=" || node->op == "<>" || node->op == "<" ||
                         node->op == "<=" || node->op == ">" || node->op == ">=");
    bool isArithmetic = (node->op == "+" || node->op == "-" || node->op == "*" ||
                         node->op == "/" || node->op == "div" || node->op == "mod");
    bool isLogical = (node->op == "and" || node->op == "or");

    if (isRelational) {
        if (!typeChecker.isValidRelationalOperator(node->op, node->left->type, node->right->type)) {
            typeChecker.reportError("operator " + node->op + " not defined for these types", node->line);
            node->type = TYPE_ERROR;
        } else {
            if (node->left->type == TYPE_STRING && node->right->type == TYPE_STRING) {
                int ll = knownStringLength(node->left);
                int rl = knownStringLength(node->right);
                if (ll >= 0 && rl >= 0 && !typeChecker.isStringLengthCompatible(ll, rl)) {
                    typeChecker.reportError("string comparison requires operands of equal length", node->line);
                }
            }
            if (node->left->type == TYPE_SUBRANGE) {
                checkSubrangeLiteral(node->left, node->right);
            }
            if (node->right->type == TYPE_SUBRANGE) {
                checkSubrangeLiteral(node->right, node->left);
            }
            node->type = typeChecker.getResultType(node->op, node->left->type, node->right->type);
        }
    } else if (isArithmetic) {
        if (!typeChecker.isValidArithmeticOperator(node->op, node->left->type, node->right->type)) {
            typeChecker.reportError("operator " + node->op + " not defined for these types", node->line);
            node->type = TYPE_ERROR;
        } else {
            node->type = typeChecker.getResultType(node->op, node->left->type, node->right->type);
        }
    } else if (isLogical) {
        if (!typeChecker.isValidLogicalOperator(node->op, node->left->type, node->right->type)) {
            typeChecker.reportError("operator " + node->op + " not defined for these types", node->line);
            node->type = TYPE_ERROR;
        } else {
            node->type = typeChecker.getResultType(node->op, node->left->type, node->right->type);
        }
    } else {
        typeChecker.reportError("unknown binary operator: " + node->op, node->line);
        node->type = TYPE_ERROR;
    }
}

void SemanticAnalyzer::visitUnaryOp(UnaryOpNode* node) {
    if (!node->operand) {
        node->type = TYPE_ERROR;
        return;
    }
    visit(node->operand);

    if (!typeChecker.isValidUnaryOperator(node->op, node->operand->type)) {
        typeChecker.reportError("operator " + node->op + " not defined for this type", node->line);
        node->type = TYPE_ERROR;
    } else {
        node->type = typeChecker.getUnaryResultType(node->op, node->operand->type);
    }
}

void SemanticAnalyzer::visitIf(IfNode* node) {
    if (node->condition) visit(node->condition);
    if (node->thenBranch) visit(node->thenBranch);
    if (node->elseBranch) visit(node->elseBranch);

    if (node->condition && !typeChecker.isBoolean(node->condition->type)) {
        typeChecker.reportError("if condition must be Boolean", node->condition->line);
    }
}

void SemanticAnalyzer::visitWhile(WhileNode* node) {
    if (node->condition) visit(node->condition);
    if (node->body) visit(node->body);

    if (node->condition && !typeChecker.isBoolean(node->condition->type)) {
        typeChecker.reportError("while condition must be Boolean", node->condition->line);
    }
}

void SemanticAnalyzer::visitRepeat(RepeatNode* node) {
    if (node->body) visit(node->body);
    if (node->condition) visit(node->condition);

    if (node->condition && !typeChecker.isBoolean(node->condition->type)) {
        typeChecker.reportError("repeat condition must be Boolean", node->condition->line);
    }
}

void SemanticAnalyzer::visitFor(ForNode* node) {
    if (node->start) visit(node->start);
    if (node->end)   visit(node->end);

    int loopVarIdx = symTab.lookup(node->varName);
    int loopVarType = TYPE_ERROR;
    int loopVarRef = -1;
    if (loopVarIdx == -1) {
        typeChecker.reportError("undeclared loop variable: " + node->varName, node->line);
        node->type = TYPE_ERROR;
    } else {
        const TabEntry& e = symTab.getTab(loopVarIdx);
        loopVarType = e.type;
        loopVarRef = e.ref;
        if (e.obj != OBJ_VARIABLE) {
            typeChecker.reportError("for loop variable must be a variable: " + node->varName, node->line);
            node->type = TYPE_ERROR;
        } else if (!typeChecker.isOrdinal(e.type)) {
            typeChecker.reportError("for loop variable must be ordinal: " + node->varName, node->line);
            node->type = TYPE_ERROR;
        } else {
            node->type = TYPE_VOID;
        }
        node->varTabIndex = loopVarIdx;
    }

    if (node->body) visit(node->body);

    auto checkLoopBound = [&](ASTNode* bound, const std::string& label) {
        if (!bound || loopVarType != TYPE_SUBRANGE ||
            loopVarRef < 0 || loopVarRef >= symTab.getAtabSize()) {
            return;
        }

        int valueType = TYPE_ERROR;
        int value = constNodeOrdinalValue(bound, valueType);
        if (valueType == TYPE_ERROR || valueType == TYPE_REAL) return;

        const AtabEntry& ae = symTab.getAtab(loopVarRef);
        if (value < ae.low || value > ae.high) {
            typeChecker.reportError("for loop " + label + " value " +
                                    std::to_string(value) + " out of subrange [" +
                                    std::to_string(ae.low) + ".." +
                                    std::to_string(ae.high) + "]", node->line);
        }
    };

    if (node->start && !typeChecker.isOrdinal(node->start->type)) {
        typeChecker.reportError("for loop starting value must be an ordinal type", node->start->line);
    }
    if (node->end && !typeChecker.isOrdinal(node->end->type)) {
        typeChecker.reportError("for loop ending value must be an ordinal type", node->end->line);
    }
    if (loopVarIdx != -1 && node->start && node->start->type != TYPE_ERROR) {
        if (!typeChecker.isAssignmentCompatible(loopVarType, node->start->type)) {
            typeChecker.reportError("for loop start value incompatible with loop variable", node->start->line);
        } else {
            checkLoopBound(node->start, "start");
        }
    }
    if (loopVarIdx != -1 && node->end && node->end->type != TYPE_ERROR) {
        if (!typeChecker.isAssignmentCompatible(loopVarType, node->end->type)) {
            typeChecker.reportError("for loop end value incompatible with loop variable", node->end->line);
        } else {
            checkLoopBound(node->end, "end");
        }
    }
}

void SemanticAnalyzer::visitCase(CaseNode* node) {
    if (node->expression) visit(node->expression);

    int exprType = TYPE_ERROR;
    if (node->expression) {
        exprType = node->expression->type;
        if (exprType != TYPE_ERROR &&
            !typeChecker.isOrdinal(exprType) && exprType != TYPE_SUBRANGE) {
            typeChecker.reportError("case expression must be of ordinal type",node->expression->line);
        }
    }

    auto expressionSubrangeRef = [&]() -> int {
        if (auto* call = dynamic_cast<ProcCallNode*>(node->expression)) {
            if (call->tabIndex >= 0 && call->tabIndex < symTab.getTabSize()) {
                const TabEntry& e = symTab.getTab(call->tabIndex);
                if (e.obj == OBJ_FUNCTION && e.type == TYPE_SUBRANGE) {
                    return valueTypeRef(e);
                }
            }
            return -1;
        }

        auto* var = dynamic_cast<VarNode*>(node->expression);
        if (!var || var->tabIndex < 0 || var->tabIndex >= symTab.getTabSize()) {
            return -1;
        }

        const TabEntry& base = symTab.getTab(var->tabIndex);
        int currentType = base.type;
        int currentRef = valueTypeRef(base);

        for (auto* component : var->components) {
            if (!component) continue;

            if (component->kind == VarComponentNode::ARRAY_ACCESS) {
                for (size_t i = 0; i < component->indices.size(); i++) {
                    if (currentType != TYPE_ARRAY ||
                        currentRef < 0 || currentRef >= symTab.getAtabSize()) {
                        return -1;
                    }
                    const AtabEntry& ae = symTab.getAtab(currentRef);
                    currentType = ae.etyp;
                    currentRef = ae.eref;
                }
            } else if (component->kind == VarComponentNode::FIELD_ACCESS) {
                if (currentType != TYPE_RECORD ||
                    currentRef < 0 || currentRef >= symTab.getBtabSize()) {
                    return -1;
                }

                int cur = symTab.getBtab(currentRef).last;
                int found = -1;
                std::string target = toLowerCopy(component->fieldName);
                while (cur > 0 && cur < symTab.getTabSize()) {
                    if (toLowerCopy(symTab.getTab(cur).id) == target) {
                        found = cur;
                        break;
                    }
                    cur = symTab.getTab(cur).link;
                }
                if (found == -1) return -1;

                const TabEntry& field = symTab.getTab(found);
                currentType = field.type;
                currentRef = field.ref;
            }
        }

        return (currentType == TYPE_SUBRANGE) ? currentRef : -1;
    };

    std::vector<int> seenLabels;

    for (auto* branch : node->branches) {
        for (auto* constant : branch->constants) {
            if (!constant) continue;
            visit(constant);

            int labelType = TYPE_ERROR;
            int labelVal = constNodeOrdinalValue(constant, labelType);
            bool labelIsOrdinal = labelType != TYPE_ERROR && typeChecker.isOrdinal(labelType);

            if (constant->type != TYPE_ERROR && !labelIsOrdinal) {
                typeChecker.reportError("case label must be of ordinal type", constant->line);
            }

            if (exprType != TYPE_ERROR && constant->type != TYPE_ERROR && labelIsOrdinal) {
                if (!typeChecker.isCompatible(exprType, constant->type)) {
                    typeChecker.reportError("case label type incompatible with case expression",constant->line);
                }
            }

            int exprSubrangeRef = expressionSubrangeRef();
            if (exprType == TYPE_SUBRANGE && labelIsOrdinal &&
                exprSubrangeRef >= 0 && exprSubrangeRef < symTab.getAtabSize()) {
                const AtabEntry& ae = symTab.getAtab(exprSubrangeRef);
                if (labelVal < ae.low || labelVal > ae.high) {
                    typeChecker.reportError("case label " + std::to_string(labelVal) +
                                            " out of subrange [" + std::to_string(ae.low) +
                                            ".." + std::to_string(ae.high) + "]", constant->line);
                }
            }

            if (labelIsOrdinal) {
                if (std::find(seenLabels.begin(), seenLabels.end(), labelVal) != seenLabels.end()) {
                    typeChecker.reportError("duplicate case label: " + std::to_string(labelVal),constant->line);
                } else {
                    seenLabels.push_back(labelVal);
                }
            }
        }
        if (branch->statement) visit(branch->statement);
    }
}

void SemanticAnalyzer::visitProcCall(ProcCallNode* node) {
    int tabIdx = symTab.lookup(node->name);
    if (tabIdx == -1) {
        typeChecker.reportError("undeclared identifier: " + node->name, node->line);
        node->type = TYPE_ERROR;
        return;
    }

    const TabEntry& e = symTab.getTab(tabIdx);
    if (e.obj != OBJ_PROCEDURE && e.obj != OBJ_FUNCTION) {
        typeChecker.reportError("'" + node->name + "' is not a procedure or function", node->line);
        node->type = TYPE_ERROR;
        return;
    }

    node->tabIndex = tabIdx;
    node->type = e.type;
    node->lev = e.lev;

    for (auto* arg : node->args) {
        if (arg) visit(arg);
    }

    std::string builtinName = toLowerCopy(e.id);
    bool builtinRead = (e.lev == 0 && (builtinName == "read" || builtinName == "readln"));
    bool builtinWrite = (e.lev == 0 && (builtinName == "write" || builtinName == "writeln"));
    if (builtinRead || builtinWrite) {
        if (builtinRead) {
            for (size_t i = 0; i < node->args.size(); i++) {
                ASTNode* arg = node->args[i];
                if (arg && arg->type != TYPE_ERROR && !arg->isLValue) {
                    typeChecker.reportError("call to '" + node->name + "': argument " +
                                            std::to_string(i + 1) +
                                            " must be a variable for read/readln", arg->line);
                }
            }
        }
        return;
    }

    int subBtab = e.ref;
    if (subBtab <= 0 || subBtab >= symTab.getBtabSize()) return;

    int lparIdx = symTab.getBtab(subBtab).lpar;
    std::vector<int> params;
    int subLev = e.lev + 1;
    int cur = lparIdx;
    while (cur > 0 && cur < symTab.getTabSize()) {
        const TabEntry& p = symTab.getTab(cur);
        if (p.lev != subLev || p.obj != OBJ_VARIABLE) break;
        if (e.obj == OBJ_FUNCTION && p.id == node->name) break;
        params.push_back(cur);
        cur = p.link;
    }
    std::reverse(params.begin(), params.end());

    if (params.size() != node->args.size()) {
        typeChecker.reportError("call to '" + node->name + "': expected " +
                                std::to_string(params.size()) + " arguments, got " +
                                std::to_string(node->args.size()), node->line);
        return;
    }
    for (size_t i = 0; i < params.size(); i++) {
        const TabEntry& p = symTab.getTab(params[i]);
        if (!node->args[i]) continue;
        int argType = node->args[i]->type;
        if (argType == TYPE_ERROR) continue;
        if (!typeChecker.isAssignmentCompatible(p.type, argType)) {
            typeChecker.reportError("call to '" + node->name + "': argument " +
                                    std::to_string(i + 1) + " type mismatch", node->line);
        }
        if (p.nrm == PARAM_REF && !node->args[i]->isLValue) {
            typeChecker.reportError("call to '" + node->name + "': argument " +
                                    std::to_string(i + 1) + " must be a variable (var parameter)", node->line);
        }
    }
}

void SemanticAnalyzer::visitVar(VarNode* node) {
    int tabIdx = symTab.lookup(node->name);
    if (tabIdx == -1) {
        typeChecker.reportError("undeclared identifier: " + node->name, node->line);
        node->type = TYPE_ERROR;
        return;
    }

    const TabEntry& e = symTab.getTab(tabIdx);
    node->tabIndex = tabIdx;
    node->lev = e.lev;
    node->type = e.type;
    node->isLValue = (e.obj == OBJ_VARIABLE);

    if (e.obj == OBJ_FUNCTION) {
        node->type = e.type;
        node->isLValue = false;
    }

    if (!node->components.empty()) {
        int currentType = node->type;
        int currentRef = valueTypeRef(e);

        for (auto* component : node->components) {
            if (!component) continue;

            if (component->kind == VarComponentNode::ARRAY_ACCESS) {
                for (auto* idx : component->indices) {
                    if (idx) visit(idx);
                    if (currentType == TYPE_ARRAY && currentRef >= 0 && currentRef < symTab.getAtabSize()) {
                        const AtabEntry& a = symTab.getAtab(currentRef);
                        if (idx && idx->type != TYPE_ERROR &&
                            !typeChecker.isCompatible(a.xtyp, idx->type)) {
                            typeChecker.reportError("array '" + node->name + "' index type mismatch", node->line);
                        }
                        currentType = a.etyp;
                        currentRef = a.eref;
                        node->isLValue = true;
                    } else if (currentType != TYPE_ERROR) {
                        typeChecker.reportError("'" + node->name + "' is not an array", node->line);
                        currentType = TYPE_ERROR;
                        currentRef = 0;
                    }
                }
            } else if (component->kind == VarComponentNode::FIELD_ACCESS) {
                if (currentType == TYPE_RECORD && currentRef >= 0 && currentRef < symTab.getBtabSize()) {
                    int last = symTab.getBtab(currentRef).last;
                    int found = -1;
                    std::string target = toLowerCopy(component->fieldName);
                    while (last > 0 && last < symTab.getTabSize()) {
                        if (toLowerCopy(symTab.getTab(last).id) == target) {
                            found = last;
                            break;
                        }
                        last = symTab.getTab(last).link;
                    }
                    if (found == -1) {
                        typeChecker.reportError("record '" + node->name + "' has no field '" +
                                                component->fieldName + "'", node->line);
                        currentType = TYPE_ERROR;
                        currentRef = 0;
                    } else {
                        currentType = symTab.getTab(found).type;
                        currentRef = symTab.getTab(found).ref;
                        node->isLValue = true;
                    }
                } else if (currentType != TYPE_ERROR) {
                    typeChecker.reportError("'" + node->name + "' is not a record", node->line);
                    currentType = TYPE_ERROR;
                    currentRef = 0;
                }
            }
        }

        node->type = currentType;
        return;
    }

    if (node->isArrayAccess && node->index) {
        visit(node->index);
        int baseRef = valueTypeRef(e);
        if (e.type == TYPE_ARRAY && baseRef >= 0 && baseRef < symTab.getAtabSize()) {
            const AtabEntry& a = symTab.getAtab(baseRef);
            if (node->index->type != TYPE_ERROR &&
                !typeChecker.isCompatible(a.xtyp, node->index->type)) {
                typeChecker.reportError("array '" + node->name + "' index type mismatch", node->line);
            }
            node->type = a.etyp;
            node->isLValue = true;
        } else {
            typeChecker.reportError("'" + node->name + "' is not an array", node->line);
            node->type = TYPE_ERROR;
        }
    }

    if (!node->fieldName.empty()) {
        int baseRef = valueTypeRef(e);
        if (e.type == TYPE_RECORD && baseRef >= 0 && baseRef < symTab.getBtabSize()) {
            int last = symTab.getBtab(baseRef).last;
            int found = -1;
            while (last > 0 && last < symTab.getTabSize()) {
                if (symTab.getTab(last).id == node->fieldName) { found = last; break; }
                last = symTab.getTab(last).link;
            }
            if (found == -1) {
                typeChecker.reportError("record '" + node->name + "' has no field '" + node->fieldName + "'", node->line);
                node->type = TYPE_ERROR;
            } else {
                node->type = symTab.getTab(found).type;
                node->isLValue = true;
            }
        } else {
            typeChecker.reportError("'" + node->name + "' is not a record", node->line);
            node->type = TYPE_ERROR;
        }
    }
}

int SemanticAnalyzer::resolveType(const std::string& typeName) const {
    std::string lower = toLowerCopy(typeName);
    if (lower == "integer") return TYPE_INTEGER;
    if (lower == "real")    return TYPE_REAL;
    if (lower == "boolean") return TYPE_BOOLEAN;
    if (lower == "char")    return TYPE_CHAR;
    if (lower == "string")  return TYPE_STRING;

    int idx = symTab.lookup(typeName);
    if (idx != -1) return symTab.getTab(idx).type;
    return TYPE_ERROR;
}

int SemanticAnalyzer::sizeOfPrimitive(int typeCode) const {
    switch (typeCode) {
        case TYPE_INTEGER: return INT_SIZE;
        case TYPE_REAL:    return REAL_SIZE;
        case TYPE_BOOLEAN: return BOOL_SIZE;
        case TYPE_CHAR:    return CHAR_SIZE;
        case TYPE_STRING:  return STRING_SIZE;
        case TYPE_ENUM:    return 1;
        case TYPE_SUBRANGE:return 1;
        default:           return 1;
    }
}

int SemanticAnalyzer::constNodeOrdinalValue(ASTNode* c, int& outType) const {
    outType = TYPE_ERROR;
    if (!c) return 0;
    if (auto* num = dynamic_cast<NumberNode*>(c)) { outType = TYPE_INTEGER; return num->value; }
    if (auto* real = dynamic_cast<RealNode*>(c))  { outType = TYPE_REAL;    return (int)real->value; }
    if (auto* ch  = dynamic_cast<CharNode*>(c))   { outType = TYPE_CHAR;    return (int)ch->value; }
    if (auto* b   = dynamic_cast<BoolNode*>(c))   { outType = TYPE_BOOLEAN; return b->value ? 1 : 0; }
    if (auto* var = dynamic_cast<VarNode*>(c)) {
        int idx = symTab.lookup(var->name);
        if (idx != -1) {
            const TabEntry& e = symTab.getTab(idx);
            if (e.obj == OBJ_CONSTANT) {
                outType = e.type;
                return e.adr;
            }
        }
    }
    if (auto* unary = dynamic_cast<UnaryOpNode*>(c)) {
        if (unary->op == "-") {
            int t = TYPE_ERROR;
            int v = constNodeOrdinalValue(unary->operand, t);
            outType = t;
            return -v;
        }
    }
    return 0;
}

SemanticAnalyzer::ResolvedType SemanticAnalyzer::resolveTypeNode(TypeNode* tn) {
    ResolvedType r(TYPE_INTEGER, 0, INT_SIZE);
    if (!tn) return r;

    if (auto* simple = dynamic_cast<SimpleTypeNode*>(tn)) {
        std::string lower = toLowerCopy(simple->name);
        if (lower == "integer")      r = ResolvedType(TYPE_INTEGER, 0, INT_SIZE);
        else if (lower == "real")    r = ResolvedType(TYPE_REAL,    0, REAL_SIZE);
        else if (lower == "boolean") r = ResolvedType(TYPE_BOOLEAN, 0, BOOL_SIZE);
        else if (lower == "char")    r = ResolvedType(TYPE_CHAR,    0, CHAR_SIZE);
        else if (lower == "string")  r = ResolvedType(TYPE_STRING,  0, STRING_SIZE);
        else {
            int idx = symTab.lookup(simple->name);
            if (idx == -1) {
                typeChecker.reportError("undeclared type identifier: " + simple->name, simple->line);
                r = ResolvedType(TYPE_ERROR, 0, 0);
            } else {
                const TabEntry& e = symTab.getTab(idx);
                if (e.obj != OBJ_TYPE) {
                    typeChecker.reportError("'" + simple->name + "' is not a type", simple->line);
                    r = ResolvedType(TYPE_ERROR, 0, 0);
                } else {
                    r = ResolvedType(e.type, e.ref, e.adr > 0 ? e.adr : sizeOfPrimitive(e.type));
                }
            }
        }
    } else if (auto* arr = dynamic_cast<ArrayTypeNode*>(tn)) {
        r = resolveArrayType(arr);
    } else if (auto* rec = dynamic_cast<RecordTypeNode*>(tn)) {
        r = resolveRecordType(rec);
    } else if (auto* rng = dynamic_cast<RangeTypeNode*>(tn)) {
        r = resolveRangeType(rng);
    } else if (auto* en = dynamic_cast<EnumeratedTypeNode*>(tn)) {
        r = resolveEnumType(en);
    }

    tn->type = r.typeCode;
    tn->ref = r.ref;
    tn->size = r.size;
    return r;
}

SemanticAnalyzer::ResolvedType SemanticAnalyzer::resolveRangeType(RangeTypeNode* rng) {
    ResolvedType r(TYPE_SUBRANGE, 0, 1);
    if (!rng->low || !rng->high) {
        return r;
    }
    int loType = TYPE_ERROR, hiType = TYPE_ERROR;
    int loVal = constNodeOrdinalValue(rng->low, loType);
    int hiVal = constNodeOrdinalValue(rng->high, hiType);
    if (loType == TYPE_REAL || hiType == TYPE_REAL) {
        typeChecker.reportError("subrange bounds must not be Real", rng->line);
        r.typeCode = TYPE_ERROR;
    }
    if (loType != TYPE_ERROR && hiType != TYPE_ERROR && loType != hiType) {
        typeChecker.reportError("subrange bounds must have the same type", rng->line);
    }
    if (loVal > hiVal) {
        typeChecker.reportError("subrange lower bound must be <= upper bound", rng->line);
    }
    if (rng->low)  rng->low->type  = (loType == TYPE_ERROR) ? TYPE_INTEGER : loType;
    if (rng->high) rng->high->type = (hiType == TYPE_ERROR) ? TYPE_INTEGER : hiType;

    if (r.typeCode == TYPE_SUBRANGE && loVal <= hiVal) {
        AtabEntry ae;
        ae.xtyp = (loType != TYPE_ERROR) ? loType : TYPE_INTEGER;
        ae.etyp = TYPE_VOID;
        ae.eref = -1;
        ae.low  = loVal;
        ae.high = hiVal;
        ae.elsz = 1;
        ae.size = hiVal - loVal + 1;
        int aidx = symTab.addToAtab(ae);
        r.ref   = aidx;
        rng->ref = aidx;
    }
    return r;
}

SemanticAnalyzer::ResolvedType SemanticAnalyzer::resolveArrayType(ArrayTypeNode* arr) {
    ResolvedType r(TYPE_ARRAY, 0, 0);
    int low = 0, high = 0, xtyp = TYPE_INTEGER;

    if (arr->indexType) {
        if (auto* rng = dynamic_cast<RangeTypeNode*>(arr->indexType)) {
            int loT = TYPE_INTEGER, hiT = TYPE_INTEGER;
            low = constNodeOrdinalValue(rng->low, loT);
            high = constNodeOrdinalValue(rng->high, hiT);
            xtyp = (loT != TYPE_ERROR) ? loT : TYPE_INTEGER;
            if (xtyp == TYPE_REAL) {
                typeChecker.reportError("array index type cannot be Real", arr->line);
                xtyp = TYPE_INTEGER;
            }
            if (low > high) {
                typeChecker.reportError("array range lower must be <= upper", arr->line);
            }
            rng->type = TYPE_SUBRANGE;
            rng->size = 1;
            arr->indexType->type = TYPE_SUBRANGE;
        } else if (auto* simple = dynamic_cast<SimpleTypeNode*>(arr->indexType)) {
            ResolvedType irt = resolveTypeNode(simple);
            xtyp = irt.typeCode;
            if (xtyp == TYPE_REAL) {
                typeChecker.reportError("array index type cannot be Real", arr->line);
                xtyp = TYPE_INTEGER;
            }
            low = 0;
            high = 0;
        } else {
            ResolvedType irt = resolveTypeNode(arr->indexType);
            xtyp = irt.typeCode;
        }
    }

    int etyp = TYPE_INTEGER;
    int eref = 0;
    int elsz = 1;
    if (arr->elementType) {
        ResolvedType er = resolveTypeNode(arr->elementType);
        etyp = er.typeCode;
        eref = er.ref;
        elsz = (er.size > 0) ? er.size : 1;
    }

    AtabEntry ae;
    ae.xtyp = xtyp;
    ae.etyp = etyp;
    ae.eref = (etyp == TYPE_ARRAY || etyp == TYPE_RECORD || etyp == TYPE_SUBRANGE) ? eref : 0;
    ae.low = low;
    ae.high = high;
    ae.elsz = elsz;
    int span = (high - low + 1);
    if (span < 0) span = 0;
    ae.size = span * elsz;

    int aidx = symTab.addToAtab(ae);
    arr->atabIndex = aidx;
    arr->ref = aidx;
    arr->size = ae.size;
    arr->type = TYPE_ARRAY;

    r.ref = aidx;
    r.size = ae.size;
    return r;
}

SemanticAnalyzer::ResolvedType SemanticAnalyzer::resolveRecordType(RecordTypeNode* rec) {
    ResolvedType r(TYPE_RECORD, 0, 0);

    BtabEntry recBlock;
    recBlock.lpar = 0;
    int recBtab = symTab.addToBtab(recBlock);

    int totalSize = 0;
    int prevLast = -1;
    int recLevel = symTab.getCurrentLevel() + 1;
    std::vector<std::string> fieldNames;

    for (auto* field : rec->fields) {
        if (!field || !field->fieldType) continue;
        std::string normalizedName = toLowerCopy(field->name);
        if (std::find(fieldNames.begin(), fieldNames.end(), normalizedName) != fieldNames.end()) {
            typeChecker.reportError("duplicate record field: " + field->name, field->line);
            continue;
        }
        fieldNames.push_back(normalizedName);

        ResolvedType fr = resolveTypeNode(field->fieldType);

        TabEntry fe(field->name, OBJ_VARIABLE, fr.typeCode, recLevel, totalSize);
        fe.ref = fr.ref;
        fe.nrm = PARAM_VALUE;
        fe.link = prevLast;
        int fidx = symTab.addToTab(fe);
        prevLast = fidx;
        symTab.getBtab(recBtab).last = fidx;
        field->tabIndex = fidx;
        field->type = fr.typeCode;
        field->lev = recLevel;

        int sz = (fr.size > 0) ? fr.size : 1;
        totalSize += sz;
    }

    symTab.getBtab(recBtab).vsze = totalSize;

    rec->btabIndex = recBtab;
    rec->ref = recBtab;
    rec->size = totalSize;
    rec->type = TYPE_RECORD;

    r.ref = recBtab;
    r.size = totalSize;
    return r;
}

SemanticAnalyzer::ResolvedType SemanticAnalyzer::resolveEnumType(EnumeratedTypeNode* en) {
    ResolvedType r(TYPE_ENUM, 0, 1);
    int curLev = symTab.getCurrentLevel();
    int btabIdx = symTab.getCurrentBtabIndex();
    for (size_t i = 0; i < en->values.size(); i++) {
        const std::string& nm = en->values[i];
        int existing = symTab.lookupInCurrentScope(nm);
        if (existing != -1) {
            typeChecker.reportError("duplicate enumerated identifier: " + nm, en->line);
            continue;
        }
        TabEntry e(nm, OBJ_CONSTANT, TYPE_ENUM, curLev, (int)i);
        e.nrm = PARAM_VALUE;
        e.ref = 0;
        e.link = symTab.getBtab(btabIdx).last;
        int idx = symTab.addToTab(e);
        symTab.getBtab(btabIdx).last = idx;
    }
    en->type = TYPE_ENUM;
    en->size = 1;
    return r;
}

std::string SemanticAnalyzer::getTokenValue(const ParseNode& node) const {
    if (!node.isTerminal) return "";

    std::string label = node.label;
    size_t start = label.find('(');
    size_t end = label.find(')');

    if (start != std::string::npos && end != std::string::npos) {
        return label.substr(start + 1, end - start - 1);
    }

    return label;
}

int SemanticAnalyzer::findLineForNode(const ParseNode& node) const {
    const ParseNode* terminal = nullptr;
    std::function<void(const ParseNode&)> findTerminal = [&](const ParseNode& cur) {
        if (terminal) return;
        if (cur.isTerminal) {
            terminal = &cur;
            return;
        }
        for (const auto& child : cur.children) findTerminal(child);
    };
    findTerminal(node);
    if (!terminal || !tokens) return 0;

    if (terminal->line > 0 && terminal->column > 0) {
        return terminal->line * 10000 + terminal->column;
    }
    if (terminal->line > 0) {
        return terminal->line;
    }

    std::string label = terminal->label;
    std::string tokenType = label;
    std::string tokenValue;
    size_t start = label.find('(');
    size_t end = label.rfind(')');
    if (start != std::string::npos) {
        tokenType = label.substr(0, start);
        if (end != std::string::npos && end > start) {
            tokenValue = label.substr(start + 1, end - start - 1);
        }
    }

    for (const auto& token : *tokens) {
        if (tokenToString(token.type) != tokenType) continue;
        if (!tokenValue.empty() && token.value != tokenValue) continue;
        if (token.line > 0 && token.column > 0) {
            return token.line * 10000 + token.column;
        }
        return token.line;
    }

    return 0;
}

bool SemanticAnalyzer::isTokenNode(const ParseNode& node, const std::string& tokenType) const {
    return node.isTerminal && (node.label == tokenType || node.label.rfind(tokenType, 0) == 0);
}

ParseNode SemanticAnalyzer::findChild(const ParseNode& node, const std::string& label) const {
    for (const auto& child : node.children) {
        if (child.label == label) return child;
    }
    return ParseNode("not_found");
}
