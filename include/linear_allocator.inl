#pragma once

#include "linear_allocator.h"

#include <memory>
#include <utility>

namespace allocator {
template <size_t S, BufferType B>
LinearAllocator<S, B>::LinearAllocator()
  requires(B == BufferType::HEAP)
    : buffer(static_cast<std::byte*>(::operator new(S))),
      data(buffer),
      capacity(S),
      offset(0),
      previous_offset(0) {}

template <size_t S, BufferType B>
LinearAllocator<S, B>::LinearAllocator()
  requires(B == BufferType::STACK)
    : buffer(),
      data(buffer.data()),
      capacity(S),
      offset(0),
      previous_offset(0) {}

template <size_t S, BufferType B>
LinearAllocator<S, B>::LinearAllocator(std::span<std::byte> buf)
  requires(B == BufferType::EXTERNAL)
    : buffer(buf.data()),
      data(buf.data()),
      capacity(buf.size()),
      offset(0),
      previous_offset(0) {}

template <size_t S, BufferType B>
LinearAllocator<S, B>::~LinearAllocator() {
  if constexpr (B == BufferType::HEAP) {
    ::operator delete(buffer);
  }
}

template <size_t S, BufferType B>
std::byte* LinearAllocator<S, B>::allocate(size_t size, size_t alignment) {
  if (!is_valid_alignment(alignment)) {
    return nullptr;
  }
  size_t aligned{align_forward(offset, alignment)};

  size_t new_offset{aligned + size};
  if (new_offset > capacity) {
    return nullptr;
  } else {
    previous_offset = aligned;
    offset = new_offset;
    return (data + aligned);
  }
}

template <size_t S, BufferType B>
void LinearAllocator<S, B>::deallocate([[maybe_unused]] std::byte* ptr) {
  // not supported in linear allocators
}

template <size_t S, BufferType B>
void LinearAllocator<S, B>::reset() {
  previous_offset = 0;
  offset = 0;
}

template <size_t S, BufferType B>
std::byte* LinearAllocator<S, B>::resize_last(std::byte* previous_memory,
                                              size_t previous_size,
                                              size_t new_size,
                                              size_t alignment) {
  size_t previous_aligned{align_forward(previous_offset, alignment)};
  if (data + previous_aligned != previous_memory) {
    return nullptr;
  }

  offset = previous_offset;
  return allocate(new_size, alignment);
}

template <size_t S, BufferType B>
template <typename T>
T* LinearAllocator<S, B>::allocate(size_t count) {
  size_t size{sizeof(T) * count};
  size_t alignment{alignof(T)};
  return reinterpret_cast<T*>(allocate(size, alignment));
}

template <size_t S, BufferType B>
template <typename T, typename... Args>
T* LinearAllocator<S, B>::emplace(Args&&... args) {
  size_t size{sizeof(T)};
  size_t alignment{alignof(T)};

  std::byte* ptr{allocate(size, alignment)};
  if (!ptr) {
    return nullptr;
  }

  return std::construct_at(reinterpret_cast<T*>(ptr),
                           std::forward<Args>(args)...);
}

template <size_t S, BufferType B>
template <typename T>
void LinearAllocator<S, B>::destroy(T* ptr) {
  size_t alignment{alignof(T)};
  size_t previous_aligned{align_forward(previous_offset, alignment)};
  if (ptr && data + previous_aligned == reinterpret_cast<std::byte*>(ptr)) {
    std::destroy_at(ptr);
    offset = previous_offset;
  }
}
}  // namespace allocator