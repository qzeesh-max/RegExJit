// RegExJit - A fast JIT compiled regex engine
// Copyright (C) 2026 Zeeshan Qazi
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

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
