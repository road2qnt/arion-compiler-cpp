# Tugas Besar IF2224 - Teori Bahasa Formal dan Automata

Compiler/Interpreter untuk bahasa **Arion** yang dikembangkan dalam empat milestone:

1. **Milestone 1 - Lexical Analysis** menggunakan DFA untuk mengubah source code menjadi token.
2. **Milestone 2 - Syntax Analysis** menggunakan Recursive Descent Parser untuk membentuk parse tree.
3. **Milestone 3 - Semantic Analysis** untuk membentuk Decorated AST, symbol table, dan melaporkan semantic error.
4. **Milestone 4 - Intermediate Code and Interpreter** untuk menghasilkan intermediate code dan menjalankan program dengan stack-machine interpreter.

## Identitas Kelompok

Kelompok `std_abs` - Kode `ABS`

| NIM | Nama |
|-----|------|
| 13524115 | Ega Luthfi Rais |
| 13524134 | Salman Faiz Assidqi |
| 13524141 | Ahmad Fauzan Putra |
| 13524146 | Leonardus Brain Fatolosja |

## Alur Kerja Program

```text
Source Code (.txt)
      |
      v
Lexical Analyzer (DFA)
      |
      | vector<Token> / daftar token
      v
Syntax Analyzer (Recursive Descent Parser)
      |
      | Parse Tree
      v
Semantic Analyzer
      |-- Konversi Parse Tree menjadi AST
      |-- Dekorasi AST dengan type, tab index, lexical level, dan l-value
      |-- Pengisian symbol table: tab, btab, atab
      |-- Type checking dan scope checking
      v
Decorated AST + Symbol Table + Semantic Errors (jika ada)
      |
      v
Intermediate Code Generator
      |
      | daftar instruksi LIT/LOD/STO/CAL/INT/JMP/JPC/OPR/RET
      v
Stack-Machine Interpreter
      |
      v
Runtime Output
```

### Milestone 1: Lexical Analyzer

Lexer membaca source code karakter per karakter dengan DFA dan menghasilkan token bahasa Arion, termasuk identifier, literal, keyword, operator, dan delimiter.

### Milestone 2: Syntax Analyzer

Parser menerima token hasil lexer atau file token, kemudian memeriksa grammar bahasa Arion dan menghasilkan parse tree. Parser menangani deklarasi, expression, assignment, procedure/function call, conditional statement, loop, array access, dan record field access.

### Milestone 3: Semantic Analyzer

Semantic analyzer bekerja di atas parse tree dalam dua fase:

1. **Pembentukan AST**: detail sintaksis yang tidak lagi diperlukan, seperti `begin`, `end`, `semicolon`, dan kurung pengelompokan, diringkas menjadi node AST yang mewakili makna program.
2. **Dekorasi AST**: AST ditelusuri menggunakan visitor untuk mengisi symbol table, melakukan lookup identifier, menghitung tipe ekspresi, memvalidasi penggunaan operator, dan melaporkan semantic error.

Implementasi Milestone 3 menyediakan:

- AST node untuk program, deklarasi, assignment, expression, procedure/function call, `if`, `while`, `repeat`, `for`, `case`, array, record, enumerated type, dan subrange.
- Symbol table tiga bagian:
  - `tab`: identifier, object kind, type, reference, lexical level, address, dan link scope.
  - `btab`: informasi block untuk program, subprogram, dan record.
  - `atab`: informasi array dan batas tipe indeks.
- Predefined identifier: `integer`, `real`, `boolean`, `char`, `string`, `true`, `false`, `read`, `readln`, `write`, dan `writeln`.
- Pemeriksaan semantic, antara lain duplicate identifier, undeclared identifier, assignment compatibility, validitas operator, kondisi Boolean, akses array/record, subrange, label `case`, serta argumen l-value untuk `read`/`readln`.

### Milestone 4: Intermediate Code and Interpreter

Milestone 4 menjalankan backend compiler di atas Decorated AST dan symbol table. `CodeGenerator` menghasilkan intermediate code berbasis instruksi stack machine, lalu `Interpreter` mengeksekusi instruksi tersebut dan mengumpulkan output runtime dari operasi `write`/`writeln`.

Implementasi Milestone 4 menyediakan:

- Instruksi intermediate code utama: `LIT`, `LOD`, `STO`, `CAL`, `INT`, `JMP`, `JPC`, `OPR`, dan `RET`.
- Instruksi pendukung akses alamat: `LDA`, `LDI`, dan `STI` untuk var parameter, array access, dan record field access.
- Operasi `OPR` untuk arithmetic, comparison, `write`, dan `writeln`.
- Stack machine dengan frame header berupa static link, dynamic link, dan return address.
- Eksekusi assignment, expression, `if`, `while`, `repeat`, `for`, `case`, procedure/function call, value parameter, var parameter, array, record, dan nested access.

## Struktur Direktori

