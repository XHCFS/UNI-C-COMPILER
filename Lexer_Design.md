# Lexer Design for the C Compiler Project

The lexer must:
- read source code from left to right,
- apply the maximal munch rule,
- recognize the supported C subset tokens,
- ignore whitespace and comments,
- build the lexer-side part of the shared symbol table,
- report lexical errors with line and column information,
- recover when possible and continue scanning,
- return a clean token stream for later parser use.

---

## 1) File Organization

```text
src/
├── common/
│   ├── Token.h / Token.cpp
│   ├── SymbolEntry.h / SymbolEntry.cpp
│   └── SymbolTable.h / SymbolTable.cpp
│
├── lexer/
│   ├── Lexer.h / Lexer.cpp
│   ├── CharStream.h / CharStream.cpp
│   ├── TokenClassifier.h / TokenClassifier.cpp
│   ├── LexerError.h / LexerError.cpp
│   ├── ReservedWords.h / ReservedWords.cpp
│   └── (no separate Lexer_Design file in src)
│
├── gui/
│   ├── MainWindow.h / MainWindow.cpp
│   ├── LexerPanel.h / LexerPanel.cpp
│   └── GUI.h / GUI.cpp   ← legacy stub, superseded by MainWindow
│
└── main.cpp

tests/
└── lexer/
    ├── basic.c                 ← variables, literals, return
    ├── operators.c             ← all operators and compound assignments
    ├── control_flow.c          ← if/else, while, do-while, for, switch
    ├── comments.c              ← line and block comments
    └── errors.c                ← intentional lexical errors for recovery testing
```

---

## 2) High-Level Lexer Design

The lexer design is split into four parts:

### A. Input handling
Reads characters safely and tracks the current position.

### B. Token recognition
Recognizes identifiers, keywords, literals, operators, delimiters, and comments.

### C. Shared compiler data
Stores tokens and builds the lexer-owned part of the symbol table.

### D. Error handling
Stores lexical errors and continues scanning when recovery is possible.

---

## 3) `src/common/`

These files are shared data structures that the lexer produces and updates.

---

## 3.1 `TokenType` and `Token`
**File:** `src/common/Token.h`, `src/common/Token.cpp`

### Purpose of `TokenType`
Defines the complete token vocabulary produced by the lexer.
The ordering of enumerators within each group is load-bearing: `isKeyword()`, `isLiteral()`, `isOperator()`, and `isDelimiter()` all rely on contiguous enum ranges.

### `TokenType`

