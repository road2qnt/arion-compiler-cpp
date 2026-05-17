#include "semanticAnalyzer.hpp"
#include <sstream>
#include <algorithm>

static std::string mapOpToken(const std::string& token) {
    if (token == "plus") return "+";
    if (token == "minus") return "-";
    if (token == "times") return "*";
    if (token == "rdiv") return "/";
    if (token == "idiv") return "div";
    if (token == "imod") return "mod";
    if (token == "andsy") return "and";
    if (token == "orsy") return "or";
    if (token == "notsy") return "not";
    if (token == "eql") return "=";
    if (token == "neq") return "<>";
    if (token == "lss") return "<";
    if (token == "leq") return "<=";
    if (token == "gtr") return ">";
    if (token == "geq") return ">=";
    return token;
}

SemanticAnalyzer::SemanticAnalyzer()
    : typeChecker(symTab), astRoot(nullptr), tokens(nullptr) {}

SemanticAnalyzer::~SemanticAnalyzer() {
    delete astRoot;
}

ProgramNode* SemanticAnalyzer::analyze(const ParseNode& parseTree, const std::vector<Token>& tok) {
    tokens = &tok;
    typeChecker.clearErrors();

    astRoot = convertProgram(parseTree);

    if (!astRoot) return nullptr;

    visit(astRoot);

    return astRoot;
}

ProgramNode* SemanticAnalyzer::convertProgram(const ParseNode& parseTree) {
    std::string progName = "unnamed";

    for (const auto& child : parseTree.children) {
        if (child.label == "<Program-header>") {
            for (const auto& headerChild : child.children) {
                if (headerChild.label.find("ident(") == 0 || headerChild.label.find("ident(") == 0) {
                    progName = getTokenValue(headerChild);
                } else if (headerChild.isTerminal && headerChild.label.rfind("ident", 0) == 0) {
                    size_t start = headerChild.label.find('(');
                    size_t end = headerChild.label.find(')');
                    if (start != std::string::npos && end != std::string::npos) {
                        progName = headerChild.label.substr(start + 1, end - start - 1);
                    }
                }
            }
        }
    }

    ProgramNode* program = new ProgramNode(progName);

    for (const auto& child : parseTree.children) {
        if (child.label == "<declaration-part>") {
            program->declarations = convertDeclarationPart(child);
        } else if (child.label == "<compound-statement>") {
            program->body = convertCompoundStatement(child);
        }
    }

    return program;
}

DeclarationsNode* SemanticAnalyzer::convertDeclarationPart(const ParseNode& node) {
    DeclarationsNode* decls = new DeclarationsNode();

    for (const auto& child : node.children) {
        if (child.label == "<const-declaration>") {
            std::vector<DeclNode*> constDecls = convertConstDecls(child);
            for (auto* d : constDecls) decls->decls.push_back(d);
        } else if (child.label == "<type-declaration>") {
            std::vector<DeclNode*> typeDecls = convertTypeDecls(child);
            for (auto* d : typeDecls) decls->decls.push_back(d);
        } else if (child.label == "<var-declaration>") {
            std::vector<DeclNode*> varDecls = convertVarDecls(child);
            for (auto* d : varDecls) decls->decls.push_back(d);
        } else if (child.label == "<subprogram-declaration>" ||
                   child.label == "<procedure-declaration>" ||
                   child.label == "<function-declaration>") {
            ASTNode* decl = convertDeclaration(child);
            if (auto* sd = dynamic_cast<SubprogramDeclNode*>(decl)) {
                decls->decls.push_back(sd);
            }
        }
    }

    return decls;
}

ASTNode* SemanticAnalyzer::convertDeclaration(const ParseNode& node) {
    if (node.label == "<subprogram-declaration>") {
        return convertSubprogramDecl(node);
    }
    if (node.label == "<procedure-declaration>" || node.label == "<function-declaration>") {
        return convertSubprogramDecl(node);
    }
    return nullptr;
}

