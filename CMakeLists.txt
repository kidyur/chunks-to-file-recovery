cmake_minimum_required(VERSION 3.23)
project(FileCollector)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
include_directories(include)

add_subdirectory(test)
add_subdirectory(include)
add_subdirectory(benchmark)