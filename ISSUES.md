# GitHub Issues

This file lists all issues to create in the repository.
If you have the GitHub CLI (`gh`), `scripts/setup.sh` creates them automatically.
Otherwise, create them manually on GitHub.

---

## Saifelden — Issues

### Issue 1: [SETUP] Repository initialization and CI/CD pipeline
**Labels:** setup, infrastructure

Set up the repository structure, CMake build system, and GitHub Actions workflows.

**Tasks:**
- [x] Initialize git repo with main/dev branch strategy
- [x] Write root and module CMakeLists.txt files
- [x] Configure GitHub Actions CI (build + test on push/PR)
- [x] Configure GitHub Actions Release workflow (binary packages on version tags)
- [x] Write .gitignore and README
- [ ] Verify CI passes on first push
- [ ] Set branch protection rules on `main`

**Definition of done:** A push to `main` triggers CI and goes green. Pushing a `v*.*.*` tag creates a GitHub Release with downloadable binaries.

---

### Issue 2: [COMMON] Token types and SymbolTable
**Labels:** common, lexer

Implement the shared `Token` and `SymbolTable` types used by all other phases.

**Files:** `src/common/Token.h`, `src/common/Token.cpp`, `src/common/SymbolTable.h`, `src/common/SymbolTable.cpp`

**Tasks:**
- [ ] Finalize the `TokenType` enum to cover all tokens in the language spec
- [ ] Implement `tokenTypeToString()`
- [ ] Implement `SymbolTable::enterScope()` and `exitScope()`
- [ ] Implement `SymbolTable::insert()` with duplicate-detection per scope
- [ ] Implement `SymbolTable::lookup()` with inner-to-outer scope walk
- [ ] Implement `SymbolTable::print()` in formatted tabular output
- [ ] Write at least 5 unit tests covering scope shadowing and lookup

**Definition of done:** All symbol table unit tests pass; the common library compiles without warnings.

---

### Issue 3: [LEXER] Full Lexer implementation
**Labels:** lexer

Implement the complete lexer in `src/lexer/Lexer.cpp`.

**Tasks:**
- [ ] Tokenize all keyword, identifier, literal, operator, and delimiter types
- [ ] Apply maximal munch (longest match) rule
- [ ] Skip line comments (`//`) and block comments (`/* */`)
- [ ] Populate the SymbolTable with every new identifier and its line number
- [ ] Collect lexical errors into `errors_` with line and column info (panic-mode recovery)
- [ ] Handle unterminated string and char literals gracefully
- [ ] Pass all tests in `tests/lexer/test_lexer.cpp`
- [ ] Add at least 5 additional test cases covering edge cases

**Definition of done:** `ctest` LexerTests passes; lexer produces correct token stream for all sample C files.

---

### Issue 4: [LEXER] Lexer GUI panel
**Labels:** lexer, gui

Build the GUI panel that displays lexer output.

**Tasks:**
- [ ] Choose and integrate a GUI library (Qt / wxWidgets / Dear ImGui)
- [ ] Display token list in a scrollable table: token type, lexeme, line, column
- [ ] Display symbol table with sortable columns
- [ ] Highlight lexical errors in red with error messages below the table
- [ ] Add a file-open button to load a `.c` source file
- [ ] Add a "Run Lexer" button that refreshes all panels
- [ ] Update CMakeLists.txt to link the GUI library

**Definition of done:** The application opens, loads a .c file, and renders the token table and symbol table correctly.

---

## Omar — Issues

### Issue 5: [PARSER] Recursive-descent Parser implementation
**Labels:** parser

Implement the full recursive-descent parser in `src/parser/Parser.cpp`.

All grammar rules are documented in `docs/c_language_specs.pdf` (Section 12).

**Tasks:**
- [ ] Implement `parseDecl()` — dispatches to var-decl or fun-decl based on lookahead
- [ ] Implement `parseVarDecl()` with optional initializer
- [ ] Implement `parseFunDecl()` including parameter list and compound body
- [ ] Implement `parseCompoundStmt()` — handles nested scopes
- [ ] Implement `parseStmt()` — dispatches to all statement types
- [ ] Implement `parseIfStmt()` with optional else branch
- [ ] Implement `parseWhileStmt()`, `parseDoWhileStmt()`, `parseForStmt()`
- [ ] Implement `parseReturnStmt()`
- [ ] Implement expression rules: `parseExpr`, `parseBoolExpr`, `parseBoolTerm`, `parseBoolFactor`, `parseRelExpr`, `parseArithExpr`, `parseTerm`, `parseFactor`
- [ ] Implement `parseCall()` and `parseArgList()`
- [ ] Implement panic-mode error recovery in `synchronize()`
- [ ] Pass all tests in `tests/parser/test_parser.cpp`

**Definition of done:** Parser builds a complete parse tree for all sample C files; all parser unit tests pass.

---

### Issue 6: [PARSER] ParseTree implementation and printing
**Labels:** parser

Implement the ParseTree data structure and its display utilities.

**Tasks:**
- [ ] Confirm `ParseNode` struct covers all needed fields
- [ ] Implement `printParseTree()` with indented ASCII output
- [ ] Add a `toJSON()` or `toDot()` export method for GUI rendering
- [ ] Write tests that construct a small tree manually and verify printed output

**Definition of done:** Parse tree prints correctly for a function with nested if and while statements.

---

### Issue 7: [PARSER] Parser error handling
**Labels:** parser

Extend the parser with robust error reporting and recovery.

**Tasks:**
- [ ] Ensure `consume()` emits a descriptive error message including line, expected token, and context
- [ ] Test `synchronize()` on at least 5 different invalid input programs
- [ ] Verify multiple errors are collected in a single parse run
- [ ] Write test cases for: missing semicolon, missing closing brace, malformed expression, wrong return type syntax, extra tokens

**Definition of done:** The parser reports all errors in the test inputs without crashing; error messages include line numbers.

---

### Issue 8: [PARSER] Parser GUI panel
**Labels:** parser, gui

Build the GUI panel that displays parser output.

**Tasks:**
- [ ] Render the parse tree as an interactive tree widget (expand/collapse nodes)
- [ ] Show parser errors in a dedicated error panel with line-number links
- [ ] Add a "Run Parser" button that calls the lexer + parser pipeline and refreshes the tree
- [ ] Highlight the corresponding source line when a parse tree node is clicked
- [ ] Match the visual style of the lexer GUI panel

**Definition of done:** The parse tree panel renders for a non-trivial input file and is navigable.

---

## Both — Issues

### Issue 9: [DOCS] Final documentation and demo preparation
**Labels:** documentation

Prepare the final submission documentation and demo.

**Tasks:**
- [ ] Update `docs/c_language_specs.pdf` to reflect any changes made during implementation
- [ ] Write implementation notes section in the docs (design decisions, known limitations)
- [ ] Record demo video: lexer output, symbol table, parser tree, error handling cases
- [ ] Prepare PowerPoint presentation covering architecture, design, and demo
- [ ] Prepare at least 3 test case .c files: valid program, lexical error, syntax error

**Due:** Saturday 9/5/2026
