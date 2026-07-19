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

RegExJit uses `FetchContent` to download and build `AsmJit` statically. No external dependencies are required to be pre-installed on the system, keeping integration seamless.

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

Unit tests and benchmarks are built by default.

### Unit Tests
You can verify correctness by running the test suite:
```bash
./build/tests/regexjit_tests
```

### Benchmarks
To see how RegExJit performs against `std::regex`:
```bash
./build/benchmarks/regexjit_benchmark
```
*Note: Use the `--verify` flag to run correctness assertions alongside the benchmarking.*

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
