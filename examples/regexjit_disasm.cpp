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
        std::cout << "Compiling pattern: " << pattern << std::endl << std::flush;
        
        auto re = Regex::compile(pattern, true);
        
        std::cout << "Successfully compiled!\n" << std::flush;
    } catch (const std::exception& e) {
        std::cerr << "Compilation failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
