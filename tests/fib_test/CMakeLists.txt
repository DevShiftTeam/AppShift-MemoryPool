cmake_minimum_required(VERSION 3.23)
project(AppShiftMatrixTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Add c++20 support for gcc
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-ld_classic")

include_directories(../../include)
include_directories(../include)

add_executable(AppShiftMatrixTest

        # Stack Pool
        ../../implementation/execution/ThreadPool.cpp
        ../../implementation/execution/ExecutionQueue.cpp

        main.cpp
        ../../include/execution/BusyFuture.h)
