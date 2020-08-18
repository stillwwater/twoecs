cmake_minimum_required(VERSION 3.1)

project(twoecs)
set(CMAKE_CXX_STANDARD 11)

if (MSVC)
    set(CMAKE_CXX_WARNING_LEVEL 4)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /EHsc /GR-")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fno-exceptions -fno-rtti")
endif()

include_directories(external)
add_executable(entity_benchmark entity_benchmark.cpp)
add_executable(entity_test entity_test.cpp)

set(BENCHMARK_ENABLE_TESTING OFF)
add_subdirectory(external/benchmark)
include_directories(external/benchmark/include)

set(BUILD_GMOCK OFF)
add_subdirectory(external/googletest)
include_directories(external/googletest/googletest/include)

target_link_libraries(entity_benchmark benchmark benchmark_main)
target_link_libraries(entity_test gtest gtest_main)
