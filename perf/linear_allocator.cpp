#include "linear_allocator.h"

#include <benchmark/benchmark.h>

#include <memory>
#include <span>
#include <string>

namespace allocator::perf {

static constexpr size_t CAPACITY{65536};
static constexpr int ROUNDS{100};

struct Obj {
  int x;
  double y;
  Obj(int a, double b) : x(a), y(b) {}
};

template <typename Allocator>
struct Setup {
  std::unique_ptr<Allocator> alloc{};
  std::unique_ptr<std::byte[]> buf{};
  std::span<std::byte> buf_span{};

  Setup() {
    if constexpr (std::is_same_v<Allocator,
                                 LinearAllocator<0, BufferType::EXTERNAL>>) {
      buf = std::make_unique<std::byte[]>(CAPACITY);
      buf_span = std::span(buf.get(), CAPACITY);
      alloc = std::make_unique<Allocator>(buf_span);
    } else {
      alloc = std::make_unique<Allocator>();
    }
  }
};

using HeapAllocator = LinearAllocator<CAPACITY>;
using StackAllocator = LinearAllocator<CAPACITY, BufferType::STACK>;
using ExternalAllocator = LinearAllocator<0, BufferType::EXTERNAL>;

template <typename Allocator>
static void BM_Allocation(::benchmark::State& state) {
  Setup<Allocator> setup{};
  for (auto _ : state) {
    std::byte* ptr{setup.alloc->allocate(64, 8)};
    ::benchmark::DoNotOptimize(ptr);
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Allocation<HeapAllocator>)->Name("BM_Allocation/Heap");
BENCHMARK(BM_Allocation<StackAllocator>)->Name("BM_Allocation/Stack");
BENCHMARK(BM_Allocation<ExternalAllocator>)->Name("BM_Allocation/External");

static void BM_Allocation_Malloc(::benchmark::State& state) {
  for (auto _ : state) {
    void* ptr{std::malloc(64)};
    ::benchmark::DoNotOptimize(ptr);
    std::free(ptr);
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Allocation_Malloc)->Name("BM_Allocation/Malloc");

template <typename Allocator>
static void BM_Emplace(::benchmark::State& state) {
  Setup<Allocator> setup{};
  for (auto _ : state) {
    Obj* obj{setup.alloc->template emplace<Obj>(15, 3.14)};
    ::benchmark::DoNotOptimize(obj);
    setup.alloc->template destroy<Obj>(obj);
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Emplace<HeapAllocator>)->Name("BM_Emplace/Heap");
BENCHMARK(BM_Emplace<StackAllocator>)->Name("BM_Emplace/Stack");
BENCHMARK(BM_Emplace<ExternalAllocator>)->Name("BM_Emplace/External");

static void BM_Emplace_New(::benchmark::State& state) {
  for (auto _ : state) {
    Obj* obj = new Obj(15, 3.14);
    ::benchmark::DoNotOptimize(obj);
    delete obj;
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Emplace_New)->Name("BM_Emplace/New");

//////////////////////////////
// workload benchmarks
//////////////////////////////

template <typename Allocator>
static void BM_Workload(::benchmark::State& state) {
  Setup<Allocator> setup{};
  Obj* objects[ROUNDS];

  for (auto _ : state) {
    for (int i{}; i < ROUNDS; ++i) {
      objects[i] = setup.alloc->template emplace<Obj>(i, i * 1.5);
      ::benchmark::DoNotOptimize(objects[i]);
    }

    for (int i{}; i < ROUNDS; ++i) {
      setup.alloc->template destroy<Obj>(objects[i]);
    }

    setup.alloc->reset();
  }
  state.SetItemsProcessed(state.iterations() * ROUNDS);
}
BENCHMARK(BM_Workload<HeapAllocator>)->Name("BM_Workload/Heap");
BENCHMARK(BM_Workload<StackAllocator>)->Name("BM_Workload/Stack");
BENCHMARK(BM_Workload<ExternalAllocator>)->Name("BM_Workload/External");

static void BM_Workload_New(::benchmark::State& state) {
  Obj* objects[ROUNDS];
  for (auto _ : state) {
    for (int i{}; i < ROUNDS; ++i) {
      objects[i] = new Obj(i, i * 1.5);
      ::benchmark::DoNotOptimize(objects[i]);
    }

    for (int i{}; i < ROUNDS; ++i) {
      delete objects[i];
    }
  }
  state.SetItemsProcessed(state.iterations() * ROUNDS);
}
BENCHMARK(BM_Workload_New)->Name("BM_Workload/New");

}  // namespace allocator::perf

BENCHMARK_MAIN();