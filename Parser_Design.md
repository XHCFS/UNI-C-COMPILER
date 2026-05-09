# Parser Design for the C Compiler Project

The parser must:
- consume the token stream produced by the lexer (`std::vector<Token>`),
- apply C precedence and associativity for expressions,
- recognize the supported C subset (declarations, statements, expressions),
- build an abstract syntax tree (AST) for valid input,
- write a fresh `SymbolTable` entry for each declared identifier through `declare()` at the active lexical scope,
- report parse errors with line and column information,
- recover when possible and continue parsing,
- return a clean AST and error list for later phases.

---

## 1) File Organization

```text
src/
├── common/
│   ├── Token.h / Token.cpp                ← unchanged (produced by the lexer)
│   ├── SymbolEntry.h / SymbolEntry.cpp    ← shared shape; parser writes new entries via declare()
│   ├── SymbolTable.h / SymbolTable.cpp    ← composite-keyed (scope, name); see Lexer_Design.md §3.3
│   ├── Ast.h / Ast.cpp                    ← AST node hierarchy
│   └── Type.h / Type.cpp                  ← C type representation
│
├── parser/
│   ├── Parser.h / Parser.cpp
│   ├── TokenStream.h / TokenStream.cpp
│   ├── ParserClassifier.h / ParserClassifier.cpp
│   ├── ParseError.h / ParseError.cpp
│   └── (no separate Parser_Design file in src)
│
├── gui/
│   ├── MainWindow.h / MainWindow.cpp      ← Run Parser action added
│   ├── ParserPanel.h / ParserPanel.cpp    ← parse tree + parse errors
│   ├── LexerPanel.h / LexerPanel.cpp      ← unchanged
│   └── GUI.h / GUI.cpp                    ← legacy stub
│
└── main.cpp

tests/
└── parser/
    ├── test_parser.cpp        ← unit-test runner with ASSERT_EQ macros, exercises parse()
    ├── declarations.c         ← variable and function declarations
    ├── expressions.c          ← full precedence cascade, unary/postfix, calls, indexing
    ├── statements.c           ← if/else, while, do-while, for, return, break, continue
    └── errors.c               ← intentional parse errors for recovery testing
```

---

## 2) High-Level Parser Design

The parser is the second stage of a sequential pipeline that the GUI runs end-to-end:

```text
Source code  →  Lexer  →  vector<Token>  →  Parser  →  unique_ptr<TranslationUnit>
                  ↓                            ↓
            LexerPanel                    ParserPanel
                            ↘            ↙
                          shared SymbolTable
                  (composite-keyed by (scope, name);
                   lexer writes at scope=-1 via insert(),
                   parser writes at scope≥0 via declare())
```

The parser does not rescan the source string and does not own the token vector — it borrows the lexer's output by reference, exactly as the lexer borrows the source string by reference inside `CharStream`.

The parser design is split into five parts:

### A. Token consumption
Reads tokens from the lexer's output safely and tracks the current cursor position with the same line/column accounting the lexer already emitted on each token.

### B. Grammar dispatch
Recursive-descent with one method per non-terminal. Expression precedence is encoded as a chain of methods, one per precedence level — the parser-side analogue of the lexer's longest-match dispatch in `scanOperatorOrDelimiter()`.

### C. AST construction
Builds typed AST nodes that own their children via `unique_ptr`. Every node carries the 1-based source line and column copied from the first token that produced it, so later phases can produce diagnostics with the same coordinates the lexer emits.

### D. Shared compiler data
Writes one new `SymbolTable` entry per declared identifier through `SymbolTable::declare(token, scope, dataType)`, keyed on the composite `(scope, name)`. The lexer's earlier `(-1, name)` entries are independent and stay in place; the parser does not mutate them.

### E. Error handling
Stores parse errors in a `vector<ParseError>` and continues parsing when recovery is possible. Errors carry the same four fields and the same `toString()` format as `LexerError`.

---

## 3) `src/common/` additions

These files are shared data structures. `Token` and `SymbolEntry` are unchanged. `SymbolTable` is shared with the lexer and is composite-keyed `(scope, name)` to support shadowing — its full API is documented in `Lexer_Design.md` §3.3. The parser writes new entries through `declare()` rather than mutating the lexer's `(-1, name)` records.

---

## 3.1 `Type`
**File:** `src/common/Type.h`, `src/common/Type.cpp`

### Purpose
Represents the resolved C type of a declared identifier or expression. Built by `Parser::parseDeclSpecifiers()` and `parseDeclarator()` from the keyword tokens (`KW_INT`, `KW_CONST`, `STAR`, etc.) the lexer emitted, and stored on the relevant AST node and on the matching `SymbolEntry` via `setDataType(type.toString())`.

### `TypeKind`

```cpp
enum class TypeKind {
    VOID, CHAR, SHORT, INT, LONG,
    FLOAT, DOUBLE, BOOL,
    INVALID
};
```

### Attributes
- `TypeKind kind_` — the underlying scalar kind.
- `int pointerDepth_` — number of `*` levels (`0` for non-pointer, `1` for `T*`, `2` for `T**`).
- `bool isConst_` — set when `const` appears in the declaration specifiers.
- `bool isVolatile_` — set when `volatile` appears.

### Methods

