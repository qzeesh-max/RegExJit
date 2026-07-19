#include <iostream>
#include <regex>
#include <string>

int main() {
    std::string text = "test@example.com";
    std::regex re("([a-z]+)@([a-z]+)\\.com");
    std::string rep = "\\1@newdomain.com";
    std::string out = std::regex_replace(text, re, rep, std::regex_constants::format_sed);
    std::cout << out << std::endl;
    return 0;
}
