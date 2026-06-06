# arion-compiler-cpp

> A complete compiler front-end and interpreter for the Arion programming language — implemented in C++17 with zero external dependencies.

**C++17** | **Full pipeline: Lexer → Parser → Semantic Analysis → IR → Stack-Machine Interpreter**

---

## Overview

`arion-compiler-cpp` is a from-scratch compiler implementation for the Arion programming language. The compiler processes source code through a complete multi-stage pipeline: lexical analysis, recursive descent parsing, semantic analysis with decorated AST construction, intermediate code generation, and execution on a custom stack-machine virtual machine.

The project covers the full spectrum of compiler theory — from regular languages and DFAs to context-free grammars and code generation — all implemented in modern C++17 without parser generators or compiler infrastructure libraries.

## Compilation Pipeline

```
Source Code
    │
    ▼
Lexical Analysis (DFA-based tokenizer)
    │
    ▼
Syntax Analysis (Recursive Descent Parser → Parse Tree)
    │
    ▼
Semantic Analysis (Decorated AST + Symbol Table + Type Checker)
    │
    ▼
Intermediate Code Generation (3-address style instructions)
    │
    ▼
Stack-Machine Interpreter (LIT, LOD, STO, CAL, JMP, JPC, OPR, RET, etc.)
    │
    ▼
Runtime Output
```

## Key Features

| Stage | Implementation |
|-------|---------------|
| **Lexer** | Deterministic Finite Automaton (DFA) for token recognition, supports identifiers, integers, strings, operators, keywords |
| **Parser** | Recursive descent parser with one-token lookahead, handles expressions, statements, arrays, records, procedures, functions |
| **Parser Support** | Control flow: `if`, `while`, `repeat`, `for`, `case` — nested and recursive constructs |
| **Semantic Analysis** | Decorated AST construction, type checking, scope resolution, three-part symbol table (tab, btab, atab) |
| **IR Generation** | Instruction set: LIT, LOD, STO, CAL, INT, JMP, JPC, OPR, RET, SYS, INC, DEC, NEG, ADR, CHK, ORD |
| **Interpreter** | Stack-based virtual machine with runtime value tracking and error handling |
| **Debugger** | Built-in debugger with instruction tracing, symbol table inspection, AST visualization |

## Build & Run

**Requirements:** `g++` (C++17), GNU Make

```bash
make clean
make
./bin/main <source_file.arion>
```

## Project Structure

```
src/
├── lexical-analysis/    # DFA definitions, token types, lexer engine
├── syntax-analysis/     # Recursive descent parser, parse tree nodes
├── semantic-analysis/   # AST, symbol tables, type checker, semantic visitors
├── intermediate-code/   # IR instruction definitions, code generator
├── interpreter/         # Stack machine, runtime values, execution engine
├── debugger/            # Debugging and visualization utilities
└── utils/               # Output formatting, command handlers
```

## Why This Matters

Writing a full compiler front-end demonstrates:
- Deep understanding of formal languages and automata theory (DFA, CFG, recursive descent)
- Tree/graph algorithms (AST construction, traversal, transformation)
- Symbol table design and scope management
- Code generation and virtual machine architecture

Compiler construction is considered one of the most challenging CS fundamentals — and is a strong signal for any role requiring algorithmic thinking, systems programming, or language design.