| Function | Purpose | What exactly it does |
|---|---|---|
| `Type()` | Construct an empty/invalid type | Initialises `kind_=INVALID`, `pointerDepth_=0`, both flags `false`. |
| `Type(TypeKind kind)` | Construct a plain scalar type | Initialises `kind_` and leaves the rest at their defaults. |
| `TypeKind getKind() const` | Return the underlying kind | Returns `kind_`. |
| `int getPointerDepth() const` | Return pointer level | Returns `pointerDepth_`. |
| `bool isConst() const` / `isVolatile() const` | Qualifier predicates | Return the corresponding flag. |
| `void setKind(TypeKind k)` | Mutator used by the parser | Stores `k` in `kind_`. |
| `void addPointer()` | Add one `*` level | Increments `pointerDepth_`. |
| `void setConst(bool v)` / `setVolatile(bool v)` | Mutators used by the parser | Store the supplied value. |
| `std::string toString() const` | Build a readable representation | Produces a string such as `"const int*"` for debugging, GUI display, and `SymbolEntry::setDataType()`. |

### Free function
| Function | Purpose |
|---|---|
| `std::string typeKindToString(TypeKind kind)` | Maps every `TypeKind` enumerator to its C name as a string (e.g. `INT` → `"int"`); returns `"unknown"` for unhandled values. Mirrors the role `tokenTypeToString` plays for `TokenType`. |

---

## 3.2 `AstNode` and concrete subclasses
**File:** `src/common/Ast.h`, `src/common/Ast.cpp`

### Purpose of `AstNode`
Common base class for every AST node. Carries the source location and exposes a single `dump()` method so the GUI parse-tree view and any text-mode pretty-printer can render the whole tree polymorphically.

### Attributes (base)
- `int line_` — 1-based line of the first token that produced this node.
- `int column_` — 1-based column of the first token that produced this node.

### Methods (base)

| Function | Purpose | What exactly it does |
|---|---|---|
| `AstNode(int line, int column)` | Construct an AST node | Stores `line_` and `column_`. |
| `virtual ~AstNode() = default` | Polymorphic destructor | Allows correct destruction through `unique_ptr<AstNode>`. |
| `int getLine() const` / `int getColumn() const` | Return source position | Return `line_` / `column_`. |
| `virtual void dump(std::ostream& os, int indent) const = 0` | Render the subtree | Each concrete class writes its own header line at the supplied indent, then recurses on its children with `indent + 1`. Plays the same role for AST nodes that `Token::toString()` plays for tokens. |

The hierarchy is split into four categories: `Expr`, `Stmt`, `Decl`, and the top-level `TranslationUnit`. `Expr`, `Stmt`, and `Decl` are abstract subclasses of `AstNode` that exist only to make the type system reflect the C grammar; they introduce no extra fields.

> **Operator storage rule:** `BinaryExpr`, `UnaryExpr`, and `AssignExpr` store their operator as the `TokenType` value lifted directly from the consumed operator token. `LiteralExpr::kind_` and `JumpStmt::kind_` use the same `TokenType` discriminator pattern. The AST never introduces a parallel "operator kind" or "node kind" enum, since `TokenType` already distinguishes every form the lexer emits.

---

### 3.2.a Expressions (`Expr` subclasses)

Each expression node stores its operands as `unique_ptr<Expr>` and copies its line/column from the first contributing token.

| Class | Attributes | Description |
|---|---|---|
| `LiteralExpr` | `TokenType kind_; std::string lexeme_` | Any of the four literal forms. `kind_` is `INT_LITERAL`, `FLOAT_LITERAL`, `CHAR_LITERAL`, or `STRING_LITERAL`; `lexeme_` is the exact source text (digits, `'…'`, or `"…"`). No numeric conversion or escape translation at parse time. |
| `IdentifierExpr` | `std::string name_` | The identifier lexeme. The parser does **not** look up the entry here; semantic analysis does. |
| `BinaryExpr` | `TokenType op_; std::unique_ptr<Expr> lhs_, rhs_` | Any binary operator from `PLUS`…`LOGICAL_OR`. |
| `UnaryExpr` | `bool isPostfix_; TokenType op_; std::unique_ptr<Expr> operand_` | Both prefix and postfix unary forms. Prefix (`isPostfix_ = false`): `+ - ! ~ ++ -- * &`. Postfix (`isPostfix_ = true`): `++` and `--` only. Consumers that need to distinguish branch on `isPostfix_`. |
| `AssignExpr` | `TokenType op_; std::unique_ptr<Expr> target_, value_` | Any assignment from `ASSIGN`…`RSHIFT_ASSIGN`. Right-associative. |
| `ConditionalExpr` | `std::unique_ptr<Expr> cond_, then_, else_` | Ternary `? :`. Right-associative. |
| `CallExpr` | `std::unique_ptr<Expr> callee_; std::vector<std::unique_ptr<Expr>> args_` | Function call `f(a, b, c)`. |
| `IndexExpr` | `std::unique_ptr<Expr> array_, index_` | Subscript `a[i]`. |

---

### 3.2.b Statements (`Stmt` subclasses)

| Class | Attributes | Description |
|---|---|---|
| `CompoundStmt` | `std::vector<std::unique_ptr<AstNode>> items_` | Brace-delimited block; `items_` may hold either `Decl` or `Stmt` nodes interleaved. Stray `;` tokens between items are silently skipped (no null-statement node — see `parseStatement()` in §4.4). |
| `IfStmt` | `std::unique_ptr<Expr> cond_; std::unique_ptr<Stmt> then_, else_` | `else_` is `nullptr` when the `else` branch is absent; either branch can also be `nullptr` when the body is a bare `;` (no dedicated null-statement node). The dangling-else binds to the nearest unmatched `if`. |
| `LoopStmt` | `bool isPostTest_; std::unique_ptr<Expr> cond_; std::unique_ptr<Stmt> body_` | Both `while` and `do-while`. `isPostTest_ = false` for `while (cond) body`; `isPostTest_ = true` for `do body while (cond);`. `body_` may be `nullptr` for `while (cond) ;`. |
| `ForStmt` | `std::unique_ptr<AstNode> init_; std::unique_ptr<Expr> cond_, step_; std::unique_ptr<Stmt> body_` | Any of `init_`, `cond_`, `step_`, `body_` may be `nullptr` for omitted clauses or an empty body. `init_` is an `AstNode` so it can be either an expression statement or a declaration. |
| `ReturnStmt` | `std::unique_ptr<Expr> value_` | `value_` is `nullptr` for bare `return;`. |
| `JumpStmt` | `TokenType kind_` | `break;` (`kind_ = KW_BREAK`) or `continue;` (`kind_ = KW_CONTINUE`). One class for both — they're structurally identical leaves. |
| `ExprStmt` | `std::unique_ptr<Expr> expr_` | An expression followed by `;`. |

