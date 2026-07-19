#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "regexjit/Regex.h"

using namespace regexjit;

struct Rule {
    Regex re;
    std::string replacement;
};

int main(int argc, char** argv) {
    if (argc < 3 || argc % 2 != 0) {
        std::cerr << "Usage: regexjit_find_substitute <find1> <subst1> [<find2> <subst2>...] <file>\n";
        return 1;
    }

    std::vector<Rule> rules;
    for (int i = 1; i < argc - 1; i += 2) {
        try {
            rules.push_back({Regex::compile(argv[i]), argv[i+1]});
        } catch (const std::exception& e) {
            std::cerr << "Failed to compile regex '" << argv[i] << "': " << e.what() << "\n";
            return 1;
        }
    }

    std::string filename = argv[argc - 1];
    std::ifstream in(filename);
    if (!in) {
        std::cerr << "regexjit_find_substitute: " << filename << ": No such file or directory\n";
        return 1;
    }

    std::string line;
    while (std::getline(in, line)) {
        std::string current = line;
        for (const auto& rule : rules) {
            current = rule.re.substitute(current, rule.replacement);
        }
        std::cout << current << "\n";
    }

    return 0;
}
