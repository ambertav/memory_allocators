#pragma once

#include <cstddef>
#include <string_view>

class IAllocator {
 public:
  virtual ~IAllocator() = default;

  virtual std::byte* allocate(size_t size, size_t alignment) = 0;
  virtual void deallocate(std::byte* ptr) = 0;
  virtual void reset() = 0;

  virtual std::string_view get_name() const = 0;
  virtual std::string_view get_type() const = 0;

 protected:
 static bool is_valid_alignment(size_t alignment) {
    return alignment > 0 && (alignment & (alignment - 1)) == 0;
 }

  static size_t align_forward(size_t offset, size_t alignment) {
    size_t remainder{offset % alignment};
    if (remainder == 0) {
      return offset;
    } else {
      return offset + (alignment - remainder);
    }
  }
};