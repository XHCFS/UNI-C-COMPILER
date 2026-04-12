# Lexer Design Plan for the C Compiler Project


The lexer must:
- read source code from left to right,
- apply the maximal munch rule,
- recognize the supported C subset tokens,
- ignore whitespace and comments,
- build the lexer-side part of the shared symbol table,
- report lexical errors with line and column information,
- recover when possible and continue scanning,
- return a clean token stream for later parser use,
- and produce an `END_OF_FILE` token at the end.

---

## 1) Updated File Organization

The lexer-related files are described here.

```text
src/
├── common/
│   ├── token.h
│   ├── symbol_entry.h
│   └── symbol_table.h
│
├── lexer/
│   ├── lexer.h
│   ├── lexer.cpp
│   ├── char_stream.h
│   ├── char_stream.cpp
│   ├── token_classifier.h
│   ├── token_classifier.cpp
│   ├── lexer_error.h
│   ├── lexer_error.cpp
│   ├── reserved_words.h
│   └── reserved_words.cpp
│
├── gui/
│   ├── main_window.h
│   ├── main_window.cpp
│   ├── lexer_panel.h
│   └── lexer_panel.cpp
│
└── main.cpp

tests/
└── lexer/
    ├── test_lexer.cpp
    ├── test_symbol_table.cpp
    └── test_lexical_errors.cpp
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
**File:** `src/common/token.h`

This file contains both the token enumeration and the token class.

### Purpose of `TokenType`
Defines the complete token vocabulary produced by the lexer.

### Suggested `TokenType`

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
    END_OF_FILE,
    INVALID
};
```

### What `TokenType` does
- gives one fixed set of token categories,
- assigns each C23 keyword its own distinct variant so the parser can switch on type without comparing lexeme strings,
- avoids string-based token comparisons in later phases,
- makes token checks simple in tests, GUI, and later compiler phases.

### Purpose of `Token`
Represents one token produced by the lexer.

### Attributes
- `TokenType type` — token category.
- `std::string lexeme` — exact source text.
- `int line` — line where the token starts.
- `int column` — column where the token starts.

### Methods

| Function | Purpose | What exactly it does |
|---|---|---|
| `Token(TokenType type, std::string lexeme, int line, int column)` | Construct a token object | Creates one token and stores its category, original lexeme, line, and column. |
| `TokenType getType() const` | Return the token category | Returns the enum value that tells what kind of token this is. |
| `const std::string& getLexeme() const` | Return the original text | Returns the exact substring taken from the source code, such as `count`, `+`, or `3.14`. |
| `int getLine() const` | Return token line | Returns the source line where the token begins. |
| `int getColumn() const` | Return token column | Returns the source column where the token begins. |
| `bool isKeyword() const` | Check whether token is a keyword | Returns `true` when the token type is any of the `KW_*` variants. |
| `bool isLiteral() const` | Check whether token is a literal | Returns `true` only for integer, floating-point, character, and string literals. |
| `bool isOperator() const` | Check whether token is an operator | Returns `true` for arithmetic, bitwise, logical, relational, assignment, and member-access operators. |
| `bool isDelimiter() const` | Check whether token is punctuation | Returns `true` for parentheses, braces, brackets, semicolon, and comma. |
| `std::string toString() const` | Build a readable representation | Converts the token into a formatted string for debugging, logs, tests, or GUI display. |

### What this file should do
- define every token kind the lexer may emit,
- give each keyword its own distinct type so the parser never needs lexeme string comparison,
- represent each token as an object,
- preserve source position for diagnostics,
- give later phases a clean token stream.

---

## 3.2 `SymbolEntry`
**File:** `src/common/symbol_entry.h`

### Purpose
Stores one identifier record inside the symbol table.

### Important ownership rule
The **lexer** fills only:
- `token` (which carries the identifier name, line, and column)

The **parser later fills**:
- `dataType`
- `scope`

So the symbol entry is created by the lexer, but it stays only partially completed during the lexer phase.

