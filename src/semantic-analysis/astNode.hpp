#ifndef ASTNODE_HPP
#define ASTNODE_HPP

#include <string>
#include <vector>

#define TYPE_VOID      0
#define TYPE_INTEGER   1
#define TYPE_REAL      2
#define TYPE_BOOLEAN   3
#define TYPE_CHAR      4
#define TYPE_ARRAY     5
#define TYPE_RECORD    6
#define TYPE_STRING    7
#define TYPE_ENUM      8
#define TYPE_SUBRANGE  9
#define TYPE_ERROR     -1

#define OBJ_CONSTANT   0
#define OBJ_VARIABLE   1
#define OBJ_TYPE       2
#define OBJ_PROCEDURE  3
#define OBJ_FUNCTION   4
#define OBJ_PROGRAM    5
#define OBJ_PARAM      OBJ_VARIABLE

#define PARAM_VALUE    1
#define PARAM_REF      0

// Type kind for TypeNode
enum class TypeKind {
    SIMPLE,
    ARRAY,
    RANGE,
    ENUMERATED,
    RECORD
};

struct ASTNode {
    int type;         // Type code (TYPE_INTEGER, TYPE_REAL, etc.)
    int tabIndex;     // Index in symbol table
    int lev;          // Lexical level
    bool isLValue;    // Can be assigned to?
    int line;         // Source line, 0 if unknown
    int column;       // Source column, 0 if unknown

    ASTNode() : type(TYPE_VOID), tabIndex(-1), lev(0), isLValue(false), line(0), column(0) {}
    virtual ~ASTNode() = default;
};

struct DeclNode : ASTNode {
    virtual ~DeclNode() = default;
};

struct VarDeclNode : DeclNode {
    std::string name;
    struct TypeNode* varType;  // Pointer to type node

    VarDeclNode(const std::string& name) : name(name), varType(nullptr) {}
    ~VarDeclNode() override;
};

struct ConstDeclNode : DeclNode {
    std::string name;
    ASTNode* value;

    ConstDeclNode(const std::string& name) : name(name), value(nullptr) {}
    ~ConstDeclNode() override;
};

struct TypeDeclNode : DeclNode {
    std::string name;
    struct TypeNode* typeDef;

    TypeDeclNode(const std::string& name) : name(name), typeDef(nullptr) {}
    ~TypeDeclNode() override;
};

struct DeclarationsNode : ASTNode {
    std::vector<DeclNode*> decls;

    DeclarationsNode() = default;
    ~DeclarationsNode() override;
};

struct TypeNode : ASTNode {
    TypeKind kind;
    int ref;
    int size;

    TypeNode(TypeKind k) : kind(k), ref(-1), size(0) {}
    virtual ~TypeNode() = default;
};

struct SimpleTypeNode : TypeNode {
    std::string name;

    SimpleTypeNode(const std::string& name)
        : TypeNode(TypeKind::SIMPLE), name(name) {}
};

struct RangeTypeNode : TypeNode {
    ASTNode* low;
    ASTNode* high;

    RangeTypeNode() : TypeNode(TypeKind::RANGE), low(nullptr), high(nullptr) {}
    ~RangeTypeNode() override;
};

struct EnumeratedTypeNode : TypeNode {
    std::vector<std::string> values;

    EnumeratedTypeNode() : TypeNode(TypeKind::ENUMERATED) {}
};

struct FieldDeclNode : ASTNode {
    std::string name;
    TypeNode* fieldType;

    FieldDeclNode(const std::string& name) : name(name), fieldType(nullptr) {}
    ~FieldDeclNode() override;
};

struct RecordTypeNode : TypeNode {
    std::vector<FieldDeclNode*> fields;
    int btabIndex;  // Index in block table

    RecordTypeNode() : TypeNode(TypeKind::RECORD), btabIndex(-1) {}
    ~RecordTypeNode() override;
};

struct ArrayTypeNode : TypeNode {
    TypeNode* indexType;
    TypeNode* elementType;
    int atabIndex;   // Index in array table

    ArrayTypeNode() : TypeNode(TypeKind::ARRAY), indexType(nullptr), elementType(nullptr), atabIndex(-1) {}
    ~ArrayTypeNode() override;
};

struct SubprogramDeclNode : DeclNode {
    std::string name;
    bool isFunction;
    std::vector<VarDeclNode*> params;
    TypeNode* returnType;
    DeclarationsNode* localDecls;
    ASTNode* body;
    int tabIndex;

    SubprogramDeclNode(const std::string& name, bool isFunc = false)
        : name(name), isFunction(isFunc), returnType(nullptr),
          localDecls(nullptr), body(nullptr), tabIndex(-1) {}
    ~SubprogramDeclNode() override;
};

struct AssignNode : ASTNode {
    struct VarNode* target;
    ASTNode* value;

    AssignNode() : target(nullptr), value(nullptr) {}
    ~AssignNode() override;
};

struct BinOpNode : ASTNode {
    std::string op;    // "+", "-", "*", "/", "div", "mod", "and", "or", "=", "<>", "<", "<=", ">", ">="
    ASTNode* left;
    ASTNode* right;

    BinOpNode(const std::string& op) : op(op), left(nullptr), right(nullptr) {}
    ~BinOpNode() override;
};

struct UnaryOpNode : ASTNode {
    std::string op;    // "+", "-", "not"
    ASTNode* operand;

    UnaryOpNode(const std::string& op) : op(op), operand(nullptr) {}
    ~UnaryOpNode() override;
};

struct NumberNode : ASTNode {
    int value;

    NumberNode(int v) : value(v) { type = TYPE_INTEGER; isLValue = false; }
};

struct RealNode : ASTNode {
    double value;

    RealNode(double v) : value(v) { type = TYPE_REAL; isLValue = false; }
};

