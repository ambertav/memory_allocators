#pragma once

#include "allocator_interface.h"

class HeapLinearAllocator : public IAllocator {
 public:
  explicit HeapLinearAllocator(size_t c);
  ~HeapLinearAllocator() override;

  std::byte* allocate(size_t size, size_t alignment) override;
  void deallocate(std::byte* ptr) override;
  void reset() override;

  std::string_view get_name() const override;
  std::string_view get_type() const override;

  std::byte* resize_allocation(std::byte* previous_memory, size_t previous_size, size_t new_size, size_t alignment);
  void resize_buffer(size_t new_capacity);

 private:
  std::byte* buffer;
  size_t capacity;
  size_t offset;
  size_t previous_offset;
  std::string_view name;
  std::string_view type;
};