#include "linear_allocator.h"

#include <algorithm>
#include <stdexcept>

#include "common.h"

LinearAllocator::LinearAllocator(size_t c)
    : capacity(c), offset(0), previous_offset(0) {
  buffer = static_cast<std::byte*>(::operator new(c));
}

LinearAllocator::~LinearAllocator() { ::operator delete(buffer); }

std::byte* LinearAllocator::allocate(size_t size, size_t alignment) {
  if (!allocator::is_valid_alignment(alignment)) {
    throw std::invalid_argument("alignment must be a power of 2");
  }
  size_t aligned{allocator::align_forward(offset, alignment)};

  size_t new_offset{aligned + size};

  if (new_offset <= capacity) {
    previous_offset = aligned;
    offset = new_offset;
    return buffer + aligned;
  } else {
    return nullptr;
  }
}

void LinearAllocator::deallocate([[maybe_unused]] std::byte* ptr) {
  // not supported in linear allocators
}

void LinearAllocator::reset() {
  std::fill_n(buffer, capacity, std::byte{0});
  offset = 0;
}

std::byte* LinearAllocator::resize_allocation(std::byte* previous_memory,
                                              size_t previous_size,
                                              size_t new_size,
                                              size_t alignment) {
  if (!allocator::is_valid_alignment(alignment)) {
    throw std::invalid_argument("alignment must be a power of 2");
  }

  if (previous_memory == nullptr || previous_size == 0) {
    return allocate(new_size, alignment);
  }

  if (previous_memory < buffer || previous_memory >= buffer + capacity) {
    throw std::invalid_argument("memory is out of bounds of this allocator");
  }

  if (buffer + previous_offset == previous_memory) {
    size_t new_offset{previous_offset + new_size};

    if (new_offset <= capacity) {
      offset = new_offset;

      if (new_size > previous_size) {
        std::fill_n(previous_memory + previous_size, new_size - previous_size,
                    std::byte{0});
      }

      return previous_memory;
    }
  }

  std::byte* new_memory{allocate(new_size, alignment)};
  if (new_memory) {
    size_t copy_size{(previous_size < new_size) ? previous_size : new_size};
    std::copy_n(previous_memory, copy_size, new_memory);
  }
  return new_memory;
}

void LinearAllocator::resize_buffer(size_t new_capacity) {
  ::operator delete(buffer);
  buffer = static_cast<std::byte*>(::operator new(new_capacity));
  capacity = new_capacity;
  offset = 0;
  previous_offset = 0;
}
