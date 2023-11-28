//
// Created by Sapir Shemer on 23/10/2023.
//

#include "../include/execution/ThreadPool.h"

AppShift::Execution::ThreadPool::ThreadPool(size_t thread_count, size_t max_events_per_thread) {
    this->max_events_per_thread = max_events_per_thread;
    startEventLoop(thread_count);
}

AppShift::Execution::ThreadPool::~ThreadPool() {
    stopEventLoop();
}

void AppShift::Execution::ThreadPool::addEvent(const std::function<void()> &event) {
    execution_queue.push(event);
}

void AppShift::Execution::ThreadPool::startEventLoop(size_t thread_count) {
    for (size_t i = 0; i < thread_count; i++)
        this->event_loop_threads.push_back(eventLoop());
}

void AppShift::Execution::ThreadPool::stopEventLoop() {
    this->stop_event_loop = true;
    this->execution_queue.setContinueEvenIfEmpty(true);

    wait([&]() { return this->execution_queue.isEmpty(); });

    for (auto &thread: this->event_loop_threads)
        if (thread.joinable()) thread.join();
}

std::thread AppShift::Execution::ThreadPool::eventLoop() {
    return std::thread([&]() {
        while (!this->stop_event_loop || !this->execution_queue.isEmpty()) {
            // Execute events
            const auto events = this->execution_queue.pop(this->max_events_per_thread);
            const auto *event_list =
                    &reinterpret_cast<std::function<void()> *>(events.event_block + 1)[events.start];
            for (size_t i = 0; i < events.count; i++)
                event_list[i]();
        }
    });
}

void AppShift::Execution::ThreadPool::wait(const std::function<bool()> &condition) {
    while (!condition()) {
        // Help clear the queue until none is left
        const auto events = this->execution_queue.pop();
        const auto *event_list =
                &reinterpret_cast<std::function<void()> *>(events.event_block + 1)[events.start];
        for (size_t i = 0; i < events.count; i++)
            event_list[i]();
    }
}
