#include <iostream>
#include <fstream>
#include <string>
#include "regexjit/Regex.h"

using namespace regexjit;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: regexjit_grep <pattern> [file...]\n";
        return 1;
    }

    std::string pattern = argv[1];
    Regex re;
    try {
        re = Regex::compile(pattern);
    } catch (const std::exception& e) {
        std::cerr << "Failed to compile regex: " << e.what() << "\n";
        return 1;
    }

    auto process_stream = [&](std::istream& in, const char* filename) {
        std::string line;
        while (std::getline(in, line)) {
            if (re.find(line).matched) {
                if (filename) {
                    std::cout << filename << ":";
                }
                std::cout << line << "\n";
            }
        }
    };

    if (argc == 2) {
        process_stream(std::cin, nullptr);
    } else {
        for (int i = 2; i < argc; ++i) {
            std::ifstream in(argv[i]);
            if (!in) {
                std::cerr << "regexjit_grep: " << argv[i] << ": No such file or directory\n";
                continue;
            }
            process_stream(in, argc > 3 ? argv[i] : nullptr);
        }
    }

    return 0;
}
