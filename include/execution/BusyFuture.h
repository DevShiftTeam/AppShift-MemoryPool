//
// Created by Sapir Shemer on 24/10/2023.
//

#ifndef APPSHIFTPOOL_BUSYFUTURE_H
#define APPSHIFTPOOL_BUSYFUTURE_H

#include <utility>

#include "ExecutionQueue.h"

namespace AppShift::Execution {
    /**
     * \brief Shared state between future and promise.
     * STD implementation allocates the state and the value separately.
     * In contrast, this implementation allocates the state and the value together.
     *
     * \tparam T    The type of the value
     */
    template<class T>
    struct SharedBusyState {
        T value{};
        std::atomic<bool> is_ready = false;
    };

    template<class T>
    class BusyFuture {
    private:
        std::shared_ptr<SharedBusyState<T>> _state;
        ExecutionQueue* _queue;
        size_t _max_events_per_wait = 1;

    public:
        BusyFuture(std::shared_ptr<SharedBusyState<T>> state, ExecutionQueue* queue, const size_t _max_events_per_wait = 1)
            : _state(state), _queue(queue), _max_events_per_wait(_max_events_per_wait) {
        }

        BusyFuture<T>& operator=(const BusyFuture<T>&other) {
            if (this == &other) return *this;

            this->_state = other._state;
            this->_queue = other._queue;
            this->_max_events_per_wait = other._max_events_per_wait;
            return *this;
        }

        BusyFuture<T>& operator=(BusyFuture<T>&&other) noexcept {
            this->_state = std::move(other._state);
            this->_queue = std::move(other._queue);
            this->_max_events_per_wait = std::move(other._max_events_per_wait);
            return *this;
        }

        /**
         * Wait for the future to be ready
         */
        void wait() {
            while (!this->_state->is_ready.load()) {
                const auto events = this->_queue->pop(this->_max_events_per_wait, true);
                const std::function<void()>* event_list =
                        &reinterpret_cast<std::function<void()> *>(events.event_block + 1)[events.start];
                for (size_t i = 0; i < events.count; i++)
                    event_list[i]();
            }
        }

        /**
         * Get the value of the future
         * @return
         */
        T get() {
            wait();
            return this->_state->value;
        }

        /**
         * Check if the future is ready
         * @return
         */
        bool isReady() {
            return this->_state->is_ready.load();
        }
    };

    template<>
    struct SharedBusyState<void> {
        std::atomic<bool> is_ready = false;
    };

    template<>
    class BusyFuture<void> {
    private:
        using State_ptr = std::shared_ptr<SharedBusyState<void>>;

        State_ptr _state;
        ExecutionQueue* _queue;
        size_t _max_events_per_wait = 1;

    public:
        BusyFuture(State_ptr state, ExecutionQueue* queue, const size_t _max_events_per_wait = 1)
            : _state(std::move(state)), _queue(queue), _max_events_per_wait(_max_events_per_wait) {
        }

        BusyFuture<void>& operator=(const BusyFuture<void>&other) {
            this->_state = other._state;
            this->_queue = other._queue;
            this->_max_events_per_wait = other._max_events_per_wait;
            return *this;
        }

        BusyFuture<void>& operator=(BusyFuture<void>&&other) noexcept {
            this->_state = std::move(other._state);
            this->_queue = other._queue;
            this->_max_events_per_wait = other._max_events_per_wait;
            return *this;
        }

        /**
         * Wait for the future to be ready
         */
        void wait() const {
            while (!this->_state->is_ready) {
                const auto events = this->_queue->pop(this->_max_events_per_wait, true);
                const std::function<void()>* event_list =
                        &reinterpret_cast<std::function<void()> *>(events.event_block + 1)[events.start];
                for (size_t i = 0; i < events.count; i++)
                    event_list[i]();
            }
        }

        /**
         * Get the value of the future
         * @return
         */
        void get() {
            wait();
        }

        /**
         * Check if the future is ready
         * @return
         */
        [[nodiscard]] bool isReady() const {
            return this->_state->is_ready;
        }
    };
}

#endif //APPSHIFTPOOL_BUSYFUTURE_H
