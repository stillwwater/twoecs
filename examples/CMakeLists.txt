cmake_minimum_required(VERSION 3.1)

project(twoecs)
set(CMAKE_CXX_STANDARD 11)

if (MSVC)
    set(CMAKE_CXX_WARNING_LEVEL 4)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /EHsc /GR-")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fno-exceptions -fno-rtti")
endif()

find_path(SDL2_INCLUDE_DIR SDL.h
	HINTS
	$ENV{SDL2DIR}
	PATH_SUFFIXES include/SDL2 include
	PATHS ${SDL2_SEARCH_PATHS}
)
include_directories(${SDL2_INCLUDE_DIR})

add_executable(example_sdl example_sdl.cpp)
target_link_libraries(example_sdl SDL2 SDL2main)

add_executable(example_minimal example.cpp)
