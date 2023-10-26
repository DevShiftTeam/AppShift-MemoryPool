//
// Created by Sapir Shemer on 25/10/2023.
//
#include "../../include/execution/EventLoop.h"
#include "../include/thread_pool/thread_pool.h"

using namespace AppShift::Execution;

int fib_event_loop(int n, EventLoop& event_loop) {
    if(n <= 1) return n;
    auto a = event_loop.addEvent<int(int, EventLoop&)>(fib_event_loop, n - 1, std::ref(event_loop));
    auto b = event_loop.addEvent<int(int, EventLoop&)>(fib_event_loop, n - 2, std::ref(event_loop));
    return a.get() + b.get();
}

int fib_thread_loop(int n, dp::thread_pool<> &tp) {
    if(n <= 1) return n;
    auto a = tp.enqueue<int(int, dp::thread_pool<>&)>(fib_thread_loop, n - 1, std::ref(tp));
    auto b = tp.enqueue<int(int, dp::thread_pool<>&)>(fib_thread_loop, n - 2, std::ref(tp));
    return a.get() + b.get();
}

int main() {
    dp::thread_pool tp = dp::thread_pool();
    EventLoop event_loop = EventLoop();

    auto start = std::chrono::high_resolution_clock::now();
    auto result = fib_event_loop(20, event_loop);
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "Event loop result: " << result << std::endl;
    std::cout << "Event loop time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;

    start = std::chrono::high_resolution_clock::now();
    result = fib_thread_loop(20, tp);
    end = std::chrono::high_resolution_clock::now();

    std::cout << "Thread pool result: " << result << std::endl;
    std::cout << "Thread pool time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
}