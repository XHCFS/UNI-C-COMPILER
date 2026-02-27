# CIE 461 — C Language Compiler (Lexer & Parser)

**Course:** Automata and Compiler Design — Zewail City  
**Instructor:** Dr. Wafaa Samy  
**Team:** Saifelden Mohamed Ismail (202100432), Omar Ayman (202100443)

---

## Project Overview

A lexer and parser for a subset of the C programming language, implemented in C++17.

## Building

Requirements: CMake ≥ 3.16, a C++17 compiler (GCC, Clang, or MSVC).

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The binary is placed at `build/bin/ccompiler` (Linux/macOS) or `build/Release/ccompiler.exe` (Windows).

## Running

```bash
./build/bin/ccompiler path/to/source.c
```

Without a file argument, the GUI launches.

## Running Tests

```bash
cd build
ctest --output-on-failure
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
  common/       Token, SymbolTable — shared by all phases
  lexer/        Lexer implementation
  parser/       Parser and ParseTree
  gui/          GUI layer
  main.cpp      Entry point
tests/
  lexer/        Lexer unit tests
  parser/       Parser unit tests
docs/           Specification and documentation PDFs
.github/
  workflows/
    ci.yml      Build + test on every push and PR
    release.yml Build, package, and publish on version tags
```
