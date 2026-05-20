#include "symbolTable.hpp"
#include <iostream>
#include <iomanip>

SymbolTable::SymbolTable() {
        display.resize(MAX_DISPLAY, -1);
    initializePredefinedIdentifiers();
}

int SymbolTable::addToTab(const TabEntry& entry) {
    if (tab.size() >= MAX_TAB_SIZE) {
        std::cerr << "Semantic error: Symbol table overflow!" << std::endl;
        return tab.size() - 1;
    }
    int idx = tab.size();
    tab.push_back(entry);
    return idx;
}

int SymbolTable::addToBtab(const BtabEntry& entry) {
    if (btab.size() >= MAX_BTAB_SIZE) {
        std::cerr << "Semantic error: Block table overflow!" << std::endl;
        return btab.size() - 1;
    }
    int idx = btab.size();
    btab.push_back(entry);
    return idx;
}

int SymbolTable::addToAtab(const AtabEntry& entry) {
    if (atab.size() >= MAX_ATAB_SIZE) {
        std::cerr << "Semantic error: Array table overflow!" << std::endl;
        return atab.size() - 1;
    }
    int idx = atab.size();
    atab.push_back(entry);
    return idx;
}

int SymbolTable::getCurrentLevel() const {
    for (int i = MAX_DISPLAY - 1; i >= 0; --i) {
        if (display[i] != -1) return i;
    }
    return 0;
}

int SymbolTable::getCurrentBtabIndex() const {
    int level = getCurrentLevel();
    return display[level];
}

void SymbolTable::pushScope() {
    int newLevel = getCurrentLevel() + 1;
    BtabEntry newBlock;
    int btabIdx = btab.size();
    btab.push_back(newBlock);
    display[newLevel] = btabIdx;
}

void SymbolTable::popScope() {
    int level = getCurrentLevel();
    if (level < 0) return;
    display[level] = -1;
}

int SymbolTable::getBtabIndexForLevel(int level) const {
    if (level >= 0 && level < (int)display.size()) {
        return display[level];
    }
    return -1;
}

static std::string lower(const std::string& s) {
    std::string r = s;
    for (char& c : r) {
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
    }
    return r;
}

int SymbolTable::lookup(const std::string& name) const {
    int level = getCurrentLevel();
    std::string target = lower(name);

    for (int l = level; l >= 0; --l) {
        int btabIdx = display[l];
        if (btabIdx == -1) continue;

        int current = btab[btabIdx].last;
        while (current != -1) {
            if (lower(tab[current].id) == target) {
                return current;
            }
            current = tab[current].link;
        }
    }

    return -1;
}

int SymbolTable::lookupInCurrentScope(const std::string& name) const {
    int level = getCurrentLevel();
    int btabIdx = display[level];
    if (btabIdx == -1) return -1;

    std::string target = lower(name);
    int current = btab[btabIdx].last;
    while (current != -1) {
        if (lower(tab[current].id) == target) {
            return current;
        }
        current = tab[current].link;
    }

    return -1;
}

void SymbolTable::initializePredefinedIdentifiers() {
    BtabEntry globalBlock;
    globalBlock.lpar = 0;
    btab.push_back(globalBlock);
    display[0] = 0;

    auto addPredef = [&](const std::string& id, int obj, int type, int adr) {
        TabEntry e(id, obj, type, 0, adr);
        e.nrm = PARAM_VALUE;
        e.ref = 0;
        e.link = btab[0].last;
        btab[0].last = addToTab(e);
    };

    addPredef("integer", OBJ_TYPE, TYPE_INTEGER, INT_SIZE);
    addPredef("real",    OBJ_TYPE, TYPE_REAL,    REAL_SIZE);
    addPredef("boolean", OBJ_TYPE, TYPE_BOOLEAN, BOOL_SIZE);
    addPredef("char",    OBJ_TYPE, TYPE_CHAR,    CHAR_SIZE);
    addPredef("string",  OBJ_TYPE, TYPE_STRING,  STRING_SIZE);

    addPredef("true",   OBJ_CONSTANT, TYPE_BOOLEAN, 1);
    addPredef("false",  OBJ_CONSTANT, TYPE_BOOLEAN, 0);

    addPredef("writeln", OBJ_PROCEDURE, TYPE_VOID, 0);
    addPredef("readln",  OBJ_PROCEDURE, TYPE_VOID, 0);
    addPredef("write",   OBJ_PROCEDURE, TYPE_VOID, 0);
    addPredef("read",    OBJ_PROCEDURE, TYPE_VOID, 0);
}

