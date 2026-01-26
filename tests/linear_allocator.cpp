#include "linear_allocator.h"

#include <gtest/gtest.h>

namespace allocator {

template <typename Allocator>
class LinearAllocatorTypedTest : public ::testing::Test {
 protected:
  void SetUp() override {
    if constexpr (std::is_same_v<Allocator,
                                 LinearAllocator<0, BufferType::EXTERNAL>>) {
      buf = std::make_unique<std::byte[]>(buf_size);
      buf_span = std::span(buf.get(), buf_size);
      alloc = std::make_unique<Allocator>(buf_span);
    } else {
      alloc = std::make_unique<Allocator>();
    }
  }

  std::unique_ptr<Allocator> alloc;

  // for buffertype::external allocator
  size_t buf_size{1024};
  std::unique_ptr<std::byte[]> buf{};
  std::span<std::byte> buf_span{};
};

using AllocatorTypes =
    ::testing::Types<LinearAllocator<1024>,                      // heap
                     LinearAllocator<1024, BufferType::STACK>,   // stack
                     LinearAllocator<0, BufferType::EXTERNAL>>;  // external

TYPED_TEST_SUITE(LinearAllocatorTypedTest, AllocatorTypes);

struct Obj {
  int x;
  double y;
  Obj(int a, double b) : x(a), y(b) {}
};

TYPED_TEST(LinearAllocatorTypedTest, BasicAllocation) {
  auto* ptr1{this->alloc->allocate(100, 8)};
  ASSERT_NE(ptr1, nullptr);

  auto* ptr2{this->alloc->allocate(100, 8)};
  ASSERT_NE(ptr2, nullptr);

  EXPECT_NE(ptr1, ptr2);
}

TYPED_TEST(LinearAllocatorTypedTest, AlignsCorrectly) {
  auto* ptr1{this->alloc->allocate(13, 1)};
  auto* ptr2{this->alloc->allocate(50, 8)};
  auto* ptr3{this->alloc->allocate(100, 16)};

  ASSERT_NE(ptr1, nullptr);
  ASSERT_NE(ptr2, nullptr);
  ASSERT_NE(ptr3, nullptr);

  EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr1) % 1, 0);
  EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr2) % 8, 0);
  EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr3) % 16, 0);
}

TYPED_TEST(LinearAllocatorTypedTest, AlignmentsPadsToCreateGaps) {
  auto* ptr1{this->alloc->allocate(13, 1)};
  auto* ptr2{this->alloc->allocate(50, 8)};

  ASSERT_NE(ptr1, nullptr);
  ASSERT_NE(ptr2, nullptr);

  auto gap{reinterpret_cast<uintptr_t>(ptr2) -
           reinterpret_cast<uintptr_t>(ptr1)};
  EXPECT_EQ(gap, 16);
}

TYPED_TEST(LinearAllocatorTypedTest, ReturnsNullptrWhenOutOfMemory) {
  auto* ptr{this->alloc->allocate(2000, 8)};
  EXPECT_EQ(ptr, nullptr);
}

TYPED_TEST(LinearAllocatorTypedTest, ResetsSuccessfully) {
  auto* ptr1{this->alloc->allocate(500, 8)};
  ASSERT_NE(ptr1, nullptr);

  this->alloc->reset();

  auto* ptr2{this->alloc->allocate(500, 8)};
  ASSERT_NE(ptr2, nullptr);

  EXPECT_EQ(ptr1, ptr2);  // should occupy same memory
}

TYPED_TEST(LinearAllocatorTypedTest, DeallocateIsNotSupported) {
  auto* ptr1{this->alloc->allocate(100, 8)};
  ASSERT_NE(ptr1, nullptr);

  this->alloc->deallocate(ptr1);

  auto* ptr2{this->alloc->allocate(100, 8)};
  ASSERT_NE(ptr2, nullptr);

  EXPECT_GT(reinterpret_cast<uintptr_t>(ptr2),
            reinterpret_cast<uintptr_t>(ptr1));
}

TYPED_TEST(LinearAllocatorTypedTest, ResizeLastInPlaceGrows) {
  auto* ptr1{this->alloc->allocate(100, 8)};
  auto* ptr2{this->alloc->allocate(50, 8)};

  ASSERT_NE(ptr1, nullptr);
  ASSERT_NE(ptr2, nullptr);

  auto* resized{
      this->alloc->resize_last(ptr2, 50, 100, 8)};  // from 50 to 100 bytes
  ASSERT_NE(resized, nullptr);
  EXPECT_EQ(resized, ptr2);
}

TYPED_TEST(LinearAllocatorTypedTest, ResizeLastInPlaceShrinks) {
  auto* ptr{this->alloc->allocate(100, 8)};
  ASSERT_NE(ptr, nullptr);

  auto* resized{
      this->alloc->resize_last(ptr, 100, 50, 8)};  // from 100 to 50 bytes

  ASSERT_NE(resized, nullptr);
  EXPECT_EQ(resized, ptr);
}

TYPED_TEST(LinearAllocatorTypedTest, ResizeLastReturnsNullptrIfTooLarge) {
  auto* ptr{this->alloc->allocate(100, 8)};
  ASSERT_NE(ptr, nullptr);

  auto* resized{this->alloc->resize_last(ptr, 100, 2000, 8)};
  EXPECT_EQ(resized, nullptr);
}

TYPED_TEST(LinearAllocatorTypedTest, ResizeLastReturnsNullptrOnOutOfBounds) {
  auto* valid{this->alloc->allocate(100, 8)};
  std::byte* invalid{valid + 10000};
  EXPECT_EQ(this->alloc->resize_last(invalid, 100, 200, 8), nullptr);
}

TYPED_TEST(LinearAllocatorTypedTest, InvalidAlignmentReturnsNullptr) {
  EXPECT_EQ(this->alloc->allocate(100, 0), nullptr);
  EXPECT_EQ(this->alloc->allocate(100, 3), nullptr);
  EXPECT_EQ(this->alloc->allocate(100, 6), nullptr);
}

TYPED_TEST(LinearAllocatorTypedTest, TypedAllocateSucceeds) {
  int n{10};
  int* ptr{this->alloc->template allocate<int>(n)};
  ASSERT_NE(ptr, nullptr);

  // verify alignment
  EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % alignof(int), 0);

  // verify allocation
  for (int i{}; i < n; ++i) {
    ptr[i] = i;
    EXPECT_EQ(ptr[i], i);
  }
}

TYPED_TEST(LinearAllocatorTypedTest, EmplaceAllocatesAndCreatesInPlace) {
  int a{15};
  double b{3.14};
  Obj* obj{this->alloc->template emplace<Obj>(a, b)};
  ASSERT_NE(obj, nullptr);

  // check construction
  EXPECT_EQ(obj->x, a);
  EXPECT_EQ(obj->y, b);
}

TYPED_TEST(LinearAllocatorTypedTest, DestroysObjectWIthoutDeallocating) {
  Obj* obj1{this->alloc->template emplace<Obj>(1, 2.0)};
  ASSERT_NE(obj1, nullptr);
  this->alloc->destroy(obj1);

  Obj* obj2{this->alloc->template emplace<Obj>(1, 2.0)};
  ASSERT_NE(obj2, nullptr);
  this->alloc->destroy(obj2);

  EXPECT_EQ(obj1, obj2);  // should occupy same memory
}
}  // namespace allocator