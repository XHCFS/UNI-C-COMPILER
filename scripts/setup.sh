#!/usr/bin/env bash
# =============================================================================
# setup.sh — CIE 461 Compiler Project Initializer
# Run this once from an empty directory to scaffold the full project.
# Requirements: git, gh (GitHub CLI, optional for issue creation)
# Usage: bash setup.sh [github-repo-url]
# Example: bash setup.sh https://github.com/yourusername/cie461-compiler
# =============================================================================

set -e

REPO_URL="${1:-}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "======================================"
echo " CIE 461 Compiler — Project Setup"
echo "======================================"

# ── 1. Git init ───────────────────────────────────────────────────────────────
if [ ! -d ".git" ]; then
    echo "[1/4] Initializing git repository..."
    git init
    git checkout -b main
else
    echo "[1/4] Git repository already exists, skipping init."
fi

# ── 2. Create directory structure ─────────────────────────────────────────────
echo "[2/4] Creating directory structure..."

mkdir -p src/common
mkdir -p src/lexer
mkdir -p src/parser
mkdir -p src/gui
mkdir -p tests/lexer
mkdir -p tests/parser
mkdir -p docs
mkdir -p scripts
mkdir -p .github/workflows

# Keep empty dirs tracked
touch src/common/.gitkeep   2>/dev/null || true
touch src/lexer/.gitkeep    2>/dev/null || true
touch src/parser/.gitkeep   2>/dev/null || true
touch src/gui/.gitkeep      2>/dev/null || true
touch tests/lexer/.gitkeep  2>/dev/null || true
touch tests/parser/.gitkeep 2>/dev/null || true
touch docs/.gitkeep         2>/dev/null || true

echo "    Directory structure created."

# ── 3. Initial commit ─────────────────────────────────────────────────────────
echo "[3/4] Creating initial commit..."
git add -A
git commit -m "chore: initial project scaffold

- CMake build system with src/ and tests/ subdirectories
- GitHub Actions CI (build + test on push/PR)
- GitHub Actions Release (binary packages on version tags)
- Stub source files for Lexer, Parser, GUI, and common types
- Unit test skeletons for lexer and parser
" || echo "    Nothing to commit (files already staged)."

# ── 4. Remote and issue creation ──────────────────────────────────────────────
if [ -n "$REPO_URL" ]; then
    echo "[4/4] Adding remote origin..."
    git remote remove origin 2>/dev/null || true
    git remote add origin "$REPO_URL"
    git push -u origin main

    # Check for GitHub CLI
    if command -v gh &> /dev/null; then
        echo "    GitHub CLI found. Creating issues..."

        # ── Saifelden's issues ─────────────────────────────────────────────
        gh issue create \
            --title "[SETUP] Repository initialization and CI/CD pipeline" \
            --body "## Owner: Saifelden

Set up the repository structure, CMake build system, and GitHub Actions workflows.

