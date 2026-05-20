#include "semanticAnalyzer.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <functional>

// === Fase 1: Convert Parse Tree -> AST ===

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

static std::string tokenValueOf(const ParseNode& node) {
    if (!node.isTerminal) return "";
    size_t start = node.label.find('(');
    size_t end = node.label.rfind(')');
    if (start != std::string::npos && end != std::string::npos && end > start) {
        return node.label.substr(start + 1, end - start - 1);
    }
    return node.label;
}

static std::string lowerCopy(const std::string& s) {
    std::string r;
    r.reserve(s.size());
    for (char c : s) r += (char)std::tolower((unsigned char)c);
    return r;
}

static std::string unquoteLiteral(const std::string& value) {
    if (value.size() >= 2 && value.front() == '\'' && value.back() == '\'') {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

static void setSource(ASTNode* node, int line, int column = 0) {
    if (!node) return;
    if (column == 0 && line > 9999) {
        node->line = line / 10000;
        node->column = line % 10000;
    } else {
        node->line = line;
        node->column = column;
    }
}

static TypeNode* cloneTypeNode(const TypeNode* node) {
    if (!node) return nullptr;

    TypeNode* copy = nullptr;
    if (auto* simple = dynamic_cast<const SimpleTypeNode*>(node)) {
        copy = new SimpleTypeNode(simple->name);
    } else if (auto* range = dynamic_cast<const RangeTypeNode*>(node)) {
        auto* r = new RangeTypeNode();
        // Type ranges are built from constants; clone the common constant node shapes.
        auto cloneConst = [](ASTNode* c) -> ASTNode* {
            if (!c) return nullptr;
            if (auto* n = dynamic_cast<NumberNode*>(c)) return new NumberNode(n->value);
            if (auto* re = dynamic_cast<RealNode*>(c)) return new RealNode(re->value);
            if (auto* ch = dynamic_cast<CharNode*>(c)) return new CharNode(ch->value);
            if (auto* st = dynamic_cast<StringNode*>(c)) return new StringNode(st->value);
            if (auto* b = dynamic_cast<BoolNode*>(c)) return new BoolNode(b->value);
            if (auto* v = dynamic_cast<VarNode*>(c)) return new VarNode(v->name);
            if (auto* u = dynamic_cast<UnaryOpNode*>(c)) {
                auto* nu = new UnaryOpNode(u->op);
                if (auto* n = dynamic_cast<NumberNode*>(u->operand)) nu->operand = new NumberNode(n->value);
                else if (auto* re = dynamic_cast<RealNode*>(u->operand)) nu->operand = new RealNode(re->value);
                else if (auto* v = dynamic_cast<VarNode*>(u->operand)) nu->operand = new VarNode(v->name);
                return nu;
            }
            return nullptr;
        };
        r->low = cloneConst(range->low);
        r->high = cloneConst(range->high);
        copy = r;
    } else if (auto* en = dynamic_cast<const EnumeratedTypeNode*>(node)) {
        auto* e = new EnumeratedTypeNode();
        e->values = en->values;
        copy = e;
    } else if (auto* arr = dynamic_cast<const ArrayTypeNode*>(node)) {
        auto* a = new ArrayTypeNode();
        a->indexType = cloneTypeNode(arr->indexType);
        a->elementType = cloneTypeNode(arr->elementType);
        copy = a;
    } else if (auto* rec = dynamic_cast<const RecordTypeNode*>(node)) {
        auto* r = new RecordTypeNode();
        for (auto* field : rec->fields) {
            if (!field) continue;
            auto* nf = new FieldDeclNode(field->name);
            nf->fieldType = cloneTypeNode(field->fieldType);
            r->fields.push_back(nf);
        }
        copy = r;
    }

    if (copy) {
        copy->type = node->type;
        copy->tabIndex = node->tabIndex;
        copy->lev = node->lev;
        copy->isLValue = node->isLValue;
        copy->line = node->line;
        copy->column = node->column;
        copy->ref = node->ref;
        copy->size = node->size;
    }
    return copy;
}

static ASTNode* terminalToScalarNode(const ParseNode& node) {
    if (!node.isTerminal) return nullptr;

    if (node.label.rfind("intcon", 0) == 0) {
        return new NumberNode(std::stoi(tokenValueOf(node)));
    }
    if (node.label.rfind("realcon", 0) == 0) {
        return new RealNode(std::stod(tokenValueOf(node)));
    }
    if (node.label.rfind("charcon", 0) == 0) {
        std::string value = unquoteLiteral(tokenValueOf(node));
        return new CharNode(value.empty() ? '\0' : value[0]);
    }
    if (node.label.rfind("string", 0) == 0) {
        return new StringNode(unquoteLiteral(tokenValueOf(node)));
    }
    if (node.label.rfind("ident", 0) == 0) {
        std::string name = tokenValueOf(node);
        std::string lower = lowerCopy(name);
        if (lower == "true") return new BoolNode(true);
        if (lower == "false") return new BoolNode(false);
        return new VarNode(name);
    }
    return nullptr;
}

ProgramNode* SemanticAnalyzer::convertProgram(const ParseNode& parseTree) {
    std::string progName = "unnamed";

    for (const auto& child : parseTree.children) {
        if (child.label == "<Program-header>") {
            for (const auto& headerChild : child.children) {
                if (headerChild.label.find("ident(") == 0) {
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
    setSource(program, findLineForNode(parseTree));

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
    setSource(decls, findLineForNode(node));

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
    if (node.label == "<subprogram-declaration>" || 
        node.label == "<procedure-declaration>" || 
        node.label == "<function-declaration>") {
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
                    vd->varType = cloneTypeNode(varType);
                    setSource(vd, findLineForNode(node.children[i - 1]));
                    result.push_back(vd);
                }
                delete varType;
            }
        } else if (node.children[i].isTerminal &&
                   node.children[i].label.rfind("ident", 0) == 0) {
            std::string name = getTokenValue(node.children[i]);
            int line = findLineForNode(node.children[i]);
            i += 2;
            if (i < node.children.size()) {
                TypeNode* varType = convertType(node.children[i]);
                VarDeclNode* vd = new VarDeclNode(name);
                vd->varType = varType;
                setSource(vd, line);
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
            setSource(cd, findLineForNode(node.children[i]));
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
            setSource(td, findLineForNode(node.children[i]));
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
    setSource(sub, findLineForNode(*inner));

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
            param->varType = cloneTypeNode(pType);
            setSource(param, findLineForNode(paramChild));
            sub->params.push_back(param);
        }
        delete pType;
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
        auto* simple = new SimpleTypeNode(getTokenValue(*target));
        setSource(simple, findLineForNode(*target));
        return simple;
    }

    if (target->label == "<array-type>") {
        ArrayTypeNode* arr = new ArrayTypeNode();
        setSource(arr, findLineForNode(*target));
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
        setSource(range, findLineForNode(*target));
        for (const auto& child : target->children) {
            if (child.label == "<constant>") {
                if (!range->low) range->low = convertConstant(child);
                else if (!range->high) range->high = convertConstant(child);
            } else if (child.isTerminal &&
                       (child.label.rfind("intcon", 0) == 0 ||
                        child.label.rfind("realcon", 0) == 0 ||
                        child.label.rfind("charcon", 0) == 0 ||
                        child.label.rfind("ident", 0) == 0)) {
                ASTNode* c = terminalToScalarNode(child);
                setSource(c, findLineForNode(child));
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
        setSource(en, findLineForNode(*target));
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
        setSource(rec, findLineForNode(*target));
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
                            fd->fieldType = cloneTypeNode(ftype);
                            setSource(fd, findLineForNode(fieldChild));
                            rec->fields.push_back(fd);
                        }
                        delete ftype;
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
    auto* block = new BlockNode();
    setSource(block, findLineForNode(node));
    return block;
}

ASTNode* SemanticAnalyzer::convertStatementList(const ParseNode& node) {
    BlockNode* block = new BlockNode();
    setSource(block, findLineForNode(node));

    for (const auto& child : node.children) {
        if (child.label == "<statement>") {
            ASTNode* stmt = convertStatement(child);
            if (stmt) block->statements.push_back(stmt);
        }
    }

    return block;
}

ASTNode* SemanticAnalyzer::convertStatement(const ParseNode& node) {
    if (node.children.empty()) {
        auto* empty = new EmptyStmtNode();
        setSource(empty, findLineForNode(node));
        return empty;
    }

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
    setSource(assign, findLineForNode(node));

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
    setSource(ifNode, findLineForNode(node));

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
    setSource(wNode, findLineForNode(node));

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
    setSource(rNode, findLineForNode(node));

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
    setSource(fNode, findLineForNode(node));

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
    setSource(cNode, findLineForNode(node));

    std::function<void(const ParseNode&)> collectCaseBlock = [&](const ParseNode& caseBlock) {
        CaseBranchNode* branch = new CaseBranchNode();
        setSource(branch, findLineForNode(caseBlock));

        for (const auto& cb : caseBlock.children) {
            if (cb.label == "<constant>") {
                branch->constants.push_back(convertConstant(cb));
            } else if (cb.label == "<statement>") {
                branch->statement = convertStatement(cb);
            }
        }

        if (!branch->constants.empty() || branch->statement) {
            cNode->branches.push_back(branch);
        } else {
            delete branch;
        }

        for (const auto& cb : caseBlock.children) {
            if (cb.label == "<case-block>") {
                collectCaseBlock(cb);
            } else {
                for (const auto& nested : cb.children) {
                    if (nested.label == "<case-block>") {
                        collectCaseBlock(nested);
                    }
                }
            }
        }
    };

    for (size_t i = 0; i < node.children.size(); i++) {
        if (node.children[i].label == "<expression>") {
            cNode->expression = convertExpression(node.children[i]);
        } else if (node.children[i].label == "<case-block>") {
            collectCaseBlock(node.children[i]);
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
    setSource(call, findLineForNode(node));

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
        setSource(binOp, findLineForNode(node));
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
        setSource(unary, findLineForNode(node));
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
                setSource(binOp, findLineForNode(node.children[i]));
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
                setSource(binOp, findLineForNode(node.children[i]));
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
        ASTNode* scalar = terminalToScalarNode(inner);
        if (scalar) {
            setSource(scalar, findLineForNode(inner));
            return scalar;
        }
    }

    if (inner.label == "<variable>") {
        if (inner.children.size() == 1 && inner.children[0].isTerminal &&
            inner.children[0].label.rfind("ident", 0) == 0) {
            std::string name = getTokenValue(inner.children[0]);
            std::string lower = lowerCopy(name);
            if (lower == "true" || lower == "false") {
                auto* b = new BoolNode(lower == "true");
                setSource(b, findLineForNode(inner.children[0]));
                return b;
            }
        }
        return convertVariable(inner);
    }

    if (inner.label == "<procedure-function-call>") {
        return convertProcedureFunctionCall(inner);
    }

    if (inner.isTerminal && inner.label == "notsy") {
        UnaryOpNode* notNode = new UnaryOpNode(mapOpToken(inner.label));
        setSource(notNode, findLineForNode(inner));
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
        ASTNode* scalar = terminalToScalarNode(inner);
        setSource(scalar, findLineForNode(inner));
        return scalar;
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
    setSource(var, findLineForNode(node));

    for (const auto& child : node.children) {
        if (child.label != "<component-variable>") continue;

        for (const auto& comp : child.children) {
            if (comp.label == "<index-list>") {
                auto* part = new VarComponentNode(VarComponentNode::ARRAY_ACCESS);
                for (const auto& idxNode : comp.children) {
                    if (!idxNode.isTerminal || idxNode.label == "comma") continue;
                    ASTNode* idx = terminalToScalarNode(idxNode);
                    setSource(idx, findLineForNode(idxNode));
                    if (idx) part->indices.push_back(idx);
                }
                if (!part->indices.empty()) {
                    if (!var->isArrayAccess && part->indices.size() == 1) {
                        var->isArrayAccess = true;
                    }
                    var->components.push_back(part);
                } else {
                    delete part;
                }
            } else if (comp.isTerminal && comp.label.rfind("ident", 0) == 0) {
                auto* part = new VarComponentNode(VarComponentNode::FIELD_ACCESS);
                part->fieldName = getTokenValue(comp);
                if (var->fieldName.empty()) var->fieldName = part->fieldName;
                var->components.push_back(part);
            }
        }
    }

    return var;
}

ASTNode* SemanticAnalyzer::convertConstant(const ParseNode& node) {
    if (node.children.empty()) return nullptr;

    std::string sign;
    ASTNode* value = nullptr;

    for (const auto& child : node.children) {
        if (!child.isTerminal) continue;
        if (child.label == "plus" || child.label == "minus") {
            sign = child.label;
            continue;
        }
        value = terminalToScalarNode(child);
        if (value) {
            setSource(value, findLineForNode(child));
            break;
        }
    }

    if (!value) return nullptr;

    if (sign == "minus") {
        if (auto* num = dynamic_cast<NumberNode*>(value)) {
            num->value = -num->value;
        } else if (auto* real = dynamic_cast<RealNode*>(value)) {
            real->value = -real->value;
        } else {
            auto* unary = new UnaryOpNode("-");
            setSource(unary, findLineForNode(node));
            unary->operand = value;
            value = unary;
        }
    }

    return value;
}
