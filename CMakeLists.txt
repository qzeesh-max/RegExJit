cmake_minimum_required(VERSION 3.25)
project(RegExJit VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Enable testing
enable_testing()

# Fetch AsmJit
include(FetchContent)
FetchContent_Declare(
    asmjit
    GIT_REPOSITORY https://github.com/asmjit/asmjit.git
    GIT_TAG        master
)
# AsmJit doesn't need to install when embedded
set(ASMJIT_EMBED ON CACHE BOOL "")
FetchContent_MakeAvailable(asmjit)

# Find Boost
find_package(Boost REQUIRED COMPONENTS unit_test_framework)

# Main Library
add_library(regexjit
    src/Regex.cpp
    src/AST.cpp
    src/Parser.cpp
    src/Compiler.cpp
)


target_include_directories(regexjit
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(regexjit
    PUBLIC
        asmjit::asmjit
)

add_subdirectory(tests)
add_subdirectory(examples)
add_subdirectory(benchmarks)
