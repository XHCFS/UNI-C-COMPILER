# CIE 461 — C Language Compiler (Lexer & Parser)

**Course:** Automata and Compiler Design — Zewail City  
**Instructor:** Dr. Wafaa Samy  
**Team:** Saifelden Mohamed Ismail (202100432), Omar Ayman (202100443)

---

## Project Overview

A lexer and parser for a subset of the C programming language, implemented in C++17 with a Qt6 GUI.

## Requirements

| Dependency | Minimum Version | Notes |
|---|---|---|
| CMake | 3.16 | Build system |
| C++ compiler | C++17 | GCC (MinGW on Windows), Clang, or MSVC |
| Ninja | any | Used as the CMake generator |
| Qt6 | 6.x | `Widgets` component required |

### Installing Qt6

Download the Qt online installer from the [Qt open-source page](https://www.qt.io/download-open-source) and install the **Qt 6.x — MinGW** (Windows) or **GCC** (Linux) kit.

After installation, set the `QTDIR` environment variable to your kit directory:

```bash
# Windows (MinGW — adjust version as needed)
set QTDIR=C:\Qt\6.11.0\mingw_64

# Linux / macOS
export QTDIR=~/Qt/6.11.0/gcc_64
```

CMake reads `QTDIR` via `CMakePresets.json` (`CMAKE_PREFIX_PATH=$env{QTDIR}`) to locate Qt automatically.

## Building

The project uses **CMake Presets**. Two presets are provided: `debug` and `release`.

```bash
# Configure
cmake --preset debug        # or: cmake --preset release

# Build
cmake --build --preset debug   # or: cmake --build --preset release
```

Binaries are placed under `build/debug/bin/` or `build/release/bin/`.

### Clean rebuild

The `build/` folder is entirely generated and safe to delete:

```bash
rm -rf build/
cmake --preset debug
cmake --build --preset debug
```

## Running

```bash
# Windows
./build/debug/bin/ccompiler.exe

# Linux / macOS
./build/debug/bin/ccompiler
```

The application opens a Qt6 GUI where you can type or open a C source file and run the lexer to inspect the token stream.

## Running Tests

```bash
cd build/debug
ctest --output-on-failure
```

Or run the test binaries directly:

```bash
./build/debug/bin/test_lexer.exe
./build/debug/bin/test_parser.exe
```

## Releases

Tagged releases are built automatically by GitHub Actions and published to the Releases page with pre-compiled binaries for Linux and Windows.

To create a release:

```bash
git tag v1.0.0
git push origin v1.0.0
```

## Project Structure

```
src/
  common/       Token, SymbolTable, SymbolEntry — shared by all phases
  lexer/        Lexer, CharStream, TokenClassifier, ReservedWords, LexerError
  parser/       Parser and ParseTree
  gui/          Qt6 GUI — MainWindow, LexerPanel
  main.cpp      Entry point
tests/
  lexer/        Lexer unit tests
  parser/       Parser unit tests
docs/           Specification and documentation PDFs
CMakeLists.txt      Root build file (requires Qt6::Widgets)
CMakePresets.json   debug / release presets (reads QTDIR from environment)
.github/
  workflows/
    ci.yml      Build + test on every push and PR
    release.yml Build, package, and publish on version tags
```
