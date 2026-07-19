#!/bin/bash
set -e

# Change to the base directory of the project
cd "$(dirname "$0")/.."

mkdir -p build_coverage
cd build_coverage

# Configure with coverage flags
cmake -DCMAKE_CXX_FLAGS="--coverage" -DCMAKE_EXE_LINKER_FLAGS="--coverage" ..
make -j4

# Run the unit tests
./tests/regexjit_tests || true

# Generate coverage data
lcov --capture --directory . --output-file coverage.info

# Filter out asmjit and boost test dependencies
lcov --remove coverage.info '/usr/*' '*/_deps/*' '*/tests/*' --output-file coverage.info

# Generate HTML report
genhtml coverage.info --output-directory out

echo "Coverage report generated at build_coverage/out/index.html"

# If on Mac, optionally open it
if [[ "$OSTYPE" == "darwin"* ]]; then
    open out/index.html
fi
