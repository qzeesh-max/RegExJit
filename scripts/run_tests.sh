#!/bin/bash
set -e

# Change to project root directory
cd "$(dirname "$0")/.."

echo "=== Building RegExJit (Native) ==="
mkdir -p build
cd build
cmake ..
make -j4

echo ""
echo "=== Running Unit Tests (Native) ==="
./tests/test_regexjit --log_level=test_suite
