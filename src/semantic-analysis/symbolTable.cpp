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

    // Remove identifiers belonging to this scope from tab
    int btabIdx = display[level];
    int lastId = btab[btabIdx].last;
    
    // Pop identifiers until we reach the end of this scope
    int targetSize = lastId + 1;
    if (targetSize < (int)tab.size()) {
        tab.resize(targetSize);
    }

    display[level] = -1;
}

int SymbolTable::getBtabIndexForLevel(int level) const {
    if (level >= 0 && level < (int)display.size()) {
        return display[level];
    }
    return -1;
}

int SymbolTable::lookup(const std::string& name) const {
    int level = getCurrentLevel();
    
    // Search from innermost scope outward
    for (int l = level; l >= 0; --l) {
        int btabIdx = display[l];
        if (btabIdx == -1) continue;
        
        int current = btab[btabIdx].last;
        while (current != -1) {
            if (tab[current].id == name) {
                return current;
            }
            current = tab[current].link;
        }
    }
    
    return -1; // Not found
}

int SymbolTable::lookupInCurrentScope(const std::string& name) const {
    int level = getCurrentLevel();
    int btabIdx = display[level];
    if (btabIdx == -1) return -1;

    int current = btab[btabIdx].last;
    while (current != -1) {
        if (tab[current].id == name) {
            return current;
        }
        current = tab[current].link;
    }

    return -1;
}

void SymbolTable::initializePredefinedIdentifiers() {
    // Create global block (btab[0])
    BtabEntry globalBlock;
    btab.push_back(globalBlock);
    display[0] = 0;

    // Predefined identifiers (indices 0-32)
    // 0: integer
    {
        TabEntry entry("integer", OBJ_TYPE, TYPE_INTEGER, 0, 0);
        entry.link = -1;
        btab[0].last = addToTab(entry);
    }
    
    // 1: real
    {
        TabEntry entry("real", OBJ_TYPE, TYPE_REAL, 0, 0);
        entry.link = btab[0].last;
        btab[0].last = addToTab(entry);
    }
    
    // 2: boolean
    {
        TabEntry entry("boolean", OBJ_TYPE, TYPE_BOOLEAN, 0, 0);
        entry.link = btab[0].last;
        btab[0].last = addToTab(entry);
    }
    
    // 3: char
    {
        TabEntry entry("char", OBJ_TYPE, TYPE_CHAR, 0, 0);
        entry.link = btab[0].last;
        btab[0].last = addToTab(entry);
    }
    
    // 4: true
    {
        TabEntry entry("true", OBJ_CONSTANT, TYPE_BOOLEAN, 0, 1);
        entry.link = btab[0].last;
        btab[0].last = addToTab(entry);
    }
    
    // 5: false
    {
        TabEntry entry("false", OBJ_CONSTANT, TYPE_BOOLEAN, 0, 0);
        entry.link = btab[0].last;
        btab[0].last = addToTab(entry);
    }
    
    // 6: maxint
    {
        TabEntry entry("maxint", OBJ_CONSTANT, TYPE_INTEGER, 0, 32767);
        entry.link = btab[0].last;
        btab[0].last = addToTab(entry);
    }

    // 7: writeln (procedure)
    {
        TabEntry entry("writeln", OBJ_PROCEDURE, TYPE_VOID, 0, 0);
        entry.link = btab[0].last;
        btab[0].last = addToTab(entry);
    }

    // 8: readln (procedure)
    {
        TabEntry entry("readln", OBJ_PROCEDURE, TYPE_VOID, 0, 0);
        entry.link = btab[0].last;
        btab[0].last = addToTab(entry);
    }

    // 9: write (procedure)
    {
        TabEntry entry("write", OBJ_PROCEDURE, TYPE_VOID, 0, 0);
        entry.link = btab[0].last;
        btab[0].last = addToTab(entry);
    }

    // 10: read (procedure)
    {
        TabEntry entry("read", OBJ_PROCEDURE, TYPE_VOID, 0, 0);
        entry.link = btab[0].last;
        btab[0].last = addToTab(entry);
    }
}

void SymbolTable::printSymbolTable(std::ostream& os) const {
    os << "\n=== Symbol Table (tab) ===\n";
    os << std::left << std::setw(5) << "Idx"
       << std::setw(15) << "Name"
       << std::setw(10) << "Kind"
       << std::setw(8) << "Type"
       << std::setw(6) << "Lev"
       << std::setw(8) << "Address"
       << std::setw(6) << "Link" << "\n";
    os << std::string(60, '-') << "\n";

    for (size_t i = 0; i < tab.size(); ++i) {
        const auto& e = tab[i];
        os << std::left << std::setw(5) << i
           << std::setw(15) << e.id
           << std::setw(10);
        
        switch (e.obj) {
            case OBJ_CONSTANT:  os << "constant"; break;
            case OBJ_TYPE:      os << "type"; break;
            case OBJ_VARIABLE:  os << "variable"; break;
            case OBJ_PROCEDURE: os << "procedure"; break;
            case OBJ_FUNCTION:  os << "function"; break;
            case OBJ_PARAM:     os << "param"; break;
            default:            os << "unknown"; break;
        }

        std::string typeStr;
        switch (e.type) {
            case TYPE_INTEGER: typeStr = "integer"; break;
            case TYPE_REAL:    typeStr = "real"; break;
            case TYPE_BOOLEAN: typeStr = "boolean"; break;
            case TYPE_CHAR:    typeStr = "char"; break;
            case TYPE_STRING:  typeStr = "string"; break;
            case TYPE_VOID:    typeStr = "void"; break;
            default:           typeStr = std::to_string(e.type); break;
        }

        os << std::setw(8) << typeStr
           << std::setw(6) << e.lev
           << std::setw(8) << e.adr
           << std::setw(6) << e.link << "\n";
    }

    os << "\n=== Block Table (btab) ===\n";
    os << std::left << std::setw(5) << "Idx"
       << std::setw(8) << "Last"
       << std::setw(8) << "LPar"
       << std::setw(8) << "PSize"
       << std::setw(8) << "VSize" << "\n";
    os << std::string(40, '-') << "\n";

    for (size_t i = 0; i < btab.size(); ++i) {
        const auto& b = btab[i];
        os << std::left << std::setw(5) << i
           << std::setw(8) << b.last
           << std::setw(8) << b.lpar
           << std::setw(8) << b.psze
           << std::setw(8) << b.vsze << "\n";
    }

    if (!atab.empty()) {
        os << "\n=== Array Table (atab) ===\n";
        os << std::left << std::setw(5) << "Idx"
           << std::setw(8) << "XTyp"
           << std::setw(8) << "ETyp"
           << std::setw(6) << "ERef"
           << std::setw(8) << "Low"
           << std::setw(8) << "High"
           << std::setw(8) << "ElSz"
           << std::setw(8) << "Size" << "\n";
        os << std::string(60, '-') << "\n";

        for (size_t i = 0; i < atab.size(); ++i) {
            const auto& a = atab[i];
            os << std::left << std::setw(5) << i
               << std::setw(8) << a.xtyp
               << std::setw(8) << a.etyp
               << std::setw(6) << a.eref
               << std::setw(8) << a.low
               << std::setw(8) << a.high
               << std::setw(8) << a.elsz
               << std::setw(8) << a.size << "\n";
        }
    }
}
