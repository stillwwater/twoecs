
cmake_minimum_required(VERSION 3.1)

project(entity_benchmark)

# Benchmark uses some C++ 17 features but entity.h only requires C++ 11
set(CMAKE_CXX_STANDARD 17)

if (MSVC)
    set(CMAKE_CXX_WARNING_LEVEL 4)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /EHsc /GR-")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fno-exceptions -fno-rtti")
endif()

include_directories(external)
add_executable(entity_benchmark entity_benchmark.cpp)

set(BENCHMARK_ENABLE_TESTING OFF)
add_subdirectory(external/benchmark)
include_directories(external/benchmark/include)

target_link_libraries(entity_benchmark benchmark benchmark_main)