```cpp
enum class TokenType {

    // --- C23 Keywords ---

    // Control flow
    KW_IF, KW_ELSE, KW_FOR, KW_WHILE, KW_DO,
    KW_SWITCH, KW_CASE, KW_DEFAULT,
    KW_BREAK, KW_CONTINUE, KW_RETURN, KW_GOTO,

    // Type specifiers
    KW_VOID, KW_CHAR, KW_SHORT, KW_INT, KW_LONG,
    KW_FLOAT, KW_DOUBLE, KW_SIGNED, KW_UNSIGNED,

    // Type qualifiers
    KW_CONST, KW_VOLATILE, KW_RESTRICT,

    // Storage class specifiers
    KW_AUTO, KW_REGISTER, KW_STATIC, KW_EXTERN, KW_TYPEDEF,

    // Function specifiers
    KW_INLINE,

    // Composite types
    KW_STRUCT, KW_UNION, KW_ENUM,

    // Expression keywords
    KW_SIZEOF,

    // C99 keywords
    KW__BOOL, KW__COMPLEX, KW__IMAGINARY,

    // C11 keywords
    KW__ALIGNAS, KW__ALIGNOF, KW__ATOMIC, KW__GENERIC,
    KW__NORETURN, KW__STATIC_ASSERT, KW__THREAD_LOCAL,

    // C23 keywords
    KW_ALIGNAS, KW_ALIGNOF, KW_BOOL, KW_CONSTEXPR,
    KW_FALSE, KW_NULLPTR, KW_STATIC_ASSERT, KW_THREAD_LOCAL,
    KW_TRUE, KW_TYPEOF, KW_TYPEOF_UNQUAL,
    KW__BITINT, KW__DECIMAL32, KW__DECIMAL64, KW__DECIMAL128,

    // --- Identifier ---
    IDENTIFIER,

    // --- Literals ---
    INT_LITERAL,
    FLOAT_LITERAL,
    CHAR_LITERAL,
    STRING_LITERAL,

    // --- Arithmetic operators ---
    PLUS,               // +
    MINUS,              // -
    STAR,               // *
    SLASH,              // /
    PERCENT,            // %
    INCREMENT,          // ++
    DECREMENT,          // --

    // --- Bitwise operators ---
    BITWISE_AND,        // &
    BITWISE_OR,         // |
    BITWISE_XOR,        // ^
    BITWISE_NOT,        // ~
    LSHIFT,             // <<
    RSHIFT,             // >>

    // --- Logical operators ---
    LOGICAL_AND,        // &&
    LOGICAL_OR,         // ||
    NOT,                // !

    // --- Relational operators ---
    EQ,                 // ==
    NEQ,                // !=
    LT,                 // <
    GT,                 // >
    LE,                 // <=
    GE,                 // >=

    // --- Assignment operators ---
    ASSIGN,             // =
    PLUS_ASSIGN,        // +=
    MINUS_ASSIGN,       // -=
    STAR_ASSIGN,        // *=
    SLASH_ASSIGN,       // /=
    PERCENT_ASSIGN,     // %=
    AND_ASSIGN,         // &=
    OR_ASSIGN,          // |=
    XOR_ASSIGN,         // ^=
    LSHIFT_ASSIGN,      // <<=
    RSHIFT_ASSIGN,      // >>=

    // --- Member access operators ---
    DOT,                // .
    ARROW,              // ->

    // --- Other operators ---
    QUESTION,           // ?
    COLON,              // :

    // --- Delimiters ---
    LPAREN,             // (
    RPAREN,             // )
    LBRACE,             // {
    RBRACE,             // }
    LBRACKET,           // [
    RBRACKET,           // ]
    SEMICOLON,          // ;
    COMMA,              // ,

    // --- Special ---
    INVALID
};
```

> **Note:** There is no `END_OF_FILE` token. `tokenize()` detects EOF via `CharStream::eof()` and discards the internal sentinel before returning.

### What `TokenType` does
- gives one fixed set of token categories,
- assigns each C23 keyword its own distinct variant so the parser can switch on type without comparing lexeme strings,
- avoids string-based token comparisons in later phases,
- makes token checks simple in tests, GUI, and later compiler phases.

### Purpose of `Token`
Represents one token produced by the lexer.

### Attributes
- `TokenType type_` — token classification.
- `std::string lexeme_` — exact source text.
- `int line_` — 1-based line where the token starts.
- `int column_` — 1-based column where the token starts.

### Methods

| Function | Purpose | What exactly it does |
|---|---|---|
| `Token(TokenType type, std::string lexeme, int line, int column)` | Construct a token object | Creates one token and stores its category, original lexeme, line, and column. |
| `TokenType getType() const` | Return the token category | Returns the enum value that tells what kind of token this is. |
| `const std::string& getLexeme() const` | Return the original text | Returns the exact substring taken from the source code, such as `count`, `+`, or `3.14`. |
| `int getLine() const` | Return token line | Returns the 1-based source line where the token begins. |
| `int getColumn() const` | Return token column | Returns the 1-based source column where the token begins. |
| `bool isKeyword() const` | Check whether token is a keyword | Returns `true` when `type_` falls within the contiguous `KW_IF`…`KW__DECIMAL128` range. |
| `bool isLiteral() const` | Check whether token is a literal | Returns `true` when `type_` falls within the contiguous `INT_LITERAL`…`STRING_LITERAL` range. |
| `bool isOperator() const` | Check whether token is an operator | Returns `true` when `type_` falls within the contiguous `PLUS`…`COLON` range. |
| `bool isDelimiter() const` | Check whether token is a delimiter | Returns `true` when `type_` falls within the contiguous `LPAREN`…`COMMA` range. |
| `std::string toString() const` | Build a readable representation | Produces `[TYPE 'lexeme' L<line>:C<col>]` for debugging, logs, or GUI display. |

### Free function
| Function | Purpose |
|---|---|
| `std::string tokenTypeToString(TokenType type)` | Maps every `TokenType` enumerator to its name as a string (e.g. `KW_INT` → `"KW_INT"`); returns `"UNKNOWN"` for unhandled values. |

---

