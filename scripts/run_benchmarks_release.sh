#!/bin/bash
set -e
cd "$(dirname "$0")/.."
echo "=== Building RegExJit (Release) ==="
mkdir -p build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4
echo ""
echo "=== Running Benchmarks (Release) ==="
./benchmarks/regexjit_benchmark "$@"
