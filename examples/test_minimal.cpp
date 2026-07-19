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
