//
// Created by Sapir Shemer on 21/11/2023.
//

#ifndef APPSHIFTPOOL_THREADPOOL_H
#define APPSHIFTPOOL_THREADPOOL_H

#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <iostream>
#include <future>
#include "ExecutionQueue.h"
#include "BusyPromise.h"

namespace AppShift::Execution {
    /**
     * The thread pool allows to add events to a queue and execute them in separate thread/s.
     * It acts as a thread pool with a queue of events.
     * The thread pool is thread safe, though it may seem that it has no locks - but under the hood the FIFOQueue is
     * thread safe and lock-based.
     * Having a lock-based queue has no performance difference to a lock-based thread pool, the reason I chose to hide
     * the locking mechanism in the queue is to make cleaner code (For me, at least).
     *
     * @author Sapir Shemer
     */
    class ThreadPool {
    public:
        explicit ThreadPool(size_t thread_count = std::thread::hardware_concurrency(),
                            size_t max_events_per_thread = 256);

        ~ThreadPool();

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
        template<class Fp, class... Args,
                typename ReturnType = std::invoke_result_t<Fp &&, Args &&...>>
        auto addPromise(Fp event, Args... args) {
            BusyPromise<ReturnType> promise = BusyPromise<ReturnType>(&execution_queue);

            execution_queue.push([promise, event, args...]() {
                if constexpr (std::is_same_v<ReturnType, void>) {
                    event(args...);
                    promise.set_value();
                } else {
                    promise.set_value(event(args...));
                }
            });

            return promise.get_future();
        }

        /// Wait for all events to finish
        void wait(const std::function<bool()> &condition = []() { return false; });

        void waitAll() { wait([&]() { return this->execution_queue.isEmpty(); }); }

    private:
        /// Stop signal
        std::atomic<bool> stop_event_loop = false;
        /// Execution queue
        ExecutionQueue execution_queue{};
        /// Threads that run the thread pool
        std::vector<std::thread> event_loop_threads{};
        /// Maximum number of events to execute in a single pop
        size_t max_events_per_thread = 256;

        /**
         * Launch the thread pool in all assigned threads.
         *
         * @param thread_count  Number of threads to launch the thread pool in
         */
        void startEventLoop(size_t thread_count = 1);

        /**
         * Stop the thread pool and wait for all threads to finish.
         *
         * @note This function is blocking
         */
        void stopEventLoop();

        /**
         * The thread pool catches events from the queue and executes them.
         *
         * @return Thread that runs the thread pool
         */
        std::thread eventLoop();
    };
}

#endif //APPSHIFTPOOL_THREADPOOL_H
