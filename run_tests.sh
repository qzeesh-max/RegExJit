#!/bin/bash
set -e

# Change to the directory where the script is located
cd "$(dirname "$0")"

echo "=> Configuring RegExJit..."
mkdir -p build
cd build
cmake ..

echo "=> Building RegExJit..."
# Use available logical CPUs for parallel build, default to 4 if detection fails
CORES=$(sysctl -n hw.logicalcpu 2>/dev/null || nproc 2>/dev/null || echo 4)
make -j"${CORES}"

echo "=> Running Unit Tests..."
# Use ctest for generic tests, and run the boost test executable with test_suite log level
./tests/test_regexjit --log_level=test_suite

echo "=> All tests passed successfully!"