std::vector<DeclNode*> SemanticAnalyzer::convertVarDecls(const ParseNode& node) {
    std::vector<DeclNode*> result;

    size_t i = 1;
    while (i < node.children.size()) {
        if (node.children[i].label == "<identifier-list>") {
            std::vector<std::string> names;
            for (const auto& child : node.children[i].children) {
                if (child.isTerminal && child.label.rfind("ident", 0) == 0) {
                    std::string name = getTokenValue(child);
                    if (!name.empty()) {
                        names.push_back(name);
                    }
                }
            }

            i += 2;
            if (i < node.children.size()) {
                TypeNode* varType = convertType(node.children[i]);
                i++;

                for (const auto& name : names) {
                    VarDeclNode* vd = new VarDeclNode(name);
                    vd->varType = varType;
                    result.push_back(vd);
                }
            }
        } else if (node.children[i].isTerminal &&
                   node.children[i].label.rfind("ident", 0) == 0) {
            std::string name = getTokenValue(node.children[i]);
            i += 2;
            if (i < node.children.size()) {
                TypeNode* varType = convertType(node.children[i]);
                VarDeclNode* vd = new VarDeclNode(name);
                vd->varType = varType;
                result.push_back(vd);
                i++;
            }
        } else {
            i++;
        }
    }

    return result;
}

std::vector<DeclNode*> SemanticAnalyzer::convertConstDecls(const ParseNode& node) {
    std::vector<DeclNode*> result;

    for (size_t i = 1; i + 2 < node.children.size(); i++) {
        if (node.children[i].isTerminal && node.children[i].label.rfind("ident", 0) == 0) {
            std::string name = getTokenValue(node.children[i]);
            ConstDeclNode* cd = new ConstDeclNode(name);
            if (i + 2 < node.children.size()) {
                cd->value = convertConstant(node.children[i + 2]);
            }
            result.push_back(cd);
            i += 3;
        }
    }

    return result;
}

std::vector<DeclNode*> SemanticAnalyzer::convertTypeDecls(const ParseNode& node) {
    std::vector<DeclNode*> result;

    for (size_t i = 1; i + 2 < node.children.size(); i++) {
        if (node.children[i].isTerminal && node.children[i].label.rfind("ident", 0) == 0) {
            std::string name = getTokenValue(node.children[i]);
            TypeDeclNode* td = new TypeDeclNode(name);
            if (i + 2 < node.children.size()) {
                td->typeDef = convertType(node.children[i + 2]);
            }
            result.push_back(td);
            i += 3;
        }
    }

    return result;
}

SubprogramDeclNode* SemanticAnalyzer::convertSubprogramDecl(const ParseNode& node) {
    std::string name = "subprogram";
    bool isFunction = false;

    if (node.label == "<procedure-declaration>") {
        for (size_t i = 1; i < node.children.size(); i++) {
            if (node.children[i].isTerminal && node.children[i].label.rfind("ident", 0) == 0) {
                name = getTokenValue(node.children[i]);
                break;
            }
        }
    } else if (node.label == "<function-declaration>") {
        isFunction = true;
        for (size_t i = 1; i < node.children.size(); i++) {
            if (node.children[i].isTerminal && node.children[i].label.rfind("ident", 0) == 0) {
                name = getTokenValue(node.children[i]);
                break;
            }
        }
    }

    SubprogramDeclNode* sub = new SubprogramDeclNode(name, isFunction);

    for (const auto& child : node.children) {
        if (child.label == "<compound-statement>") {
            sub->body = convertCompoundStatement(child);
        } else if (child.label == "<block>") {
            for (const auto& blockChild : child.children) {
                if (blockChild.label == "<compound-statement>") {
                    sub->body = convertCompoundStatement(blockChild);
                }
            }
        } else if (child.label == "<formal-parameter-list>") {
            for (const auto& paramChild : child.children) {
                if (paramChild.label == "<parameter-group>") {
                    for (size_t p = 0; p < paramChild.children.size(); p++) {
                        if (paramChild.children[p].isTerminal &&
                            paramChild.children[p].label.rfind("ident", 0) == 0) {
                            VarDeclNode* param = new VarDeclNode(getTokenValue(paramChild.children[p]));
                            sub->params.push_back(param);
                        }
                    }
                }
            }
        }
    }

    return sub;
}