```text
bin/                         Hasil build program
src/
  lexical-analysis/          DFA lexer dan token
  syntax-analysis/           Recursive Descent Parser dan parse tree
  semantic-analysis/         AST, semantic visitor, type checker, symbol table
  intermediate-code/         Intermediate code generator dan definisi instruksi
  interpreter/               Runtime value, stack machine, dan interpreter
  utils/                     Pembacaan/penulisan output dan command handler
  debugger/                  Utilitas debugging/visualisasi
doc/                         Spesifikasi dan laporan milestone
test/
  milestone-1/               Input/output lexical analysis
  milestone-2/               Input/output syntax analysis
  milestone-3/               Input/output semantic analysis
  milestone-4/               Input pengujian intermediate code dan interpreter
```

## Requirements

- `g++` dengan dukungan C++17
- GNU Make

## Cara Instalasi dan Penggunaan Program
## Build

```bash
make clean
make
```

Binary hasil build:

```text
bin/arion
```

## Menjalankan Program
### Mode Interaktif

```bash
make run
```

Program menyediakan pilihan berikut:

| Mode | Input | Proses | Output File Opsional |
|------|-------|--------|-----------------------|
| `1` / `lexical` | Source code pada `test/milestone-1/` | Lexical analysis | Token pada `test/milestone-1/` |
| `2` / `syntax` | File token pada `test/milestone-2/` | Syntax analysis | Parse tree pada `test/milestone-2/` |
| `3` / `keduanya` | Source code pada `test/milestone-1/` | Lexical + syntax analysis | Token dan parse tree |
| `4` / `semantic` | Source code pada `test/milestone-1/` | Lexical + syntax + semantic analysis secara internal | Decorated AST/symbol table pada `test/milestone-3/` |
| `5` / `codegen` / `milestone4` | Source code pada `test/milestone-1/` | Lexical + syntax + semantic + codegen + interpreter | Intermediate code pada `test/milestone-4/` |
| `6` / `semua` | Source code pada `test/milestone-1/` | Seluruh pipeline | Token, parse tree, dan hasil semantic; intermediate code/runtime dicetak ke terminal |

Catatan: mode `semantic` menjalankan lexer dan parser terlebih dahulu untuk membentuk parse tree, lalu hanya meminta nama file keluaran semantic.

### Mode Command

Mode command menerima source file secara langsung. Pada mode ini, program selalu menjalankan pipeline lengkap: lexical analysis, syntax analysis, semantic analysis, lalu code generation dan interpreter jika tidak ada semantic error. Argumen nama output bersifat opsional.

```bash
make run FILE=<source_file_path>
make run FILE=<source_file_path> LEXOUT=<lexical_output_file>
make run FILE=<source_file_path> LEXOUT=<lexical_output_file> SYNOUT=<syntax_output_file>
make run FILE=<source_file_path> LEXOUT=<lexical_output_file> SYNOUT=<syntax_output_file> SEMOUT=<semantic_output_file>
```

Catatan: target `make run` meneruskan argumen sampai `SEMOUT`. Untuk menyimpan output intermediate code melalui argumen positional kelima, jalankan binary secara langsung.

Arti argumen:

| Argumen | Keterangan | Lokasi File Output |
|---------|------------|---------------------|
| `FILE` | Path source code yang diproses | - |
| `LEXOUT` | Nama file output token | `test/milestone-1/` |
| `SYNOUT` | Nama file output parse tree | `test/milestone-2/` |
| `SEMOUT` | Nama file output decorated AST, symbol table, dan semantic error | `test/milestone-3/` |
| argumen positional ke-5 binary langsung | Nama file output intermediate code | `test/milestone-4/` |

Contoh menjalankan pengujian Milestone 3:

```bash
make run FILE=test/milestone-3/input-5.txt
```

Contoh menjalankan pengujian sekaligus menyimpan keluaran semantic:

```bash
make run FILE=test/milestone-3/input-5.txt LEXOUT=token-output-5.txt SYNOUT=parse-tree-output-5.txt SEMOUT=semantic-output-5.txt
```

Binary juga dapat dijalankan langsung:

```bash
./bin/arion test/milestone-3/input-1.txt token-output.txt parse-tree-output.txt semantic-output.txt
./bin/arion test/milestone-4/input-1.txt token-output.txt parse-tree-output.txt semantic-output.txt code-output.txt
```

Argumen file output bersifat positional: output semantic hanya dapat diberikan setelah output lexical dan syntax juga diberikan, dan output intermediate code hanya dapat diberikan setelah output semantic juga diberikan. Jika file output telah ada, isi file akan ditimpa. Seluruh hasil fase yang dijalankan tetap ditampilkan ke terminal.

## Format Input Syntax Analyzer

Mode `syntax` interaktif membaca file token hasil lexical analyzer dengan format satu token per baris:

```text
programsy
ident(Hello)
semicolon
beginsy
endsy
period
```

Token komentar diabaikan oleh parser. Token yang tidak dikenal dapat menghasilkan syntax error.

## Keluaran Program

### Lexical Analysis

Menghasilkan daftar token dari source code.

### Syntax Analysis

