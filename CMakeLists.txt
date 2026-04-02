cmake_minimum_required(VERSION 3.10)

project(CShort VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(hello hello.cpp)

target_compile_options(hello PRIVATE -Wall -Wextra)
