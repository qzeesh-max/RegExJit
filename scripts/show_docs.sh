#!/bin/bash
set -e

# Change to the base directory of the project
cd "$(dirname "$0")/.."

# Check for doxygen
if ! command -v doxygen &> /dev/null; then
    echo "Doxygen is required but not installed."
    echo "Please install it via: brew install doxygen"
    exit 1
fi

doxygen Doxyfile

echo "Documentation generated at html/index.html"

# If on Mac, open it
if [[ "$OSTYPE" == "darwin"* ]]; then
    open html/index.html
fi
