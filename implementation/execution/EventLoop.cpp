//
// Created by Sapir Shemer on 23/10/2023.
//

#include "../include/execution/EventLoop.h"

AppShift::Execution::EventLoop::EventLoop(size_t thread_count, size_t max_events_per_thread) {
    this->max_events_per_thread = max_events_per_thread;
    startEventLoop(thread_count);
}

AppShift::Execution::EventLoop::~EventLoop() {
    stopEventLoop();
}

void AppShift::Execution::EventLoop::addEvent(const std::function<void()> &event) {
    execution_queue.push(event);
}

void AppShift::Execution::EventLoop::startEventLoop(size_t thread_count) {
    for (size_t i = 0; i < thread_count; i++)
        this->event_loop_threads.push_back(eventLoop());
}

void AppShift::Execution::EventLoop::stopEventLoop() {
    this->stop_event_loop = true;

    wait([this]() { return this->execution_queue.isEmpty(); });

    for (auto &thread: this->event_loop_threads)
        if (thread.joinable()) thread.join();
}

std::thread AppShift::Execution::EventLoop::eventLoop() {
    return std::thread([&]() {
        while (!this->stop_event_loop || !this->execution_queue.isEmpty()) {
            // Execute events
            auto events = this->execution_queue.pop(this->max_events_per_thread);
            auto *event_list =
                    &reinterpret_cast<std::function<void()> *>(events.event_block + 1)[events.start];
            for (size_t i = 0; i < events.count; i++)
                event_list[i]();
        }
    });
}

void AppShift::Execution::EventLoop::wait(const std::function<bool()> &condition) {
    while (!condition()) {
        // Help clear the queue until none is left
        auto events = this->execution_queue.pop(this->max_events_per_thread);
        auto *event_list =
                &reinterpret_cast<std::function<void()> *>(events.event_block + 1)[events.start];
        for (size_t i = 0; i < events.count; i++)
            event_list[i]();
    }
}