## 3.2 `SymbolEntry`
**File:** `src/common/SymbolEntry.h`, `src/common/SymbolEntry.cpp`

### Purpose
Stores one identifier record inside the symbol table.

### Important ownership rule
The **lexer** fills only:
- `token_` (which carries the identifier name, line, and column)

The **parser later fills**:
- `dataType_`
- `scope_`

So the symbol entry is created by the lexer but stays only partially completed during the lexer phase.

### Attributes
- `Token token_` — the identifier token, carrying name, line, and column.
- `std::string dataType_` — left empty during lexing; filled by the parser.
- `int scope_` — initialised to `-1` during lexing; filled by the parser.

### Methods

| Function | Purpose | What exactly it does |
|---|---|---|
| `SymbolEntry(const Token& token)` | Construct one symbol-table entry | Stores the token; initialises `dataType_` to empty and `scope_` to `-1`. |
| `const std::string& getName() const` | Return identifier name | Delegates to `token_.getLexeme()`. |
| `int getLine() const` | Return first appearance line | Delegates to `token_.getLine()`. |
| `const Token& getToken() const` | Return the stored token | Returns the full `Token` object for this identifier. |
| `void setDataType(const std::string& type)` | Assign data type | Stores the resolved C type string; intended for the parser phase. |
| `const std::string& getDataType() const` | Return data type | Returns the stored type string, or empty if not yet set. |
| `void setScope(int scopeLevel)` | Assign scope level | Stores the lexical scope depth; intended for the parser phase. |
| `int getScope() const` | Return scope level | Returns the stored scope depth, or `-1` if not yet assigned. |

---

## 3.3 `SymbolTable`
**File:** `src/common/SymbolTable.h`, `src/common/SymbolTable.cpp`

### Purpose
Acts as the identifier table built during the lexer phase.

### Important design rule
Only the operations the lexer actually needs are included:
- inserting identifiers,
- checking whether an identifier already exists,
- retrieving an existing entry,
- returning all entries,
- clearing the table for a new run.

Scope management, type assignment, and declaration context belong to the parser phase.

### Attributes
- `std::unordered_map<std::string, SymbolEntry> entries_` — hash map keyed by identifier name.

### Methods

| Function | Purpose | What exactly it does |
|---|---|---|
| `SymbolTable()` | Initialize the symbol table | Creates an empty table. |
| `bool insert(const Token& token)` | Insert a new identifier entry | Inserts a new `SymbolEntry` keyed on `token.getLexeme()` and returns `true`. Returns `false` without inserting if the name already exists. Scope resolution is left to the parser. |
| `bool exists(const std::string& name) const` | Check whether identifier is already stored | Returns `true` if the name is present in `entries_`. |
| `SymbolEntry* find(const std::string& name)` | Retrieve an identifier entry | Returns a mutable pointer to the entry, or `nullptr` if not found. |
| `const std::unordered_map<std::string, SymbolEntry>& getAllEntries() const` | Return the full table | Returns a read-only reference to the whole map for GUI display or later phases. |
| `void clear()` | Reset the table | Removes all entries; called before re-running the lexer on new input. |

---

## 4) `src/lexer/`

These files implement the lexer itself.

---

## 4.1 `CharStream`
**File:** `src/lexer/CharStream.h`, `src/lexer/CharStream.cpp`

### Purpose
Provides controlled character-by-character access to the source code while tracking line and column.

### Attributes
- `std::string source_` — immutable copy of the full source text.
- `size_t pos_` — index of the next character to be read.
- `int line_` — current 1-based line number; incremented on each `'\n'`.
- `int column_` — current 1-based column number; reset to `1` after each `'\n'`.

### Methods

| Function | Purpose | What exactly it does |
|---|---|---|
| `CharStream(const std::string& source)` | Construct the input stream | Stores the full source text and initialises `pos_=0`, `line_=1`, `column_=1`. |
| `char peek(int offset = 0) const` | Look ahead without consuming | Returns the character at `pos_ + offset` without moving the cursor. Returns `'\0'` when out of range. |
| `char get()` | Consume one character | Returns the current character, advances `pos_`, increments `line_` on `'\n'` or `column_` otherwise. |
| `bool eof() const` | Check end of file | Returns `true` when `pos_ >= source_.size()`. |
| `int getLine() const` | Return current line | Returns the 1-based line of the character that the next `get()` will return. |
| `int getColumn() const` | Return current column | Returns the 1-based column of the character that the next `get()` will return. |
| `bool match(char expected)` | Match and consume one exact character | Consumes the current character and returns `true` only if it equals `expected`. |
| `std::string consumeWhile(std::function<bool(char)> pred)` | Consume a character run | Calls `get()` repeatedly while `pred(peek())` is true; returns the accumulated string. |

