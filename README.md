# Tugas Besar IF2224 - Teori Bahasa Formal dan Automata

Compiler  untuk bahasa **Arion** yang dikembangkan dalam tiga milestone:

1. **Milestone 1 - Lexical Analysis** menggunakan DFA untuk mengubah source code menjadi token.
2. **Milestone 2 - Syntax Analysis** menggunakan Recursive Descent Parser untuk membentuk parse tree.
3. **Milestone 3 - Semantic Analysis** untuk membentuk Decorated AST, symbol table, dan melaporkan semantic error.

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

## Struktur Direktori

```text
bin/                         Hasil build program
src/
  lexical-analysis/          DFA lexer dan token
  syntax-analysis/           Recursive Descent Parser dan parse tree
  semantic-analysis/         AST, semantic visitor, type checker, symbol table
  utils/                     Pembacaan/penulisan output dan command handler
  debugger/                  Utilitas debugging/visualisasi
doc/                         Spesifikasi dan laporan milestone
test/
  milestone-1/               Input/output lexical analysis
  milestone-2/               Input/output syntax analysis
  milestone-3/               Input/output semantic analysis
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
| `5` / `semua` | Source code pada `test/milestone-1/` | Seluruh pipeline | Token, parse tree, dan hasil semantic |

Catatan: mode `semantic` menjalankan lexer dan parser terlebih dahulu untuk membentuk parse tree, lalu hanya meminta nama file keluaran semantic.

### Mode Command

Mode command menerima source file secara langsung. Pada mode ini, program selalu menjalankan pipeline lengkap: lexical analysis, syntax analysis, lalu semantic analysis. Argumen nama output bersifat opsional.

```bash
make run FILE=<source_file_path>
make run FILE=<source_file_path> LEXOUT=<lexical_output_file>
make run FILE=<source_file_path> LEXOUT=<lexical_output_file> SYNOUT=<syntax_output_file>
make run FILE=<source_file_path> LEXOUT=<lexical_output_file> SYNOUT=<syntax_output_file> SEMOUT=<semantic_output_file>
```

Arti argumen:

| Argumen | Keterangan | Lokasi File Output |
|---------|------------|---------------------|
| `FILE` | Path source code yang diproses | - |
| `LEXOUT` | Nama file output token | `test/milestone-1/` |
| `SYNOUT` | Nama file output parse tree | `test/milestone-2/` |
| `SEMOUT` | Nama file output decorated AST, symbol table, dan semantic error | `test/milestone-3/` |

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
```

Argumen file output bersifat positional: output semantic hanya dapat diberikan setelah output lexical dan syntax juga diberikan. Jika file output telah ada, isi file akan ditimpa. Seluruh hasil fase yang dijalankan tetap ditampilkan ke terminal.

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
