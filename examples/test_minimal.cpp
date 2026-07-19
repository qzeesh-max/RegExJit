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

using namespace regexjit;

int main() {
    std::cout << "Compiling regex..." << std::endl;
    auto re = Regex::compile("abc");
    std::cout << "Running match..." << std::endl;
    bool m = re.match("abc").matched;
    std::cout << "Match result: " << m << std::endl;
    return 0;
}