TypeNode* SemanticAnalyzer::convertType(const ParseNode& node) {
    if (node.children.empty()) return nullptr;

    const ParseNode& inner = node.children[0];

    if (inner.isTerminal && inner.label.rfind("ident", 0) == 0) {
        std::string name = getTokenValue(inner);
        return new SimpleTypeNode(name);
    }

    if (inner.label == "<array-type>") {
        ArrayTypeNode* arr = new ArrayTypeNode();
        for (size_t i = 2; i < inner.children.size(); i++) {
            if (inner.children[i].label == "<range>") {
                arr->indexType = convertType(inner.children[i]);
            } else if (inner.children[i].isTerminal &&
                       inner.children[i].label.rfind("ident", 0) == 0) {
                arr->indexType = new SimpleTypeNode(getTokenValue(inner.children[i]));
            }
        }
        for (size_t i = 0; i < inner.children.size(); i++) {
            if (inner.children[i].isTerminal && inner.children[i].label == "ofsy") {
                if (i + 1 < inner.children.size() && inner.children[i+1].label == "<type>") {
                    arr->elementType = convertType(inner.children[i+1]);
                }
                break;
            }
        }
        return arr;
    }

    if (inner.label == "<range>") {
        RangeTypeNode* range = new RangeTypeNode();
        for (const auto& child : inner.children) {
            if (child.label == "<constant>") {
                if (!range->low) {
                    range->low = convertConstant(child);
                } else if (!range->high) {
                    range->high = convertConstant(child);
                }
            }
        }
        return range;
    }

    if (inner.label == "<enumerated>") {
        EnumeratedTypeNode* en = new EnumeratedTypeNode();
        for (const auto& child : inner.children) {
            if (child.isTerminal && child.label.rfind("ident", 0) == 0) {
                en->values.push_back(getTokenValue(child));
            }
        }
        return en;
    }

    if (inner.label == "<record-type>") {
        RecordTypeNode* rec = new RecordTypeNode();
        for (const auto& child : inner.children) {
            if (child.label == "<field-list>") {
                for (const auto& fieldChild : child.children) {
                    if (fieldChild.label == "<field-part>") {
                        std::vector<std::string> fieldNames;
                        for (const auto& fp : fieldChild.children) {
                            if (fp.isTerminal && fp.label.rfind("ident", 0) == 0) {
                                fieldNames.push_back(getTokenValue(fp));
                            }
                        }
                        for (const auto& fname : fieldNames) {
                            FieldDeclNode* fd = new FieldDeclNode(fname);
                            rec->fields.push_back(fd);
                        }
                    }
                }
            }
        }
        return rec;
    }

    return nullptr;
}

ASTNode* SemanticAnalyzer::convertCompoundStatement(const ParseNode& node) {
    for (const auto& child : node.children) {
        if (child.label == "<statement-list>") {
            return convertStatementList(child);
        }
    }
    return nullptr;
}

ASTNode* SemanticAnalyzer::convertStatementList(const ParseNode& node) {
    BlockNode* block = new BlockNode();

    for (const auto& child : node.children) {
        if (child.label == "<statement>") {
            ASTNode* stmt = convertStatement(child);
            if (stmt) {
                block->statements.push_back(stmt);
            }
        }
    }

    if (block->statements.empty()) {
        delete block;
        return nullptr;
    }

    return block;
}

ASTNode* SemanticAnalyzer::convertStatement(const ParseNode& node) {
    if (node.children.empty()) return nullptr;

    const ParseNode& inner = node.children[0];

    if (inner.label == "<assignment-statement>") {
        return convertAssignmentStatement(inner);
    }
    if (inner.label == "<if-statement>") {
        return convertIfStatement(inner);
    }
    if (inner.label == "<while-statement>") {
        return convertWhileStatement(inner);
    }
    if (inner.label == "<repeat-statement>") {
        return convertRepeatStatement(inner);
    }
    if (inner.label == "<for-statement>") {
        return convertForStatement(inner);
    }
    if (inner.label == "<case-statement>") {
        return convertCaseStatement(inner);
    }
    if (inner.label == "<procedure-function-call>") {
        return convertProcedureFunctionCall(inner);
    }

    return nullptr;
}

ASTNode* SemanticAnalyzer::convertAssignmentStatement(const ParseNode& node) {
    AssignNode* assign = new AssignNode();

    for (const auto& child : node.children) {
        if (child.label == "<variable>") {
            assign->target = dynamic_cast<VarNode*>(convertVariable(child));
        } else if (child.label == "<expression>") {
            assign->value = convertExpression(child);
        }
    }

    return assign;
}

ASTNode* SemanticAnalyzer::convertIfStatement(const ParseNode& node) {
    IfNode* ifNode = new IfNode();

    for (size_t i = 0; i < node.children.size(); i++) {
        if (node.children[i].label == "<expression>") {
            if (!ifNode->condition) {
                ifNode->condition = convertExpression(node.children[i]);
            }
        } else if (node.children[i].label == "<statement>") {
            if (!ifNode->thenBranch) {
                ifNode->thenBranch = convertStatement(node.children[i]);
            } else {
                ifNode->elseBranch = convertStatement(node.children[i]);
            }
        }
    }

    return ifNode;
}

