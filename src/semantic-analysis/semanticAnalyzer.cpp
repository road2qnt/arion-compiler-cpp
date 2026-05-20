#include "semanticAnalyzer.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

static std::string toLowerCopy(const std::string& s) {
    std::string r;
    r.reserve(s.size());
    for (char c : s) r += (char)std::tolower((unsigned char)c);
    return r;
}

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
    const ParseNode* inner = &node;
    if (node.label == "<subprogram-declaration>") {
        for (const auto& child : node.children) {
            if (child.label == "<procedure-declaration>" || child.label == "<function-declaration>") {
                inner = &child;
                break;
            }
        }
    }

    std::string name = "subprogram";
    bool isFunction = (inner->label == "<function-declaration>");

    for (size_t i = 1; i < inner->children.size(); i++) {
        if (inner->children[i].isTerminal && inner->children[i].label.rfind("ident", 0) == 0) {
            name = getTokenValue(inner->children[i]);
            break;
        }
    }

    SubprogramDeclNode* sub = new SubprogramDeclNode(name, isFunction);

    auto extractParamGroup = [&](const ParseNode& paramChild) {
        std::vector<std::string> pnames;
        TypeNode* pType = nullptr;
        bool seenColon = false;
        for (const auto& pc : paramChild.children) {
            if (pc.isTerminal && pc.label == "colon") { seenColon = true; continue; }
            if (!seenColon) {
                if (pc.label == "<identifier-list>") {
                    for (const auto& il : pc.children) {
                        if (il.isTerminal && il.label.rfind("ident", 0) == 0) {
                            pnames.push_back(getTokenValue(il));
                        }
                    }
                } else if (pc.isTerminal && pc.label.rfind("ident", 0) == 0) {
                    pnames.push_back(getTokenValue(pc));
                }
            } else {
                if (pc.label == "<type>") {
                    pType = convertType(pc);
                } else if (pc.isTerminal && pc.label.rfind("ident", 0) == 0 && !pType) {
                    pType = new SimpleTypeNode(getTokenValue(pc));
                }
            }
        }
        for (const auto& pn : pnames) {
            VarDeclNode* param = new VarDeclNode(pn);
            param->varType = pType;
            sub->params.push_back(param);
        }
    };

    auto walkParts = [&](const ParseNode& container) {
        for (const auto& child : container.children) {
            if (child.label == "<compound-statement>") {
                sub->body = convertCompoundStatement(child);
            } else if (child.label == "<Block>" || child.label == "<block>") {
                for (const auto& blockChild : child.children) {
                    if (blockChild.label == "<compound-statement>") {
                        sub->body = convertCompoundStatement(blockChild);
                    } else if (blockChild.label == "<declaration-part>") {
                        sub->localDecls = convertDeclarationPart(blockChild);
                    }
                }
            } else if (child.label == "<declaration-part>") {
                sub->localDecls = convertDeclarationPart(child);
            } else if (child.label == "<formal-parameter-list>") {
                for (const auto& paramChild : child.children) {
                    if (paramChild.label == "<parameter-group>") {
                        extractParamGroup(paramChild);
                    }
                }
            } else if (child.label == "<type>" && isFunction) {
                sub->returnType = convertType(child);
            } else if (child.isTerminal && child.label.rfind("ident", 0) == 0 && isFunction) {
                if (!sub->returnType) {
                    bool seenColon = false;
                    for (size_t k = 0; k < container.children.size(); k++) {
                        if (&container.children[k] == &child) break;
                        if (container.children[k].isTerminal && container.children[k].label == "colon") {
                            seenColon = true;
                            break;
                        }
                    }
                    if (seenColon) {
                        sub->returnType = new SimpleTypeNode(getTokenValue(child));
                    }
                }
            }
        }
    };

    walkParts(*inner);

    return sub;
}

