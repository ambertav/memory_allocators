#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <type_traits>
#include <cstdint>

#include "common.h"

namespace allocator {


struct Header {
    size_t block_size;
    size_t padding;
};


struct Node {
    Node* next;
  size_t size;
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

  [[nodiscard]] std::byte* allocate(size_t size, size_t alignment) noexcept;
  void deallocate(std::byte* ptr) noexcept;
  void reset() noexcept;

  size_t get_used() noexcept;
  size_t get_free() noexcept;

  //////////////////////
  // type-safe helpers
  //////////////////////
  template <typename T>
  [[nodiscard]] T* allocate(size_t count = 1) noexcept;

  template <typename T>
  void deallocate(T* ptr) noexcept;

  template <typename T, typename... Args>
  [[nodiscard]] T* emplace(Args&&... args);

  template <typename T>
  void destroy(T* ptr) noexcept;

 private:
  std::pair<Node* /* previous */, Node* /*current*/> find_first_fit(
      size_t size, size_t alignment) noexcept
    requires(F == FitStrategy::FIRST);

  std::pair<Node* /* previous */, Node* /* current */> find_best_fit(
      size_t size, size_t alignment) noexcept
    requires(F == FitStrategy::BEST);

  std::pair<uintptr_t /* aligned_block */, size_t /* padding */>
  get_allocation_requirements(Node* current, size_t size,
                              size_t alignment) noexcept;

  Node* handle_next_free(Node* current, size_t required_space,
                         size_t remaining) noexcept;
  void handle_links(Node* previous, Node* next) noexcept;

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