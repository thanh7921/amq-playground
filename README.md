# Some AMQs code

## How to Build & Run

### A. Run Verification

```bash
# Configure & compile with sanitizers for safety testing
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
cmake --build build -j

# Execute test
./build/verify
```

### B. Run Benchmarks (Release Mode)

```bash
# Clean build in Release mode (disable sanitizers for true performance)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_SANITIZERS=OFF
cmake --build build -j

# Run the benchmark
./build/bench_amq
```

### C. Run Micro-architectural Profiling (`perf`) - need update

On Linux:

```bash
perf stat -e cycles,instructions,L1-dcache-load-misses,branch-misses ./build/bench_amq --benchmark_filter=BM_Standard_LookupHit
perf stat -e cycles,instructions,L1-dcache-load-misses,branch-misses ./build/bench_amq --benchmark_filter=BM_Standard_LookupMiss
perf stat -e cycles,instructions,L1-dcache-load-misses,branch-misses ./build/bench_amq --benchmark_filter=BM_Counting_LookupHit
perf stat -e cycles,instructions,L1-dcache-load-misses,branch-misses ./build/bench_amq --benchmark_filter=BM_Counting_LookupHit
```
