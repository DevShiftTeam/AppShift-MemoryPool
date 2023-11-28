cmake_minimum_required(VERSION 3.23)
project(AppShiftPoolTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Add c++20 support for gcc
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-ld_classic")

include_directories(../../include)
include_directories(../include)

add_executable(AppShiftPoolTest

        # Stack Pool
        ../../implementation/stack/common.cpp
        ../../implementation/execution/ThreadPool.cpp
        ../../implementation/execution/ExecutionQueue.cpp

        LinkedList.h
        main.cpp
        ../../include/execution/BusyFuture.h)