---

### 3.2.c Declarations (`Decl` subclasses)

| Class | Attributes | Description |
|---|---|---|
| `VarDecl` | `Type type_; std::string name_; std::unique_ptr<Expr> init_` | Variable declaration with optional initializer. `init_` is `nullptr` when absent. |
| `ParamDecl` | `Type type_; std::string name_` | One parameter in a function signature. `name_` may be empty for unnamed parameters. |
| `FunctionDecl` | `Type returnType_; std::string name_; std::vector<std::unique_ptr<ParamDecl>> params_; std::unique_ptr<CompoundStmt> body_` | Function definition. `body_` is `nullptr` for prototypes (deferred to v2). |

---

### 3.2.d Top-level node

| Class | Attributes | Description |
|---|---|---|
| `TranslationUnit` | `std::vector<std::unique_ptr<Decl>> decls_` | Root of the AST. Holds one `Decl` per external declaration in source order. |

---

## 3.3 `SymbolTable` (parser writes)

### Three-lane sharing
The symbol table is one `unordered_map<Key, SymbolEntry, KeyHash>` shared between phases (defined in `Lexer_Design.md` §3.3). Three lanes coexist, distinguished by the `scope` half of the key:

| Lane | Owner | Written by |
|---|---|---|
| `scope = -1` | Lexer | `insert(token)` for first-seen identifier names |
| `scope = 0` | Parser | `declare(token, 0, dataType)` for file-scope declarations |
| `scope ≥ 1` | Parser | `declare(token, n, dataType)` for nested-scope declarations (one fresh ID per `{`) |

Two `int x` declarations in different scopes produce two distinct entries — `(0, "x")` and `(2, "x")` — that coexist freely in the same map.

### Parser-side scope tracking
The parser maintains two members:
- `int nextScopeId_ = 0` — monotonic counter for fresh scope IDs. File scope occupies `0`.
- `std::vector<int> scopeStack_ = {0}` — chain of currently active scope IDs, innermost at the back. File scope is preloaded.

Push/pop rules:
- `parseCompoundStmt()` (general case) pushes a fresh ID before iterating items: `scopeStack_.push_back(++nextScopeId_)`. Pops on the matching `}`.
- `parseFunctionDecl()` pushes one fresh ID for the function's block scope before parsing the parameter list. Per C99 6.2.1, the parameters and the function body share that single block scope, so `parseFunctionDecl` parses the body via the helper `parseBlockItems()` (§4.4), which does **not** push another scope.

### Parser-side writes
- `parseVarDecl()` and `parseParamDecl()` call `symbolTable_.declare(nameTok, scopeStack_.back(), type.toString())`. A `false` return means the same `(scope, name)` is already present — the parser reports `"redeclaration of 'name'"`.
- `parseFunctionDecl()` calls `declare()` for the function name at the *outer* scope (whichever scope was active before the function pushed its own).

### Identifier-use resolution is deferred
The parser does **not** call `lookup()`. `IdentifierExpr` records only the lexeme. Resolving uses to declarations is a semantic-analysis concern; semantic analysis later calls `symbolTable_.lookup(name, activeScopes)` with whatever active-scope chain it is currently traversing.

---

## 4) `src/parser/`

These files implement the parser itself. Their layout mirrors `src/lexer/`: a stream class for input, a classifier file of free functions, an error class, and the main analyzer class.

---

## 4.1 `TokenStream`
**File:** `src/parser/TokenStream.h`, `src/parser/TokenStream.cpp`

### Purpose
Provides controlled token-by-token access to the lexer's output while exposing the current 1-based line and column. Plays the same role for the parser that `CharStream` plays for the lexer: a thin cursor with peek-ahead, consume, and EOF detection.

### Attributes
- `const std::vector<Token>& tokens_` — non-owning reference to the lexer's output (the parser does not copy the vector).
- `size_t pos_` — index of the next token to be read.
- `Token eofSentinel_` — sentinel returned by `peek()` and `get()` when `pos_ >= tokens_.size()`. Built once in the constructor with `TokenType::INVALID`, empty lexeme, and the line/column of the last real token (or `1:1` if the stream is empty). Plays the same role that `'\0'` plays for `CharStream::peek()` past end of input.

### Methods