---

## 4.2 `ReservedWords`
**File:** `src/lexer/ReservedWords.h`, `src/lexer/ReservedWords.cpp`

### Purpose
Maps every C keyword string to its specific `TokenType` so the lexer can reclassify identifier-shaped text into the correct keyword token.

### Attributes
- `std::unordered_map<std::string, TokenType> keywordMap_` — maps each keyword string (e.g. `"int"`) to its `KW_*` type (e.g. `KW_INT`).

### Methods

| Function | Purpose | What exactly it does |
|---|---|---|
| `ReservedWords()` | Initialize the keyword map | Fills `keywordMap_` with every C keyword from C89 through C23 (50+ entries). |
| `bool isKeyword(const std::string& word) const` | Check whether lexeme is reserved | Returns `true` if the word is a key in `keywordMap_`. O(1) average. |
| `TokenType getKeywordType(const std::string& word) const` | Return the specific keyword token type | Returns the mapped `KW_*` type, or `TokenType::INVALID` if the word is not a keyword. |

---

## 4.3 Token classifier functions
**File:** `src/lexer/TokenClassifier.h`, `src/lexer/TokenClassifier.cpp`

### Purpose
Stateless free functions that classify individual characters or character pairs. Used by `Lexer::nextToken()` to dispatch to the correct scanner and by `scanOperatorOrDelimiter()` to classify tokens.

### Functions

| Function | Purpose | What exactly it does |
|---|---|---|
| `bool isIdentifierStart(char c)` | Check valid identifier start | Returns `true` for letters and `'_'`. |
| `bool isIdentifierPart(char c)` | Check valid identifier continuation | Returns `true` for letters, digits, and `'_'`. |
| `bool isDigit(char c)` | Check decimal digit | Returns `true` for `'0'`–`'9'`. |
| `bool isWhitespace(char c)` | Check whitespace | Returns `true` for space, tab, `'\n'`, `'\r'`, form-feed, and vertical-tab. |
| `bool isOperatorStart(char c)` | Check possible operator start | Returns `true` for: `+ - * / % = ! < > & \| ^ ~ . ? :` |
| `bool isDelimiterChar(char c)` | Check punctuation token | Returns `true` for: `( ) { } [ ] ; ,` |
| `TokenType classifySingleChar(char c)` | Classify one-character token | Maps a single operator or delimiter character to its `TokenType`; returns `INVALID` for unknown characters. |
| `TokenType classifyDoubleChar(char first, char second)` | Classify two-character token | Checks all valid two-character C operators (`++`, `--`, `->`, `==`, `!=`, `<=`, `>=`, `<<`, `>>`, `&&`, `\|\|`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `\|=`, `^=`); returns `INVALID` if no match. |

> **Note:** The function is `isDelimiterChar`, not `isDelimiter`.

---

## 4.4 `LexerError`
**File:** `src/lexer/LexerError.h`, `src/lexer/LexerError.cpp`

### Purpose
Represents one lexical error detected while scanning. Errors are collected into a vector so the lexer can recover and continue rather than aborting on the first bad token.

### Attributes
- `std::string message_` — human-readable description of what went wrong.
- `int line_` — 1-based source line where the error was detected.
- `int column_` — 1-based source column where the error was detected.
- `std::string offendingText_` — the exact text that triggered the error, if available.

### Methods

| Function | Purpose | What exactly it does |
|---|---|---|
| `LexerError(std::string message, int line, int column, std::string offendingText = "")` | Construct an error object | Stores all four fields; `offendingText` is optional. |
| `std::string toString() const` | Build readable error text | Returns `"LexerError [line:col] message (\"offendingText\")"`. The parenthesised text is omitted when `offendingText_` is empty. |
| `const std::string& getMessage() const` | Return the message | Returns the plain error description without location or offending text. |
| `int getLine() const` | Return error line | Returns the 1-based line where the error was detected. |
| `int getColumn() const` | Return error column | Returns the 1-based column where the error was detected. |

