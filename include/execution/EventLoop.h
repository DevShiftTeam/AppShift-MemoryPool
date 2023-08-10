//
// Created by Sapir Shemer on 26/07/2023.
//

#ifndef APPSHIFTPOOL_EVENTLOOP_H
#define APPSHIFTPOOL_EVENTLOOP_H

#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <iostream>
#include <future>
#include "FIFOQueue.h"

namespace AppShift::Execution {
    /**
     * The event loop allows to add events to a queue and execute them in separate thread/s.
     * It acts as a thread pool with a queue of events.
     * The event loop is thread safe, though it may seem that it has no locks - but under the hood the FIFOQueue is
     * thread safe and lock-based.
     * Having a lock-based queue has no performance difference to a lock-based event loop, the reason I chose to hide
     * the locking mechanism in the queue is to make cleaner code (For me, at least).
     *
     * @author Sapir Shemer
     */
    class EventLoop {
    public:
        explicit EventLoop(size_t thread_count = std::thread::hardware_concurrency()) {
            startEventLoop(thread_count);
        }
        ~EventLoop() { stopEventLoop(); }

        // Add event to loop
        void addEvent(const std::function<void()>& event) { execution_queue.push(event); }

    private:
        // Stop signal
        std::atomic<bool> stop_event_loop = false;
        // Execution queue
        FIFOQueue<std::function<void()>> execution_queue {[]() {}};
        // Thread
        std::vector<std::thread> event_loop_threads {};

        void stopEventLoop() {
            this->stop_event_loop = true;
            for(auto& thread : this->event_loop_threads)
                if(thread.joinable()) thread.join();
        }

        std::function<void()> getEvent() { return this->execution_queue.pop(); }

        std::thread eventLoop() {
            return std::thread([&]() {
                while (true) {
                    // Exit if no events and stop signal
                    if (this->execution_queue.isEmpty() && this->stop_event_loop) break;

                    // Execute event
                    getEvent()();
                }
            });
        }

        void startEventLoop(size_t thread_count = 1) {
            for(size_t i = 0; i < thread_count; i++)
                this->event_loop_threads.push_back(eventLoop());
        }
    };
}

#endif //APPSHIFTPOOL_EVENTLOOP_H
