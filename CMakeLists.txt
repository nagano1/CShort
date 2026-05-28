cmake_minimum_required(VERSION 3.12)
project(main)

enable_language(CXX)
set(CMAKE_CXX_STANDARD 11) # 11, 14, C++14...
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) #...without compiler extensions like gnu++11

include_directories(src/include)
include_directories(src)

add_subdirectory(src/tests)