Menghasilkan parse tree berdasarkan grammar bahasa Arion.

### Semantic Analysis

Menghasilkan tiga bagian keluaran:

```text
========== Decorated AST ==========
Program: Hello [tab=...]
  Declarations:
    VarDecl: a [tab=...] type=integer
  Block: [...]
    Assign:
      Var: a [tab=...] type=integer ...
      Number: 5 type=integer

=== Symbol Table (tab) ===
...
=== Block Table (btab) ===
...
=== Array Table (atab) ===
...

========== Semantic Errors ==========
... hanya ditampilkan jika ditemukan error ...
```

Decorated AST dapat menampilkan informasi seperti:

- `type`: tipe akhir suatu node.
- `tab`: indeks identifier pada symbol table.
- `lev`: lexical level identifier.
- `lval`: apakah node dapat menjadi target assignment.
- `line`: posisi sumber untuk membantu pelaporan error.

### Intermediate Code dan Runtime Output

Jika semantic analysis tidak memiliki error, program menghasilkan intermediate code dan runtime output:

```text
========== Intermediate Code ==========
1 JMP 0 1 ; skip subprogram bodies
2 INT 0 5 ; main program frame (5 slots)
3 LIT 0 5 ; int 5
4 STO 0 3 ; store a
...

========== Runtime Output ==========
Result = 15
```

Intermediate code dapat memuat instruksi stack-machine seperti `LIT`, `LOD`, `STO`, `JMP`, `JPC`, `CAL`, `OPR`, dan `RET`. Runtime output berisi hasil aktual dari `write` dan `writeln`.

## Pengujian Milestone 3

Test semantic analysis tersedia pada folder `test/milestone-3/`.

| File | Cakupan Utama |
|------|---------------|
| `input-1.txt` | Deklarasi variabel, assignment, expression, dan `writeln` |
| `input-2.txt` | Operator dan expression |
| `input-3.txt` | Penanganan komentar |
| `input-4.txt` | Literal `char`, `string`, dan `real` |
| `input-5.txt` | Array, `for`, `repeat`, `while`, dan array access |

Output pembanding masing-masing pengujian tersedia sebagai `output-1.txt` sampai `output-5.txt` pada folder yang sama.

## Pengujian Milestone 4

Test intermediate code dan interpreter tersedia pada folder `test/milestone-4/`.

| File | Cakupan Utama |
|------|---------------|
| `input-1.txt` | Assignment, expression, `writeln`, dan empty `writeln` |
| `input-2.txt` | `if`/`else` |
| `input-3.txt` | `while` dan `repeat` |
| `input-4.txt` | `for to` dan `for downto` |
| `input-5.txt` | Nested control flow |
| `input-6.txt` - `input-10.txt` | Procedure, function, nested subprogram, dan komposisi function |
| `input-11.txt` - `input-15.txt` | `var` parameter dan forwarding parameter |
| `input-16.txt` - `input-20.txt` | Record, array of record, nested record field, dan nested array/record access |

## Pembagian Tugas

### Milestone 1

| Nama | Pembagian Tugas | Kontribusi |
|------|----------------|------------|
| Ega Luthfi Rais | Project Manager, inisiasi GitHub, desain struktur GitHub, assist desain dan implementasi DFA | 25% |
| Salman Faiz Assidqi | Laporan implementasi DFA dan debugging | 25% |
| Ahmad Fauzan Putra | Implementasi DFA (`token.hpp` dan `token.cpp`) dan pengujian | 25% |
| Leonardus Brain Fatolosja | Pembuatan DFA dan laporan teori/penjelasan DFA | 25% |

### Milestone 2

| Nama | Pembagian Tugas | Kontribusi |
|------|----------------|------------|
| Ega Luthfi Rais | Project Manager, inisiasi GitHub, desain struktur, assist desain dan implementasi awal Recursive Descent | 25% |
| Salman Faiz Assidqi | Laporan | 25% |
| Ahmad Fauzan Putra | Laporan dan testing | 25% |
| Leonardus Brain Fatolosja | Laporan, perancangan program, implementasi Recursive Descent Parser, dan testing | 25% |

### Milestone 3

| Nama | Pembagian Tugas | Kontribusi |
|------|----------------|------------|
| Ega Luthfi Rais | Inisiasi awal code, implementasi, dan debugging | 33.3% |
| Salman Faiz Assidqi | - | 0% |
| Ahmad Fauzan Putra | Implementasi, debugging, dan laporan | 33.3% |
| Leonardus Brain Fatolosja | Implementasi, debugging, dan laporan | 33.3% |

### Milestone 4

| Nama | Pembagian Tugas | Kontribusi |
|------|----------------|------------|
| Ega Luthfi Rais | Implementasi code, testing, dan debugging | 33.3% |
| Salman Faiz Assidqi | - | 0% |
| Ahmad Fauzan Putra | Laporan dan testing | 33.3% |
| Leonardus Brain Fatolosja | Inisiasi awal code, debugging, dan laporan | 33.3% |