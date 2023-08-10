//
// Created by Sapir Shemer on 30/07/2023.
//

#ifndef APPSHIFTPOOL_FIFOQUEUE_H
#define APPSHIFTPOOL_FIFOQUEUE_H

#include <stddef.h>
#include <utility>
#include "../object/ObjectPool.h"

namespace AppShift::Execution {
    template<typename T>
    class FIFOQueue {
    public:
        explicit FIFOQueue(const T& default_item = T()) : default_item(default_item), array(new T[_size] {nullptr})
        {}
        // With initializer list
        FIFOQueue(const std::initializer_list<T>& list) {
            array = new T[_size] {nullptr};
            default_item = *list.begin();
        }


        /**
         * Push item into the rear of the queue
         * @param item
         */
        void push(const T& item) {
            std::lock_guard<std::mutex> lock(mutex);
            // If front and rear are the same then reset them to `0`
            if(front == rear) {
                front = 0;
                rear = 0;
            }
            // If rear is at the end, but front at start then resize the array
            else if(rear - front == _size) {
                _size *= 2;
                T* new_array = new T[_size] {nullptr};
                for(size_t i = front; i <= rear; i++) {
                    new_array[i - front] = array[i];
                }
                delete[] array;
                array = new_array;
            }
            // If rear is at end then move items to the beginning
            else if(rear == _size) {
                for(size_t i = front; i <= rear; i++) {
                    array[i - front] = array[i];
                }
                rear -= front;
                front = 0;
            }

            // Move item into the array
            array[rear++] = item;
        }

        /**
         * Pop item from the front of the queue
         * @return
         */
        const T& pop() {
            std::lock_guard<std::mutex> lock(mutex);
            if(isEmpty()) return default_item;
            return array[front++];
        }

        /**
         * Check if queue is empty
         * @return
         */
        bool isEmpty() {
            return front == rear;
        }
    private:
        size_t _size = 1 << 10;
        // End of the queue
        size_t rear = 0;
        // Start of the queue
        size_t front = 0;
        // Array of items
        T* array;
        // Default item
        T default_item;
        // Mutex
        std::mutex mutex;
    };
}

#endif //APPSHIFTPOOL_FIFOQUEUE_H
