#ifndef SYMBOLTABLE_HPP
#define SYMBOLTABLE_HPP

#include <string>
#include <vector>
#include "astNode.hpp"

// Maximum sizes for symbol tables
#define MAX_TAB_SIZE   500
#define MAX_BTAB_SIZE  100
#define MAX_ATAB_SIZE  100
#define MAX_LEVEL      32
#define MAX_DISPLAY    32

// Size constants
#define INT_SIZE    4
#define REAL_SIZE   8
#define BOOL_SIZE   1
#define CHAR_SIZE   1
#define ADDR_SIZE   4  // Size of a pointer/address
#define MAX_ADR     32000

struct TabEntry {
    std::string id;     // Identifier name
    int link;           // Link to previous identifier in same scope (-1 = none)
    int obj;            // Object kind (OBJ_CONSTANT, OBJ_VARIABLE, etc.)
    int type;           // Type code
    int ref;            // Reference to complex type (index in atab or btab)
    int nrm;            // Parameter mode (PARAM_VALUE or PARAM_REF)
    int lev;            // Lexical level
    int adr;            // Address/offset/value

    TabEntry() : link(-1), obj(0), type(TYPE_VOID), ref(-1), nrm(PARAM_VALUE), lev(0), adr(0) {}
    TabEntry(const std::string& id, int obj, int type, int lev, int adr)
        : id(id), link(-1), obj(obj), type(type), ref(-1), nrm(PARAM_VALUE), lev(lev), adr(adr) {}
};

struct BtabEntry {
    int last;   // Index of last identifier in this block in tab
    int lpar;   // Index of last parameter in this block in tab
    int psze;   // Total parameter size
    int vsze;   // Total local variable size

    BtabEntry() : last(-1), lpar(-1), psze(0), vsze(0) {}
};

struct AtabEntry {
    int xtyp;   // Type of index
    int etyp;   // Element type
    int eref;   // Reference for composite element type
    int low;    // Lower bound
    int high;   // Upper bound
    int elsz;   // Element size
    int size;   // Total size

    AtabEntry() : xtyp(TYPE_VOID), etyp(TYPE_VOID), eref(-1), low(0), high(0), elsz(0), size(0) {}
};

class SymbolTable {
private:
    std::vector<TabEntry> tab;       // Identifier table
    std::vector<BtabEntry> btab;     // Block table
    std::vector<AtabEntry> atab;     // Array table
    std::vector<int> display;        // display[lev] = btab index

public:
    SymbolTable();

    int getTabSize() const { return tab.size(); }
    TabEntry& getTab(int idx) { return tab[idx]; }
    const TabEntry& getTab(int idx) const { return tab[idx]; }
    int addToTab(const TabEntry& entry);

    int getBtabSize() const { return btab.size(); }
    BtabEntry& getBtab(int idx) { return btab[idx]; }
    const BtabEntry& getBtab(int idx) const { return btab[idx]; }
    int addToBtab(const BtabEntry& entry);

    int getAtabSize() const { return atab.size(); }
    AtabEntry& getAtab(int idx) { return atab[idx]; }
    const AtabEntry& getAtab(int idx) const { return atab[idx]; }
    int addToAtab(const AtabEntry& entry);

    int getCurrentLevel() const;
    int getCurrentBtabIndex() const;
    void pushScope();
    void popScope();
    int getBtabIndexForLevel(int level) const;

    int lookup(const std::string& name) const;
    int lookupInCurrentScope(const std::string& name) const;

    void initializePredefinedIdentifiers();

    void printSymbolTable(std::ostream& os) const;
};

#endif // SYMBOLTABLE_HPP