| Function | Purpose | What exactly it does |
|---|---|---|
| `TokenStream(const std::vector<Token>& tokens)` | Construct the input stream | Stores the reference, sets `pos_=0`, builds `eofSentinel_`. |
| `const Token& peek(int offset = 0) const` | Look ahead without consuming | Returns `tokens_[pos_ + offset]`, or `eofSentinel_` when out of range. Same shape as `CharStream::peek(int offset = 0)`. |
| `const Token& get()` | Consume one token | Returns the current token and advances `pos_`. Returns `eofSentinel_` past end without advancing. Same shape as `CharStream::get()`. |
| `bool eof() const` | Check end of stream | Returns `pos_ >= tokens_.size()`. Same shape as `CharStream::eof()`. |
| `int getLine() const` | Return current line | Returns the 1-based line of the next-to-be-consumed token (or the sentinel's line at EOF). |
| `int getColumn() const` | Return current column | Returns the 1-based column of the next-to-be-consumed token. |
| `bool match(TokenType expected)` | Match-and-consume | Calls `get()` and returns `true` only if its type equals `expected`; otherwise leaves the cursor unchanged. Same semantics as `CharStream::match(char)`. |
| `bool check(TokenType expected) const` | Match-without-consume | Returns `true` iff `peek().getType() == expected`. |

---

## 4.2 `ParserClassifier`
**File:** `src/parser/ParserClassifier.h`, `src/parser/ParserClassifier.cpp`

### Purpose
Stateless free functions that classify individual `TokenType` values. Used by `Parser::parseStatement()` and `parseDeclSpecifiers()` to dispatch on the next token, and by the expression cascade to decide whether a token starts a unary operator, an assignment, etc. Plays the same structural role for tokens that `TokenClassifier` plays for characters.

### Functions

| Function | Purpose | What exactly it does |
|---|---|---|
| `bool isTypeSpecifier(TokenType t)` | Check type-specifier keyword | Returns `true` for `KW_VOID`, `KW_CHAR`, `KW_SHORT`, `KW_INT`, `KW_LONG`, `KW_FLOAT`, `KW_DOUBLE`, `KW_SIGNED`, `KW_UNSIGNED`, `KW__BOOL`, `KW_BOOL`. |
| `bool isTypeQualifier(TokenType t)` | Check type-qualifier keyword | Returns `true` for `KW_CONST`, `KW_VOLATILE`, `KW_RESTRICT`. |
| `bool isStorageClassSpecifier(TokenType t)` | Check storage-class keyword | Returns `true` for `KW_AUTO`, `KW_REGISTER`, `KW_STATIC`, `KW_EXTERN`, `KW_TYPEDEF`. |
| `bool isDeclarationStart(TokenType t)` | Check declaration-leading token | Returns `isTypeSpecifier(t) \|\| isTypeQualifier(t) \|\| isStorageClassSpecifier(t)`. Used by `parseStatement()` to decide between a declaration and an expression statement inside a compound block. |
| `bool isAssignmentOperator(TokenType t)` | Check assignment operator | Returns `true` for `ASSIGN`, `PLUS_ASSIGN`, `MINUS_ASSIGN`, `STAR_ASSIGN`, `SLASH_ASSIGN`, `PERCENT_ASSIGN`, `AND_ASSIGN`, `OR_ASSIGN`, `XOR_ASSIGN`, `LSHIFT_ASSIGN`, `RSHIFT_ASSIGN`. |
| `bool isUnaryPrefixOperator(TokenType t)` | Check prefix-unary operator | Returns `true` for `PLUS`, `MINUS`, `NOT`, `BITWISE_NOT`, `STAR`, `BITWISE_AND`, `INCREMENT`, `DECREMENT`. |
| `bool isPostfixOperator(TokenType t)` | Check postfix operator | Returns `true` for `INCREMENT`, `DECREMENT`. |
| `bool isLiteralToken(TokenType t)` | Check literal token | Returns `true` for `INT_LITERAL`, `FLOAT_LITERAL`, `CHAR_LITERAL`, `STRING_LITERAL`. Note: this checks raw `TokenType`; the existing `Token::isLiteral()` member uses the same range. |

---

## 4.3 `ParseError`
**File:** `src/parser/ParseError.h`, `src/parser/ParseError.cpp`

### Purpose
Represents one parse error detected while building the AST. Errors are collected into a vector so the parser can recover and continue rather than aborting on the first bad token. The class is structurally identical to `LexerError` and carries the same four fields with the same names; the GUI and any text-mode reporter render `ParseError::toString()` and `LexerError::toString()` in the same format.

### Attributes
- `std::string message_` — human-readable description of what went wrong.
- `int line_` — 1-based source line where the error was detected.
- `int column_` — 1-based source column where the error was detected.
- `std::string offendingText_` — the exact text that triggered the error, if available (typically the offending token's lexeme).

### Methods

| Function | Purpose | What exactly it does |
|---|---|---|
| `ParseError(std::string message, int line, int column, std::string offendingText = "")` | Construct an error object | Stores all four fields; `offendingText` is optional. Same signature as `LexerError`. |
| `std::string toString() const` | Build readable error text | Returns `"ParseError [line:col] message ("offendingText")"`. The parenthesised text is omitted when `offendingText_` is empty. Identical formatting to `LexerError::toString()` except for the `"ParseError"` prefix. |
| `const std::string& getMessage() const` | Return the message | Returns the plain error description without location or offending text. |
| `int getLine() const` | Return error line | Returns the 1-based line where the error was detected. |
| `int getColumn() const` | Return error column | Returns the 1-based column where the error was detected. |

---

## 4.4 `Parser`
**File:** `src/parser/Parser.h`, `src/parser/Parser.cpp`

### Purpose
The main syntactic analyzer. Consumes the lexer's token stream, builds the AST, writes a fresh `SymbolEntry` per declared identifier into the shared `SymbolTable` via `declare()`, and records parse errors.

### Attributes
- `TokenStream input_` — token-level reader over the lexer's output (analogue of `CharStream input_` in `Lexer`).
- `SymbolTable& symbolTable_` — shared symbol table; referenced, not owned. Written through `declare()` for each declaration the parser recognises.
- `std::vector<ParseError> errors_` — accumulated error list, built during `parse()` (analogue of `Lexer::errors_`).
- `int nextScopeId_ = 0` — monotonic counter for fresh scope IDs assigned by `parseCompoundStmt()` and `parseFunctionDecl()`. File scope occupies `0`.
- `std::vector<int> scopeStack_ = {0}` — currently active scope chain, innermost at the back. The back element is the destination scope for `declare()` calls; semantic analysis later passes (a snapshot of) this vector to `SymbolTable::lookup()`.

### Public methods

| Function | Purpose | What exactly it does |
|---|---|---|
| `Parser(const std::vector<Token>& tokens, SymbolTable& symbolTable)` | Construct parser | Binds the token vector to a `TokenStream`, stores the symbol-table reference, initialises `nextScopeId_=0` and `scopeStack_={0}`. Same shape as `Lexer::Lexer(const string&, SymbolTable&)`. |
| `std::unique_ptr<TranslationUnit> parse()` | Run full syntactic analysis | Loops calling `parseExternalDeclaration()` until `input_.eof()`, collecting non-null `Decl` results into a fresh `TranslationUnit`, and returns it. On an unrecoverable failure inside a top-level production it calls `synchronize()` and continues from the next safe point. The returned tree is always non-null even when errors were recorded. |
| `const std::vector<ParseError>& getErrors() const` | Return collected parse errors | Returns the full error list accumulated during the most recent `parse()` call. Same shape as `Lexer::getErrors()`. |

### Private methods — top-level dispatch

| Function | Purpose | What exactly it does |
|---|---|---|
| `std::unique_ptr<Decl> parseExternalDeclaration()` | Parse one top-level declaration | Calls `parseDeclSpecifiers()`, then a `parseDeclarator()` head, then looks at the next token: `LPAREN` → `parseFunctionDecl()`, otherwise → `parseVarDecl()` (with optional initializer and `,` chain). Reports an error and calls `synchronize()` on missing `SEMICOLON`. |
| `Type parseDeclSpecifiers()` | Parse the leading specifier run | Loops over `isDeclarationStart()` tokens, building one `Type`. Type-specifier keywords (`void`/`char`/`short`/`int`/`long`/`float`/`double`/`bool`/`_Bool`) set `kind_`; `const`/`volatile` set the matching qualifier flag; sign modifiers (`signed`/`unsigned`), the second `long` in `long long`, `restrict`, and storage-class keywords are consumed and discarded as documented in §3.1. Reports a duplicate-specifier error and keeps the first kind when conflicts arise. |
| `void parseDeclarator(Type& type, std::string& outName)` | Parse `*…* identifier` | Consumes leading `STAR` tokens (calling `type.addPointer()` for each), then expects an `IDENTIFIER` and writes its lexeme into `outName`. |
| `std::unique_ptr<FunctionDecl> parseFunctionDecl(Type returnType, std::string name, int line, int column)` | Parse function definition | Calls `symbolTable_.declare(funcTok, scopeStack_.back(), …)` for the function name at the **outer** scope, then `scopeStack_.push_back(++nextScopeId_)` to open the function's block scope. Parses `LPAREN params RPAREN` (each parameter is `declare()`d at the new scope), then parses the body via `parseBlockItems()` — which does **not** push another scope, since C99 6.2.1 places parameters and body in the same block scope. Pops the scope on exit. |
| `std::unique_ptr<VarDecl> parseVarDecl(Type type, std::string name, int line, int column)` | Parse single variable declaration | Optionally consumes `ASSIGN` and an initializer expression, then calls `symbolTable_.declare(nameTok, scopeStack_.back(), type.toString())`. Reports `"redeclaration of 'name'"` if `declare()` returns `false`. |
| `std::vector<std::unique_ptr<ParamDecl>> parseParamList()` | Parse `( … )` parameter list | Returns an empty vector for `()` or `(void)`, otherwise loops on `COMMA`-separated `parseDeclSpecifiers + parseDeclarator` pairs. Each parameter is registered via `symbolTable_.declare(paramTok, scopeStack_.back(), type.toString())` into the function's block scope. |

### Private methods — statements

| Function | Purpose | What exactly it does |
|---|---|---|
| `std::unique_ptr<Stmt> parseStatement()` | Dispatch on first token | Looks at `peek().getType()`: `LBRACE` → `parseCompoundStmt`, `KW_IF` → `parseIfStmt`, `KW_WHILE`/`KW_DO` → `parseLoopStmt`, `KW_FOR` → `parseForStmt`, `KW_RETURN` → `parseReturnStmt`, `KW_BREAK`/`KW_CONTINUE` → `parseJumpStmt`, `SEMICOLON` → consume the `;` silently and return `nullptr` (no null-statement node; callers tolerate `nullptr` as "empty"), otherwise → `parseExprStmt`. Mirrors the dispatch shape of `Lexer::nextToken()`. |
| `std::unique_ptr<CompoundStmt> parseCompoundStmt()` | Parse `{ … }` block (general case) | Consumes `LBRACE`, executes `scopeStack_.push_back(++nextScopeId_)` to open a fresh scope, delegates the inner item loop to `parseBlockItems()`, consumes `RBRACE`, then `scopeStack_.pop_back()`. Reports unterminated-block errors on EOF and calls `synchronize()`. |
| `std::unique_ptr<CompoundStmt> parseBlockItems()` | Inner-loop helper | Loops until `RBRACE` or EOF: when `isDeclarationStart(peek())` is true, pushes a declaration into `items_`; otherwise calls `parseStatement()` and pushes the result if non-null (`parseStatement()` returns `nullptr` for a stray `;`, which is silently skipped). Does **not** touch `scopeStack_`; the surrounding push/pop is the caller's responsibility (`parseCompoundStmt` for general blocks, `parseFunctionDecl` for function bodies sharing the parameter scope). |
| `std::unique_ptr<IfStmt> parseIfStmt()` | Parse `if (…) S [else S]` | Standard recursive descent with the dangling-else bound to the nearest unmatched `if`. `then_` and `else_` accept `nullptr` from `parseStatement()` (the empty-body case). |
| `std::unique_ptr<LoopStmt> parseLoopStmt()` | Parse `while`/`do-while` | On `KW_WHILE`: consumes `while ( cond ) body` and returns a `LoopStmt` with `isPostTest_ = false`. On `KW_DO`: consumes `do body while ( cond ) ;` and returns a `LoopStmt` with `isPostTest_ = true`. Reports a missing-`while` error and synchronises if the do-while trailer is malformed. |
| `std::unique_ptr<ForStmt> parseForStmt()` | Parse `for (init; cond; step) S` | `init` is either a declaration (when `isDeclarationStart`) or an expression statement; any of `cond`, `step`, `body` may be empty. |
| `std::unique_ptr<ReturnStmt> parseReturnStmt()` | Parse `return [expr];` | `value_` is `nullptr` when the next token is `SEMICOLON`. |
| `std::unique_ptr<JumpStmt> parseJumpStmt()` | Parse `break;` / `continue;` | Consumes either `KW_BREAK` or `KW_CONTINUE` (storing it as `kind_`), then expects `SEMICOLON`. |
| `std::unique_ptr<ExprStmt> parseExprStmt()` | Parse `expr;` | Calls `parseExpression()` then `expect(SEMICOLON, …)`. |

### Private methods — expressions (precedence cascade)

Each method calls the next-higher-precedence method, then loops while the current operator class matches. This is the parser's analogue of the lexer's longest-match dispatch in `scanOperatorOrDelimiter()`: every call reads as much as legally fits at this precedence level before returning.

| Function | Precedence (low → high) | What exactly it does |
|---|---|---|
| `std::unique_ptr<Expr> parseExpression()` | top — assignment | Calls `parseAssignment()`. (Comma operator deferred to v2.) |
| `std::unique_ptr<Expr> parseAssignment()` | assignment, right-assoc | Parses a `parseConditional()` left-hand side; if the next token satisfies `isAssignmentOperator()`, consumes it and recursively calls `parseAssignment()` for the right-hand side. |
| `std::unique_ptr<Expr> parseConditional()` | `? :`, right-assoc | Parses `parseLogicalOr()`; on `QUESTION`, parses the `then` (`parseExpression()`), expects `COLON`, parses the `else` (`parseConditional()`). |
| `std::unique_ptr<Expr> parseLogicalOr()` | `\|\|`, left-assoc | Loops over `LOGICAL_OR` calling `parseLogicalAnd()`. |
| `std::unique_ptr<Expr> parseLogicalAnd()` | `&&`, left-assoc | Loops over `LOGICAL_AND` calling `parseBitwiseOr()`. |
| `std::unique_ptr<Expr> parseBitwiseOr()` | `\|`, left-assoc | Loops over `BITWISE_OR` calling `parseBitwiseXor()`. |
| `std::unique_ptr<Expr> parseBitwiseXor()` | `^`, left-assoc | Loops over `BITWISE_XOR` calling `parseBitwiseAnd()`. |
| `std::unique_ptr<Expr> parseBitwiseAnd()` | `&`, left-assoc | Loops over `BITWISE_AND` calling `parseEquality()`. |
| `std::unique_ptr<Expr> parseEquality()` | `==`, `!=`, left-assoc | Loops over `EQ`, `NEQ` calling `parseRelational()`. |
| `std::unique_ptr<Expr> parseRelational()` | `< > <= >=`, left-assoc | Loops over `LT`, `GT`, `LE`, `GE` calling `parseShift()`. |
| `std::unique_ptr<Expr> parseShift()` | `<< >>`, left-assoc | Loops over `LSHIFT`, `RSHIFT` calling `parseAdditive()`. |
| `std::unique_ptr<Expr> parseAdditive()` | `+ -`, left-assoc | Loops over `PLUS`, `MINUS` calling `parseMultiplicative()`. |
| `std::unique_ptr<Expr> parseMultiplicative()` | `* / %`, left-assoc | Loops over `STAR`, `SLASH`, `PERCENT` calling `parseUnary()`. |
| `std::unique_ptr<Expr> parseUnary()` | prefix unary | If `isUnaryPrefixOperator(peek())`, consumes the operator and recurses into `parseUnary()` for the operand; otherwise delegates to `parsePostfix()`. |
| `std::unique_ptr<Expr> parsePostfix()` | postfix `++ -- ( ) [ ]` | Calls `parsePrimary()`, then loops: `INCREMENT`/`DECREMENT` → `UnaryExpr` with `isPostfix_ = true`; `LPAREN` → `CallExpr` (parses comma-separated argument list and expects `RPAREN`); `LBRACKET` → `IndexExpr` (parses index and expects `RBRACKET`). |
| `std::unique_ptr<Expr> parsePrimary()` | atoms | Dispatches on `peek().getType()`: `INT_LITERAL`/`FLOAT_LITERAL`/`CHAR_LITERAL`/`STRING_LITERAL` → `LiteralExpr` with the matching `kind_`; `IDENTIFIER` → `IdentifierExpr`; `LPAREN` → consumes `LPAREN`, calls `parseExpression()`, expects `RPAREN`, and returns the inner expression directly (grouping is preserved structurally by precedence — no dedicated `ParenExpr` node is introduced). Any other token reports `"expected expression"` and returns `nullptr` after a single-token recovery skip. |

### Private methods — helpers

| Function | Purpose | What exactly it does |
|---|---|---|
| `void addError(const std::string& message, int line, int column, const std::string& text = "")` | Store a parse error | Appends a new `ParseError` to `errors_`. Identical signature and semantics to `Lexer::addError`. |
| `bool expect(TokenType type, const std::string& message)` | Require a specific token | If `input_.match(type)` is true, returns `true`; otherwise calls `addError(message, peek().getLine(), peek().getColumn(), peek().getLexeme())` and returns `false` without consuming. |
| `void synchronize()` | Recover after a parse error | Consumes tokens until the cursor is at a synchronisation point: a `SEMICOLON` (which is itself consumed), or any of `LBRACE`, `RBRACE`, `KW_IF`, `KW_WHILE`, `KW_FOR`, `KW_DO`, `KW_RETURN`, `KW_BREAK`, `KW_CONTINUE` (left for the next `parseStatement()` to see), or EOF. Plays the same structural role for the parser that `recoverFromInvalidCharacter()` and `recoverFromUnterminatedString()` play for the lexer. |

### Actual flow inside `parse()`
1. Construct an empty `TranslationUnit` with line/col `1:1`.
2. While `!input_.eof()`, call `parseExternalDeclaration()`; push non-null results into `decls_`.
3. On a null result returned from a recovery path, call `synchronize()` and continue the loop.
4. Return the populated `TranslationUnit` regardless of whether `errors_` is empty.

> **Note:** The parser never throws across method boundaries; every grammar method returns a node (or `nullptr` after recovery) and any caller can decide whether to keep parsing the surrounding construct. There is no end-of-stream token in the input — the parser uses `input_.eof()` exactly as `Lexer::tokenize()` uses `input_.eof()` on its `CharStream`.

---

## 5) `src/gui/`

The same `MainWindow` hosts both phases. The right-hand side of the existing horizontal `QSplitter` (editor | results) becomes a vertical `QSplitter` containing the existing `LexerPanel` on top and the new `ParserPanel` on the bottom. The user runs the full pipeline from a single window: the editor feeds the lexer, the lexer's `vector<Token>` feeds the parser, and the two output panels update side-by-side.

---

## 5.1 `MainWindow` (unified lexer + parser window)
**File:** `src/gui/MainWindow.h`, `src/gui/MainWindow.cpp`

### Layout

```text
+--------------------------------------------------+
|  &File   &Analyze                                |   ← menus + toolbar
+--------------------------------------------------+
|                |                                 |
|                |  +---------------------------+  |
|                |  |  LexerPanel               |  |
|   Source       |  |  Tokens / Symbol Table /  |  |
|   editor       |  |  Errors  (3 tabs)         |  |
|   (QPlainText  |  +---------------------------+  |
|    Edit)       |  +---------------------------+  |
|                |  |  ParserPanel              |  |
|                |  |  Parse Tree / Errors      |  |
|                |  |  (2 tabs)                 |  |
|                |  +---------------------------+  |
+--------------------------------------------------+
| Status: 15 token(s) | 2 identifier(s) | 0 lex   |
|         err | 1 decl(s) | 0 parse err          |
+--------------------------------------------------+
```

The outer container is the existing horizontal `QSplitter` (editor on the left, results on the right). The right pane becomes a vertical `QSplitter` holding `lexerPanel_` (top) and `parserPanel_` (bottom).

### Added responsibilities (in addition to the lexer-only behaviour described in `Lexer_Design.md` §5.1)
- The existing `Lexer` menu is renamed to `Analyze` and gains a second action.
  - `Run Lexer` — `F5`, unchanged. Calls `onRunLexer`. Lexer-only run; leaves `parserPanel_` untouched.
  - `Run Parser` — `F6`. Calls `onRunParser`. Runs the full pipeline and updates both panels.
  - `Clear` — `Ctrl+R`, unchanged shortcut. Calls `onClear` (now also clears `parserPanel_`).
- `onRunParser` performs the full pipeline in this exact order:
  1. Read source from `editor_->toPlainText()`. If empty, show `"Nothing to analyze."` in the status bar and return.
  2. Construct a fresh `SymbolTable` and `Lexer`, call `tokenize()`, capture the returned `vector<Token>` and `lexer.getErrors()`.
  3. Forward the tokens, symbol table, and lexer errors to `lexerPanel_->showResults(...)` exactly as `onRunLexer` does.
  4. Construct a `Parser` over the same token vector and the same `SymbolTable`, call `parse()`, capture the returned `unique_ptr<TranslationUnit>` and `parser.getErrors()`.
  5. Forward the AST root and parse errors to `parserPanel_->showResults(...)`.
  6. Update the status bar with one line covering both phases:
     `"%1 token(s)  |  %2 identifier(s)  |  %3 lex err  |  %4 decl(s)  |  %5 parse err"`,
     formatted with the same `QString::arg` chain that `onRunLexer` already uses.
- `onRunLexer` (existing) is unchanged.
- `onClear` (existing) is extended with `parserPanel_->clear();` after `lexerPanel_->clear();`.
- Lexer errors do **not** abort the parser. The parser is given whatever token vector the lexer produced — even an empty one — so the user can see partial parser output alongside lexer errors.

### Added widgets
- `ParserPanel* parserPanel_` — placed below `lexerPanel_` in a vertical `QSplitter` on the right side of the main horizontal splitter. Constructed in `buildCentral()` immediately after `lexerPanel_`.

### Header changes
- `MainWindow.h` adds:
  - `private slots: void onRunParser();`
  - `private: ParserPanel* parserPanel_;`

---

## 5.2 `ParserPanel`
**File:** `src/gui/ParserPanel.h`, `src/gui/ParserPanel.cpp`

### Purpose
A `QWidget` subclass that displays parser results in two tabs. Layout, fonts, and tab-title-with-count behaviour mirror `LexerPanel` so the two panels feel uniform inside the unified window described in §5.1.

### Tabs
- **Parse Tree** — `QTreeWidget` populated by walking the AST and adding one `QTreeWidgetItem` per node, labelled with the node's class name plus a short payload (e.g. `BinaryExpr '+'`, `VarDecl 'x' int`, `IntLiteralExpr '42'`). Children are added recursively.
- **Errors** — `QListWidget` of formatted `ParseError::toString()` strings. Identical configuration to `LexerPanel::errorList_`: monospaced `Consolas 9`, populated by iterating the error list returned from `Parser::getErrors()`.

### Public API (mirrors `LexerPanel`)

| Function | Purpose | What exactly it does |
|---|---|---|
| `explicit ParserPanel(QWidget* parent = nullptr)` | Construct the panel | Creates and configures both tab widgets; identical structure to `LexerPanel`'s constructor. |
| `void showResults(const TranslationUnit* root, const std::vector<ParseError>& errors)` | Populate both tabs | Walks the AST to populate the tree, populates the error list, updates the tab titles with counts, and auto-switches to the Errors tab when `errors` is non-empty (otherwise switches to the Parse Tree tab). Same call shape as `LexerPanel::showResults`. |
| `void clear()` | Reset the panel | Empties both widgets and resets tab titles to `"Parse Tree"` and `"Errors"` (no counts). Switches back to the Parse Tree tab. Same shape as `LexerPanel::clear`. |

### Behaviour
- Tab titles show counts: `"Parse Tree (N)"` where `N` is `root->decls_.size()`, and `"Errors (M)"`. Same naming pattern as `LexerPanel`.
- Automatically switches to the Errors tab when any parse errors are present, otherwise switches to the Parse Tree tab. Same auto-switch logic as `LexerPanel::showResults`.

---

## 5.3 GUI build wiring
**File:** `src/gui/CMakeLists.txt`

`add_library(gui STATIC ...)` is extended with `ParserPanel.cpp`. The `target_link_libraries(gui PUBLIC common lexer parser ...)` line already names `parser`, so no further linker changes are required once the new `parser` static library is built from `src/parser/CMakeLists.txt`.

---

## 6) Downstream Compatibility Requirements

### The parser guarantees the following:
- Every AST node carries the exact 1-based `line` and `column` of the token that started it.
- The AST root is always a non-null `TranslationUnit`, even when `getErrors()` is non-empty; later phases can walk the partial tree.
- `BinaryExpr`, `UnaryExpr`, and `AssignExpr` store their operator as a `TokenType` value taken directly from the input token, so consumers can switch on operator without comparing strings. `LiteralExpr::kind_` and `JumpStmt::kind_` use the same `TokenType` discriminator pattern.
- Numeric, character, and string literals retain their exact source lexeme; numeric conversion is deferred to semantic analysis.
- Every declared identifier produces a fresh `SymbolEntry` in the shared `SymbolTable` keyed on `(scope, name)`, with `dataType_` set to `Type::toString()` and `scope_` mirroring the key.
- Two declarations of the same name in different scopes coexist as two distinct entries (e.g., `(0, "x")` and `(2, "x")`); same-scope redeclaration is rejected by `declare()` and reported as a parse error.
- The lexer's `(-1, name)` entries are not modified by the parser. They remain visible to the GUI's lexer panel and are intentionally invisible to `lookup()`, which walks only `scopeStack_` (real scope IDs, `≥ 0`).
- The parser does **not** resolve identifier uses against the symbol table; `IdentifierExpr` carries only the name. Name resolution is a semantic-analysis concern.
- Recovery skips ahead to the next `;`, brace boundary, or statement-leading keyword; the parser never reorders tokens or skips silently.
- Parse errors are reported with the same `[line:col] message ("text")` format that `LexerError` uses, so the GUI and any text-mode reporter render both kinds of errors identically.
- The parser does not consume an end-of-stream token (the lexer does not emit one); end-of-input is detected via `TokenStream::eof()` exactly as the lexer detects it via `CharStream::eof()`.

---

## 7) Final Design Summary

The parser is centred around a `Parser` class that consumes tokens through `TokenStream`, uses the free functions in `ParserClassifier.h` to dispatch on token kind, builds an AST whose nodes own their children via `unique_ptr`, writes one fresh `SymbolEntry` per declaration into the composite-keyed `SymbolTable` via `declare()`, records problems as `ParseError` objects with the same shape as `LexerError`, and returns a `TranslationUnit` root. Shadowing is supported because the table key is `(scope, name)`: the parser pushes a fresh scope ID on each `{` (except the function-body case, which reuses the parameter scope per C99 6.2.1) and pops on `}`. Expression precedence is encoded as a chain of one method per level, mirroring the lexer's per-character dispatch in `scanOperatorOrDelimiter()`. The grammar methods never throw; recovery is funnelled through a single `synchronize()` helper that plays the same role for the parser that `recoverFromInvalidCharacter()` and `recoverFromUnterminatedString()` play for the lexer. There is no `END_OF_FILE` token; end-of-stream is signalled structurally by `TokenStream::eof()`, exactly as the lexer signals end-of-input through `CharStream::eof()`. In the GUI, the parser shares a single `MainWindow` with the lexer: the user runs the full source-to-AST pipeline through the `Run Parser` action (F6), and `LexerPanel` and `ParserPanel` display tokens, symbol-table entries, parse tree, and errors stacked in the same window.
