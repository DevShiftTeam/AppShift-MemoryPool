cmake_minimum_required(VERSION 3.15)
project(MemoryPool)

set(CMAKE_CXX_STANDARD 17)

# Release mode
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -O2")

# Debug mode
# set(CMAKE_BUILD_TYPE Debug)

add_executable(MemoryPool_OND "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/../MemoryPool.cpp")