TypeNode* SemanticAnalyzer::convertType(const ParseNode& node) {
    const ParseNode* target = &node;
    if (node.label == "<type>" && !node.children.empty()) {
        target = &node.children[0];
    }

    if (target->isTerminal && target->label.rfind("ident", 0) == 0) {
        return new SimpleTypeNode(getTokenValue(*target));
    }

    if (target->label == "<array-type>") {
        ArrayTypeNode* arr = new ArrayTypeNode();
        bool sawOf = false;
        for (size_t i = 0; i < target->children.size(); i++) {
            const ParseNode& ch = target->children[i];
            if (ch.isTerminal && ch.label == "ofsy") { sawOf = true; continue; }
            if (!sawOf) {
                if (ch.label == "<range>") {
                    arr->indexType = convertType(ch);
                } else if (ch.isTerminal && ch.label.rfind("ident", 0) == 0) {
                    arr->indexType = new SimpleTypeNode(getTokenValue(ch));
                }
            } else {
                if (ch.label == "<type>") {
                    arr->elementType = convertType(ch);
                } else if (ch.isTerminal && ch.label.rfind("ident", 0) == 0) {
                    arr->elementType = new SimpleTypeNode(getTokenValue(ch));
                }
            }
        }
        return arr;
    }

    if (target->label == "<range>") {
        RangeTypeNode* range = new RangeTypeNode();
        for (const auto& child : target->children) {
            if (child.label == "<constant>") {
                if (!range->low) range->low = convertConstant(child);
                else if (!range->high) range->high = convertConstant(child);
            } else if (child.isTerminal &&
                       (child.label.rfind("intcon", 0) == 0 ||
                        child.label.rfind("realcon", 0) == 0 ||
                        child.label.rfind("charcon", 0) == 0 ||
                        child.label.rfind("ident", 0) == 0)) {
                ASTNode* c = nullptr;
                if (child.label.rfind("intcon", 0) == 0) {
                    c = new NumberNode(std::stoi(getTokenValue(child)));
                } else if (child.label.rfind("realcon", 0) == 0) {
                    c = new RealNode(std::stod(getTokenValue(child)));
                } else if (child.label.rfind("charcon", 0) == 0) {
                    std::string v = getTokenValue(child);
                    if (!v.empty()) c = new CharNode(v[0]);
                } else if (child.label.rfind("ident", 0) == 0) {
                    c = new VarNode(getTokenValue(child));
                }
                if (c) {
                    if (!range->low) range->low = c;
                    else if (!range->high) range->high = c;
                    else delete c;
                }
            }
        }
        return range;
    }

    if (target->label == "<enumerated>") {
        EnumeratedTypeNode* en = new EnumeratedTypeNode();
        for (const auto& child : target->children) {
            if (child.isTerminal && child.label.rfind("ident", 0) == 0) {
                en->values.push_back(getTokenValue(child));
            } else if (child.label == "<identifier-list>") {
                for (const auto& il : child.children) {
                    if (il.isTerminal && il.label.rfind("ident", 0) == 0) {
                        en->values.push_back(getTokenValue(il));
                    }
                }
            }
        }
        return en;
    }

    if (target->label == "<record-type>") {
        RecordTypeNode* rec = new RecordTypeNode();
        for (const auto& child : target->children) {
            if (child.label == "<field-list>") {
                for (const auto& fieldChild : child.children) {
                    if (fieldChild.label == "<field-part>") {
                        std::vector<std::string> fieldNames;
                        TypeNode* ftype = nullptr;
                        bool sawColon = false;
                        for (const auto& fp : fieldChild.children) {
                            if (fp.isTerminal && fp.label == "colon") { sawColon = true; continue; }
                            if (!sawColon) {
                                if (fp.label == "<identifier-list>") {
                                    for (const auto& il : fp.children) {
                                        if (il.isTerminal && il.label.rfind("ident", 0) == 0) {
                                            fieldNames.push_back(getTokenValue(il));
                                        }
                                    }
                                } else if (fp.isTerminal && fp.label.rfind("ident", 0) == 0) {
                                    fieldNames.push_back(getTokenValue(fp));
                                }
                            } else {
                                if (fp.label == "<type>") {
                                    ftype = convertType(fp);
                                } else if (fp.isTerminal && fp.label.rfind("ident", 0) == 0 && !ftype) {
                                    ftype = new SimpleTypeNode(getTokenValue(fp));
                                }
                            }
                        }
                        for (const auto& fname : fieldNames) {
                            FieldDeclNode* fd = new FieldDeclNode(fname);
                            fd->fieldType = ftype;
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
        } else if (child.label == "<compound-statement>") {
            wNode->body = convertCompoundStatement(child);
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
        } else if (node.children[i].label == "<compound-statement>") {
            fNode->body = convertCompoundStatement(node.children[i]);
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

    if (node->declarations) visit(node->declarations);

    symTab.pushScope();
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
        typeChecker.reportError("duplicate identifier: " + node->name);
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
        typeChecker.reportError("duplicate identifier: " + node->name);
        return;
    }

    int constType = TYPE_INTEGER;
    int constValue = 0;

    if (node->value) {
        if (auto* num = dynamic_cast<NumberNode*>(node->value)) {
            constType = TYPE_INTEGER;
            constValue = num->value;
            num->type = TYPE_INTEGER;
        } else if (auto* real = dynamic_cast<RealNode*>(node->value)) {
            constType = TYPE_REAL;
            constValue = (int)real->value;
            real->type = TYPE_REAL;
        } else if (auto* ch = dynamic_cast<CharNode*>(node->value)) {
            constType = TYPE_CHAR;
            constValue = (int)ch->value;
            ch->type = TYPE_CHAR;
        } else if (auto* str = dynamic_cast<StringNode*>(node->value)) {
            constType = TYPE_STRING;
            constValue = 0;
            str->type = TYPE_STRING;
        } else if (auto* b = dynamic_cast<BoolNode*>(node->value)) {
            constType = TYPE_BOOLEAN;
            constValue = b->value ? 1 : 0;
            b->type = TYPE_BOOLEAN;
        } else if (auto* var = dynamic_cast<VarNode*>(node->value)) {
            int idx = symTab.lookup(var->name);
            if (idx == -1) {
                typeChecker.reportError("undeclared identifier: " + var->name);
            } else {
                const TabEntry& e = symTab.getTab(idx);
                if (e.obj != OBJ_CONSTANT) {
                    typeChecker.reportError("constant initializer must be a constant: " + var->name);
                } else {
                    constType = e.type;
                    constValue = e.adr;
                }
                var->tabIndex = idx;
                var->type = e.type;
                var->lev = e.lev;
            }
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
        typeChecker.reportError("duplicate identifier: " + node->name);
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
        typeChecker.reportError("duplicate identifier: " + node->name);
        return;
    }

    int returnType = TYPE_VOID;
    int returnRef = 0;
    if (node->isFunction && node->returnType) {
        ResolvedType rrt = resolveTypeNode(node->returnType);
        returnType = rrt.typeCode;
        returnRef = rrt.ref;
    }

    int parentBtab = symTab.getCurrentBtabIndex();
    int parentLev = symTab.getCurrentLevel();

    TabEntry subEntry(node->name,
                      node->isFunction ? OBJ_FUNCTION : OBJ_PROCEDURE,
                      returnType, parentLev, 0);
    subEntry.ref = returnRef;
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
        ResolvedType pr;
        if (param->varType) {
            pr = resolveTypeNode(param->varType);
        } else {
            pr = ResolvedType(TYPE_INTEGER, 0, INT_SIZE);
        }

        TabEntry pe(param->name, OBJ_VARIABLE, pr.typeCode, subLev, paramAdr);
        pe.ref = pr.ref;
        pe.nrm = PARAM_VALUE;
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

    if (node->localDecls) visit(node->localDecls);

    if (node->isFunction) {
        int rfAdr = symTab.getBtab(subBtab).psze + symTab.getBtab(subBtab).vsze;
        TabEntry rfe(node->name, OBJ_VARIABLE, returnType, subLev, rfAdr);
        rfe.ref = returnRef;
        rfe.nrm = PARAM_VALUE;
        rfe.link = symTab.getBtab(subBtab).last;
        int rfIdx = symTab.addToTab(rfe);
        symTab.getBtab(subBtab).last = rfIdx;
    }

    if (node->body) visit(node->body);

    symTab.popScope();
}

void SemanticAnalyzer::visitAssign(AssignNode* node) {
    if (node->target) visit(node->target);
    if (node->value)  visit(node->value);

    if (node->target && !node->target->isLValue && node->target->type != TYPE_ERROR) {
        typeChecker.reportError("cannot assign to non-variable: " + node->target->name);
    }

    if (node->target && node->value) {
        if (node->target->type != TYPE_ERROR && node->value->type != TYPE_ERROR) {
            if (!typeChecker.isAssignmentCompatible(node->target->type, node->value->type)) {
                typeChecker.reportError("incompatible types");
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
    if (node->end)   visit(node->end);

    int loopVarIdx = symTab.lookup(node->varName);
    int loopVarType = TYPE_ERROR;
    if (loopVarIdx == -1) {
        typeChecker.reportError("undeclared loop variable: " + node->varName);
    } else {
        const TabEntry& e = symTab.getTab(loopVarIdx);
        loopVarType = e.type;
        if (e.obj != OBJ_VARIABLE) {
            typeChecker.reportError("for loop variable must be a variable: " + node->varName);
        } else if (!typeChecker.isOrdinal(e.type)) {
            typeChecker.reportError("for loop variable must be ordinal: " + node->varName);
        }
        node->varTabIndex = loopVarIdx;
        node->type = TYPE_VOID;
    }

    if (node->body) visit(node->body);

    if (node->start && !typeChecker.isOrdinal(node->start->type)) {
        typeChecker.reportError("for loop starting value must be an ordinal type");
    }
    if (node->end && !typeChecker.isOrdinal(node->end->type)) {
        typeChecker.reportError("for loop ending value must be an ordinal type");
    }
    if (loopVarIdx != -1 && node->start && node->start->type != TYPE_ERROR) {
        if (!typeChecker.isAssignmentCompatible(loopVarType, node->start->type)) {
            typeChecker.reportError("for loop start value incompatible with loop variable");
        }
    }
    if (loopVarIdx != -1 && node->end && node->end->type != TYPE_ERROR) {
        if (!typeChecker.isAssignmentCompatible(loopVarType, node->end->type)) {
            typeChecker.reportError("for loop end value incompatible with loop variable");
        }
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
        node->type = TYPE_ERROR;
        return;
    }

    const TabEntry& e = symTab.getTab(tabIdx);
    if (e.obj != OBJ_PROCEDURE && e.obj != OBJ_FUNCTION) {
        typeChecker.reportError("'" + node->name + "' is not a procedure or function");
        node->type = TYPE_ERROR;
        return;
    }

    node->tabIndex = tabIdx;
    node->type = e.type;

    for (auto* arg : node->args) {
        if (arg) visit(arg);
    }

    bool variadic = (e.lev == 0 && (node->name == "writeln" || node->name == "readln" ||
                                    node->name == "write"   || node->name == "read"));
    if (variadic) return;

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
                                std::to_string(node->args.size()));
        return;
    }
    for (size_t i = 0; i < params.size(); i++) {
        const TabEntry& p = symTab.getTab(params[i]);
        if (!node->args[i]) continue;
        int argType = node->args[i]->type;
        if (argType == TYPE_ERROR) continue;
        if (!typeChecker.isAssignmentCompatible(p.type, argType)) {
            typeChecker.reportError("call to '" + node->name + "': argument " +
                                    std::to_string(i + 1) + " type mismatch");
        }
        if (p.nrm == PARAM_REF && !node->args[i]->isLValue) {
            typeChecker.reportError("call to '" + node->name + "': argument " +
                                    std::to_string(i + 1) + " must be a variable (var parameter)");
        }
    }
}

void SemanticAnalyzer::visitVar(VarNode* node) {
    int tabIdx = symTab.lookup(node->name);
    if (tabIdx == -1) {
        typeChecker.reportError("undeclared identifier: " + node->name);
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

    if (node->isArrayAccess && node->index) {
        visit(node->index);
        if (e.type == TYPE_ARRAY && e.ref >= 0 && e.ref < symTab.getAtabSize()) {
            const AtabEntry& a = symTab.getAtab(e.ref);
            if (node->index->type != TYPE_ERROR &&
                !typeChecker.isCompatible(a.xtyp, node->index->type)) {
                typeChecker.reportError("array '" + node->name + "' index type mismatch");
            }
            node->type = a.etyp;
            node->isLValue = true;
        } else {
            typeChecker.reportError("'" + node->name + "' is not an array");
            node->type = TYPE_ERROR;
        }
    }

    if (!node->fieldName.empty()) {
        if (e.type == TYPE_RECORD && e.ref >= 0 && e.ref < symTab.getBtabSize()) {
            int last = symTab.getBtab(e.ref).last;
            int found = -1;
            while (last > 0 && last < symTab.getTabSize()) {
                if (symTab.getTab(last).id == node->fieldName) { found = last; break; }
                last = symTab.getTab(last).link;
            }
            if (found == -1) {
                typeChecker.reportError("record '" + node->name + "' has no field '" + node->fieldName + "'");
                node->type = TYPE_ERROR;
            } else {
                node->type = symTab.getTab(found).type;
                node->isLValue = true;
            }
        } else {
            typeChecker.reportError("'" + node->name + "' is not a record");
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
                typeChecker.reportError("undeclared type identifier: " + simple->name);
                r = ResolvedType(TYPE_ERROR, 0, 0);
            } else {
                const TabEntry& e = symTab.getTab(idx);
                if (e.obj != OBJ_TYPE) {
                    typeChecker.reportError("'" + simple->name + "' is not a type");
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
        typeChecker.reportError("subrange bounds must not be Real");
        r.typeCode = TYPE_ERROR;
    }
    if (loType != TYPE_ERROR && hiType != TYPE_ERROR && loType != hiType) {
        typeChecker.reportError("subrange bounds must have the same type");
    }
    if (loVal > hiVal) {
        typeChecker.reportError("subrange lower bound must be <= upper bound");
    }
    if (rng->low)  rng->low->type  = (loType == TYPE_ERROR) ? TYPE_INTEGER : loType;
    if (rng->high) rng->high->type = (hiType == TYPE_ERROR) ? TYPE_INTEGER : hiType;
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
                typeChecker.reportError("array index type cannot be Real");
                xtyp = TYPE_INTEGER;
            }
            if (low > high) {
                typeChecker.reportError("array range lower must be <= upper");
            }
            rng->type = TYPE_SUBRANGE;
            rng->size = 1;
            arr->indexType->type = TYPE_SUBRANGE;
        } else if (auto* simple = dynamic_cast<SimpleTypeNode*>(arr->indexType)) {
            ResolvedType irt = resolveTypeNode(simple);
            xtyp = irt.typeCode;
            if (xtyp == TYPE_REAL) {
                typeChecker.reportError("array index type cannot be Real");
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
    ae.eref = (etyp == TYPE_ARRAY || etyp == TYPE_RECORD) ? eref : 0;
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

    for (auto* field : rec->fields) {
        if (!field || !field->fieldType) continue;
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
            typeChecker.reportError("duplicate enumerated identifier: " + nm);
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
            case TYPE_ARRAY:   typeStr = "array"; break;
            case TYPE_RECORD:  typeStr = "record"; break;
            case TYPE_ENUM:    typeStr = "enum"; break;
            case TYPE_SUBRANGE:typeStr = "subrange"; break;
            case TYPE_VOID:    typeStr = "void"; break;
            default:           typeStr = std::to_string(vd->type); break;
        }
        os << " type=" << typeStr << "\n";
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
        if (cd->value) {
            if (auto* num = dynamic_cast<NumberNode*>(cd->value)) os << " value=" << num->value;
            else if (auto* re = dynamic_cast<RealNode*>(cd->value)) os << " value=" << re->value;
            else if (auto* ch = dynamic_cast<CharNode*>(cd->value)) os << " value='" << ch->value << "'";
            else if (auto* st = dynamic_cast<StringNode*>(cd->value)) os << " value=\"" << st->value << "\"";
            else if (auto* b = dynamic_cast<BoolNode*>(cd->value)) os << " value=" << (b->value ? "true" : "false");
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
        os << " type=" << typeStr << "\n";
    } else if (auto* sd = dynamic_cast<SubprogramDeclNode*>(node)) {
        os << indent << (sd->isFunction ? "Function: " : "Procedure: ") << sd->name;
        if (sd->tabIndex >= 0) os << " [tab=" << sd->tabIndex << "]";
        os << " lev=" << sd->lev;
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
