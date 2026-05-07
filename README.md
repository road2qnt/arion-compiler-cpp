# Tugas Besar IF2224 - Teori Bahasa Formal dan Automata

### Identitas Kelompok

Kelompok std_abs
Kode ABS
| NIM | Nama |
|-----|------|
| 13524115 | Ega Luthfi Rais |
| 13524134 | Salman Faiz Assidqi |
| 13524141 | Ahmad Fauzan Putra |
| 13524146 | Leonardus Brain Fatolosja |

### Deskripsi Program

Program ini merupakan implementasi Lexical Analyzer berbasis Deterministic Finite Automata (DFA) untuk bahasa Arion. Lexer membaca source code dari file teks karakter per karakter, melakukan transisi state sesuai DFA, dan menghasilkan daftar token yang sesuai.

Program ini juga sudah dilengkapi Syntax Analyzer berbasis Recursive Descent Parser. Parser menerima list token dari lexer atau file hasil tokenisasi, memeriksa susunan token berdasarkan grammar bahasa Arion, menampilkan syntax error jika ditemukan ketidaksesuaian, dan menghasilkan parse tree.

### Struktur Direktori

bin/            binary
src/            Source code 
doc/            Laporan
test/           File input dan output pengujian
  milestone-1/  Pengujian milestone 1
  milestone-2/  Pengujian milestone 2


### Requirements

- g++ dengan C++17
- GNU Make

### Cara Instalasi dan Penggunaan Program ###

### Build

```bash
make clean           
make       
```

### Penggunaan Lexical + Syntax Analyzer

Program utama hasil build adalah `bin/arion`.

#### Mode Interaktif

```bash
make run
```

Program akan menanyakan:
- Mode yang ingin dijalankan: lexical, syntax, atau keduanya
- File input
- File output lexical atau syntax jika ingin disimpan ke file

Pada mode interaktif, nama file cukup ditulis relatif terhadap folder yang diminta program:
- Mode lexical memakai folder `test/milestone-1/`
- Mode syntax memakai folder `test/milestone-2/`
- Mode keduanya memakai input source dari `test/milestone-1/`, output lexical ke `test/milestone-1/`, dan output syntax ke `test/milestone-2/`

#### Mode Command

```bash
make run FILE=<source_file_path>
make run FILE=<source_file_path> LEXOUT=<lexical_output_file_name>
make run FILE=<source_file_path> LEXOUT=<lexical_output_file_name> SYNOUT=<syntax_output_file_name>
```

Pada mode command:
- `FILE` adalah path source file yang akan dibaca lexer.
- `LEXOUT` adalah nama file output lexical dan akan disimpan ke `test/milestone-1/`.
- `SYNOUT` adalah nama file output syntax dan akan disimpan ke `test/milestone-2/`.

Contoh:
```bash
make run FILE=test/milestone-1/input-1.txt
make run FILE=test/milestone-1/input-1.txt LEXOUT=lexical-output-1.txt
make run FILE=test/milestone-1/input-1.txt LEXOUT=lexical-output-1.txt SYNOUT=syntax-output-1.txt
```

Contoh di atas akan menyimpan:
- Token lexical ke `test/milestone-1/lexical-output-1.txt`
- Parse tree syntax ke `test/milestone-2/syntax-output-1.txt`

Atau jalankan binary langsung:
```bash
./bin/arion test/milestone-1/input-1.txt
./bin/arion test/milestone-1/input-1.txt lexical-output-1.txt syntax-output-1.txt
```

Binary langsung mengikuti aturan yang sama untuk output:
- Argumen kedua disimpan sebagai output lexical di `test/milestone-1/`
- Argumen ketiga disimpan sebagai output syntax di `test/milestone-2/`

Jika file output belum ada, program akan membuat file tersebut. Jika file output sudah ada, isi file akan ditimpa.

### Format File Token

Syntax analyzer dapat membaca file hasil lexical analyzer dengan format:

```text
programsy
ident(Hello)
semicolon
unknown(.23.e-+/1.e-+/1)
```

Token `comment(...)` akan diabaikan oleh parser. Token `unknown(...)` tetap diproses sebagai token tidak dikenal dan dapat menghasilkan syntax error.

### Output

- Lexical analyzer menghasilkan daftar token.
- Syntax analyzer menghasilkan parse tree.
- Jika output file tidak diberikan, hasil ditampilkan ke terminal.
- Jika output file diberikan, hasil tetap ditampilkan ke terminal dan juga disimpan ke file.


### Pembagian Tugas

#### Milestone 1
| Nama | Pembagian Tugas | Kontribusi |
|------|----------------|------------|
| Ega Luthfi Rais | Project Manager, Inisiasi GitHub, Design struktur GitHub, Assist Design dan Implementasi DFA | 25% |
| Salman Faiz Assidqi | Laporan (Implementasi DFA) dan debugging  | 25% |
| Ahmad Fauzan Putra | Implementasi DFA (token.hpp dan token.cpp), pengujian | 25% |
| Leonardus Brain Fatolosja | Pembuatan DFA, Laporan (Teori singkat dan Penjelasan DFA) | 25% |

#### Milestone 2
| Nama | Pembagian Tugas | Kontribusi |
|------|----------------|------------|
| Ega Luthfi Rais | Project Manager, Inisiasi GitHub, Design struktur gitHub, Assist Design dan Implementasi awal Recursive Descent | 25% |
| Salman Faiz Assidqi | Laporan  | 25% |
| Ahmad Fauzan Putra | Laporan dan Testing | 25% |
| Leonardus Brain Fatolosja | Laporan, Perancangan Program, Implementasi Parser dengan menggunakan Recursive Descent dan Testing | 25% |
