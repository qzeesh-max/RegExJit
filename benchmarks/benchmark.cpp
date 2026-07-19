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
#include <chrono>
#include <string>
#include <vector>
#include <regex>
#include <cmath>
#include <iomanip>
#include "regexjit/Regex.h"

using namespace regexjit;
using namespace std::chrono;

struct BenchmarkResult {
    std::string name;
    double find_jit_mean;
    double find_jit_stddev;
    double find_std_mean;
    double find_std_stddev;
    
    bool has_sub;
    double sub_jit_mean;
    double sub_jit_stddev;
    double sub_std_mean;
    double sub_std_stddev;
};

std::vector<BenchmarkResult> results;

std::string generate_large_text() {
    std::string text = "This is a base text. It contains some numbers like 12345, some emails like test@example.com, and some random symbols like #@!.\n";
    text += "It also has animals like a cat and a dog, and double words like test test and hello hello.\n";
    std::string large_text;
    large_text.reserve(text.size() * 2000);
    for (int i = 0; i < 2000; i++) {
        large_text += text;
    }
    // Append something at the very end to test "not found" or "found at end"
    large_text += "THE_ULTIMATE_TARGET_STRING_99999\n";
    return large_text;
}

void compute_stats(const std::vector<long long>& times, double& mean, double& stddev) {
    if (times.empty()) {
        mean = 0;
        stddev = 0;
        return;
    }
    double sum = 0;
    for (auto t : times) sum += t;
    mean = sum / times.size();
    
    double sq_sum = 0;
    for (auto t : times) sq_sum += (t - mean) * (t - mean);
    stddev = std::sqrt(sq_sum / times.size());
}

void run_benchmark(const std::string& name, const std::string& pattern, const std::string& text, const std::string& replacement, bool verify) {
    std::cout << "======================================" << std::endl;
    std::cout << "Benchmark: " << name << std::endl;
    
    Regex re_jit = Regex::compile(pattern);
    std::regex re_std(pattern);
    
    const int iterations = 5;
    std::vector<long long> find_jit_times, find_std_times;
    std::vector<long long> sub_jit_times, sub_std_times;
    
    bool has_sub = !replacement.empty();

    for (int i = 0; i < iterations; i++) {
        // Find 
        auto start_find_jit = high_resolution_clock::now();
        auto find_jit = re_jit.find(text);
        auto end_find_jit = high_resolution_clock::now();
        find_jit_times.push_back(duration_cast<milliseconds>(end_find_jit - start_find_jit).count());
        
        auto start_find_std = high_resolution_clock::now();
        std::smatch find_std_m;
        bool found_std = std::regex_search(text, find_std_m, re_std);
        auto end_find_std = high_resolution_clock::now();
        find_std_times.push_back(duration_cast<milliseconds>(end_find_std - start_find_std).count());
        
        // Verify find in first iteration
        if (i == 0 && verify) {
            if (find_jit.matched != found_std) {
                std::cerr << "VERIFICATION FAILED: RegExJit find (matched=" << find_jit.matched << ") != std::regex find (matched=" << found_std << ")\n";
                exit(1);
            }
            if (find_jit.matched && found_std && find_jit.match_str != find_std_m.str(0)) {
                std::cerr << "VERIFICATION FAILED: RegExJit find string ('" << find_jit.match_str << "') != std::regex find string ('" << find_std_m.str(0) << "')\n";
                exit(1);
            }
        }

        // Substitute
        if (has_sub) {
            auto start_sub_jit = high_resolution_clock::now();
            auto sub_jit = re_jit.substitute(text, replacement);
            auto end_sub_jit = high_resolution_clock::now();
            sub_jit_times.push_back(duration_cast<milliseconds>(end_sub_jit - start_sub_jit).count());
            
            auto start_sub_std = high_resolution_clock::now();
            auto sub_std = std::regex_replace(text, re_std, replacement, std::regex_constants::format_sed);
            auto end_sub_std = high_resolution_clock::now();
            sub_std_times.push_back(duration_cast<milliseconds>(end_sub_std - start_sub_std).count());
            
            if (i == 0 && verify && sub_jit != sub_std) {
                std::cerr << "VERIFICATION FAILED: RegExJit substitute output does not match std::regex output.\n";
                exit(1);
            }
        }
    }
    
    BenchmarkResult res;
    res.name = name;
    compute_stats(find_jit_times, res.find_jit_mean, res.find_jit_stddev);
    compute_stats(find_std_times, res.find_std_mean, res.find_std_stddev);
    res.has_sub = has_sub;
    if (has_sub) {
        compute_stats(sub_jit_times, res.sub_jit_mean, res.sub_jit_stddev);
        compute_stats(sub_std_times, res.sub_std_mean, res.sub_std_stddev);
    }
    results.push_back(res);
    
    std::cout << "Done (" << iterations << " iterations)." << std::endl;
}