### Attributes
- `Token token` — the identifier token itself, which carries the name, line, and column as its lexeme and position fields.
- `std::string dataType` — left empty during lexing and filled by a later phase.
- `int scope` — left unset during lexing and filled by a later phase.

### Methods

| Function | Purpose | What exactly it does |
|---|---|---|
| `SymbolEntry(const Token& token)` | Construct one symbol-table entry | Creates a new symbol entry for an identifier found by the lexer, storing its token. Leaves `dataType` empty and `scope` unset. |
| `const std::string& getName() const` | Return identifier name | Returns the lexeme of the stored token, which is the exact identifier text. |
| `int getLine() const` | Return first appearance line | Returns the line number from the stored token. |
| `const Token& getToken() const` | Return identifier token | Returns the stored token object for that identifier entry. |
| `void setDataType(const std::string& type)` | Assign data type | Stores the identifier's resolved type string. This method is intended for use by the parser or semantic analyzer in a later phase and has no effect during lexing. |
| `const std::string& getDataType() const` | Return data type | Returns the stored type string, which is empty until a later phase fills it. |
| `void setScope(int scopeLevel)` | Assign scope level | Stores the scope depth at which the identifier was declared. This method is intended for use in a later phase and has no effect during lexing. |
| `int getScope() const` | Return scope level | Returns the stored scope level, which is unset until a later phase fills it. |

### What this file should do
- store exactly the identifier information that belongs to one symbol-table entry,
- derive name and line from the stored token to avoid redundant copies,
- let the lexer create entries using only the identifier token,
- leave parser-owned fields available for later completion.

---

## 3.3 `SymbolTable`
**File:** `src/common/symbol_table.h`

### Purpose
Acts as the identifier table built during the lexer phase.

### Important design rule
This class should include only the operations the lexer actually needs.

That means the symbol table in this plan should only support:
- inserting identifiers,
- checking whether an identifier already exists,
- retrieving an existing entry,
- returning all entries,
- and clearing the table for a new run.

Anything related to declaration context, scope management, or type assignment should not be part of the lexer workflow.

### Attributes
- `std::unordered_map<std::string, SymbolEntry> entries` — hashmap storing identifier records keyed by identifier name for fast lookup.

### Methods

| Function | Purpose | What exactly it does |
|---|---|---|
| `SymbolTable()` | Initialize the symbol table | Creates an empty symbol table ready to store identifier entries found by the lexer. |
| `bool insert(const Token& token)` | Insert a new identifier entry | Checks whether the identifier name (`token.getLexeme()`) is already stored in the hashmap. If it is not, creates a new `SymbolEntry` from the token, inserts it using the lexeme as the key, and returns `true`. If it already exists, leaves the table unchanged and returns `false`. |
| `bool exists(const std::string& name) const` | Check whether identifier is already stored | Searches the hashmap by identifier name and returns `true` if an entry with that name already exists, otherwise returns `false`. |
| `SymbolEntry* find(const std::string& name)` | Retrieve an identifier entry | Searches the hashmap for the identifier name and returns a pointer to its symbol-table entry if found. Returns `nullptr` if the name is not in the table. |
| `const std::unordered_map<std::string, SymbolEntry>& getAllEntries() const` | Return the full table | Returns a read-only reference to the whole hashmap so the GUI, tests, or later phases can inspect all stored symbol-table entries. |
| `void clear()` | Reset the table | Removes all stored entries from the hashmap so the lexer can start fresh when analyzing a new source file. |

### What the class should do
- let the lexer register identifiers as soon as they are recognized,
- make identifier lookup fast using a hashmap,
- avoid duplicate entries for the same identifier name,
- support symbol-table display in the GUI,
- keep data type and scope assignment outside the lexer phase.


---

## 4) `src/lexer/`

These files implement the lexer itself.

---

## 4.1 `CharStream`
**File:** `src/lexer/char_stream.h`, `src/lexer/char_stream.cpp`

### Purpose
Provides controlled character-by-character access to the source code while tracking position.

### Attributes
- `std::string source` — full input source code.
- `size_t pos` — current character index.
- `int line` — current line number.
- `int column` — current column number.

### Methods