ASTNode* SemanticAnalyzer::convertWhileStatement(const ParseNode& node) {
    WhileNode* wNode = new WhileNode();

    for (const auto& child : node.children) {
        if (child.label == "<expression>") {
            wNode->condition = convertExpression(child);
        } else if (child.label == "<statement>") {
            wNode->body = convertStatement(child);
        }
    }

    return wNode;
}

ASTNode* SemanticAnalyzer::convertRepeatStatement(const ParseNode& node) {
    RepeatNode* rNode = new RepeatNode();

    for (const auto& child : node.children) {
        if (child.label == "<statement-list>") {
            rNode->body = convertStatementList(child);
        } else if (child.label == "<expression>") {
            rNode->condition = convertExpression(child);
        }
    }

    return rNode;
}

ASTNode* SemanticAnalyzer::convertForStatement(const ParseNode& node) {
    ForNode* fNode = new ForNode("");

    for (size_t i = 0; i < node.children.size(); i++) {
        if (node.children[i].isTerminal && node.children[i].label.rfind("ident", 0) == 0) {
            if (fNode->varName.empty()) {
                fNode->varName = getTokenValue(node.children[i]);
            }
        } else if (node.children[i].label == "<expression>") {
            if (!fNode->start) {
                fNode->start = convertExpression(node.children[i]);
            } else if (!fNode->end) {
                fNode->end = convertExpression(node.children[i]);
            }
        } else if (node.children[i].isTerminal &&
                   (node.children[i].label == "downtosy")) {
            fNode->isDownto = true;
        } else if (node.children[i].label == "<statement>") {
            fNode->body = convertStatement(node.children[i]);
        }
    }

    return fNode;
}

ASTNode* SemanticAnalyzer::convertCaseStatement(const ParseNode& node) {
    CaseNode* cNode = new CaseNode();

    for (size_t i = 0; i < node.children.size(); i++) {
        if (node.children[i].label == "<expression>") {
            cNode->expression = convertExpression(node.children[i]);
        } else if (node.children[i].label == "<case-block>") {
            CaseBranchNode* branch = new CaseBranchNode();
            for (const auto& cb : node.children[i].children) {
                if (cb.label == "<constant>") {
                    branch->constants.push_back(convertConstant(cb));
                } else if (cb.label == "<statement>") {
                    branch->statement = convertStatement(cb);
                }
            }
            cNode->branches.push_back(branch);
        }
    }

    return cNode;
}

ASTNode* SemanticAnalyzer::convertProcedureFunctionCall(const ParseNode& node) {
    std::string name;

    for (const auto& child : node.children) {
        if (child.isTerminal && child.label.rfind("ident", 0) == 0) {
            name = getTokenValue(child);
        }
    }

    ProcCallNode* call = new ProcCallNode(name);

    for (const auto& child : node.children) {
        if (child.label == "<parameter-list>") {
            for (const auto& param : child.children) {
                if (param.label == "<expression>") {
                    call->args.push_back(convertExpression(param));
                }
            }
        }
    }

    return call;
}

ASTNode* SemanticAnalyzer::convertExpression(const ParseNode& node) {
    if (node.children.empty()) return nullptr;

    ASTNode* left = convertSimpleExpression(node.children[0]);
    if (!left) return nullptr;

    if (node.children.size() >= 3) {
        std::string op;
        const ParseNode& opNode = node.children[1];
        if (!opNode.children.empty() && opNode.children[0].isTerminal) {
            op = mapOpToken(opNode.children[0].label);
        }

        BinOpNode* binOp = new BinOpNode(op);
        binOp->left = left;
        binOp->right = convertSimpleExpression(node.children[2]);
        return binOp;
    }

    return left;
}

