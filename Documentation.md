# UNI-C Compiler — Project Documentation

A C-subset compiler frontend in C++17 with a Qt 6 GUI. Two phases are implemented:

1. **Lexical analysis** — turns source text into a stream of tokens
2. **Parsing** — turns the token stream into an abstract syntax tree (AST)

---

## Table of Contents

1. [Project overview](#1-project-overview)
2. [Pipeline and architecture](#2-pipeline-and-architecture)
3. [Phase 1 — Lexical analysis](#3-phase-1--lexical-analysis)
4. [Phase 2 — Parsing](#4-phase-2--parsing)
5. [Shared `SymbolTable`](#5-shared-symboltable)
6. [Worked examples](#6-worked-examples)
7. [The GUI](#7-the-gui)
8. [Building and testing](#8-building-and-testing)
9. [Limitations](#9-limitations)

---

## 1. Project overview

The compiler accepts a C-style source file and produces:

- A `vector<Token>` — every lexical unit, in source order, with line/column metadata.
- A `unique_ptr<TranslationUnit>` — the AST root, with a typed hierarchy of declarations, statements, and expressions.
- A populated `SymbolTable` — composite-keyed by `(scope, name)`, with one lane for lexer-seen identifiers and one lane per parser scope.
- A `vector<LexerError>` and a `vector<ParseError>` — both phases are error-recovering: they record diagnostics and keep going rather than aborting on the first bad input.

The code is organised into four static libraries (`common`, `lexer`, `parser`, `gui`) plus a single executable (`ccompiler`) that hosts the Qt GUI. Two test executables (`test_lexer`, `test_parser`) exercise the libraries from the bottom up.

---

## 2. Pipeline and architecture

```text
                                 shared SymbolTable
                                 ┌─────────────────┐
                          insert │  scope = -1     │
                            ┌───▶│  (lexer lane)   │
                            │    │  scope = 0,1,…  │◀──┐ declare
                            │    │  (parser lanes) │   │
                            │    └─────────────────┘   │
                            │                          │
   ┌────────┐   ┌─────────┐ │  ┌───────────────┐   ┌──────────┐   ┌─────────────────┐
   │ source │──▶│ Lexer   │─┴─▶│ vector<Token> │──▶│ Parser   │──▶│ TranslationUnit │
   │ string │   │         │    │               │   │          │   │   (AST root)    │
   └────────┘   └────┬────┘    └───────────────┘   └────┬─────┘   └─────────────────┘
                     │                                  │
                     ▼                                  ▼
                vector<LexerError>                vector<ParseError>
```

### Module layout

| Directory | Library | Purpose |
|---|---|---|
| `src/common/` | `common` | Shared types: `Token`, `SymbolEntry`, `SymbolTable`, `Type`, `Ast` (AST node hierarchy) |
| `src/lexer/` | `lexer` | `CharStream`, `TokenClassifier`, `ReservedWords`, `LexerError`, `Lexer` |
| `src/parser/` | `parser` | `TokenStream`, `ParserClassifier`, `ParseError`, `Parser` |
| `src/gui/` | `gui` | `MainWindow`, `LexerPanel`, `ParserPanel` (Qt 6) |
| `src/main.cpp` | `ccompiler` | Application entry point |
| `tests/lexer/` | — | `test_lexer` executable, `.c` fixtures |
| `tests/parser/` | — | `test_parser` executable, `.c` fixtures |

The `lexer` library depends on `common`. The `parser` library depends on `common` and `lexer`. The `gui` library depends on all three. This stratification keeps the lexer and parser testable in isolation; the GUI sits on top.

---

## 3. Phase 1 — Lexical analysis

### Goal

Read a `std::string` of source code from left to right and produce a `std::vector<Token>` that:

- contains every lexeme in source order,
- skips whitespace and comments entirely (they don't appear as tokens),
- classifies each lexeme into one of ~80 `TokenType` values,
- records the 1-based `(line, column)` of every token,
- inserts every distinct identifier name into the shared `SymbolTable` (in the lexer lane, `scope = -1`),
- continues past lexical errors rather than aborting.

The output token stream has no `END_OF_FILE` token; end-of-stream is signalled by reaching the end of the vector.

### Components

| File | Class / Functions | Role |
|---|---|---|
| `Token.{h,cpp}` | `enum class TokenType`, `class Token`, `tokenTypeToString` | Token vocabulary and value type |
| `CharStream.{h,cpp}` | `CharStream` | Character cursor over the source string with line/column tracking and a `'\0'` past-end sentinel |
| `TokenClassifier.{h,cpp}` | `isIdentifierStart`, `isIdentifierPart`, `isDigit`, `isWhitespace`, `isOperatorStart`, `isDelimiterChar`, `classifySingleChar`, `classifyDoubleChar` | Stateless character predicates and 1-/2-character operator classifiers |
| `ReservedWords.{h,cpp}` | `ReservedWords` | C89→C23 keyword string → `TokenType` map (~59 entries) |
| `LexerError.{h,cpp}` | `LexerError` | Diagnostic record: message, line, column, offending text |
| `Lexer.{h,cpp}` | `Lexer` | The main class. Holds a `CharStream`, a `ReservedWords`, a reference to the shared `SymbolTable`, the produced token list, and the produced error list |

### The `nextToken` dispatch

`Lexer::tokenize()` is a thin loop that calls `nextToken()` repeatedly until EOF. The real work lives in `nextToken()`:

```text
loop forever:
    skip whitespace and comments alternately until neither makes progress
    if EOF: return an internal sentinel (empty lexeme, type INVALID; tokenize() discards it)
    look at the next character c:
        if isIdentifierStart(c) → scanIdentifierOrKeyword()
        if isDigit(c)           → scanNumber()
        if c == '"'             → scanStringLiteral()
        if c == '\''            → scanCharLiteral()
        if isOperatorStart(c) or isDelimiterChar(c) → scanOperatorOrDelimiter()
        otherwise               → recoverFromInvalidCharacter() and loop
```

The outer `loop forever` is what lets the lexer recover from an invalid character without recursing — `recoverFromInvalidCharacter` records an error and consumes one or more bad characters, then control flows back to the top of the loop to try again.

### Maximal munch

The lexer always reads the longest valid token at the current position. This matters most in `scanOperatorOrDelimiter`, which dispatches in priority order:

1. **Three-character first**: `<<=` and `>>=` are checked before two-character forms, so `>>=` parses as `RSHIFT_ASSIGN` (one token), not `>>` then `=` (two tokens).
2. **Two-character next**: `classifyDoubleChar` covers `++ -- -> == != <= >= << >> && || += -= *= /= %= &= |= ^=`.
3. **One-character last**: `classifySingleChar` covers the remaining single-character operators and delimiters.

### Token vocabulary

`TokenType` (in `Token.h`) groups its enumerators into contiguous ranges so tokens can be classified by integer comparison rather than per-name switches:

| Range | Members | Predicate |
|---|---|---|
| `KW_IF` … `KW__DECIMAL128` | All C89-C23 keywords (~59) | `Token::isKeyword()` |
| `IDENTIFIER` | One value | — |
| `INT_LITERAL` … `STRING_LITERAL` | Four values | `Token::isLiteral()` |
| `PLUS` … `COLON` | All operator forms (~30) | `Token::isOperator()` |
| `LPAREN` … `COMMA` | Eight delimiters | `Token::isDelimiter()` |
| `INVALID` | Past-end sentinel and error placeholder | — |

Every keyword has its own `KW_*` value (for example `KW_INT`, `KW_RETURN`, `KW_CONST`) so downstream phases can `switch` on `TokenType` without comparing lexeme strings.

### Comments

`skipComment` handles two C comment forms:

- `// line comment` — consumed up to (but not past) the next `'\n'`.
- `/* block comment */` — consumed up to and including the closing `*/`. If the stream ends before `*/`, the lexer records `"unterminated block comment"` and stops.

Comments are entirely invisible to the parser; they don't produce tokens.

### Symbol-table integration

When `scanIdentifierOrKeyword` produces an `IDENTIFIER` token (not a keyword), the lexer calls `symbolTable_.insert(token)`. The default scope argument of `-1` puts the entry in the **lexer lane** of the composite-keyed table. The lexer lane is dedup'd by name: each unique identifier name gets exactly one entry, recording the line/column of its first occurrence. See [Section 5](#5-shared-symboltable) for how this interacts with the parser's lanes.

### Error recovery

Two recovery routines:

- **`recoverFromInvalidCharacter`**: when `nextToken` sees a character that doesn't fit any known production, the lexer consumes the bad character **plus any immediately following identifier characters** as one unit (maximal munch on the error side), records a single `"invalid character sequence"` error, and loops. So `@yz` reports one error containing `@yz`, not one error for `@` followed by an `IDENTIFIER yz`.
- **`recoverFromUnterminatedString`**: when a string literal hits a newline or EOF before its closing `"`, the lexer records the error and skips up to (not past) the next newline so the next line can be lexed cleanly.

### What the lexer doesn't do

- Numeric literals are decimal-only — no `0x...` hex, no `0b...` binary, no octal, no exponent (`1e10`), no suffixes (`1L`, `2.5f`).
- Escape sequences in `'...'` and `"..."` are kept as raw text (`'\n'` is the three-character lexeme `\\n` plus the surrounding quotes); semantic interpretation is deferred.
- `#include`, `#define`, and other preprocessor directives are not recognised.

---

## 4. Phase 2 — Parsing

### Goal

Consume the lexer's `vector<Token>` and produce a `unique_ptr<TranslationUnit>` containing a typed AST plus a `vector<ParseError>`. Along the way:

- declare every named identifier in the shared `SymbolTable` at the active lexical scope,
- recover from syntax errors via `synchronize()` rather than aborting,
- reuse precise line/column metadata from the original tokens on every AST node.

### Strategy

Recursive-descent. Every grammar non-terminal becomes a method on `Parser`. The expression cascade has one method per precedence level. The methods build AST nodes that own their children via `unique_ptr`; errors propagate as `nullptr` operands rather than exceptions.

### Components

| File | Class / Functions | Role |
|---|---|---|
| `TokenStream.{h,cpp}` | `TokenStream` | Token-level cursor mirroring `CharStream`'s shape: `peek`, `get`, `eof`, `match`, `check`, `getLine`, `getColumn`, with an EOF sentinel |
| `ParserClassifier.{h,cpp}` | `isTypeSpecifier`, `isTypeQualifier`, `isStorageClassSpecifier`, `isDeclarationStart`, `isAssignmentOperator`, `isUnaryPrefixOperator`, `isPostfixOperator`, `isLiteralToken` | Stateless predicates over `TokenType` |
| `ParseError.{h,cpp}` | `ParseError` | Mirrors `LexerError`: same fields, same `toString` format with a `"ParseError"` prefix |
| `Parser.{h,cpp}` | `Parser` | Constructor takes `vector<Token>&` and `SymbolTable&`. Public: `parse`, `parseExpression`, `parseStatement`, `getErrors`. Private: ~30 grammar methods + `addError` + `expect` + `synchronize` |
| `Ast.{h,cpp}` | `AstNode` and 23 concrete subclasses | The AST hierarchy: `Expr`/`Stmt`/`Decl` abstract intermediates, all concrete node classes with `dump()` overrides |

### Recursive-descent shape

Every grammar method follows the same outline:

1. Capture `int line = input_.peek().getLine(); int col = input_.peek().getColumn();` at entry. This is the position the resulting AST node will carry.
2. Parse the production (consume tokens, recurse into sub-productions).
3. Return a `unique_ptr` to a freshly-built AST node.

Errors are reported via `addError` (which appends to `errors_`) and `expect(type, msg)` (which consumes-on-match, otherwise reports an error pointing at the current token). When an inner production can't proceed, it returns `nullptr` and the outer one decides what to do (typically: attach the `nullptr` as a child and continue — the AST tolerates null children, the dumper skips them).

### Expression cascade

14 methods, low-to-high precedence:

```
parseExpression           ← top entry point
  parseAssignment         right-associative; handles =, +=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=
    parseConditional      right-associative ?:
      parseLogicalOr      ||
        parseLogicalAnd   &&
          parseBitwiseOr  |
            parseBitwiseXor ^
              parseBitwiseAnd &
                parseEquality ==, !=
                  parseRelational <, >, <=, >=
                    parseShift   <<, >>
                      parseAdditive   +, -
                        parseMultiplicative *, /, %
                          parseUnary  prefix +, -, !, ~, ++, --, *, &
                            parsePostfix postfix ++/--, calls f(...), index a[i]
                              parsePrimary literal | identifier | ( expr )
```

Every left-associative binary level shares the same shape: parse the next-higher level into `left`, then loop while the operator(s) at this level appear, consuming the operator and the next-higher-level operand each iteration. Left-associativity falls out for free because the loop wraps `left` after each operator consumption.

`parseAssignment` and `parseConditional` recurse into themselves for the right-hand side, which gives right-associativity (`a = b = c` → `a = (b = c)`).

### Statement dispatch

`parseStatement` looks at the first token only:

| First token | Routes to |
|---|---|
| `LBRACE` | `parseCompoundStmt` |
| `KW_IF` | `parseIfStmt` |
| `KW_WHILE` or `KW_DO` | `parseLoopStmt` (one method handles both forms) |
| `KW_FOR` | `parseForStmt` |
| `KW_RETURN` | `parseReturnStmt` |
| `KW_BREAK` or `KW_CONTINUE` | `parseJumpStmt` (one method handles both) |
| `SEMICOLON` | consume the `;` and return `nullptr` (no null-statement node) |
| anything else | `parseExprStmt` |

`parseCompoundStmt` pushes a new scope, calls `parseBlockItems` (which interleaves declarations and statements), and pops on the closing `}`.

### Declaration parsing

Top-level (and block-level) declarations follow the same pattern:

1. **`parseDeclSpecifiers`** loops over the specifier run, building one `Type`. Type-specifier keywords (`void`/`char`/`short`/`int`/`long`/`float`/`double`/`bool`) set `kind_`; `const`/`volatile` set qualifier flags; `signed`/`unsigned`, `restrict`, the second `long` in `long long`, and storage-class keywords are accepted but discarded.
2. **`parseDeclarator`** consumes leading `*`s (incrementing `Type::pointerDepth_`) and then the `IDENTIFIER`.
3. **Dispatch on the next token**: `(` → `parseFunctionDecl`; otherwise → `parseVarDecl` plus optional `,`-separated chain; trailing `;`.

`parseFunctionDecl` declares the function name at the *outer* scope, then pushes a fresh scope shared by parameters and body (per C99 6.2.1), parses `parseParamList`, then `{ parseBlockItems }`, and pops. It uses `parseBlockItems` directly rather than `parseCompoundStmt` so the body isn't isolated from the parameter scope.

`parseVarDecl` optionally consumes an `= expr` initializer, then calls `symbolTable_.declare(token, scopeStack_.back(), type.toString())`. Same-scope redeclaration causes `declare()` to return `false`, which produces a `"redeclaration of '<name>'"` error. If the declarator had no name, `parseVarDecl` returns `nullptr` instead of building a phantom unnamed node.

### Symbol-table integration

The parser maintains two members:

- `int nextScopeId_` — monotonic counter; each new `{` consumes one fresh ID
- `vector<int> scopeStack_` — the chain of currently active scope IDs, innermost at the back. File scope is preloaded as `0`.

Push/pop rules:

| Site | Scope action |
|---|---|
| `parseCompoundStmt` (general blocks) | Push fresh ID before items, pop on `}` |
| `parseFunctionDecl` | Push fresh ID before parameter list. Body shares this scope (per C99 6.2.1). Pop after `}` |
| `parseForStmt` | Push fresh ID before init clause, pop after body — so `for (int i = 0; …)` scopes `i` to the loop |

Every declaration calls `symbolTable_.declare(tok, scopeStack_.back(), type.toString())`. The `(scope, name)` composite key allows shadowing — `int x; { int x; }` produces two coexisting entries.

The parser does **not** call `lookup()`. Resolving identifier *uses* against declarations is deferred to a future semantic-analysis phase.

### Error recovery

Three layers:

- **`expect(type, msg)`** records an error pointing at the current token if the expected type doesn't match; it does **not** consume on miss. Nearly every grammar method uses `expect` for required tokens (`)`, `;`, etc.).
- **`synchronize()`** advances the cursor until it sits at a known safe boundary: a `;` (consumed), or any of `{` `}` `if` `while` `for` `do` `return` `break` `continue` (left in place for the next grammar method).
- **Forward-progress guard in `parse()`**: if neither `parseExternalDeclaration` nor `synchronize` advanced the cursor in one iteration, the loop force-consumes one token. This guarantees the parser cannot infinite-loop on garbage input.

`parsePrimary` has a special case in its error path: it leaves *terminator-shaped* tokens (`;`, `)`, `]`, `,`, `}`) in the stream rather than consuming them, so the parent production (e.g., `parseExprStmt` looking for `;`) can still find its expected boundary. Without this rule, `a = 2 + ;` would produce two cascading errors instead of one accurate one.

### AST hierarchy

23 concrete node classes after the Tier-1 simplifications described in `Parser_Design.md`. Every node inherits from `AstNode`, which carries `(line, column)` and a `virtual void dump(ostream&, int indent) const` for textual rendering.

```
AstNode (abstract)
├── Expr (abstract)
│   ├── LiteralExpr        (TokenType kind_, string lexeme_)
│   ├── IdentifierExpr     (string name_)
│   ├── BinaryExpr         (TokenType op_, lhs_, rhs_)
│   ├── UnaryExpr          (bool isPostfix_, TokenType op_, operand_)
│   ├── AssignExpr         (TokenType op_, target_, value_)
│   ├── ConditionalExpr    (cond_, then_, else_)
│   ├── CallExpr           (callee_, args_[])
│   └── IndexExpr          (array_, index_)
├── Stmt (abstract)
│   ├── CompoundStmt       (vector<unique_ptr<AstNode>> items_)
│   ├── IfStmt             (cond_, then_, else_)
│   ├── LoopStmt           (bool isPostTest_, cond_, body_)  -- while/do-while
│   ├── ForStmt            (init_, cond_, step_, body_)
│   ├── ReturnStmt         (value_)
│   ├── JumpStmt           (TokenType kind_)                  -- break/continue
│   └── ExprStmt           (expr_)
├── Decl (abstract)
│   ├── VarDecl            (Type, name, init_)
│   ├── ParamDecl          (Type, name)
│   └── FunctionDecl       (returnType, name, params_[], body_)
└── TranslationUnit        (decls_[])
```

`dump(os, indent)` produces a stable textual form: each node prints one header line at the current indent (e.g., `BinaryExpr PLUS`, `VarDecl 'x' int`), then recurses on its children with `indent + 1`. Two spaces per indent level. Null children are silently skipped. This format is what the GUI's `ParserPanel` uses to populate its `QTreeWidget`.

---

## 5. Shared `SymbolTable`

The same `SymbolTable` is shared between the lexer and the parser. It is a `std::unordered_map` keyed on the composite:

```cpp
struct Key {
    int    scope;
    string name;
};
```

with a custom `KeyHash`. Three lanes coexist in the one map, distinguished by the `scope` half of the key:

| Lane | Owner | Written by | Holds |
|---|---|---|---|
| `scope = -1` | Lexer | `insert(token)` (default scope is `-1`) | First-seen position of every unique identifier name in source |
| `scope = 0` | Parser | `declare(token, 0, dataType)` | File-scope declarations |
| `scope ≥ 1` | Parser | `declare(token, n, dataType)` | Nested-scope declarations (one fresh ID per `{`) |

Because the key is `(scope, name)`, two declarations of the same name at different scopes coexist as two distinct entries. `int x; { int x; }` produces both `(0, "x")` and (say) `(2, "x")`.

The API:

```cpp
class SymbolTable {
public:
    bool insert(const Token& token, int scope = -1);                  // lexer (default arg)
    bool declare(const Token& token, int scope,                       // parser
                 const string& dataType);
    SymbolEntry* find(const string& name, int scope);                 // exact-scope
    SymbolEntry* lookup(const string& name,                           // shadowing-aware
                        const vector<int>& activeScopes);
    bool exists(const string& name, int scope) const;
    const unordered_map<Key, SymbolEntry, KeyHash>& getAllEntries() const;
    void clear();
};
```

`lookup` walks `activeScopes` innermost-to-outermost and returns the first hit. The lexer's `scope = -1` lane is intentionally never visited by `lookup` — callers pass only real scope IDs. The lane exists only for the GUI's lexer panel to display "names this source uses."

---

## 6. Worked examples

Each example shows the source, the lexer's output (tokens), the parser's output (parse tree), and — where interesting — the resulting symbol-table state. The token format is `[TYPE 'lexeme' L<line>:C<col>]`. The parse-tree format is the canonical `AstNode::dump` output: two-space indents, one node per line.

### Example 1 — minimal variable declaration

**Source**

```c
int x;
```

**Tokens**

```
[KW_INT 'int' L1:C1]
[IDENTIFIER 'x' L1:C5]
[SEMICOLON ';' L1:C7]
```

**Parse tree**

```
TranslationUnit
  VarDecl 'x' int
```

`VarDecl` carries `type = int`, `name = "x"`, `init = nullptr`. No initializer is emitted because the source has none.

**Symbol table after parsing**

| scope | name | dataType |
|---|---|---|
| -1 | x | (empty) |
| 0 | x | int |

The lexer-lane entry was created by the lexer when it first saw `x`. The parser-lane entry was created by `parseVarDecl`'s call to `declare(tok, 0, "int")`.

---

### Example 2 — variable with initializer

**Source**

```c
int x = 42;
```

**Tokens**

```
[KW_INT 'int' L1:C1]
[IDENTIFIER 'x' L1:C5]
[ASSIGN '=' L1:C7]
[INT_LITERAL '42' L1:C9]
[SEMICOLON ';' L1:C11]
```

**Parse tree**

```
TranslationUnit
  VarDecl 'x' int
    LiteralExpr INT_LITERAL '42'
```

The `LiteralExpr` is the initializer subtree of `VarDecl`. It carries `kind = INT_LITERAL` and `lexeme = "42"`. No numeric conversion is performed at parse time — the literal stays as a text string until a future semantic phase.

---

### Example 3 — operator precedence

The parser's expression cascade is what gives operators their precedence. Multiplication binds tighter than addition because `parseAdditive` calls `parseMultiplicative` for each operand, so multiplications get fully reduced before additions even consider their operands.

**Source**

```c
int y = 1 + 2 * 3;
```

**Tokens**

```
[KW_INT 'int' L1:C1]
[IDENTIFIER 'y' L1:C5]
[ASSIGN '=' L1:C7]
[INT_LITERAL '1' L1:C9]
[PLUS '+' L1:C11]
[INT_LITERAL '2' L1:C13]
[STAR '*' L1:C15]
[INT_LITERAL '3' L1:C17]
[SEMICOLON ';' L1:C18]
```

**Parse tree**

```
TranslationUnit
  VarDecl 'y' int
    BinaryExpr PLUS
      LiteralExpr INT_LITERAL '1'
      BinaryExpr STAR
        LiteralExpr INT_LITERAL '2'
        LiteralExpr INT_LITERAL '3'
```

Read the tree structurally: the outer `BinaryExpr PLUS` has `1` on the left and a nested `BinaryExpr STAR` on the right. That mirrors the grouping `1 + (2 * 3)`. If multiplication had the same precedence as addition, the tree would instead be `((1 + 2) * 3)` — which is *not* what the source means.

---

### Example 4 — parenthesized grouping

**Source**

```c
int z = (1 + 2) * 3;
```

**Tokens**

```
[KW_INT 'int' L1:C1]
[IDENTIFIER 'z' L1:C5]
[ASSIGN '=' L1:C7]
[LPAREN '(' L1:C9]
[INT_LITERAL '1' L1:C10]
[PLUS '+' L1:C12]
[INT_LITERAL '2' L1:C14]
[RPAREN ')' L1:C15]
[STAR '*' L1:C17]
[INT_LITERAL '3' L1:C19]
[SEMICOLON ';' L1:C20]
```

**Parse tree**

```
TranslationUnit
  VarDecl 'z' int
    BinaryExpr STAR
      BinaryExpr PLUS
        LiteralExpr INT_LITERAL '1'
        LiteralExpr INT_LITERAL '2'
      LiteralExpr INT_LITERAL '3'
```

Compare with Example 3: same operators, same operands, *different shape*. Parentheses force the addition to evaluate first, so `BinaryExpr PLUS` is now the *left* child of `BinaryExpr STAR`. Note that there is no `ParenExpr` wrapper — `parsePrimary` returns the inner expression directly. Grouping is preserved structurally by the tree shape, not by an explicit wrapper node.

---

### Example 5 — function definition

**Source**

```c
int main() {
    return 0;
}
```

**Tokens**

```
[KW_INT 'int' L1:C1]
[IDENTIFIER 'main' L1:C5]
[LPAREN '(' L1:C9]
[RPAREN ')' L1:C10]
[LBRACE '{' L1:C12]
[KW_RETURN 'return' L2:C5]
[INT_LITERAL '0' L2:C12]
[SEMICOLON ';' L2:C13]
[RBRACE '}' L3:C1]
```

**Parse tree**

```
TranslationUnit
  FunctionDecl 'main' returns int
    CompoundStmt
      ReturnStmt
        LiteralExpr INT_LITERAL '0'
```

`FunctionDecl` carries return type, name, an empty params vector, and a `CompoundStmt` body. The `()` after the name takes the empty-parameter-list path in `parseParamList`.

**Symbol table after parsing**

| scope | name | dataType |
|---|---|---|
| -1 | main | (empty) |
| 0 | main | int |

Note that the function name `main` is declared at the *outer* scope (`scope = 0`), not inside the function's body scope. The body scope is `1`, but it's empty here because there are no declarations inside the function body.

---

### Example 6 — function with parameters

**Source**

```c
int add(int a, int b) {
    return a + b;
}
```

**Tokens**

```
[KW_INT 'int' L1:C1]
[IDENTIFIER 'add' L1:C5]
[LPAREN '(' L1:C8]
[KW_INT 'int' L1:C9]
[IDENTIFIER 'a' L1:C13]
[COMMA ',' L1:C14]
[KW_INT 'int' L1:C16]
[IDENTIFIER 'b' L1:C20]
[RPAREN ')' L1:C21]
[LBRACE '{' L1:C23]
[KW_RETURN 'return' L2:C5]
[IDENTIFIER 'a' L2:C12]
[PLUS '+' L2:C14]
[IDENTIFIER 'b' L2:C16]
[SEMICOLON ';' L2:C17]
[RBRACE '}' L3:C1]
```

**Parse tree**

```
TranslationUnit
  FunctionDecl 'add' returns int
    ParamDecl 'a' int
    ParamDecl 'b' int
    CompoundStmt
      ReturnStmt
        BinaryExpr PLUS
          IdentifierExpr 'a'
          IdentifierExpr 'b'
```

Inside the body, `a` and `b` appear as `IdentifierExpr` nodes — the parser does not resolve them against the symbol table; that's a job for semantic analysis. The names come straight from the lexeme of the consumed `IDENTIFIER` tokens.

**Symbol table after parsing**

| scope | name | dataType |
|---|---|---|
| -1 | add | (empty) |
| -1 | a | (empty) |
| -1 | b | (empty) |
| 0 | add | int |
| 1 | a | int |
| 1 | b | int |

Both parameters `a` and `b` are declared in the function's block scope (`scope = 1`). If the function body had declared a local variable `int c;`, it would also live at scope 1 — per C99 6.2.1, parameters and the function body share one block scope.

---

### Example 7 — if / else

**Source**

```c
int main() {
    int x = 0;
    if (x > 0)
        x = 1;
    else
        x = -1;
    return x;
}
```

**Tokens** (selected — full sequence is mechanical from the rules)

```
[KW_INT 'int' L1:C1]      [IDENTIFIER 'main' L1:C5]   [LPAREN '(' L1:C9]
[RPAREN ')' L1:C10]       [LBRACE '{' L1:C12]
[KW_INT 'int' L2:C5]      [IDENTIFIER 'x' L2:C9]      [ASSIGN '=' L2:C11]
[INT_LITERAL '0' L2:C13]  [SEMICOLON ';' L2:C14]
[KW_IF 'if' L3:C5]        [LPAREN '(' L3:C8]          [IDENTIFIER 'x' L3:C9]
[GT '>' L3:C11]           [INT_LITERAL '0' L3:C13]    [RPAREN ')' L3:C14]
[IDENTIFIER 'x' L4:C9]    [ASSIGN '=' L4:C11]         [INT_LITERAL '1' L4:C13]
[SEMICOLON ';' L4:C14]
[KW_ELSE 'else' L5:C5]
[IDENTIFIER 'x' L6:C9]    [ASSIGN '=' L6:C11]         [MINUS '-' L6:C13]
[INT_LITERAL '1' L6:C14]  [SEMICOLON ';' L6:C15]
[KW_RETURN 'return' L7:C5]  [IDENTIFIER 'x' L7:C12]   [SEMICOLON ';' L7:C13]
[RBRACE '}' L8:C1]
```

**Parse tree**

```
TranslationUnit
  FunctionDecl 'main' returns int
    CompoundStmt
      VarDecl 'x' int
        LiteralExpr INT_LITERAL '0'
      IfStmt
        BinaryExpr GT
          IdentifierExpr 'x'
          LiteralExpr INT_LITERAL '0'
        ExprStmt
          AssignExpr ASSIGN
            IdentifierExpr 'x'
            LiteralExpr INT_LITERAL '1'
        ExprStmt
          AssignExpr ASSIGN
            IdentifierExpr 'x'
            UnaryExpr MINUS prefix
              LiteralExpr INT_LITERAL '1'
      ReturnStmt
        IdentifierExpr 'x'
```

Things to notice:

- The condition `x > 0` becomes a `BinaryExpr GT` child of `IfStmt`.
- The `then` branch and the `else` branch are both children of `IfStmt` directly — there is no separate "else" wrapper. `IfStmt` has three slots: `cond_`, `then_`, `else_`.
- `-1` parses as `UnaryExpr MINUS prefix` with a `LiteralExpr 1` operand. The lexer doesn't combine `-` with `1` into a negative literal; the parser builds a unary-minus node.
- The dangling-else rule (binding `else` to the nearest unmatched `if`) falls out automatically because of recursive descent — only the most recent `parseIfStmt` invocation looks for a trailing `else`.

---

### Example 8 — while loop

**Source**

```c
int main() {
    int i = 0;
    while (i < 10) {
        i = i + 1;
    }
    return i;
}
```

**Parse tree**

```
TranslationUnit
  FunctionDecl 'main' returns int
    CompoundStmt
      VarDecl 'i' int
        LiteralExpr INT_LITERAL '0'
      LoopStmt while
        BinaryExpr LT
          IdentifierExpr 'i'
          LiteralExpr INT_LITERAL '10'
        CompoundStmt
          ExprStmt
            AssignExpr ASSIGN
              IdentifierExpr 'i'
              BinaryExpr PLUS
                IdentifierExpr 'i'
                LiteralExpr INT_LITERAL '1'
      ReturnStmt
        IdentifierExpr 'i'
```

`LoopStmt` carries `isPostTest_ = false` (which is what the `while` annotation in the dump means). For a `do { … } while (cond);` form, the same node would dump as `LoopStmt do-while` and `isPostTest_` would be `true`. The condition and body slots are in the same order regardless.

---

### Example 9 — for loop with declaration init

**Source**

```c
int main() {
    int sum = 0;
    for (int i = 0; i < 10; i = i + 1) {
        sum = sum + i;
    }
    return sum;
}
```

**Parse tree**

```
TranslationUnit
  FunctionDecl 'main' returns int
    CompoundStmt
      VarDecl 'sum' int
        LiteralExpr INT_LITERAL '0'
      ForStmt
        VarDecl 'i' int
          LiteralExpr INT_LITERAL '0'
        BinaryExpr LT
          IdentifierExpr 'i'
          LiteralExpr INT_LITERAL '10'
        AssignExpr ASSIGN
          IdentifierExpr 'i'
          BinaryExpr PLUS
            IdentifierExpr 'i'
            LiteralExpr INT_LITERAL '1'
        CompoundStmt
          ExprStmt
            AssignExpr ASSIGN
              IdentifierExpr 'sum'
              BinaryExpr PLUS
                IdentifierExpr 'sum'
                IdentifierExpr 'i'
      ReturnStmt
        IdentifierExpr 'sum'
```

`ForStmt` has four ordered slots: `init_`, `cond_`, `step_`, `body_`. In this example all four are populated. The init slot is a `VarDecl` (the declaration form) rather than an `ExprStmt` — `parseForStmt` checks `isDeclarationStart(peek())` and routes to declaration parsing if so.

**Symbol-table effect of the for-statement scope**

| scope | name | dataType |
|---|---|---|
| 0 | main | int |
| 1 | sum | int |
| **2** | **i** | **int** |

`for (int i = …)` opens a new scope (the for-statement scope), so `i` lives at `scope = 2`, not at the function-body scope. This means `i` is visible inside the body of the loop but not after the loop ends — matching C99 semantics. Because the loop body is itself a `CompoundStmt`, it would push *another* scope (3) — but no declarations live there in this example, so nothing is added to the table at scope 3.

---

### Example 10 — shadowing

**Source**

```c
int x = 1;
int main() {
    int x = 2;
    {
        int x = 3;
        return x;
    }
}
```

**Parse tree**

```
TranslationUnit
  VarDecl 'x' int
    LiteralExpr INT_LITERAL '1'
  FunctionDecl 'main' returns int
    CompoundStmt
      VarDecl 'x' int
        LiteralExpr INT_LITERAL '2'
      CompoundStmt
        VarDecl 'x' int
          LiteralExpr INT_LITERAL '3'
        ReturnStmt
          IdentifierExpr 'x'
```

The AST is unambiguous about the three `x` declarations because they're three separate `VarDecl` nodes in three different `CompoundStmt`s. But the more interesting outcome is the symbol table:

| Key (scope, name) | dataType | line |
|---|---|---|
| (-1, "main") | (empty) | 2 |
| (-1, "x") | (empty) | 1 |
| (0, "main") | int | 2 |
| (0, "x") | int | 1 |
| (1, "x") | int | 3 |
| (2, "x") | int | 5 |

Three distinct parser-lane entries for `x` coexist — at file scope (`0`), at the function body scope (`1`), and at the inner block scope (`2`). The composite `(scope, name)` key allows all three. The lexer-lane entry for `x` is dedup'd to one record holding the *first* occurrence at line 1, regardless of how many times `x` appears in source.

If a future semantic-analysis phase wanted to resolve the `return x;` at line 6, it would walk the active scope chain at that point — `[0, 1, 2]` — and call `symbolTable_.lookup("x", {0, 1, 2})`. `lookup` walks innermost-first, finds `(2, "x")`, and returns it. The inner `x` shadows the outer two, exactly as C requires.

---

### Example 11 — error recovery in action

**Source** (several intentional errors, real test fixture `tests/parser/errors.c`)

```c
// (1) Missing ';' after declaration.
int x = 5

// (2) Same-scope redeclaration.
int x;

// (3) Missing identifier after the type specifier.
int = 99;

// (6) Bare operator with no rhs.
int withBareOp() {
    int a = 0;
    a = 2 + ;
    return a;
}
```

**Errors reported**

```
ParseError [4:1]  expected ';' after declaration ("int")
ParseError [4:5]  redeclaration of 'x' ("x")
ParseError [7:5]  expected identifier in declarator ("=")
ParseError [11:13] expected expression (";")
```

(Line numbers are illustrative — they'll match wherever the snippet is placed.)

**Parse tree (recovered)**

```
TranslationUnit
  VarDecl 'x' int
    LiteralExpr INT_LITERAL '5'
  VarDecl 'x' int
  FunctionDecl 'withBareOp' returns int
    CompoundStmt
      VarDecl 'a' int
        LiteralExpr INT_LITERAL '0'
      ExprStmt
        AssignExpr ASSIGN
          IdentifierExpr 'a'
          BinaryExpr PLUS
            LiteralExpr INT_LITERAL '2'
      ReturnStmt
        IdentifierExpr 'a'
```

What recovery did:

- Error (1) was reported at the next token after the missing `;`. The `VarDecl 'x' int` for the first declaration was still produced — the cursor didn't get stuck.
- Error (2) was reported when the second `int x;` collided with the first in the symbol table; the AST still records the redeclaration as a `VarDecl` so a later phase can audit it.
- Error (3) was reported when `parseDeclarator` saw `=` instead of an identifier. Because the resulting name was empty, `parseVarDecl` returned `nullptr` rather than emitting a phantom unnamed `VarDecl`. The trailing `;` was still consumed for forward progress.
- Error (4) was reported at the `;` that should have been the right-hand side of `2 +`. Crucially, `parsePrimary` did **not** consume the `;` — it left the terminator for `parseExprStmt`'s `expect(SEMICOLON)`, which then accepted it, *no cascading second error*. The `BinaryExpr PLUS` ends up with one rendered child (the `2`); the null right-hand side is correctly skipped by `dump()`.
- The next valid declaration (`int withBareOp() { … }`) parsed completely cleanly — recovery from the earlier errors didn't bleed into it.

The exact output above is what the test suite asserts on `tests/parser/errors.c` (with seven planted errors over the full fixture). The `errors.c` fixture passes the test `testFixtureErrorsReportsErrors`, which checks `parser.getErrors().size() >= 1` and `tu->getDecls().size() >= 1`.

---

## 7. The GUI

`MainWindow` (Qt 6) is a single-window application:

```text
+--------------------------------------------------+
|  &File   &Analyze                                |   ← menus + toolbar
+--------------------------------------------------+
|                |                                 |
|                |  +---------------------------+  |
|                |  |  LexerPanel               |  |
|   Source       |  |  Tokens / Symbol Table /  |  |
|   editor       |  |  Errors  (3 tabs)         |  |
|                |  +---------------------------+  |
|                |  +---------------------------+  |
|                |  |  ParserPanel              |  |
|                |  |  Parse Tree / Errors      |  |
|                |  |  (2 tabs)                 |  |
|                |  +---------------------------+  |
+--------------------------------------------------+
| Status: 15 token(s) | 2 identifier(s) | 0 lex   |
|         err | 1 decl(s) | 0 parse err          |
+--------------------------------------------------+
```

The layout is an outer horizontal `QSplitter` (editor on the left, results on the right). The right pane is itself a vertical `QSplitter` holding `LexerPanel` over `ParserPanel`.

### Actions

| Action | Shortcut | Behaviour |
|---|---|---|
| File → Open | Ctrl+O | Load a `.c` or `.h` file into the editor |
| File → Exit | Ctrl+Q | Quit |
| Analyze → Run Lexer | F5 | Run the lexer only; populate `LexerPanel`; leave `ParserPanel` untouched |
| Analyze → Run Parser | F6 | Run the full pipeline (lexer then parser); populate both panels |
| Analyze → Clear | Ctrl+R | Clear the editor and both panels |

The parser is *not* skipped when the lexer reports errors — it's given whatever token vector was produced and runs as best it can, so the user sees partial results alongside the lexer diagnostics.

### `LexerPanel` (`src/gui/LexerPanel.cpp`)

Three tabs:

- **Tokens** — five-column table: `#`, `Type`, `Lexeme`, `Line`, `Col`.
- **Symbol Table** — two-column table: `Identifier`, `First seen (line)`. Filters to lexer-lane entries (`scope == -1`) and sorts by source order.
- **Errors** — list of formatted `LexerError::toString()` strings.

Tab titles show counts: `"Tokens (15)"`, `"Symbol Table (2)"`, `"Errors (0)"`. The panel auto-switches to the Errors tab when any errors are present.

### `ParserPanel` (`src/gui/ParserPanel.cpp`)

Two tabs:

- **Parse Tree** — `QTreeWidget` populated by walking the result of `tu->dump()`. The dump's two-space indent encodes depth, which the populator reconstructs as `QTreeWidgetItem` parent/child relationships.
- **Errors** — list of formatted `ParseError::toString()` strings (same monospaced layout as the lexer's error tab).

Tab titles show counts (`"Parse Tree (3)"`, `"Errors (0)"`); auto-switch to errors when any are reported.

### Window icon

The `MainWindow` constructor calls `setWindowIcon(makeAppIcon())`, where `makeAppIcon()` builds a 256x256 `QPixmap` via `QPainter`: a rounded square with a blue radial gradient and a bold white "C" centred on it. Programmatic so there's no resource file to ship, and Qt rescales it for the title bar, taskbar, and Alt-Tab.

---

## 8. Building and testing

### Prerequisites

- CMake 3.21+
- A C++17 compiler (MinGW-w64 g++, MSVC, or clang work)
- Qt 6 (Widgets module). On Windows, `aqtinstall` or the Qt online installer; on CI, Qt's `install-qt-action`

### Build

```bash
cmake -S . -B build
cmake --build build
```

This produces:

- `build/bin/ccompiler.exe` — the GUI application
- `build/bin/test_lexer.exe`, `build/bin/test_parser.exe` — unit-test executables
- `build/lib/libcommon.a`, `liblexer.a`, `libparser.a`, `libgui.a` — the static libraries

### Run

```bash
build/bin/ccompiler.exe
```

### Tests

```bash
ctest --test-dir build --output-on-failure
```

Two test suites:

- **`LexerTests`** — 51 assertions covering the full lexer surface (keywords, identifiers, numeric/char/string literals, all operator forms, comments, error recovery, symbol-table behaviour with the composite-key API).
- **`ParserTests`** — 177 assertions covering `TokenStream`, `ParseError`, `Parser` construction, `Type`, AST construction and `dump()` output, the full expression cascade with precedence and associativity, every statement form (with edge cases like dangling-else, empty bodies, `for(;;)`, stray-semicolon handling), declaration parsing (vars, functions, params, comma chains, pointer reset per declarator, qualifier handling, storage-class discarding, `unsigned`/`long long` modifier handling), symbol-table integration (file scope, function-body shared scope, three-level shadowing, redeclaration detection), error reporting on missing `;` / `)` / identifiers, and end-to-end `parse()` against the four `.c` fixtures in `tests/parser/`.

Each test asserts via a small `ASSERT_EQ(actual, expected, label)` macro that increments local `passed`/`failed` counters; the program exits with `0` only if `failed == 0`.

### Test fixtures

The four `.c` files under `tests/parser/` (`declarations.c`, `expressions.c`, `statements.c`, `errors.c`) are loaded by `runFixture` via the compile-time `PARSER_FIXTURES_DIR` define. The first three should parse with zero errors; `errors.c` is intentionally broken and asserts that recovery still produces a non-empty TU and at least one reported error.

---

## 9. Limitations

The frontend handles a deliberate subset of C. The following are accepted **syntactically** but discarded (not surfaced on AST nodes or in the symbol table beyond the parse trace):

- Storage-class keywords: `auto`, `register`, `static`, `extern`, `typedef`
- Sign modifiers: `signed`, `unsigned`
- Length redundancy: the second `long` in `long long`
- The `restrict` qualifier
- The `inline` function specifier — currently not parsed at all (would need to be added to `isDeclarationStart`)

The following are **not implemented** in v1:

- Hex (`0x...`), octal, or binary integer literals; floating-point exponent (`1e10`); literal suffixes (`1L`, `2.5f`)
- `struct`, `union`, `enum`, `typedef` (recognised as keywords but not parsable as type-shapes)
- Function prototypes (`int f();` without a body) — only definitions are accepted
- Multi-declarator init in `for`: `for (int i = 0, j = 0; …)` only takes a single declarator
- The comma operator at expression level
- `switch` statements, `goto`, labels
- Preprocessor directives (`#include`, `#define`, etc.)
- Scope-aware identifier resolution against uses — the parser doesn't call `lookup`; the `IdentifierExpr` node carries only the name. This is deferred to a future semantic-analysis phase.
- Type checking of any kind (assignment compatibility, function arity, etc.)
- Code generation

The parser's recovery strategy is **lenient** in the same way clang/gcc/rustc are: it builds partial AST nodes around errors so downstream tools can still operate on broken code. A future `bool isInvalidDecl()`-style flag could mark partially-recovered nodes if a later phase wanted to refuse them.
