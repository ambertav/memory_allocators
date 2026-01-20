#include "linear_allocator.h"

#include <gtest/gtest.h>

class HeapLinearAllocatorTest : public ::testing::Test {
 protected:
  HeapLinearAllocator allocator{1024};
};

TEST_F(HeapLinearAllocatorTest, BasicAllocation) {
  auto* ptr{allocator.allocate(100, 8)};
  ASSERT_NE(ptr, nullptr);

  auto* second_ptr{allocator.allocate(100, 8)};
  ASSERT_NE(second_ptr, nullptr);

  EXPECT_NE(ptr, second_ptr);
}

TEST_F(HeapLinearAllocatorTest, AlignsCorrectly) {
  auto* ptr1{allocator.allocate(13, 1)};
  auto* ptr2{allocator.allocate(50, 8)};
  auto* ptr3{allocator.allocate(100, 16)};

  ASSERT_NE(ptr1, nullptr);
  ASSERT_NE(ptr2, nullptr);
  ASSERT_NE(ptr3, nullptr);

  EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr1) % 1, 0);
  EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr2) % 8, 0);
  EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr3) % 16, 0);
}

TEST_F(HeapLinearAllocatorTest, AlignmentsPadsToCreateGaps) {
  auto* ptr1{allocator.allocate(13, 1)};
  auto* ptr2{allocator.allocate(50, 8)};

  ASSERT_NE(ptr1, nullptr);
  ASSERT_NE(ptr2, nullptr);

  auto gap{reinterpret_cast<uintptr_t>(ptr2) -
           reinterpret_cast<uintptr_t>(ptr1)};
  EXPECT_EQ(gap, 16);
}

TEST_F(HeapLinearAllocatorTest, ReturnsNullptrWhenOutOfMemory) {
  auto* ptr{allocator.allocate(2000, 8)};
  EXPECT_EQ(ptr, nullptr);
}

TEST_F(HeapLinearAllocatorTest, ResetsSuccessfully) {
  auto* ptr1{allocator.allocate(500, 8)};
  ASSERT_NE(ptr1, nullptr);

  allocator.reset();

  auto* ptr2{allocator.allocate(500, 8)};
  ASSERT_NE(ptr2, nullptr);

  EXPECT_EQ(ptr1, ptr2);
}

TEST_F(HeapLinearAllocatorTest, DeallocateIsNotSupported) {
  auto* ptr1{allocator.allocate(100, 8)};
  ASSERT_NE(ptr1, nullptr);

  allocator.deallocate(ptr1);

  auto* ptr2{allocator.allocate(100, 8)};
  ASSERT_NE(ptr2, nullptr);

  EXPECT_GT(reinterpret_cast<uintptr_t>(ptr2),
            reinterpret_cast<uintptr_t>(ptr1));
}

TEST_F(HeapLinearAllocatorTest, ResizeAllocationInPlaceGrows) {
  auto* ptr1{allocator.allocate(100, 8)};
  auto* ptr2{allocator.allocate(50, 8)};

  ASSERT_NE(ptr1, nullptr);
  ASSERT_NE(ptr2, nullptr);

  auto* resized{
      allocator.resize_allocation(ptr2, 50, 100, 8)};  // from 50 to 100 bytes
  ASSERT_NE(resized, nullptr);
  EXPECT_EQ(resized, ptr2);
}

TEST_F(HeapLinearAllocatorTest, ResizeAllocationInPlaceShrinks) {
  auto* ptr{allocator.allocate(100, 8)};
  ASSERT_NE(ptr, nullptr);

  auto* resized{
      allocator.resize_allocation(ptr, 100, 50, 8)};  // from 100 to 50 bytes

  ASSERT_NE(resized, nullptr);
  EXPECT_EQ(resized, ptr);
}

TEST_F(HeapLinearAllocatorTest, ResizeAllocationReturnsNullptrIfTooLarge) {
  auto* ptr{allocator.allocate(100, 8)};
  ASSERT_NE(ptr, nullptr);

  auto* resized{allocator.resize_allocation(ptr, 100, 2000, 8)};
  EXPECT_EQ(resized, nullptr);
}

TEST_F(HeapLinearAllocatorTest, ResizeAllocationOnNullptrAllocatesNew) {
  auto* ptr{allocator.resize_allocation(nullptr, 0, 100, 8)};
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % 8, 0);
}

TEST_F(HeapLinearAllocatorTest, ResizeAllocationThrowsOnOutOfBounds) {
  auto* valid{allocator.allocate(100, 8)};
  std::byte* invalid{valid + 10000};
  EXPECT_THROW(allocator.resize_allocation(invalid, 100, 200, 8),
               std::invalid_argument);
}

TEST_F(HeapLinearAllocatorTest, ResizeBufferGrows) {
  auto* ptr1{allocator.allocate(900, 8)};
  ASSERT_NE(ptr1, nullptr);

  auto* ptr2{allocator.allocate(200, 8)};
  EXPECT_EQ(ptr2, nullptr);

  EXPECT_NO_THROW(allocator.resize_buffer(2048));

  auto* ptr3{allocator.allocate(1900, 8)};
  EXPECT_NE(ptr3, nullptr);
}

TEST_F(HeapLinearAllocatorTest, ResizeBufferShrinks) {
  EXPECT_NO_THROW(allocator.resize_buffer(512));
  auto* ptr{allocator.allocate(512, 8)};
  EXPECT_NE(ptr, nullptr);

  auto* ptr2{allocator.allocate(1, 1)};
  EXPECT_EQ(ptr2, nullptr);
}

TEST_F(HeapLinearAllocatorTest, InvalidAlignmentThrows) {
  EXPECT_THROW(allocator.allocate(100, 0), std::invalid_argument);
  EXPECT_THROW(allocator.allocate(100, 3), std::invalid_argument);
  EXPECT_THROW(allocator.allocate(100, 6), std::invalid_argument);
}