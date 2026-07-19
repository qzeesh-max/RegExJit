#include "regexjit/Regex.h"
#include <iostream>
#include <string>

using namespace regexjit;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <regex_pattern>\n";
        std::cerr << "Compiles the regex pattern and prints the JIT machine code assembly.\n";
        return 1;
    }

    std::string pattern = argv[1];

    try {
        std::cout << "Compiling pattern: " << pattern << "\n";
        // Passing disassemble = true
        Regex::compile(pattern, true);
    } catch (const std::exception& e) {
        std::cerr << "Compilation failed: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
