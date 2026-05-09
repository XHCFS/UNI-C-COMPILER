#pragma once
#include <string>
using namespace std;


// Underlying scalar kind of a C type recognised by the parser.
// Sign modifiers (signed/unsigned), length redundancy (the second 'long' in
// 'long long'), and storage-class keywords are accepted syntactically but not
// stored on Type — see Parser_Design.md §3.1.
enum class TypeKind {
    VOID, CHAR, SHORT, INT, LONG,
    FLOAT, DOUBLE, BOOL,
    INVALID
};

// Represents the resolved C type of a declared identifier or expression.
// Built incrementally by Parser::parseDeclSpecifiers() and parseDeclarator()
// from the keyword tokens the lexer emitted; stored on the relevant AST node
// and on the matching SymbolEntry via setDataType(type.toString()).
class Type {
public:
    // Constructs an invalid type: kind=INVALID, pointerDepth=0, no qualifiers.
    Type();

    // Constructs a plain scalar type with no qualifiers and no pointer levels.
    explicit Type(TypeKind kind);

    // Returns the underlying scalar kind.
    TypeKind getKind() const;

    // Returns the number of '*' levels (0 = not a pointer, 1 = T*, 2 = T**, ...).
    int getPointerDepth() const;

    // Returns true if the const qualifier was present in the declaration.
    bool isConst() const;

    // Returns true if the volatile qualifier was present.
    bool isVolatile() const;

    // Mutators used by the parser to merge specifiers into one Type.
    void setKind(TypeKind k);
    void addPointer();
    void setConst(bool v);
    void setVolatile(bool v);

    // Builds a readable representation, e.g. "const int*" or "volatile char**".
    // Used for debugging, GUI display, and SymbolEntry::setDataType().
    string toString() const;

private:
    TypeKind kind_;          // underlying scalar kind
    int      pointerDepth_;  // number of '*' levels
    bool     isConst_;       // 'const' qualifier present
    bool     isVolatile_;    // 'volatile' qualifier present
};

// Maps every TypeKind enumerator to its C name as a string (e.g. INT -> "int");
// returns "unknown" for unhandled values. Mirrors tokenTypeToString's role for TokenType.
string typeKindToString(TypeKind kind);
