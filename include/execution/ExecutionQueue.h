//
// Created by Sapir Shemer on 30/07/2023.
//

#ifndef APPSHIFTPOOL_EXECUTIONQUEUE_H
#define APPSHIFTPOOL_EXECUTIONQUEUE_H

#include <cstddef>
#include <utility>
#include <mutex>
#include <functional>
#include <atomic>
#include <iostream>

namespace AppShift::Execution {
    using Callable = std::function<void()>;

    /**
     * The queue block is the header of each block in the queue.
     * It contains the reference count of the block, and the next block in the chain.
     * The blocks are connected in a loop, to allow starting the queue from any point in the chain.
     */
    struct ExecutionQueueBlock {
        ExecutionQueueBlock *next = this;
        size_t size = 0;
        std::atomic<size_t> ref_count = 0;
    };

    /**
     * The result of the queue pop operation.
     * It contains the block of the event, the start index of the event, and the count of the events returned.
     * This wrapper allows its owner to decrease the reference count of the block when it is no longer needed.
     */
    struct ExecutionQueueResult {
        ExecutionQueueBlock *event_block = nullptr;
        size_t start = 0;
        size_t count = 0;

        ExecutionQueueResult();
        ~ExecutionQueueResult();
    };

    class ExecutionQueue {
    public:
        explicit ExecutionQueue(size_t size = 1 << 20);

        /**
         * Push item into the rear of the queue
         * @param item
         */
        void push(const Callable &item);

        /**
         * Pop item from the front of the queue
         * @return
         */
        ExecutionQueueResult pop(size_t count = 1, bool continue_if_empty = false);

        /**
         * Check if queue is empty
         * @return
         */
        [[nodiscard]] bool isEmpty() const {
            return front == rear && first_block == current_block;
        }

        void setContinueEvenIfEmpty(const bool value) {
            continue_even_if_empty.store(value);
            condition_variable.notify_all();
        }

    private:
        size_t _size = 1 << 20;
        ExecutionQueueBlock *first_block = nullptr;
        ExecutionQueueBlock *current_block = nullptr;
        size_t rear = 0;
        size_t front = 0;
        // Mutex
        std::mutex mutex;
        // Condition variable
        std::condition_variable condition_variable;
        // Atomic flag reference to indicate if the queue is stoping
        std::atomic<bool> continue_even_if_empty = false;
    };
}
#endif //APPSHIFTPOOL_EXECUTIONQUEUE_H