ASTNode* SemanticAnalyzer::convertSimpleExpression(const ParseNode& node) {
    if (node.children.empty()) return nullptr;

    size_t startIdx = 0;
    bool hasUnary = false;
    std::string unaryOp;

    if (node.children[0].isTerminal &&
        (node.children[0].label == "plus" || node.children[0].label == "minus")) {
        unaryOp = node.children[0].label;
        hasUnary = true;
        startIdx = 1;
    }

    if (startIdx >= node.children.size()) return nullptr;

    ASTNode* result = convertTerm(node.children[startIdx]);
    if (!result) return nullptr;

    if (hasUnary && (unaryOp == "minus")) {
        UnaryOpNode* unary = new UnaryOpNode(mapOpToken(unaryOp));
        unary->operand = result;
        result = unary;
    }

    for (size_t i = startIdx + 1; i < node.children.size(); i++) {
        if (node.children[i].label == "<additive-operator>") {
            std::string op;
            if (!node.children[i].children.empty() && node.children[i].children[0].isTerminal) {
                op = mapOpToken(node.children[i].children[0].label);
            }

            if (i + 1 < node.children.size()) {
                ASTNode* right = convertTerm(node.children[i + 1]);
                BinOpNode* binOp = new BinOpNode(op);
                binOp->left = result;
                binOp->right = right;
                result = binOp;
            }
            i++;
        }
    }

    return result;
}

ASTNode* SemanticAnalyzer::convertTerm(const ParseNode& node) {
    if (node.children.empty()) return nullptr;

    ASTNode* result = convertFactor(node.children[0]);
    if (!result) return nullptr;

    for (size_t i = 1; i < node.children.size(); i++) {
        if (node.children[i].label == "<multiplicative-operator>") {
            std::string op;
            if (!node.children[i].children.empty() && node.children[i].children[0].isTerminal) {
                op = mapOpToken(node.children[i].children[0].label);
            }

            if (i + 1 < node.children.size()) {
                ASTNode* right = convertFactor(node.children[i + 1]);
                BinOpNode* binOp = new BinOpNode(op);
                binOp->left = result;
                binOp->right = right;
                result = binOp;
            }
            i++;
        }
    }

    return result;
}

ASTNode* SemanticAnalyzer::convertFactor(const ParseNode& node) {
    if (node.children.empty()) return nullptr;

    const ParseNode& inner = node.children[0];

    if (inner.isTerminal) {
        if (inner.label.rfind("intcon", 0) == 0) {
            std::string val = getTokenValue(inner);
            return new NumberNode(std::stoi(val));
        }
        if (inner.label.rfind("realcon", 0) == 0) {
            std::string val = getTokenValue(inner);
            return new RealNode(std::stod(val));
        }
        if (inner.label.rfind("charcon", 0) == 0) {
            std::string val = getTokenValue(inner);
            if (!val.empty()) return new CharNode(val[0]);
        }
        if (inner.label.rfind("string", 0) == 0) {
            return new StringNode(getTokenValue(inner));
        }
    }

    if (inner.label == "<variable>") {
        return convertVariable(inner);
    }

    if (inner.label == "<procedure-function-call>") {
        return convertProcedureFunctionCall(inner);
    }

    if (inner.isTerminal && inner.label == "notsy") {
        UnaryOpNode* notNode = new UnaryOpNode(mapOpToken(inner.label));
        if (node.children.size() > 1) {
            notNode->operand = convertFactor(node.children[1]);
        }
        return notNode;
    }

    if (inner.isTerminal && inner.label == "lparent") {
        for (const auto& child : node.children) {
            if (child.label == "<expression>") {
                return convertExpression(child);
            }
        }
    }

        if (inner.isTerminal && inner.label.rfind("ident", 0) == 0) {
        return new VarNode(getTokenValue(inner));
    }

    return nullptr;
}

ASTNode* SemanticAnalyzer::convertVariable(const ParseNode& node) {
    std::string name;

    for (const auto& child : node.children) {
        if (child.isTerminal && child.label.rfind("ident", 0) == 0) {
            name = getTokenValue(child);
            break;
        }
    }

    VarNode* var = new VarNode(name);
    VarNode* current = var;

    for (const auto& child : node.children) {
        if (child.label == "<component-variable>") {
            for (const auto& comp : child.children) {
                if (comp.isTerminal && comp.label == "lbrack") {
                    current->isArrayAccess = true;
                } else if (comp.isTerminal && comp.label == "period") {
                } else if (comp.isTerminal && comp.label.rfind("ident", 0) == 0) {
                    current->fieldName = getTokenValue(comp);
                } else if (comp.label == "<index-list>") {
                }
            }
        }
    }

    return var;
}

