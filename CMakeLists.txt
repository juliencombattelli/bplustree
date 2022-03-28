cmake_minimum_required(VERSION 3.12)
project(BPlusTree)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(bplustree INTERFACE)
target_include_directories(bplustree INTERFACE include)

add_executable(bplustree-test test/main.cpp)
target_link_libraries(bplustree-test PUBLIC bplustree)
