cmake_minimum_required(VERSION 3.20)
project(memory-allocators VERSION 0.1 LANGUAGES CXX)

enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include(FetchContent)

# =====================================
# dependencies
# =====================================

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.7.1
)
FetchContent_MakeAvailable(benchmark)

# =====================================
# allocator library
# =====================================

add_library(allocators INTERFACE)
target_include_directories(allocators INTERFACE ${CMAKE_SOURCE_DIR}/include)

# =====================================
# app executable
# =====================================

add_executable(app ${CMAKE_SOURCE_DIR}/src/main.cpp)
target_link_libraries(app PRIVATE allocators)
set_target_properties(app PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# =====================================
# tests
# =====================================

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/tests/*.cpp
)

add_executable(tests ${TEST_SOURCES})
target_link_libraries(tests PRIVATE 
    allocators
    GTest::gtest_main
    GTest::gmock
)
set_target_properties(tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

include(GoogleTest)
gtest_discover_tests(tests)

# =====================================
# performance
# =====================================

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/perf/*.cpp
)

add_executable(perf ${TEST_SOURCES})
target_link_libraries(perf PRIVATE 
    allocators
    benchmark::benchmark
)
set_target_properties(perf PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})