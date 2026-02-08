#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <type_traits>

#include "common.h"

namespace allocator {

struct Node {
  size_t size;
  Node* next;
};

template <size_t S, BufferType B = BufferType::HEAP,
          FitStrategy F = FitStrategy::FIRST>
class FreeListAllocator {
 public:
  explicit FreeListAllocator()
    requires(B == BufferType::HEAP);
  explicit FreeListAllocator()
    requires(B == BufferType::STACK);
  explicit FreeListAllocator(std::span<std::byte> buf)
    requires(B == BufferType::EXTERNAL);
  ~FreeListAllocator() noexcept;

  FreeListAllocator(const FreeListAllocator&) = delete;
  FreeListAllocator& operator=(const FreeListAllocator&) = delete;

  FreeListAllocator(FreeListAllocator&&) = delete;
  FreeListAllocator& operator=(FreeListAllocator&&) = delete;

  [[nodiscard]] std::byte* allocate(size_t size, size_t alignment);
  void deallocate(std::byte* ptr, size_t size);
  void reset() noexcept;

  //////////////////////
  // type-safe helpers
  //////////////////////
  template <typename T>
  [[nodiscard]] T* allocate(size_t count = 1);

  template <typename T, typename... Args>
  [[nodiscard]] T* emplace(Args&&... args);

  template <typename T>
  void destroy(T* ptr) noexcept;

 private:
  std::pair<Node* /* previous */, Node* /*current*/> find_first_fit(
      size_t size, size_t alignment)
    requires(F == FitStrategy::FIRST);

  std::pair<Node* /* previous */, Node* /* current */> find_best_fit(
      size_t size, size_t alignment)
    requires(F == FitStrategy::BEST);

std::pair<size_t /* aligned address */, size_t /* required space */> get_allocation_requirements(Node* current, size_t size, size_t alignment);

  Node* handle_next_free(Node* current, size_t required_space,
                         size_t remaining);
  void handle_links(Node* previous, Node* next);

  std::conditional_t<B == BufferType::STACK, std::array<std::byte, S>,
                     std::byte*>
      buffer;
  std::byte* data;
  size_t capacity;
  size_t used;
  Node* head;
};
}  // namespace allocator

#include "free_list_allocator.inl"