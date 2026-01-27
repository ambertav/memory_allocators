#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <type_traits>

#include "common.h"

namespace allocator {
template <size_t S, BufferType B = BufferType::HEAP>
class LinearAllocator {
 public:
  explicit LinearAllocator()
    requires(B == BufferType::HEAP);
  explicit LinearAllocator()
    requires(B == BufferType::STACK);
  explicit LinearAllocator(std::span<std::byte> buf)
    requires(B == BufferType::EXTERNAL);
  ~LinearAllocator();

  std::byte* allocate(size_t size, size_t alignment);
  void deallocate(std::byte* ptr);
  void reset();

  std::byte* resize_last(std::byte* previous_memory, size_t previous_size,
                         size_t new_size, size_t alignment);

  //////////////////////
  // type-safe helpers
  //////////////////////
  template <typename T>
  T* allocate(size_t count = 1);

  template <typename T, typename... Args>
  T* emplace(Args&&... args);

  template <typename T>
  void destroy(T* ptr);

 private:
  std::conditional_t<B == BufferType::STACK, std::array<std::byte, S>,
                     std::byte*>
      buffer;
  std::byte* data;
  size_t capacity;
  size_t offset;
  size_t previous_offset;
};
}  // namespace allocator

#include "linear_allocator.inl"