ASTNode* SemanticAnalyzer::convertConstant(const ParseNode& node) {
    if (node.children.empty()) return nullptr;

    for (const auto& child : node.children) {
        if (child.isTerminal) {
            if (child.label.rfind("intcon", 0) == 0) {
                return new NumberNode(std::stoi(getTokenValue(child)));
            }
            if (child.label.rfind("realcon", 0) == 0) {
                return new RealNode(std::stod(getTokenValue(child)));
            }
            if (child.label.rfind("charcon", 0) == 0) {
                std::string val = getTokenValue(child);
                if (!val.empty()) return new CharNode(val[0]);
            }
            if (child.label.rfind("string", 0) == 0) {
                return new StringNode(getTokenValue(child));
            }
            if (child.label.rfind("ident", 0) == 0) {
                return new VarNode(getTokenValue(child));
            }
        }
    }

    return nullptr;
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
    } else if (auto* call = dynamic_cast<ProcCallNode*>(node)) {
        visitProcCall(call);
    } else if (auto* var = dynamic_cast<VarNode*>(node)) {
        visitVar(var);
    }
}

void SemanticAnalyzer::visitProgram(ProgramNode* node) {
    int tabIdx = symTab.lookup(node->name);
    if (tabIdx == -1) {
        TabEntry entry(node->name, OBJ_PROCEDURE, TYPE_VOID, symTab.getCurrentLevel(), 0);
        entry.link = symTab.getBtab(symTab.getCurrentBtabIndex()).last;
        int idx = symTab.addToTab(entry);
        symTab.getBtab(symTab.getCurrentBtabIndex()).last = idx;
        node->tabIndex = idx;
    }

    if (node->declarations) {
        visit(node->declarations);
    }

    if (node->body) {
        visit(node->body);
    }
}

void SemanticAnalyzer::visitDeclarations(DeclarationsNode* node) {
    for (auto* decl : node->decls) {
        visit(decl);
    }
}

void SemanticAnalyzer::visitVarDecl(VarDeclNode* node) {
    int existing = symTab.lookupInCurrentScope(node->name);
    if (existing != -1) {
        typeChecker.reportError("duplicate identifier: " + node->name);
        return;
    }

    int typeCode = TYPE_INTEGER;
    if (node->varType) {
        if (auto* simple = dynamic_cast<SimpleTypeNode*>(node->varType)) {
            typeCode = resolveType(simple->name);
        }
    }

    int adr = symTab.getBtab(symTab.getCurrentBtabIndex()).vsze;
    int size = INT_SIZE;
    if (typeCode == TYPE_REAL) size = REAL_SIZE;
    else if (typeCode == TYPE_BOOLEAN || typeCode == TYPE_CHAR) size = 1;

    symTab.getBtab(symTab.getCurrentBtabIndex()).vsze += size;

    TabEntry entry(node->name, OBJ_VARIABLE, typeCode, symTab.getCurrentLevel(), adr);
    entry.link = symTab.getBtab(symTab.getCurrentBtabIndex()).last;
    int idx = symTab.addToTab(entry);
    symTab.getBtab(symTab.getCurrentBtabIndex()).last = idx;
    node->tabIndex = idx;
    node->type = typeCode;
}

void SemanticAnalyzer::visitConstDecl(ConstDeclNode* node) {
    int existing = symTab.lookupInCurrentScope(node->name);
    if (existing != -1) {
        typeChecker.reportError("duplicate identifier: " + node->name);
        return;
    }

    int constType = TYPE_INTEGER;
    int constValue = 0;

    if (node->value) {
        if (auto* num = dynamic_cast<NumberNode*>(node->value)) {
            constType = TYPE_INTEGER;
            constValue = num->value;
        } else if (auto* real = dynamic_cast<RealNode*>(node->value)) {
            constType = TYPE_REAL;
            constValue = (int)real->value;
        } else if (auto* ch = dynamic_cast<CharNode*>(node->value)) {
            constType = TYPE_CHAR;
            constValue = (int)ch->value;
        } else if (auto* b = dynamic_cast<BoolNode*>(node->value)) {
            constType = TYPE_BOOLEAN;
            constValue = b->value ? 1 : 0;
        }
    }

    TabEntry entry(node->name, OBJ_CONSTANT, constType, symTab.getCurrentLevel(), constValue);
    entry.link = symTab.getBtab(symTab.getCurrentBtabIndex()).last;
    int idx = symTab.addToTab(entry);
    symTab.getBtab(symTab.getCurrentBtabIndex()).last = idx;
    node->tabIndex = idx;
    node->type = constType;
}