---

## 4.5 `Lexer`
**File:** `src/lexer/Lexer.h`, `src/lexer/Lexer.cpp`

### Purpose
The main lexical analyzer. Reads source code, produces tokens, updates the symbol table, and records lexical errors.

### Attributes
- `CharStream input_` — character-level reader over the source text.
- `ReservedWords reservedWords_` — keyword lookup table; owned by the Lexer.
- `SymbolTable& symbolTable_` — shared symbol table; referenced, not owned.
- `std::vector<Token> tokens_` — accumulated token output, built by `tokenize()`.
- `std::vector<LexerError> errors_` — accumulated error list, built during scanning.

### Public methods

| Function | Purpose | What exactly it does |
|---|---|---|
| `Lexer(const std::string& source, SymbolTable& symbolTable)` | Construct lexer | Binds the source to a `CharStream` and stores a reference to the shared symbol table. |
| `std::vector<Token> tokenize()` | Run full lexical analysis | Loops calling `nextToken()` until EOF; discards the EOF sentinel (empty-lexeme `INVALID` token) and returns the final token vector. No `END_OF_FILE` token is appended. |
| `const std::vector<LexerError>& getErrors() const` | Return collected lexical errors | Returns the full error list accumulated during the most recent `tokenize()` call. |

### Private methods

| Function | Purpose | What exactly it does |
|---|---|---|
| `Token nextToken()` | Scan one token | Alternately calls `skipWhitespace()` and `skipComment()` in an inner loop until neither makes progress, then dispatches on the next character to the appropriate scanner. On invalid characters, calls `recoverFromInvalidCharacter()` and loops instead of returning; on EOF returns a sentinel with empty lexeme and `INVALID` type which `tokenize()` discards. |
| `void skipWhitespace()` | Ignore whitespace | Calls `input_.get()` for each whitespace character. |
| `bool skipComment()` | Ignore comment text | Detects `//` (consumes to end of line) or `/* */` (consumes to closing delimiter, records error on unterminated). Returns `true` if a comment was consumed, `false` otherwise. |
| `Token scanIdentifierOrKeyword()` | Scan identifier-like text | Calls `input_.consumeWhile(isIdentifierPart)`, then `reservedWords_.getKeywordType()`. Returns the `KW_*` token for keywords or `IDENTIFIER` for user names; inserts `IDENTIFIER` tokens into `symbolTable_`. |
| `Token scanNumber()` | Scan numeric literal | Reads digits, then optionally a `.` followed by more digits. Returns `INT_LITERAL` or `FLOAT_LITERAL`. Reports and returns `INVALID` for invalid suffixes like `3.14abc`. |
| `Token scanCharLiteral()` | Scan character literal | Reads `'`, handles one character or escape sequence, reads closing `'`. Records an error for unterminated literals. |
| `Token scanStringLiteral()` | Scan string literal | Reads `"`, handles characters and escape sequences until closing `"`. On EOF or newline before closing, calls `addError` then `recoverFromUnterminatedString()`. |
| `Token scanOperatorOrDelimiter()` | Scan operator or punctuation | Applies maximal munch: checks three characters for `<<=`/`>>=` first, then two characters via `classifyDoubleChar`, then one character via `classifySingleChar`. |
| `Token makeToken(TokenType type, const std::string& lexeme, int line, int column)` | Build token consistently | Thin wrapper around the `Token` constructor. |
| `void addError(const std::string& message, int line, int column, const std::string& text = "")` | Store a lexical error | Appends a new `LexerError` to `errors_`. |
| `void recoverFromInvalidCharacter()` | Recover after unknown input | Consumes the invalid character **and any immediately following identifier characters** as one unit (maximal munch), then records them as a single `"invalid character sequence"` error. For example, `@yz` becomes one error, not `@` + identifier `yz`. Does not produce a token; `nextToken()` loops to the next character. |
| `void recoverFromUnterminatedString()` | Recover after unterminated string | Advances the stream past the remainder of the current line so the next line can be scanned cleanly. |
| `void recoverFromUnterminatedComment()` | Placeholder | Empty body; unterminated block-comment recovery is handled inline inside `skipComment()` which records the error and returns when EOF is hit. Present for design completeness only. |

