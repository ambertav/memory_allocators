#pragma once

#include <cstddef>

class LinearAllocator {
 public:
  explicit LinearAllocator(size_t c);
  ~LinearAllocator();

  std::byte* allocate(size_t size, size_t alignment);
  void deallocate(std::byte* ptr);
  void reset();

  std::byte* resize_allocation(std::byte* previous_memory, size_t previous_size, size_t new_size, size_t alignment);
  void resize_buffer(size_t new_capacity);

 private:
  std::byte* buffer;
  size_t capacity;
  size_t offset;
  size_t previous_offset;
};