static const char* objLabel(int obj) {
    switch (obj) {
        case OBJ_CONSTANT:  return "constant";
        case OBJ_VARIABLE:  return "variable";
        case OBJ_TYPE:      return "type";
        case OBJ_PROCEDURE: return "procedure";
        case OBJ_FUNCTION:  return "function";
        case OBJ_PROGRAM:   return "program";
        default:            return "?";
    }
}

static const char* typeLabel(int t) {
    switch (t) {
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
        default:            return "?";
    }
}

void SymbolTable::printSymbolTable(std::ostream& os) const {
    os << "\n=== Symbol Table (tab) ===\n";
    os << std::left
       << std::setw(4)  << "idx"
       << std::setw(14) << "id"
       << std::setw(11) << "obj"
       << std::setw(10) << "type"
       << std::setw(5)  << "ref"
       << std::setw(5)  << "nrm"
       << std::setw(5)  << "lev"
       << std::setw(7)  << "adr"
       << std::setw(5)  << "link" << "\n";
    os << std::string(66, '-') << "\n";

    for (size_t i = 0; i < tab.size(); ++i) {
        const auto& e = tab[i];
        int link = (e.link < 0) ? 0 : e.link;
        os << std::left
           << std::setw(4)  << i
           << std::setw(14) << e.id
           << std::setw(11) << objLabel(e.obj)
           << std::setw(10) << typeLabel(e.type)
           << std::setw(5)  << e.ref
           << std::setw(5)  << e.nrm
           << std::setw(5)  << e.lev
           << std::setw(7)  << e.adr
           << std::setw(5)  << link << "\n";
    }

    os << "\n=== Block Table (btab) ===\n";
    os << std::left
       << std::setw(5) << "idx"
       << std::setw(7) << "last"
       << std::setw(7) << "lpar"
       << std::setw(7) << "psze"
       << std::setw(7) << "vsze" << "\n";
    os << std::string(33, '-') << "\n";

    for (size_t i = 0; i < btab.size(); ++i) {
        const auto& b = btab[i];
        int last = (b.last < 0) ? 0 : b.last;
        int lpar = (b.lpar < 0) ? 0 : b.lpar;
        os << std::left
           << std::setw(5) << i
           << std::setw(7) << last
           << std::setw(7) << lpar
           << std::setw(7) << b.psze
           << std::setw(7) << b.vsze << "\n";
    }

    os << "\n=== Array Table (atab) ===\n";
    os << std::left
       << std::setw(5) << "idx"
       << std::setw(7) << "xtyp"
       << std::setw(7) << "etyp"
       << std::setw(7) << "eref"
       << std::setw(7) << "low"
       << std::setw(7) << "high"
       << std::setw(7) << "elsz"
       << std::setw(7) << "size" << "\n";
    os << std::string(47, '-') << "\n";

    if (atab.empty()) {
        os << "(empty)\n";
    } else {
        for (size_t i = 0; i < atab.size(); ++i) {
            const auto& a = atab[i];
            os << std::left
               << std::setw(5) << i
               << std::setw(7) << a.xtyp
               << std::setw(7) << a.etyp
               << std::setw(7) << a.eref
               << std::setw(7) << a.low
               << std::setw(7) << a.high
               << std::setw(7) << a.elsz
               << std::setw(7) << a.size << "\n";
        }
    }
}