### Actual flow inside `tokenize()`
1. While `!input_.eof()`, call `nextToken()`.
2. If the returned token's lexeme is non-empty, push it into `tokens_`. The empty-lexeme sentinel produced at EOF is discarded here.
3. Return `tokens_`.

> **Note:** There is no `END_OF_FILE` token appended. The parser uses `isAtEnd()` (checking `pos_ >= tokens_.size()`) to detect the end of the stream.

---

## 5) `src/gui/`

Only lexer-related GUI responsibilities are described here.

---

## 5.1 `MainWindow`
**File:** `src/gui/MainWindow.h`, `src/gui/MainWindow.cpp`

### Purpose
The application's main window. It is a `QMainWindow` subclass that hosts the source editor on the left and the `LexerPanel` on the right inside a horizontal splitter.

### Responsibilities
- Load source file text via a file dialog (`onOpenFile`).
- Run the lexer on the editor contents (`onRunLexer`): creates a fresh `SymbolTable` and `Lexer`, calls `tokenize()`, forwards results to `LexerPanel`, and updates the status bar with token/identifier/error counts.
- Clear the editor and panel (`onClear`).
- Build menus (File: Open, Exit; Lexer: Run Lexer F5, Clear Ctrl+R) and toolbar.

### Widgets
- `QPlainTextEdit* editor_` — monospaced source code editor.
- `LexerPanel* lexerPanel_` — tabbed output panel.

---

## 5.2 `LexerPanel`
**File:** `src/gui/LexerPanel.h`, `src/gui/LexerPanel.cpp`

### Purpose
A `QWidget` subclass that displays lexer results in three tabs.

### Tabs
- **Tokens** — `QTableWidget` with columns: `#`, `Type`, `Lexeme`, `Line`, `Col`.
- **Symbol Table** — `QTableWidget` with columns: `Identifier`, `First seen (line)`. Entries are sorted by first-occurrence line/column so they appear in source order.
- **Errors** — `QListWidget` of formatted `LexerError::toString()` strings.

### Behaviour
- Tab titles show counts: `"Tokens (15)"`, `"Symbol Table (2)"`, `"Errors (0)"`.
- Automatically switches to the Errors tab when any errors are present.
- `clear()` empties all three tables and resets tab titles.

---

## 5.3 `GUI` (legacy stub)
**File:** `src/gui/GUI.h`, `src/gui/GUI.cpp`

This class pre-dates the Qt implementation. Its `run()` prints a placeholder message to stdout. `showLexerOutput()` and `showParserOutput()` dump results to stdout. It is not used by `main.cpp` and exists only as a design reference.

---

## 6) Parser Compatibility Requirements

### The lexer guarantees the following:
- Every identifier-shaped name is emitted as `IDENTIFIER` unless it is a reserved keyword, in which case it is emitted with its exact `KW_*` token type.
- Function names and variable names are **not** separated by the lexer; both remain `IDENTIFIER` and are distinguished later by context.
- Every token carries exact 1-based `line` and `column` information.
- The token stream has **no** `END_OF_FILE` token; the parser detects end-of-stream via `isAtEnd()`.
- Multi-character operators use longest-match (maximal munch), including three-character compound shift-assignment operators (`<<=`, `>>=`).
- Invalid character sequences are consumed as a unit (maximal munch on the error side too), so `@yz` produces one error token, not `@` plus identifier `yz`.
- Comments and whitespace are removed cleanly and do not appear as tokens.
- Lexical recovery continues far enough to still produce as many valid tokens as possible.
- Tokens are returned in source order without gaps or reordering.
- Each symbol-table entry is constructed from the identifier token alone; `name` and `line` are derived from that token rather than stored separately.
- `dataType` and `scope` in `SymbolEntry` are left at their defaults (`""` and `-1`) by the lexer; the parser fills them in.

---

## 7) Final Design Summary

The lexer is centred around a `Lexer` class that reads characters from `CharStream`, uses the free functions in `TokenClassifier.h` and the `ReservedWords` map to recognise lexemes, produces `Token` objects, registers identifiers in `SymbolTable` via `insert(token)`, records problems as `LexerError` objects, and returns a clean token vector. Every C23 keyword has its own `KW_*` token type so the parser can switch on type without comparing strings. The symbol table exposes only the operations needed during lexical analysis, and `dataType` and `scope` are left to be filled in by later phases. There is no `END_OF_FILE` token; end-of-stream is signalled structurally by the end of the token vector.