| Function | Purpose | What exactly it does |
|---|---|---|
| `CharStream(const std::string& source)` | Construct the input stream | Stores the full source text and initializes the reading position at the start of the file. |
| `char peek(int offset = 0) const` | Look ahead without consuming | Returns the character at position `pos + offset` without changing the current position. Returns `'\0'` when `pos + offset` is at or past the end of the source, so callers never need to check bounds separately. |
| `char get()` | Consume one character | Returns the current character and then advances the position, updating line and column counters correctly. |
| `bool eof() const` | Check end of file | Returns `true` when the reading position has reached the end of the source string. |
| `int getLine() const` | Return current line | Returns the current line number tracked by the stream. |
| `int getColumn() const` | Return current column | Returns the current column number tracked by the stream. |
| `bool match(char expected)` | Match and consume one exact character | Checks whether the current character equals `expected`; if yes, consumes it and returns `true`, otherwise returns `false`. |
| `std::string consumeWhile(std::function<bool(char)> pred)` | Consume a character run | Repeatedly consumes characters while the predicate returns `true`, then returns the consumed substring. |

### What the class should do
- hide raw index manipulation from the lexer,
- make line and column tracking consistent,
- support lookahead for multi-character tokens,
- return `'\0'` for any out-of-bounds peek so callers never need separate bounds checks.

---

## 4.2 `ReservedWords`
**File:** `src/lexer/reserved_words.h`, `src/lexer/reserved_words.cpp`

### Purpose
Maps every C23 keyword string to its specific `TokenType` so the lexer can reclassify identifier-shaped text into the correct keyword token.

### Attributes
- `std::unordered_map<std::string, TokenType> keywordMap` — maps each keyword string to its dedicated `KW_*` token type.

### Methods

| Function | Purpose | What exactly it does |
|---|---|---|
| `ReservedWords()` | Initialize the keyword map | Fills `keywordMap` with every C23 keyword paired with its corresponding `KW_*` token type. |
| `bool isKeyword(const std::string& word) const` | Check whether lexeme is reserved | Returns `true` if the given string is present as a key in `keywordMap`. |
| `TokenType getKeywordType(const std::string& word) const` | Return the specific keyword token type | Looks up the word in `keywordMap` and returns its exact `KW_*` token type. Returns `INVALID` if the word is not a keyword. |

### What the class should do
- let the lexer scan identifier-shaped text first,
- then look up the result to reclassify it as the correct keyword type,
- return the exact `KW_*` token type so the parser can match on type directly without comparing lexeme strings.

---

## 4.3 Token classifier functions
**File:** `src/lexer/token_classifier.h`, `src/lexer/token_classifier.cpp`

### Purpose
Provides reusable helper logic for deciding what kind of token can start with a given character or character pair. All functions are free functions with no shared state.

### Functions

| Function | Purpose | What exactly it does |
|---|---|---|
| `bool isIdentifierStart(char c)` | Check valid identifier start | Returns `true` when the character can legally start an identifier, typically a letter or underscore. |
| `bool isIdentifierPart(char c)` | Check valid identifier continuation | Returns `true` when the character can appear after the first character of an identifier, typically a letter, digit, or underscore. |
| `bool isDigit(char c)` | Check decimal digit | Returns `true` when the character is between `0` and `9`. |
| `bool isWhitespace(char c)` | Check whitespace | Returns `true` for spaces, tabs, newlines, and other whitespace characters that the lexer should skip. |
| `bool isOperatorStart(char c)` | Check possible operator start | Returns `true` when the character may begin an operator token such as `+`, `-`, `*`, `/`, `%`, `=`, `!`, `<`, `>`, `&`, `\|`, `^`, `~`, `.`, `?`, or `:`. |
| `bool isDelimiter(char c)` | Check punctuation token | Returns `true` when the character is a delimiter such as `(`, `)`, `{`, `}`, `[`, `]`, `;`, or `,`. |
| `TokenType classifySingleChar(char c)` | Classify one-character token | Maps a single-character operator or delimiter to its `TokenType`, or returns `INVALID` if the character does not map to any supported single-character token. |
| `TokenType classifyDoubleChar(char first, char second)` | Classify two-character token | Checks whether the two-character sequence forms a valid multi-character operator such as `==`, `!=`, `<=`, `>=`, `&&`, `\|\|`, `++`, `--`, `->`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `\|=`, `^=`, `<<`, or `>>`. Returns `INVALID` if no match. |

