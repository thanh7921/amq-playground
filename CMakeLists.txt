cmake_minimum_required(VERSION 3.24)
project(amq_playground LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Option to enable Address & Undefined Behavior Sanitizers
option(ENABLE_SANITIZERS "Enable Address and Undefined sanitizers" OFF)

set(COMMON_COMPILE_OPTIONS -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
if (ENABLE_SANITIZERS)
  list(APPEND COMMON_COMPILE_OPTIONS -fsanitize=address,undefined
         -fno-omit-frame-pointer -g)
endif()

# Header-only core library
add_library(amq_core INTERFACE)
target_include_directories(amq_core INTERFACE
                           ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(amq_core INTERFACE ${COMMON_COMPILE_OPTIONS})
if (ENABLE_SANITIZERS)
  target_link_options(amq_core INTERFACE -fsanitize=address,undefined)
endif()

# 1. Verification executable (Correctness + Empirical FPR)
add_executable(verify src/verify.cpp)
target_link_libraries(verify PRIVATE amq_core)

# 2. Benchmarking executable (Google Benchmark via FetchContent)
include(FetchContent)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    google_benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.10.0
)
FetchContent_MakeAvailable(google_benchmark)

add_executable(bench_amq bench/bench_amq.cpp)
target_link_libraries(bench_amq PRIVATE amq_core benchmark::benchmark
                      benchmark::benchmark_main)