struct CharNode : ASTNode {
    char value;

    CharNode(char v) : value(v) { type = TYPE_CHAR; isLValue = false; }
};

struct StringNode : ASTNode {
    std::string value;

    StringNode(const std::string& v) : value(v) { type = TYPE_STRING; isLValue = false; }
};

struct BoolNode : ASTNode {
    bool value;

    BoolNode(bool v) : value(v) { type = TYPE_BOOLEAN; isLValue = false; }
};

struct VarComponentNode {
    enum Kind {
        ARRAY_ACCESS,
        FIELD_ACCESS
    };

    Kind kind;
    std::vector<ASTNode*> indices;
    std::string fieldName;

    VarComponentNode(Kind k) : kind(k) {}
    ~VarComponentNode();
};

struct VarNode : ASTNode {
    std::string name;
    bool isArrayAccess;
    ASTNode* index;         // Array index expression (if isArrayAccess)
    std::string fieldName;  // Record field name (if accessing record field)
    VarNode* base;          // Base variable for chained access (e.g., a.b.c)
    std::vector<VarComponentNode*> components;

    VarNode(const std::string& name)
        : name(name), isArrayAccess(false), index(nullptr), base(nullptr) {}
    ~VarNode() override;
};

struct EmptyStmtNode : ASTNode {
    EmptyStmtNode() { type = TYPE_VOID; }
};

struct ProcCallNode : ASTNode {
    std::string name;
    std::vector<ASTNode*> args;
    int tabIndex;

    ProcCallNode(const std::string& name) : name(name), tabIndex(-1) {}
    ~ProcCallNode() override;
};

struct BlockNode : ASTNode {
    std::vector<ASTNode*> statements;

    BlockNode() = default;
    ~BlockNode() override;
};

struct IfNode : ASTNode {
    ASTNode* condition;
    ASTNode* thenBranch;
    ASTNode* elseBranch;

    IfNode() : condition(nullptr), thenBranch(nullptr), elseBranch(nullptr) {}
    ~IfNode() override;
};

struct WhileNode : ASTNode {
    ASTNode* condition;
    ASTNode* body;

    WhileNode() : condition(nullptr), body(nullptr) {}
    ~WhileNode() override;
};

struct RepeatNode : ASTNode {
    ASTNode* body;
    ASTNode* condition;

    RepeatNode() : body(nullptr), condition(nullptr) {}
    ~RepeatNode() override;
};

struct ForNode : ASTNode {
    std::string varName;
    ASTNode* start;
    ASTNode* end;
    bool isDownto;
    ASTNode* body;
    int varTabIndex;

    ForNode(const std::string& varName)
        : varName(varName), start(nullptr), end(nullptr), isDownto(false), body(nullptr), varTabIndex(-1) {}
    ~ForNode() override;
};

struct CaseBranchNode : ASTNode {
    std::vector<ASTNode*> constants;
    ASTNode* statement;

    CaseBranchNode() : statement(nullptr) {}
    ~CaseBranchNode() override;
};

struct CaseNode : ASTNode {
    ASTNode* expression;
    std::vector<CaseBranchNode*> branches;

    CaseNode() : expression(nullptr) {}
    ~CaseNode() override;
};

struct ProgramNode : ASTNode {
    std::string name;
    DeclarationsNode* declarations;
    ASTNode* body;  // Compound statement

    ProgramNode(const std::string& name)
        : name(name), declarations(nullptr), body(nullptr) {}
    ~ProgramNode() override;
};

inline DeclarationsNode::~DeclarationsNode() {
    for (auto* d : decls) delete d;
}

inline VarDeclNode::~VarDeclNode() {
    delete varType;
}

inline ConstDeclNode::~ConstDeclNode() {
    delete value;
}

inline TypeDeclNode::~TypeDeclNode() {
    delete typeDef;
}

inline RangeTypeNode::~RangeTypeNode() {
    delete low;
    delete high;
}

inline FieldDeclNode::~FieldDeclNode() {
    delete fieldType;
}

inline RecordTypeNode::~RecordTypeNode() {
    for (auto* f : fields) delete f;
}

inline ArrayTypeNode::~ArrayTypeNode() {
    delete indexType;
    delete elementType;
}

inline SubprogramDeclNode::~SubprogramDeclNode() {
    for (auto* p : params) delete p;
    delete returnType;
    delete localDecls;
    delete body;
}

inline BlockNode::~BlockNode() {
    for (auto* s : statements) delete s;
}

inline AssignNode::~AssignNode() {
    delete target;
    delete value;
}

inline BinOpNode::~BinOpNode() {
    delete left;
    delete right;
}

inline UnaryOpNode::~UnaryOpNode() {
    delete operand;
}

inline VarNode::~VarNode() {
    delete index;
    delete base;
    for (auto* c : components) delete c;
}

inline VarComponentNode::~VarComponentNode() {
    for (auto* i : indices) delete i;
}

inline ProcCallNode::~ProcCallNode() {
    for (auto* a : args) delete a;
}

inline IfNode::~IfNode() {
    delete condition;
    delete thenBranch;
    delete elseBranch;
}

inline WhileNode::~WhileNode() {
    delete condition;
    delete body;
}

inline RepeatNode::~RepeatNode() {
    delete body;
    delete condition;
}

inline ForNode::~ForNode() {
    delete start;
    delete end;
    delete body;
}

inline CaseBranchNode::~CaseBranchNode() {
    for (auto* c : constants) delete c;
    delete statement;
}

inline CaseNode::~CaseNode() {
    delete expression;
    for (auto* b : branches) delete b;
}

inline ProgramNode::~ProgramNode() {
    delete declarations;
    delete body;
}

#endif // ASTNODE_HPP