### What these functions should do
- keep low-level recognition rules outside `Lexer`,
- reduce repeated condition chains in the lexer body,
- make token classification logic easy to test independently.

---

## 4.4 `LexerError`
**File:** `src/lexer/lexer_error.h`, `src/lexer/lexer_error.cpp`

### Purpose
Represents one lexical error detected while scanning.

### Attributes
- `std::string message` — human-readable error message.
- `int line` — line where the error occurred.
- `int column` — column where the error occurred.
- `std::string offendingText` — bad character or invalid fragment if available.

### Methods

| Function | Purpose | What exactly it does |
|---|---|---|
| `LexerError(std::string message, int line, int column, std::string offendingText = "")` | Construct an error object | Stores the error message, location, and optional offending text for later reporting. |
| `std::string toString() const` | Build readable error text | Produces a formatted string that can be shown in logs, tests, or GUI error panels. |

### What the class should do
- store lexical errors cleanly,
- separate errors from tokens,
- make GUI display and testing easy.

---

## 4.5 `Lexer`
**File:** `src/lexer/lexer.h`, `src/lexer/lexer.cpp`

### Purpose
This is the main lexical analyzer. It reads source code, produces tokens, updates the symbol table, and records lexical errors.

### Attributes
- `CharStream input` — source reader.
- `ReservedWords reservedWords` — keyword lookup.
- `SymbolTable& symbolTable` — shared symbol table.
- `std::vector<Token> tokens` — produced token sequence.
- `std::vector<LexerError> errors` — all lexical errors.

### Public methods

| Function | Purpose | What exactly it does |
|---|---|---|
| `Lexer(const std::string& source, SymbolTable& symbolTable)` | Construct lexer | Creates the lexer over the given source code and stores a reference to the shared symbol table. |
| `std::vector<Token> tokenize()` | Run full lexical analysis | Repeatedly scans tokens from the source until the end of file is reached, stores them, appends `END_OF_FILE`, and returns the final token vector. |
| `const std::vector<LexerError>& getErrors() const` | Return collected lexical errors | Gives access to the full list of errors produced during scanning. |

### Private methods

| Function | Purpose | What exactly it does |
|---|---|---|
| `Token nextToken()` | Scan one token | Loops calling `skipWhitespace()` and `skipComment()` alternately until neither makes progress, then decides which token type starts at the current position, scans it fully, and returns the resulting token. When an invalid character is encountered and recovery is performed, the loop restarts internally so that `nextToken()` always returns a well-formed, non-`INVALID` token. |
| `void skipWhitespace()` | Ignore whitespace | Consumes spaces, tabs, and newlines until the next non-whitespace character is reached. |
| `bool skipComment()` | Ignore comment text | Detects whether the current input starts a single-line (`//`) or multi-line (`/* */`) comment, consumes it fully, and returns `true` if a comment was skipped or `false` if no comment was found. Because whitespace and comments may alternate, `nextToken()` calls both `skipWhitespace()` and `skipComment()` in a loop until neither makes progress before scanning the next token. |
| `Token scanIdentifierOrKeyword()` | Scan identifier-like text | Consumes a valid identifier sequence, then calls `reservedWords.getKeywordType()` on the result. Returns a token with the exact `KW_*` type for keywords, or `IDENTIFIER` for non-reserved names. Identifiers are inserted into the symbol table via `symbolTable.insert(token)`. |
| `Token scanNumber()` | Scan numeric literal | Consumes digits and optionally a fractional part, then returns either `INT_LITERAL` or `FLOAT_LITERAL` depending on the matched pattern. |
| `Token scanCharLiteral()` | Scan character literal | Consumes a character literal including supported escape sequences, reports an error if malformed, and returns the appropriate token. |
| `Token scanStringLiteral()` | Scan string literal | Consumes a string literal including escape sequences, reports unterminated strings, and returns the appropriate token or recovery result. |
| `Token scanOperatorOrDelimiter()` | Scan operator or punctuation token | Applies maximal munch by first checking three characters for compound shift-assignment operators (`<<=`, `>>=`), then two characters using `classifyDoubleChar` for all other multi-character operators, and finally one character using `classifySingleChar` as a fallback. Returns the matching token. |
| `Token makeToken(TokenType type, const std::string& lexeme, int line, int column)` | Build token consistently | Creates a token object with the given information so all token construction happens through one helper. |
| `void addError(const std::string& message, int line, int column, const std::string& text = "")` | Store a lexical error | Appends a new `LexerError` object to the internal error list. |
| `void recoverFromInvalidCharacter()` | Recover after unknown input | Calls `addError` to record the offending character, then consumes it so the scanning loop in `nextToken()` can restart from the next character. Does not produce a token. |
| `void recoverFromUnterminatedString()` | Recover after unterminated string | Calls `addError`, then consumes input until the next newline or end of file so scanning can resume from a stable position. |
| `void recoverFromUnterminatedComment()` | Recover after unterminated block comment | Calls `addError` and stops comment scanning at end of file since no valid closing delimiter exists. |

