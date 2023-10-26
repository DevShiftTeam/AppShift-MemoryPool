//
// Created by Sapir Shemer on 25/10/2023.
//

#ifndef APPSHIFTPOOL_BUSYPROMISE_H
#define APPSHIFTPOOL_BUSYPROMISE_H

#include "BusyFuture.h"

namespace AppShift::Execution {
    template<class T>
    class BusyPromise {
    private:
        using State_ptr = std::shared_ptr<SharedBusyState<T>>;

        State_ptr _state;
        ExecutionQueue* _queue{};
        size_t _max_events_per_wait = 256;

    public:
        explicit BusyPromise(ExecutionQueue *queue, size_t _max_events_per_wait = 256)
                : _queue(queue), _state(std::make_shared<SharedBusyState<T>>())
                , _max_events_per_wait(_max_events_per_wait) {}

        BusyPromise(const BusyPromise<T> &other) {
            std::lock_guard<std::mutex> lock(other._state->_mutex);
            this->_state = other._state;
            this->_queue = other._queue;
            this->_max_events_per_wait = other._max_events_per_wait;
        }

        BusyPromise<T> &operator=(const BusyPromise<T> &other) {
            std::lock_guard<std::mutex> lock(other._state->_mutex);
            this->_state = other._state;
            this->_queue = other._queue;
            this->_max_events_per_wait = other._max_events_per_wait;
            return *this;
        }

        /**
         * Set the value of the promise
         * @param value
         */
        void set_value(const T &value) const {
            std::lock_guard<std::mutex> lock(_state->_mutex);
            this->_state->value = value;
            this->_state->is_ready = true;
        }

        /**
         * Set the value of the promise
         * @param value
         */
        void set_value(T &&value) const {
            std::lock_guard<std::mutex> lock(_state->_mutex);
            this->_state->value = std::move(value);
            this->_state->is_ready = true;
        }

        /**
         * Get the future of the promise
         * @return
         */
        BusyFuture<T> get_future() const {
            return BusyFuture<T>(this->_state, this->_queue, this->_max_events_per_wait);
        }
    };

    template<>
    class BusyPromise<void> {
    private:
        using State_ptr = std::shared_ptr<SharedBusyState<void>>;

        State_ptr _state;
        ExecutionQueue* _queue;
        size_t _max_events_per_wait = 256;

    public:
        explicit BusyPromise(ExecutionQueue* queue, size_t _max_events_per_wait = 256)
                : _queue(queue), _state(std::make_shared<SharedBusyState<void>>())
                , _max_events_per_wait(_max_events_per_wait) {}

        BusyPromise(const BusyPromise<void> &other) {
            this->_state = other._state;
            this->_queue = other._queue;
            this->_max_events_per_wait = other._max_events_per_wait;
        }

        BusyPromise<void> &operator=(const BusyPromise<void> &other) {
            std::lock_guard<std::mutex> lock(other._state->_mutex);
            this->_state = other._state;
            this->_queue = other._queue;
            this->_max_events_per_wait = other._max_events_per_wait;
            return *this;
        }

        BusyPromise<void> &operator=(BusyPromise<void> &&other) noexcept {
            std::lock_guard<std::mutex> lock(other._state->_mutex);
            this->_state = std::move(other._state);
            this->_queue = other._queue;
            this->_max_events_per_wait = other._max_events_per_wait;
            return *this;
        }

        /**
         * Set the value of the promise
         */
        void set_value() const {
            this->_state->is_ready = true;
        }

        /**
         * Get the future of the promise
         * @return
         */
        BusyFuture<void> get_future() {
            return {this->_state, this->_queue, this->_max_events_per_wait};
        }
    };
}

#endif //APPSHIFTPOOL_BUSYPROMISE_H
