#!/bin/bash
set -e

# Change directory to project root
cd "$(dirname "$0")/.."

# Function to prompt for user input
prompt_user() {
    local prompt_msg="$1"
    local response
    read -r -p "$prompt_msg [y/N]: " response
    case "$response" in
        [yY][eE][sS]|[yY]) 
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

# Function to check if docker is installed
check_docker() {
    if ! command -v docker &> /dev/null; then
        echo "Error: Docker is not installed or not in PATH."
        echo "This script uses Docker to cross-compile and run tests on Linux x86_64."
        echo ""
        
        if [[ "$OSTYPE" == "darwin"* ]]; then
            echo "On macOS, you can install Docker Desktop or Colima."
            if prompt_user "Would you like to open the Docker Desktop download page?"; then
                open "https://www.docker.com/products/docker-desktop"
            fi
            echo "Alternatively, you can install it via Homebrew: brew install --cask docker"
        elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
            echo "On Linux, you can install Docker using your package manager."
            echo "Example for Ubuntu/Debian: sudo apt-get install docker.io"
        elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "win32" ]]; then
            echo "On Windows, you can install Docker Desktop."
            if prompt_user "Would you like to open the Docker Desktop download page?"; then
                start "https://www.docker.com/products/docker-desktop"
            fi
        fi
        exit 1
    fi
    
    # Check if docker daemon is running
    if ! docker info &> /dev/null; then
        echo "Error: Docker is installed but the daemon is not running."
        echo "Please start Docker (e.g., open Docker Desktop) and try again."
        exit 1
    fi
}

echo "=== RegExJit Cross-Platform Testing ==="
check_docker

echo ""
echo "=== 1. Running Native Tests ==="
./scripts/run_tests.sh

echo ""
echo "=== 2. Running Linux x86_64 Tests via Docker ==="

# We use an Ubuntu container to build and test for x86_64.
# Note: For ARM64 Macs, Docker will run this via QEMU user-mode emulation.
echo "-> Starting Ubuntu 24.04 x86_64 container..."
docker run --rm \
    --platform linux/amd64 \
    -v "$PWD:/app" \
    -w /app \
    ubuntu:24.04 \
    bash -c "\
        echo '-> Installing dependencies in container (this may take a moment)...' && \
        apt-get update -qq && \
        apt-get install -y -qq build-essential cmake git libboost-test-dev > /dev/null 2>&1 && \
        echo '-> Building RegExJit for Linux x86_64...' && \
        mkdir -p build_amd64 && \
        cd build_amd64 && \
        cmake .. && \
        make -j4 && \
        echo '-> Running Unit Tests...' && \
        ./tests/test_regexjit --log_level=test_suite \
    " || {
        echo ""
        echo "=========================================================================="
        echo "WARNING: The Linux x86_64 tests failed."
        echo "If you are running this on an ARM64 host (like an Apple Silicon Mac),"
        echo "this failure may be due to a known QEMU user-mode emulation bug where"
        echo "dynamically generated JIT code triggers segmentation faults or ICEs."
        echo "This is an emulator limitation, not a defect in the x86 code generator."
        echo "=========================================================================="
        exit 1
    }

echo ""
echo "=== Cross-Platform Testing Completed Successfully ==="