**Tasks:**
- [x] Initialize git repo with main/dev branch strategy
- [x] Write root and module CMakeLists.txt files
- [x] Configure GitHub Actions CI (build + test on push/PR)
- [x] Configure GitHub Actions Release workflow (binary packages on version tags)
- [x] Write .gitignore and README
- [ ] Verify CI passes on first push
- [ ] Set branch protection rules on \`main\`

**Definition of done:** A push to \`main\` triggers CI and goes green. Pushing a \`v*.*.*\` tag creates a GitHub Release with downloadable binaries." \
            --label "setup,infrastructure" 2>/dev/null || \
            echo "    (Labels may need creating first — issue body printed above)"

        gh issue create \
            --title "[COMMON] Token types and SymbolTable" \
            --body "## Owner: Saifelden

Implement the shared \`Token\` and \`SymbolTable\` types used by all other phases.

**Files:** \`src/common/Token.h\`, \`src/common/Token.cpp\`, \`src/common/SymbolTable.h\`, \`src/common/SymbolTable.cpp\`

**Tasks:**
- [ ] Finalize the \`TokenType\` enum to cover all tokens in the language spec
- [ ] Implement \`tokenTypeToString()\`
- [ ] Implement \`SymbolTable::enterScope()\` and \`exitScope()\`
- [ ] Implement \`SymbolTable::insert()\` with duplicate-detection per scope
- [ ] Implement \`SymbolTable::lookup()\` with inner-to-outer scope walk
- [ ] Implement \`SymbolTable::print()\` in formatted tabular output
- [ ] Write at least 5 unit tests covering scope shadowing and lookup

**Definition of done:** All symbol table unit tests pass; the common library compiles without warnings." \
            --label "common,lexer" 2>/dev/null || true

        gh issue create \
            --title "[LEXER] Full Lexer implementation" \
            --body "## Owner: Saifelden

Implement the complete lexer in \`src/lexer/Lexer.cpp\`.

**Tasks:**
- [ ] Tokenize all keyword, identifier, literal, operator, and delimiter types
- [ ] Apply maximal munch (longest match) rule
- [ ] Skip line comments (\`//\`) and block comments (\`/* */\`)
- [ ] Populate the SymbolTable with every new identifier and its line number
- [ ] Collect lexical errors into \`errors_\` with line and column info (panic-mode recovery)
- [ ] Handle unterminated string and char literals gracefully
- [ ] Pass all tests in \`tests/lexer/test_lexer.cpp\`
- [ ] Add at least 5 additional test cases covering edge cases (empty input, nested operators, multiline strings)

**Definition of done:** \`ctest\` LexerTests passes; lexer produces correct token stream for all sample C files in \`docs/\`." \
            --label "lexer" 2>/dev/null || true

        gh issue create \
            --title "[LEXER] Lexer GUI panel" \
            --body "## Owner: Saifelden

Build the GUI panel that displays lexer output.

**Tasks:**
- [ ] Choose and integrate a GUI library (Qt / wxWidgets / Dear ImGui)
- [ ] Display token list in a scrollable table: token type, lexeme, line, column
- [ ] Display symbol table with sortable columns
- [ ] Highlight lexical errors in red with error messages below the table
- [ ] Add a file-open button to load a \`.c\` source file
- [ ] Add a \"Run Lexer\" button that refreshes all panels
- [ ] Update CMakeLists.txt to link the GUI library

**Definition of done:** The application opens, loads a .c file, and renders the token table and symbol table correctly." \
            --label "lexer,gui" 2>/dev/null || true

        # ── Omar's issues ──────────────────────────────────────────────────
        gh issue create \
            --title "[PARSER] Recursive-descent Parser implementation" \
            --body "## Owner: Omar

Implement the full recursive-descent parser in \`src/parser/Parser.cpp\`.

All grammar rules are documented in \`docs/c_language_specs.pdf\` (Section 12).

**Tasks:**
- [ ] Implement \`parseDecl()\` — dispatches to var-decl or fun-decl based on lookahead
- [ ] Implement \`parseVarDecl()\` with optional initializer
- [ ] Implement \`parseFunDecl()\` including parameter list and compound body
- [ ] Implement \`parseCompoundStmt()\` — handles nested scopes
- [ ] Implement \`parseStmt()\` — dispatches to all statement types
- [ ] Implement \`parseIfStmt()\` with optional else branch
- [ ] Implement \`parseWhileStmt()\`, \`parseDoWhileStmt()\`, \`parseForStmt()\`
- [ ] Implement \`parseReturnStmt()\`
- [ ] Implement expression rules: \`parseExpr\`, \`parseBoolExpr\`, \`parseBoolTerm\`, \`parseBoolFactor\`, \`parseRelExpr\`, \`parseArithExpr\`, \`parseTerm\`, \`parseFactor\`
- [ ] Implement \`parseCall()\` and \`parseArgList()\`
- [ ] Implement panic-mode error recovery in \`synchronize()\`
- [ ] Pass all tests in \`tests/parser/test_parser.cpp\`

**Definition of done:** Parser builds a complete parse tree for all sample C files; all parser unit tests pass." \
            --label "parser" 2>/dev/null || true

        gh issue create \
            --title "[PARSER] ParseTree implementation and printing" \
            --body "## Owner: Omar

Implement the ParseTree data structure and its display utilities.

**Files:** \`src/parser/ParseTree.h\`, \`src/parser/ParseTree.cpp\`

**Tasks:**
- [ ] Confirm \`ParseNode\` struct covers all needed fields (label, token pointer, children)
- [ ] Implement \`printParseTree()\` with indented ASCII output
- [ ] Add a \`toJSON()\` or \`toDot()\` export method for GUI rendering
- [ ] Write tests that construct a small tree manually and verify printed output

**Definition of done:** Parse tree prints correctly for a function with nested if and while statements." \
            --label "parser" 2>/dev/null || true

        gh issue create \
            --title "[PARSER] Parser error handling" \
            --body "## Owner: Omar

Extend the parser with robust error reporting and recovery.

**Tasks:**
- [ ] Ensure \`consume()\` emits a descriptive error message including line, expected token, and context
- [ ] Test \`synchronize()\` on at least 5 different invalid input programs
- [ ] Verify multiple errors are collected in a single parse run (not just the first)
- [ ] Write test cases for: missing semicolon, missing closing brace, malformed expression, wrong return type syntax, extra tokens

**Definition of done:** The parser reports all errors in the test inputs without crashing; error messages include line numbers." \
            --label "parser" 2>/dev/null || true

        gh issue create \
            --title "[PARSER] Parser GUI panel" \
            --body "## Owner: Omar

Build the GUI panel that displays parser output.

**Tasks:**
- [ ] Render the parse tree as an interactive tree widget (expand/collapse nodes)
- [ ] Show parser errors in a dedicated error panel with line-number links
- [ ] Add a \"Run Parser\" button that calls the lexer + parser pipeline and refreshes the tree
- [ ] Highlight the corresponding source line when a parse tree node is clicked
- [ ] Match the visual style of the lexer GUI panel

**Definition of done:** The parse tree panel renders for a non-trivial input file and is navigable." \
            --label "parser,gui" 2>/dev/null || true

        gh issue create \
            --title "[DOCS] Final documentation and demo preparation" \
            --body "## Owner: Both

Prepare the final submission documentation and demo.

**Tasks:**
- [ ] Update \`docs/c_language_specs.pdf\` to reflect any changes made during implementation
- [ ] Write implementation notes section in the docs (design decisions, known limitations)
- [ ] Record demo video: lexer output, symbol table, parser tree, error handling cases
- [ ] Prepare PowerPoint presentation covering architecture, design, and demo
- [ ] Prepare at least 3 test case .c files: valid program, lexical error, syntax error

**Due:** Saturday 9/5/2026" \
            --label "documentation" 2>/dev/null || true

        echo "    Issues created successfully."
    else
        echo "    GitHub CLI (gh) not found. Skipping issue creation."
        echo "    Install gh from https://cli.github.com/ and re-run, or create issues manually."
        echo "    See ISSUES.md for the full issue list."
    fi
else
    echo "[4/4] No GitHub repo URL provided. Skipping remote setup and issue creation."
    echo "    To push later: git remote add origin <url> && git push -u origin main"
    echo "    To create issues: gh issue create ... (see ISSUES.md)"
fi

echo ""
echo "======================================"
echo " Setup complete."
echo "======================================"
echo ""
echo " Build the project:"
echo "   cmake -B build -DCMAKE_BUILD_TYPE=Release"
echo "   cmake --build build --parallel"
echo ""
echo " Run tests:"
echo "   cd build && ctest --output-on-failure"
echo ""
echo " Create a release:"
echo "   git tag v1.0.0 && git push origin v1.0.0"
echo ""
