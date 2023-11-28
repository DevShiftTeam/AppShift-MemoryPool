//
// Created by Sapir Shemer on 25/10/2023.
//
#include "../../include/execution/ThreadPool.h"


using namespace AppShift::Execution;

int fib_event_loop(const int n, ThreadPool& event_loop) {
    if(n <= 1) return n;
    auto a = fib_event_loop(n - 1, event_loop);
    auto b = event_loop.addPromise(fib_event_loop, n - 2, std::ref(event_loop));
    return a + b.get();
}

int fib(const int n) {
    if(n <= 1) return n;
    auto a = fib(n - 1);
    auto b = fib(n - 2);
    return a + b;
}

int main() {
    {
        ThreadPool event_loop = ThreadPool();

        const auto start = std::chrono::high_resolution_clock::now();
        const auto result = fib_event_loop(30, event_loop);
        const auto end = std::chrono::high_resolution_clock::now();

        std::cout << "Event loop result: " << result << std::endl;
        std::cout << "Event loop time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
    }

    {
        const auto start = std::chrono::high_resolution_clock::now();
        const auto result = fib(30);
        const auto end = std::chrono::high_resolution_clock::now();

        std::cout << "Async result: " << result << std::endl;
        std::cout << "Async time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
    }
}