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
#include "ExecutionQueue.h"

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
        explicit EventLoop(size_t thread_count = std::thread::hardware_concurrency(),
                           size_t max_events_per_thread = 256);

        ~EventLoop();

        /// Add event to loop
        void addEvent(const std::function<void()> &event);

        /**
         * Event with promise to allow awaiting
         *
         * @tparam Fp            Return type of the event
         * @tparam Args         Arguments of the event
         * @param event         Event to add
         * @param args          Arguments of the event
         * @return              Promise to await the event
         */
        template<class Fp, class ...Args>
        auto addEvent(const std::function<Fp> &event, Args... args);

    private:
        /// Stop signal
        std::atomic<bool> stop_event_loop = false;
        /// Execution queue
        ExecutionQueue execution_queue{};
        /// Threads that run the event loop
        std::vector<std::thread> event_loop_threads{};
        /// Maximum number of events to execute in a single pop
        size_t max_events_per_thread = 256;

        /**
         * Launch the event loop in all assigned threads.
         *
         * @param thread_count  Number of threads to launch the event loop in
         */
        void startEventLoop(size_t thread_count = 1);

        /**
         * Stop the event loop and wait for all threads to finish.
         *
         * @note This function is blocking
         */
        void stopEventLoop();

        /**
         * The event loop catches events from the queue and executes them.
         *
         * @return Thread that runs the event loop
         */
        std::thread eventLoop();
    };
}

template<class Fp, class ...Args>
auto AppShift::Execution::EventLoop::addEvent(const std::function<Fp>& event, Args... args) {
    using return_type = std::function<Fp>::result_type;
    std::shared_ptr<std::promise<return_type>> promise = std::make_shared<std::promise<return_type>>();
    auto future = promise->get_future();

    execution_queue.push([promise, event, args...]() {
        if constexpr (std::is_same_v<return_type, void>) {
            event(args...);
            promise->set_value();
        } else {
            promise->set_value(event(args...));
        }
    });

    return future;
}

#endif //APPSHIFTPOOL_EVENTLOOP_H
