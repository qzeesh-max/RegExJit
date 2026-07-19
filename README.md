# RegExJit

RegExJit is a fast, Just-In-Time (JIT) compiled regular expression engine written in C++23. It aims to provide a lightweight, embeddable subset of typical regex operations with significant performance improvements over standard libraries (like `std::regex`) by leveraging `AsmJit` to emit machine code tailored specifically to the given pattern.

## Features

- **JIT Compilation**: Generates native machine code dynamically using `AsmJit` (supports AArch64 and x86_64).
- **Core Regex Support**:
  - Literals, Wildcards (`.`), Anchors (`^`, `$`)
  - Character Classes (`[a-z]`, `[^0-9]`)
  - Quantifiers (`*`, `+`, `?`, `{min,max}`)
  - Grouping and Captures (`(...)`)
  - Alternations (`a|b`)
  - Backreferences in matching and substitutions (`\1`)
- **Operations**:
  - `match(subject)` - strict matching of the whole subject.
  - `find(subject)` - substring search.
  - `substitute(subject, replacement)` - replace matched patterns with a replacement string (supporting capture group injections like `\1`).

## Requirements

- **C++ Compiler**: A compiler supporting C++23 (e.g. Clang 16+, GCC 13+).
- **CMake**: Version 3.14 or later.

## Building the Project

RegExJit uses `asmjit` as its core JIT compilation engine. Instead of downloading it at build-time, `asmjit` is included as a git submodule to guarantee reproducibility.

The project is currently built and tested against AsmJit `master` branch commit SHA: `0bd5787b54b575ed94bf32ac452153b34385c514`.

Before building, make sure you clone the repository with its submodules:
```bash
git clone --recursive https://github.com/qzeesh-max/RegExJit.git
cd RegExJit
```
*(If you already cloned the repository without `--recursive`, run `git submodule update --init --recursive` to fetch AsmJit.)*

### macOS & Linux

```bash
mkdir build
cd build
cmake ..
make -j4
```

### Windows (MSVC)

Using Developer Command Prompt:
```cmd
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

## Running Tests

RegExJit includes a comprehensive set of unit tests and benchmarks. We provide convenient wrapper scripts to build and run these easily.

### Unit Tests
To build and run the unit tests natively on your host machine:
```bash
./scripts/run_tests.sh
```

### Cross-Platform Testing
To ensure the x86_64 JIT compiler backend works correctly even if you are on an ARM64 machine (e.g., Apple Silicon), or vice versa, you can use the cross-platform testing script. This script automatically provisions an Ubuntu Docker container to compile and run the tests. 
```bash
./scripts/run_cross_platform_tests.sh
```
*Note: This script requires Docker to be installed. It is intelligent enough to detect Docker's presence and will prompt you with installation instructions if it is missing.*

### Benchmarks
To see how RegExJit performs against `std::regex`:
```bash
./scripts/run_benchmarks.sh
```
*Note: You can pass the `--verify` flag to the script to run correctness assertions alongside the benchmarking (e.g., `./scripts/run_benchmarks.sh --verify`).*

## Utilities

### Disassembler (`regexjit_disasm`)
You can inspect the generated AArch64 or x86_64 assembly for a given regex pattern.

```bash
./build/examples/regexjit_disasm "([a-z]+) \1"
```

### Coverage Script
To generate a unit test coverage report:
```bash
./scripts/coverage.sh
```
*(Requires `lcov`)*

### Documentation
To generate HTML documentation for the API:
```bash
./scripts/show_docs.sh
```
*(Requires `doxygen`)*

## Usage

```cpp
#include <iostream>
#include "regexjit/Regex.h"

using namespace regexjit;

int main() {
    // Compile a pattern
    auto re = Regex::compile("([a-z]+)@([a-z]+)\\.com");
    
    // Find a match
    auto result = re.find("Contact me at user@domain.com please.");
    if (result.matched) {
        std::cout << "Found email: " << result.match_str << "\n";
        std::cout << "Username: " << result.captures[1] << "\n";
        std::cout << "Domain: " << result.captures[2] << "\n";
    }

    // Substitution with backreferences
    std::string hidden = re.substitute("user@domain.com", "REDACTED@\\2.com");
    std::cout << "Substituted: " << hidden << "\n"; // Output: REDACTED@domain.com
    
    return 0;
}
```

## Limitations
This engine implements a simplified JIT approach (currently without a backtracking stack inside the compiled code). As such, some highly complex patterns involving ambiguous variable-length repetitions alongside extensive backtracking may behave greedily or simplistically compared to PCRE.