### What the class should do
- control the full tokenization process,
- apply maximal munch,
- ignore comments and whitespace,
- create clean token objects,
- insert identifiers into the symbol table,
- collect errors without crashing,
- produce output that later phases can consume directly.

### Recommended flow inside `tokenize()`
1. While not at end of input, call `nextToken()`.
2. Push the returned token into the token list. Because `nextToken()` handles recovery internally and never returns `INVALID`, no filtering step is needed.
3. At the end, append one `END_OF_FILE` token.
4. Return the final token vector.

---

## 5) `src/gui/`

Only lexer-related GUI responsibilities are described here.

---

## 5.1 `MainWindow`
**File:** `src/gui/main_window.h`, `src/gui/main_window.cpp`

### Purpose
Acts as the main GUI controller for the lexer workflow.

### Suggested responsibilities
- load source file text,
- let the user run the lexer,
- clear and refresh results,
- send lexer output to the lexer panel.

---

## 5.2 `LexerPanel`
**File:** `src/gui/lexer_panel.h`, `src/gui/lexer_panel.cpp`

### Purpose
Displays lexer results.

### Suggested responsibilities
- show tokens,
- show symbol table entries,
- show lexical errors.

---

## 6) Parser Compatibility Requirements


### The lexer should guarantee the following:
- Every identifier-shaped name is emitted as `IDENTIFIER` unless it is a reserved keyword, in which case it is emitted with its exact `KW_*` token type.
- Function names and variable names are **not** separated by the lexer; they both remain `IDENTIFIER` and are distinguished later by context.
- Every token keeps exact `line` and `column` information.
- The token stream ends with exactly one `END_OF_FILE` token.
- Multi-character operators use longest-match behavior, including three-character compound shift-assignment operators.
- Comments and whitespace are removed cleanly and do not appear as tokens.
- Lexical recovery should continue far enough to still produce as many valid later tokens as possible.
- Tokens should be returned in source order without gaps or reordering.
- Each symbol-table entry created by the lexer is constructed from the identifier token alone; `name` and `line` are derived from that token rather than stored separately.
- `dataType` and `scope` belong to later parser-side completion and should not be assigned by the lexer.

---

## 7) Final Design Summary

The lexer should be centered around a `Lexer` class that reads characters from `CharStream`, uses the free functions in `token_classifier.h` and the `ReservedWords` map to recognize lexemes, produces `Token` objects from `token.h`, registers identifiers in `SymbolTable` using `insert(token)`, records problems as `LexerError` objects, and returns a clean token stream that later phases can consume directly. Every C23 keyword has its own `KW_*` token type so the parser can switch on type without comparing strings. The symbol table exposes only the operations needed during lexical analysis, and `dataType` and `scope` are left to be filled in by later phases.
