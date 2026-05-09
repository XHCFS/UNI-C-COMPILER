#pragma once
#include "Token.h"
#include <string>
using namespace std;


// Represents one identifier entry in the symbol table.
// The lexer populates the token field; the parser fills in dataType and scope later.
class SymbolEntry {
public:
    // Constructs an entry from the identifier token; dataType defaults to empty, scope to -1.
    explicit SymbolEntry(const Token& token);

    // Returns the identifier name (delegates to token_.getLexeme()).
    const string& getName()  const;

    // Returns the source line where this identifier was first seen (delegates to token_.getLine()).
    int  getLine()  const;

    // Returns the full Token that introduced this identifier.
    const Token& getToken() const;

    // Sets the resolved C type string for this identifier (called by the parser).
    void setDataType(const string& type);

    // Returns the resolved C type string, or empty if not yet set by the parser.
    const string& getDataType() const;

    // Sets the lexical scope depth at which this identifier was declared.
    void setScope(int scopeLevel);

    // Returns the scope depth, or -1 if not yet assigned by the parser.
    int getScope() const;

private:
    Token token_;     // The identifier token as scanned by the lexer.
    string dataType_;  // C type of this identifier, resolved during parsing.
    int scope_;     // Lexical scope depth; -1 means uninitialized.
};
