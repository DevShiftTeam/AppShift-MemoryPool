cmake_minimum_required(VERSION 3.15)
project(MemoryPool)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE Release)
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -O2")

add_executable(MemoryPool "main.cpp" "CPPShift.h" "MemoryPool.cpp" "MemoryPool.h" "String.cpp" "String.h")