void SemanticAnalyzer::visitTypeDecl(TypeDeclNode* node) {
    int existing = symTab.lookupInCurrentScope(node->name);
    if (existing != -1) {
        typeChecker.reportError("duplicate identifier: " + node->name);
        return;
    }

    int typeCode = TYPE_INTEGER;
    if (node->typeDef) {
        if (auto* simple = dynamic_cast<SimpleTypeNode*>(node->typeDef)) {
            typeCode = resolveType(simple->name);
        }
    }

    TabEntry entry(node->name, OBJ_TYPE, typeCode, symTab.getCurrentLevel(), 0);
    entry.link = symTab.getBtab(symTab.getCurrentBtabIndex()).last;
    int idx = symTab.addToTab(entry);
    symTab.getBtab(symTab.getCurrentBtabIndex()).last = idx;
    node->tabIndex = idx;
    node->type = typeCode;
}

void SemanticAnalyzer::visitSubprogramDecl(SubprogramDeclNode* node) {
    symTab.pushScope();

    int existing = symTab.lookupInCurrentScope(node->name);
    if (existing != -1) {
    }

    for (auto* param : node->params) {
        int paramType = TYPE_INTEGER;
        if (param->varType) {
            if (auto* simple = dynamic_cast<SimpleTypeNode*>(param->varType)) {
                paramType = resolveType(simple->name);
            }
        }

        int adr = symTab.getBtab(symTab.getCurrentBtabIndex()).psze;
        int size = INT_SIZE;
        if (paramType == TYPE_REAL) size = REAL_SIZE;

        symTab.getBtab(symTab.getCurrentBtabIndex()).psze += size;

        TabEntry entry(param->name, OBJ_PARAM, paramType, symTab.getCurrentLevel(), adr);
        entry.link = symTab.getBtab(symTab.getCurrentBtabIndex()).last;
        int idx = symTab.addToTab(entry);
        symTab.getBtab(symTab.getCurrentBtabIndex()).last = idx;
        param->tabIndex = idx;
    }

    if (node->body) {
        visit(node->body);
    }

    symTab.popScope();
}