int main(int argc, char** argv) {
    bool verify = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--verify") {
            verify = true;
            std::cout << "Verification mode ENABLED." << std::endl;
        }
    }

    std::cout << "Generating large text (~0.4MB)..." << std::endl;
    std::string text = generate_large_text();
    std::cout << "Text generation complete. Size: " << text.size() / 1024 << " KB\n" << std::endl;

    run_benchmark("Email Pattern (Found)", "[a-z]+@[a-z]+\\.com", text, "REDACTED_EMAIL", verify);
    run_benchmark("Target at end (Find)", "ULTIMATE_TARGET_STRING_[0-9]+", text, "FOUND_IT", verify);
    run_benchmark("Complex Not Found", "([0-9]{3})-[0-9]{2}-[0-9]{4}", text, "SSN", verify);
    run_benchmark("Alphanumeric seq", "([a-zA-Z0-9]+)", text, "ALNUM", verify);
    run_benchmark("Alternatives", "(cat|dog|bird|fish)", text, "ANIMAL", verify);
    run_benchmark("Backreference search", "([a-z]+) \\1", text, "DOUBLE_WORD", verify);
    run_benchmark("Backref substitute", "([a-z]+)@([a-z]+)\\.com", text, "\\1@newdomain.com", verify);
    
    // Negative test cases
    run_benchmark("Negative: Wrong domain", "[a-z]+@[a-z]+\\.net", text, "REDACTED", verify);
    run_benchmark("Negative: Backref mismatch", "([0-9]+) [a-z]+ \\1", text, "FAIL", verify);
    run_benchmark("Negative: Anchor mismatch", "^This is a base text$", text, "REPLACED", verify);

    std::cout << "\n========================================================================================================================\n";
    std::cout << std::left << std::setw(30) << "Benchmark Name" 
              << std::right << std::setw(20) << "Find JIT (mean±std)" 
              << std::setw(25) << "Find std (mean±std)" 
              << std::setw(20) << "Sub JIT (mean±std)" 
              << std::setw(25) << "Sub std (mean±std)" << "\n";
    std::cout << "------------------------------------------------------------------------------------------------------------------------\n";
    
    for (const auto& r : results) {
        std::cout << std::left << std::setw(30) << r.name << std::right;
        
        char buf[100];
        snprintf(buf, sizeof(buf), "%.1f ± %.1f ms", r.find_jit_mean, r.find_jit_stddev);
        std::cout << std::setw(20) << buf;
        
        snprintf(buf, sizeof(buf), "%.1f ± %.1f ms", r.find_std_mean, r.find_std_stddev);
        std::cout << std::setw(25) << buf;
        
        if (r.has_sub) {
            snprintf(buf, sizeof(buf), "%.1f ± %.1f ms", r.sub_jit_mean, r.sub_jit_stddev);
            std::cout << std::setw(20) << buf;
            
            snprintf(buf, sizeof(buf), "%.1f ± %.1f ms", r.sub_std_mean, r.sub_std_stddev);
            std::cout << std::setw(25) << buf;
        } else {
            std::cout << std::setw(20) << "-" << std::setw(25) << "-";
        }
        std::cout << "\n";
    }
    std::cout << "========================================================================================================================\n";
    
    return 0;
}
