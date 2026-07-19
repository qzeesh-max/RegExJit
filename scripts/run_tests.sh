#!/bin/bash
set -e

cd "$(dirname "$0")"

echo "=== Building RegExJit ==="
mkdir -p build
cd build
cmake ..
make -j4

echo ""
echo "=== Running Unit Tests ==="
./tests/test_regexjit --log_level=test_suite
