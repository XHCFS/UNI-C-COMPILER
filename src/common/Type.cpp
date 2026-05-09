#include "Type.h"
using namespace std;

// Default ctor — invalid kind, no qualifiers, not a pointer.
Type::Type()
    : kind_(TypeKind::INVALID), pointerDepth_(0),
      isConst_(false), isVolatile_(false) {}

// Plain-scalar ctor — sets kind, leaves the rest at their defaults.
Type::Type(TypeKind kind)
    : kind_(kind), pointerDepth_(0),
      isConst_(false), isVolatile_(false) {}

TypeKind Type::getKind()         const { return kind_; }
int      Type::getPointerDepth() const { return pointerDepth_; }
bool     Type::isConst()         const { return isConst_; }
bool     Type::isVolatile()      const { return isVolatile_; }

void Type::setKind(TypeKind k) { kind_ = k; }
void Type::addPointer()        { ++pointerDepth_; }
void Type::setConst(bool v)    { isConst_ = v; }
void Type::setVolatile(bool v) { isVolatile_ = v; }

// Composes "[const ][volatile ]<kind><stars>" — e.g. "const int*" or "volatile char**".
string Type::toString() const {
    string out;
    if (isConst_)    out += "const ";
    if (isVolatile_) out += "volatile ";
    out += typeKindToString(kind_);
    for (int i = 0; i < pointerDepth_; ++i) out += "*";
    return out;
}

// Maps each enumerator to its C name; returns "unknown" for unhandled values.
string typeKindToString(TypeKind kind) {
    switch (kind) {
        case TypeKind::VOID:    return "void";
        case TypeKind::CHAR:    return "char";
        case TypeKind::SHORT:   return "short";
        case TypeKind::INT:     return "int";
        case TypeKind::LONG:    return "long";
        case TypeKind::FLOAT:   return "float";
        case TypeKind::DOUBLE:  return "double";
        case TypeKind::BOOL:    return "bool";
        case TypeKind::INVALID: return "invalid";
        default:                return "unknown";
    }
}
