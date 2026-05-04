#ifndef PARSER_TRACE_HPP
#define PARSER_TRACE_HPP

#include <iostream>
#include <string>

// Ubah ke 1 untuk menyalakan trace, 0 untuk mematikan
#define ENABLE_PARSER_TRACE 1

class ParserTrace {
private:
    std::string funcName;
    inline static int depth = 0;

    void printIndent() const {
        for (int i = 0; i < depth; ++i) {
            std::cout << "│   ";
        }
    }

public:
    ParserTrace(const std::string& name) : funcName(name) {
#if ENABLE_PARSER_TRACE
        printIndent();
        std::cout << "├── [+] Masuk: " << funcName << std::endl;
        depth++;
#endif
    }

    ~ParserTrace() {
#if ENABLE_PARSER_TRACE
        depth--;
        printIndent();
        std::cout << "└── [-] Keluar: " << funcName << std::endl;
#endif
    }

    static void log(const std::string& msg) {
#if ENABLE_PARSER_TRACE
        for (int i = 0; i < depth; ++i) {
            std::cout << "│   ";
        }
        std::cout << "    [!] " << msg << std::endl;
#endif
    }
};

#if ENABLE_PARSER_TRACE
#define TRACE() ParserTrace __trace__(__func__)
#define TRACE_MSG(msg) ParserTrace::log(msg)
#else
#define TRACE()
#define TRACE_MSG(msg)
#endif

#endif
