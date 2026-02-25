#include "benchmark_setup.h"
#include <benchmark/benchmark.h>


namespace allocator::perf {
static void BM_Allocation_Malloc(::benchmark::State& state) {
  for (auto _ : state) {
    void* ptr{std::malloc(64)};
    ::benchmark::DoNotOptimize(ptr);
    std::free(ptr);
  }
  state.SetItemsProcessed(state.iterations());
}

static void BM_Emplace_New(::benchmark::State& state) {
  for (auto _ : state) {
    Obj* obj {new Obj{15, 3.14}};
    ::benchmark::DoNotOptimize(obj);
    delete obj;
  }
  state.SetItemsProcessed(state.iterations());
}

static void BM_Workload_New(::benchmark::State& state) {
  Obj* objects[ROUNDS];
  for (auto _ : state) {
    for (int i{}; i < ROUNDS; ++i) {
      objects[i] = new Obj{i, i * 1.5};
      ::benchmark::DoNotOptimize(objects[i]);
    }

    for (int i{}; i < ROUNDS; ++i) {
      delete objects[i];
    }
  }
  state.SetItemsProcessed(state.iterations() * ROUNDS);
}

BENCHMARK(BM_Allocation_Malloc)->Name("BM_Allocation/STL/Malloc");
BENCHMARK(BM_Emplace_New)->Name("BM_Emplace/STL/New");
BENCHMARK(BM_Workload_New)->Name("BM_Workload/STL/New");
}  // namespace allocator::perf