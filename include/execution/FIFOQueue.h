//
// Created by Sapir Shemer on 30/07/2023.
//

#ifndef APPSHIFTPOOL_FIFOQUEUE_H
#define APPSHIFTPOOL_FIFOQUEUE_H

#include <cstddef>
#include <utility>
#include <condition_variable>
#include <mutex>

namespace AppShift::Execution {
    struct FIFOQueueBlock {
        FIFOQueueBlock* next = this;
        size_t size = 0;
        std::atomic<size_t> ref_count = 0;
    };

    template<typename T>
    struct FIFOQueueResult {
        FIFOQueueResult() {
            event_block = nullptr;
            start = 0;
            count = 0;
        }

        FIFOQueueBlock* event_block = nullptr;
        size_t start = 0;
        size_t count = 0;

        ~FIFOQueueResult() {
            if(event_block != nullptr)
                event_block->ref_count--;
        }

        // Move constructor
        FIFOQueueResult(FIFOQueueResult&& other) noexcept {
            event_block = other.event_block;
            start = other.start;
            count = other.count;
            other.event_block = nullptr;
        }
    };

    template<typename T>
    class FIFOQueue {
    public:
        explicit FIFOQueue(size_t size = 1 << 20): _size(size) {
            first_block = static_cast<FIFOQueueBlock *>(malloc(sizeof(FIFOQueueBlock) + _size * sizeof(T)));
            current_block = first_block;
            first_block->next = first_block;
            first_block->size = _size;
            first_block->ref_count = 0;
        }

        /**
         * Push item into the rear of the queue
         * @param item
         */
        void push(const T &item) {
            std::unique_lock<std::mutex> lock(mutex);
            T* queue;
            // If empty then reset front and rear
            // Notice we ignore an empty block if it is in use
            if(isEmpty() && current_block->ref_count == 0) {
                rear = 0;
                front = 0;
            }

            // Handle end of current queue block
            if(rear == current_block->size) {
                auto* next_block = current_block->next;

                // If next block is the first block or in use, then allocate new block in between
                if(next_block == first_block || next_block->ref_count != 0) {
                    auto *new_block = reinterpret_cast<FIFOQueueBlock*>(
                            malloc(sizeof(FIFOQueueBlock) + _size * sizeof(T)));
                    new_block->next = next_block;
                    new_block->size = _size;
                    new_block->ref_count = 0;
                    current_block->next = new_block;
                    current_block = new_block;
                }
                    // Otherwise, just use the next block
                else {
                    current_block = next_block;
                }
            }

            // Add to queue
            queue = reinterpret_cast<T*>(current_block + 1);
            queue[rear++] = item;
        }

        /**
         * Pop item from the front of the queue
         * @return
         */
        FIFOQueueResult<T> pop(size_t count = 1) {
            std::unique_lock<std::mutex> lock(mutex);
            FIFOQueueResult<T> result;

            // If empty then return empty result
            if(isEmpty()) {
                result.event_block = nullptr;
                result.count = 0;
                return result;
            }

            result.event_block = first_block;
            result.start = front;

            // Calculate count available to determine amount of events to return
            auto size_available =
                    first_block == current_block ?
                    rear - front :
                    first_block->size - front;
            result.count = std::min(count, size_available);

            front += result.count;
            result.event_block->ref_count++;

            // If front is at the end of the block then move to next block
            if(front == first_block->size && first_block->next != first_block) {
                front = 0;
                first_block = first_block->next;
            }

            return result;
        }

        /**
         * Check if queue is empty
         * @return
         */
        bool isEmpty() {
            return front == rear && first_block == current_block;
        }

    private:
        size_t _size = 1 << 20;
        FIFOQueueBlock *first_block = nullptr;
        FIFOQueueBlock *current_block = nullptr;
        size_t rear = 0;
        size_t front = 0;
        // Mutex
        std::mutex mutex;
    };
}
#endif //APPSHIFTPOOL_FIFOQUEUE_H
