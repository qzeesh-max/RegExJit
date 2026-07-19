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

#### Benchmark Results (Windows / MSVC)
```
========================================================================================================================
Benchmark Name                Find JIT (mean±std)     Find std (mean±std) Sub JIT (mean±std)      Sub std (mean±std)
------------------------------------------------------------------------------------------------------------------------
Email Pattern (Found)               1.2 ± 0.4 µs           5.4 ± 0.8 µs  1974.4 ± 60.2 µs      15605.4 ± 78.1 µs
Target at end (Find)               35.8 ± 4.6 µs          33.2 ± 2.5 µs    64.8 ± 31.8 µs        824.6 ± 50.4 µs
Complex Not Found               1015.2 ± 14.0 µs       1182.0 ± 24.3 µs  1129.2 ± 52.2 µs       1966.2 ± 55.9 µs
Alphanumeric seq                    0.4 ± 0.5 µs           0.4 ± 0.5 µs  1761.0 ± 47.2 µs      9228.2 ± 261.9 µs
Alternatives                        0.4 ± 0.5 µs           0.2 ± 0.4 µs  1617.6 ± 46.5 µs       1423.8 ± 25.5 µs
Backreference search                0.8 ± 0.4 µs           2.0 ± 0.0 µs  2388.0 ± 61.0 µs     21041.2 ± 472.1 µs
Backref substitute                  1.2 ± 0.4 µs           5.6 ± 0.8 µs  2452.4 ± 73.5 µs     20588.2 ± 551.3 µs
Negative: Wrong domain          2055.4 ± 84.4 µs     16519.2 ± 401.1 µs  2171.0 ± 59.1 µs     17832.0 ± 360.7 µs
Negative: Backref mismatch       935.0 ± 19.8 µs       1402.8 ± 41.8 µs  1043.2 ± 55.9 µs       2261.0 ± 34.7 µs
Negative: Anchor mismatch         721.6 ± 6.3 µs           0.0 ± 0.0 µs   892.0 ± 37.1 µs        891.0 ± 24.4 µs
========================================================================================================================
```

#### Benchmark Results (Linux / GCC)
```
========================================================================================================================
Benchmark Name                Find JIT (mean±std)     Find std (mean±std) Sub JIT (mean±std)      Sub std (mean±std)
------------------------------------------------------------------------------------------------------------------------
Email Pattern (Found)               4.8 ± 5.2 µs           6.4 ± 1.6 µs 2039.2 ± 192.8 µs     23616.0 ± 415.9 µs
Target at end (Find)                5.0 ± 0.6 µs       6055.0 ± 45.9 µs    84.4 ± 10.4 µs      6569.0 ± 272.1 µs
Complex Not Found                950.2 ± 22.4 µs       9104.6 ± 52.7 µs  1047.0 ± 14.7 µs       9606.0 ± 35.2 µs
Alphanumeric seq                    0.0 ± 0.0 µs           0.2 ± 0.4 µs2487.0 ± 1672.6 µs     18296.4 ± 173.0 µs
Alternatives                        0.4 ± 0.5 µs           6.4 ± 0.5 µs  1444.0 ± 94.9 µs     19224.2 ± 235.6 µs
Backreference search                0.0 ± 0.0 µs           0.2 ± 0.4 µs  2255.6 ± 72.0 µs     31615.8 ± 291.5 µs
Backref substitute                  1.0 ± 0.0 µs           6.2 ± 0.4 µs  2260.0 ± 70.0 µs     32030.6 ± 780.8 µs
Negative: Wrong domain         2113.4 ± 109.8 µs     25401.0 ± 219.4 µs  1970.8 ± 20.8 µs     25775.2 ± 132.7 µs
Negative: Backref mismatch       879.4 ± 10.1 µs       9643.8 ± 75.7 µs   964.8 ± 29.8 µs     10114.2 ± 102.8 µs
Negative: Anchor mismatch        748.2 ± 11.9 µs       5886.4 ± 76.8 µs   765.2 ± 18.2 µs       6327.0 ± 97.0 µs
========================================================================================================================
```

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
