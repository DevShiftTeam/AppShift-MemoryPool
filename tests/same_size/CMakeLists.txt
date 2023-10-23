cmake_minimum_required(VERSION 3.23)
project(AppShiftPoolTest)

set(CMAKE_CXX_STANDARD 20)

include_directories(../../include)
include_directories(../include)

add_executable(AppShiftPoolTest

        # Stack Pool
        ../../implementation/stack/common.cpp
        ../../implementation/execution/EventLoop.cpp
        ../../implementation/execution/ExecutionQueue.cpp

        LinkedList.h
        main.cpp)
