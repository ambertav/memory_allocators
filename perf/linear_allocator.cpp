#include "benchmark_setup.h"
#include "linear_allocator.h"

#include <benchmark/benchmark.h>

#include <memory>
#include <span>


namespace allocator::perf {

using HeapAllocator = LinearAllocator<CAPACITY>;
using StackAllocator = LinearAllocator<CAPACITY, BufferType::STACK>;
using ExternalAllocator = LinearAllocator<0, BufferType::EXTERNAL>;

BENCHMARK(BM_Allocation<HeapAllocator>)->Name("BM_Allocation/Linear/Heap");
BENCHMARK(BM_Allocation<StackAllocator>)->Name("BM_Allocation/Linear/Stack");
BENCHMARK(BM_Allocation<ExternalAllocator>)->Name("BM_Allocation/Linear/External");

BENCHMARK(BM_Emplace<HeapAllocator>)->Name("BM_Emplace/Linear/Heap");
BENCHMARK(BM_Emplace<StackAllocator>)->Name("BM_Emplace/Linear/Stack");
BENCHMARK(BM_Emplace<ExternalAllocator>)->Name("BM_Emplace/Linear/External");

//////////////////////////////
// workload benchmarks
//////////////////////////////

BENCHMARK(BM_Workload<HeapAllocator>)->Name("BM_Workload/Linear/Heap");
BENCHMARK(BM_Workload<StackAllocator>)->Name("BM_Workload/Linear/Stack");
BENCHMARK(BM_Workload<ExternalAllocator>)->Name("BM_Workload/Linear/External");

}  // namespace allocator::perf