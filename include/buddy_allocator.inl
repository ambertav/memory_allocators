#include <cassert>

#include "buddy_allocator.h"

namespace allocator {
template <size_t S, BufferType B>
BuddyAllocator<S, B>::BuddyAllocator()
  requires(S > 0 && (S & (S - 1)) == 0 && B == BufferType::HEAP)
    : buffer(static_cast<std::byte*>(::operator new(S))),
      data(buffer),
      capacity(S),
      used(0) {
  Block* block{reinterpret_cast<Block*>(data)};
  block->next = nullptr;
  block->level = max_level;
  free_blocks[max_level] = block;
}

template <size_t S, BufferType B>
BuddyAllocator<S, B>::BuddyAllocator()
  requires(S > 0 && (S & (S - 1)) == 0 && B == BufferType::STACK)
    : buffer(std::array<std::byte, S>{}),
      data(buffer.data()),
      capacity(S),
      used(0) {
  Block* block{reinterpret_cast<Block*>(data)};
  block->next = nullptr;
  block->level = max_level;
  free_blocks[max_level] = block;
}

template <size_t S, BufferType B>
BuddyAllocator<S, B>::BuddyAllocator(std::array<std::byte, S>& buf)
  requires(S > 0 && (S & (S - 1)) == 0 && B == BufferType::EXTERNAL)
    : buffer(buf.data()), data(buf.data()), capacity(buf.size()), used(0) {
  Block* block{reinterpret_cast<Block*>(data)};
  block->next = nullptr;
  block->level = max_level;
  free_blocks[max_level] = block;
}

template <size_t S, BufferType B>
BuddyAllocator<S, B>::~BuddyAllocator() noexcept {
  if constexpr (B == BufferType::HEAP) {
    ::operator delete(buffer);
  }
}

template <size_t S, BufferType B>
std::byte* BuddyAllocator<S, B>::allocate(size_t size) noexcept {
  size_t effective_size{std::bit_ceil(std::max(size, sizeof(Block)))};
  size_t level{std::bit_width(effective_size / sizeof(Block)) - 1};

  if (level > max_level) {
    return nullptr;
  }

  size_t current{level};
  while (free_blocks[current] == nullptr) {
    ++current;
    if (current > max_level) {
      return nullptr;
    }
  }

  Block* block{free_blocks[current]};
  free_blocks[current] = block->next;

  while (current > level) {
    --current;

    Block* buddy{get_buddy(block, current)};
    buddy->next = free_blocks[current];
    buddy->level = current;
    free_blocks[current] = buddy;

    block->level = current;
  }

  size_t index{(reinterpret_cast<std::byte*>(block) - data) / sizeof(Block)};
  bitmap.set(index);

  used += (size_t{1} << level) * sizeof(Block);

  return reinterpret_cast<std::byte*>(block);
}

template <size_t S, BufferType B>
void BuddyAllocator<S, B>::deallocate(std::byte* ptr) noexcept {
  if (ptr == nullptr) {
    return;
  }

  assert(ptr >= data && ptr < data + capacity && "pointer is out of bounds");

  Block* block{reinterpret_cast<Block*>(ptr)};
  size_t index{(reinterpret_cast<std::byte*>(block) - data) / sizeof(Block)};
  bitmap.reset(index);

  size_t level{block->level};
  used -= (size_t{1} << level) * sizeof(Block);

  while (true) {
    if (level == max_level) {
      break;
    }

    Block* buddy{get_buddy(block, level)};
    size_t buddy_index{(reinterpret_cast<std::byte*>(buddy) - data) /
                       sizeof(Block)};

    if (bitmap.test(buddy_index) || buddy->level != level ||
        level == max_level) {
      break;
    }

    Block* previous{nullptr};
    Block* current{free_blocks[level]};
    while (current != nullptr && current != buddy) {
      previous = current;
      current = current->next;
    }

    if (previous == nullptr) {
      free_blocks[level] = buddy->next;
    } else {
      previous->next = buddy->next;
    }

    if (block > buddy) {
      block = buddy;
    }

    ++level;
  }

  block->level = level;
  block->next = free_blocks[level];
  free_blocks[level] = block;
}

template <size_t S, BufferType B>
void BuddyAllocator<S, B>::reset() noexcept {
  bitmap.reset();
  free_blocks = {};

  Block* block{reinterpret_cast<Block*>(data)};
  block->next = nullptr;
  block->level = max_level;
  free_blocks[max_level] = block;
}

template <size_t S, BufferType B>
size_t BuddyAllocator<S, B>::get_used() noexcept {
  return used;
}

template <size_t S, BufferType B>
size_t BuddyAllocator<S, B>::get_free() noexcept {
  return capacity - used;
}

//////////////////////
// type-safe helpers
//////////////////////

template <size_t S, BufferType B>
template <typename T>
T* BuddyAllocator<S, B>::allocate(size_t count) noexcept {
  if (count > SIZE_MAX / sizeof(T)) {
    return nullptr;
  }

  return reinterpret_cast<T*>(allocate(sizeof(T) * count));
}

template <size_t S, BufferType B>
template <typename T>
void BuddyAllocator<S, B>::deallocate(T* ptr) noexcept {
  deallocate(reinterpret_cast<std::byte*>(ptr));
}

template <size_t S, BufferType B>
template <typename T, typename... Args>
T* BuddyAllocator<S, B>::emplace(Args&&... args) {
  std::byte* ptr{allocate(sizeof(T))};
  if (!ptr) {
    return nullptr;
  }

  return std::construct_at(reinterpret_cast<T*>(ptr),
                           std::forward<Args>(args)...);
}

template <size_t S, BufferType B>
template <typename T>
void BuddyAllocator<S, B>::destroy(T* ptr) noexcept {
  // asymmetric, does not deallocate (only reset does)
  if (ptr) {
    std::destroy_at(ptr);
  }
}

//////////////////////
// helpers
//////////////////////

template <size_t S, BufferType B>
Block* BuddyAllocator<S, B>::get_buddy(Block* block,
                                       size_t level) const noexcept {
  size_t offset{(reinterpret_cast<std::byte*>(block) - data) ^
                (size_t{1} << level) * sizeof(Block)};
  return reinterpret_cast<Block*>(data + offset);
}

}  // namespace allocator