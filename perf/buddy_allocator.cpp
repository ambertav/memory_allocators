#include "buddy_allocator.h"

#include <benchmark/benchmark.h>

#include "benchmark_setup.h"

namespace allocator::perf {
using BuddyAllocatorHeap = BuddyAllocator<CAPACITY>;
using BuddyAllocatorStack = BuddyAllocator<CAPACITY, BufferType::STACK>;
using BuddyAllocatorExternal = BuddyAllocator<CAPACITY, BufferType::EXTERNAL>;

//////////////////////////////
// allocation benchmarks
//////////////////////////////

BENCHMARK(BM_Allocation<BuddyAllocatorHeap>)->Name("BM_Allocation/Buddy/Heap");
BENCHMARK(BM_Allocation<BuddyAllocatorStack>)->Name("BM_Allocation/Buddy/Stack");
BENCHMARK(BM_Allocation<BuddyAllocatorExternal>)->Name("BM_Allocation/Buddy/External");

//////////////////////////////
// emplace benchmarks
//////////////////////////////

BENCHMARK(BM_Emplace<BuddyAllocatorHeap>)->Name("BM_Emplace/Buddy/Heap");
BENCHMARK(BM_Emplace<BuddyAllocatorStack>)->Name("BM_Emplace/Buddy/Stack");
BENCHMARK(BM_Emplace<BuddyAllocatorExternal>)->Name("BM_Emplace/Buddy/External");

//////////////////////////////
// workload benchmarks
//////////////////////////////

BENCHMARK(BM_Workload<BuddyAllocatorHeap>)->Name("BM_Workload/Buddy/Heap");
BENCHMARK(BM_Workload<BuddyAllocatorStack>)->Name("BM_Workload/Buddy/Stack");
BENCHMARK(BM_Workload<BuddyAllocatorExternal>)->Name("BM_Workload/Buddy/External");

}  // namespace allocator::perf