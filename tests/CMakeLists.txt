cmake_minimum_required(VERSION 3.15)
project(MemoryPool)

set(CMAKE_CXX_STANDARD 17)

# Build all tests
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/simple_garbage_collection)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/advanced_garbage_collection)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/overriding_new_delete)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/performance_mode)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thread_safety)