void SemanticAnalyzer::visitAssign(AssignNode* node) {
    if (node->target) {
        visit(node->target);
    }
    if (node->value) {
        visit(node->value);
    }

    if (node->target && node->value) {
        if (!typeChecker.isAssignmentCompatible(node->target->type, node->value->type)) {
            typeChecker.reportError("incompatible types");
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


    bool isRelational = (node->op == "=" || node->op == "<>" || node->op == "<" ||
                         node->op == "<=" || node->op == ">" || node->op == ">=");
    bool isArithmetic = (node->op == "+" || node->op == "-" || node->op == "*" ||
                         node->op == "/" || node->op == "div" || node->op == "mod");
    bool isLogical = (node->op == "and" || node->op == "or");

    if (isRelational) {
        if (!typeChecker.isValidRelationalOperator(node->left->type, node->right->type)) {
            typeChecker.reportError("operator " + node->op + " not defined for these types");
            node->type = TYPE_ERROR;
        } else {
            node->type = typeChecker.getResultType(node->op, node->left->type, node->right->type);
        }
    } else if (isArithmetic) {
        if (!typeChecker.isValidArithmeticOperator(node->op, node->left->type, node->right->type)) {
            typeChecker.reportError("operator " + node->op + " not defined for these types");
            node->type = TYPE_ERROR;
        } else {
            node->type = typeChecker.getResultType(node->op, node->left->type, node->right->type);
        }
    } else if (isLogical) {
        if (!typeChecker.isValidLogicalOperator(node->op, node->left->type, node->right->type)) {
            typeChecker.reportError("operator " + node->op + " not defined for these types");
            node->type = TYPE_ERROR;
        } else {
            node->type = typeChecker.getResultType(node->op, node->left->type, node->right->type);
        }
    }
}

void SemanticAnalyzer::visitUnaryOp(UnaryOpNode* node) {
    if (!node->operand) {
        node->type = TYPE_ERROR;
        return;
    }
    visit(node->operand);

    if (!typeChecker.isValidUnaryOperator(node->op, node->operand->type)) {
        typeChecker.reportError("operator " + node->op + " not defined for this type");
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
        typeChecker.reportError("if condition must be Boolean");
    }
}

void SemanticAnalyzer::visitWhile(WhileNode* node) {
    if (node->condition) visit(node->condition);
    if (node->body) visit(node->body);

    if (node->condition && !typeChecker.isBoolean(node->condition->type)) {
        typeChecker.reportError("while condition must be Boolean");
    }
}

void SemanticAnalyzer::visitRepeat(RepeatNode* node) {
    if (node->body) visit(node->body);
    if (node->condition) visit(node->condition);

    if (node->condition && !typeChecker.isBoolean(node->condition->type)) {
        typeChecker.reportError("repeat condition must be Boolean");
    }
}

void SemanticAnalyzer::visitFor(ForNode* node) {
    if (node->start) visit(node->start);
    if (node->end) visit(node->end);
    if (node->body) visit(node->body);

    if (node->start && !typeChecker.isOrdinal(node->start->type)) {
        typeChecker.reportError("for loop starting value must be an ordinal type");
    }
    if (node->end && !typeChecker.isOrdinal(node->end->type)) {
        typeChecker.reportError("for loop ending value must be an ordinal type");
    }
}

void SemanticAnalyzer::visitCase(CaseNode* node) {
    if (node->expression) visit(node->expression);

    for (auto* branch : node->branches) {
        if (branch->statement) visit(branch->statement);
    }
}

void SemanticAnalyzer::visitProcCall(ProcCallNode* node) {
    int tabIdx = symTab.lookup(node->name);
    if (tabIdx == -1) {
        typeChecker.reportError("undeclared identifier: " + node->name);
        return;
    }

    node->tabIndex = tabIdx;
    node->type = symTab.getTab(tabIdx).type;

    for (auto* arg : node->args) {
        visit(arg);
    }
}

void SemanticAnalyzer::visitVar(VarNode* node) {
    int tabIdx = symTab.lookup(node->name);
    if (tabIdx == -1) {
        typeChecker.reportError("undeclared identifier: " + node->name);
        node->type = TYPE_ERROR;
        return;
    }

    node->tabIndex = tabIdx;
    node->lev = symTab.getTab(tabIdx).lev;
    node->type = symTab.getTab(tabIdx).type;
    node->isLValue = (symTab.getTab(tabIdx).obj == OBJ_VARIABLE ||
                      symTab.getTab(tabIdx).obj == OBJ_PARAM);
}

int SemanticAnalyzer::resolveType(const std::string& typeName) const {
    if (typeName == "integer") return TYPE_INTEGER;
    if (typeName == "real") return TYPE_REAL;
    if (typeName == "boolean") return TYPE_BOOLEAN;
    if (typeName == "char") return TYPE_CHAR;
    if (typeName == "string") return TYPE_STRING;

    int idx = symTab.lookup(typeName);
    if (idx != -1) {
        return symTab.getTab(idx).type;
    }

    return TYPE_INTEGER;
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
    (void)node;
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

void SemanticAnalyzer::printDecoratedAST(std::ostream& os, ASTNode* node, int depth) const {
    if (!node) return;

    std::string indent(depth * 2, ' ');

    if (auto* prog = dynamic_cast<ProgramNode*>(node)) {
        os << indent << "Program: " << prog->name;
        if (prog->tabIndex >= 0) os << " [tab=" << prog->tabIndex << "]";
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
            case TYPE_VOID:    typeStr = "void"; break;
            default:           typeStr = std::to_string(vd->type); break;
        }
        os << " type=" << typeStr << "\n";
    } else if (auto* cd = dynamic_cast<ConstDeclNode*>(node)) {
        os << indent << "ConstDecl: " << cd->name;
        if (cd->value) os << " value=" << cd->value->type;
        os << "\n";
    } else if (auto* td = dynamic_cast<TypeDeclNode*>(node)) {
        os << indent << "TypeDecl: " << td->name << "\n";
    } else if (auto* sd = dynamic_cast<SubprogramDeclNode*>(node)) {
        os << indent << (sd->isFunction ? "Function: " : "Procedure: ") << sd->name;
        os << " [lev=" << sd->lev << "]\n";
        if (!sd->params.empty()) {
            os << indent << "  Params:\n";
            for (auto* p : sd->params) printDecoratedAST(os, p, depth + 2);
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
        os << indent << "UnaryOp: " << unary->op << "\n";
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
        os << " lval=" << (var->isLValue ? "true" : "false") << "\n";
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
