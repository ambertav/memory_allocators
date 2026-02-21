#pragma once

#include <cassert>
#include <cstdint>

#include "free_list_allocator.h"

namespace allocator {
template <size_t S, BufferType B, FitStrategy F>
FreeListAllocator<S, B, F>::FreeListAllocator()
  requires(B == BufferType::HEAP)
    : buffer(static_cast<std::byte*>(::operator new(S))),
      data(buffer),
      capacity(S),
      used(0),
      head(reinterpret_cast<Node*>(buffer)) {
  head->size = S - sizeof(Node);
  head->next = nullptr;
}

template <size_t S, BufferType B, FitStrategy F>
FreeListAllocator<S, B, F>::FreeListAllocator()
  requires(B == BufferType::STACK)
    : buffer(std::array<std::byte, S>{}),
      data(buffer.data()),
      capacity(S),
      used(0),
      head(reinterpret_cast<Node*>(buffer.data())) {
  head->next = nullptr;
  head->size = S - sizeof(Node);
}

template <size_t S, BufferType B, FitStrategy F>
FreeListAllocator<S, B, F>::FreeListAllocator(std::span<std::byte> buf)
  requires(B == BufferType::EXTERNAL)
    : buffer(buf.data()),
      data(buf.data()),
      capacity(buf.size()),
      used(0),
      head(reinterpret_cast<Node*>(buf.data())) {
  head->next = nullptr;
  head->size = capacity - sizeof(Node);
}

template <size_t S, BufferType B, FitStrategy F>
FreeListAllocator<S, B, F>::~FreeListAllocator() noexcept {
  if constexpr (B == BufferType::HEAP) {
    ::operator delete(buffer);
  }
}

template <size_t S, BufferType B, FitStrategy F>
std::byte* FreeListAllocator<S, B, F>::allocate(size_t size,
                                                size_t alignment) noexcept {
  if (!is_valid_alignment(alignment)) {
    return nullptr;
  }

  std::pair<Node*, Node*> placement{};
  if constexpr (F == FitStrategy::FIRST) {
    placement = find_first_fit(size, alignment);
  } else if constexpr (F == FitStrategy::BEST) {
    placement = find_best_fit(size, alignment);
  }

  auto& [previous, current] = placement;

  if (current == nullptr) {
    return nullptr;
  }

  auto [aligned_block, padding] =
      get_allocation_requirements(current, size, alignment);
    size_t required_space{size + padding};

  size_t remaining{current->size - required_space};

  Node* next{handle_next_free(current, required_space, remaining)};
  handle_links(previous, next);

  Header* header{reinterpret_cast<Header*>(aligned_block - sizeof(Header))};
  header->block_size = required_space;
  header->padding = padding;

  used += required_space;

  return reinterpret_cast<std::byte*>(aligned_block);
}

template <size_t S, BufferType B, FitStrategy F>
void FreeListAllocator<S, B, F>::deallocate(std::byte* ptr) noexcept {
  if (!ptr) {
    return;
  }

  Header* header{reinterpret_cast<Header*>(ptr - sizeof(Header))};
  assert(ptr >= data && ptr <= data + capacity && "pointer is out of bounds");

  Node* current{head};
  Node* previous{};

  while (current != nullptr) {
    if (reinterpret_cast<std::byte*>(current) > ptr) {
      break;
    }

    previous = current;
    current = current->next;
  }

  std::byte* block_start{ptr - sizeof(Header) - header->padding};
  std::byte* block_end{block_start + header->block_size};
  std::byte* current_start{};
  std::byte* previous_end{};

  if (previous) {
    previous_end = reinterpret_cast<std::byte*>(previous) + previous->size + sizeof(Node);
  }
  if (current) {
    current_start = reinterpret_cast<std::byte*>(current) + sizeof(Node);
  }

  if (previous_end == block_start && current_start == block_end) {
    previous->size += header->block_size + sizeof(Node) + current->size;
    previous->next = current->next;

  } else if (previous_end == block_start) {
    previous->size += header->block_size + sizeof(Node);

  } else if (current_start == block_end) {
    Node* new_node{reinterpret_cast<Node*>(ptr - sizeof(Header) - header->padding)};
    new_node->size = header->block_size + sizeof(Node) + current->size;
    new_node->next = current->next;
    handle_links(previous, new_node);

  } else {
    Node* new_node{reinterpret_cast<Node*>(ptr - sizeof(Header) - header->padding)};
    new_node->size = header->block_size;
    new_node->next = current;
    handle_links(previous, new_node);
  }

  used -= header->block_size;
}

template <size_t S, BufferType B, FitStrategy F>
void FreeListAllocator<S, B, F>::reset() noexcept {
  used = 0;

  if constexpr (B == BufferType::STACK) {
    head = reinterpret_cast<Node*>(buffer.data());
  } else {
    head = reinterpret_cast<Node*>(buffer);
  }

  head->size = S - sizeof(Node);
  head->next = nullptr;
}

template <size_t S, BufferType B, FitStrategy F>
size_t FreeListAllocator<S, B, F>::get_used() noexcept {
    return used;
}

template <size_t S, BufferType B, FitStrategy F>
size_t FreeListAllocator<S, B, F>::get_free() noexcept {
    return capacity - used;
}


//////////////////////
// type-safe helpers
//////////////////////

template <size_t S, BufferType B, FitStrategy F>
template <typename T>
T* FreeListAllocator<S, B, F>::allocate(size_t count) noexcept {
  if (count > SIZE_MAX / sizeof(T)) {
    return nullptr;
  }

  size_t size{sizeof(T) * count};
  size_t alignment{alignof(T)};
  return reinterpret_cast<T*>(allocate(size, alignment));
}

template <size_t S, BufferType B, FitStrategy F>
template <typename T>
void FreeListAllocator<S, B, F>::deallocate(T* ptr) noexcept {
  deallocate(reinterpret_cast<std::byte*>(ptr));
}

template <size_t S, BufferType B, FitStrategy F>
template <typename T, typename... Args>
T* FreeListAllocator<S, B, F>::emplace(Args&&... args) {
  size_t size{sizeof(T)};
  size_t alignment{alignof(T)};

  std::byte* ptr{allocate(size, alignment)};
  if (!ptr) {
    return nullptr;
  }

  return std::construct_at(reinterpret_cast<T*>(ptr),
                           std::forward<Args>(args)...);
}

template <size_t S, BufferType B, FitStrategy F>
template <typename T>
void FreeListAllocator<S, B, F>::destroy(T* ptr) noexcept {
  if (ptr) {
    std::destroy_at(ptr);
  }
}

//////////////////////
// helpers
//////////////////////

template <size_t S, BufferType B, FitStrategy F>
std::pair<Node*, Node*> FreeListAllocator<S, B, F>::find_first_fit(
    size_t size, size_t alignment) noexcept
  requires(F == FitStrategy::FIRST)
{
  Node* current{head};
  Node* previous{};

  while (current != nullptr) {
    auto [aligned_block, padding] =
        get_allocation_requirements(current, size, alignment);
    size_t required_space{size + padding};

    if (current->size >= required_space) {
      return {previous, current};
    }

    previous = current;
    current = current->next;
  }

  return {nullptr, nullptr};
}

template <size_t S, BufferType B, FitStrategy F>
std::pair<Node*, Node*> FreeListAllocator<S, B, F>::find_best_fit(
    size_t size, size_t alignment) noexcept
  requires(F == FitStrategy::BEST)
{
  size_t min_diff{SIZE_MAX};
  Node* best_fit{};
  Node* prev_best_fit{};

  Node* current{head};
  Node* previous{};

  while (current != nullptr) {
    auto [aligned_block, padding] =
        get_allocation_requirements(current, size, alignment);
    size_t required_space{size + padding};

    if (current->size >= required_space) {
      size_t diff{current->size - required_space};

      if (diff == 0) {
        return {previous, current};
      }

      if (diff < min_diff) {
        min_diff = diff;
        best_fit = current;
        prev_best_fit = previous;
      }
    }

    previous = current;
    current = current->next;
  }

  return {prev_best_fit, best_fit};
}

template <size_t S, BufferType B, FitStrategy F>
std::pair<uintptr_t, size_t>
FreeListAllocator<S, B, F>::get_allocation_requirements(
    Node* current, size_t size, size_t alignment) noexcept {
  uintptr_t block{reinterpret_cast<uintptr_t>(current) + sizeof(Node)};
  size_t effective_alignment{std::max(alignment, alignof(Node))};
  size_t aligned_block{align_forward(block, effective_alignment)};
  size_t padding{aligned_block - block};

if (padding < sizeof(Header)) {
  aligned_block += effective_alignment;
  padding += effective_alignment;
}

  return {aligned_block, padding};
}

template <size_t S, BufferType B, FitStrategy F>
Node* FreeListAllocator<S, B, F>::handle_next_free(Node* current,
                                                   size_t required_space,
                                                   size_t remaining) noexcept {
  if (remaining <= sizeof(Node)) {
    return current->next;
  }

  Node* split{reinterpret_cast<Node*>(reinterpret_cast<std::byte*>(current) +
                                      sizeof(Node) + required_space)};

  split->size = remaining - sizeof(Node);
  split->next = current->next;

  return split;
}

template <size_t S, BufferType B, FitStrategy F>
void FreeListAllocator<S, B, F>::handle_links(Node* previous,
                                              Node* next) noexcept {
  if (previous == nullptr) {
    head = next;
  } else {
    previous->next = next;
  }
}

}  // namespace allocator