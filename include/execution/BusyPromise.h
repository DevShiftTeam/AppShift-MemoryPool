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
        std::shared_ptr<SharedBusyState<T>> _state;
        ExecutionQueue* _queue{};
        size_t _max_events_per_wait = 1;

    public:
        explicit BusyPromise(ExecutionQueue *queue, const size_t _max_events_per_wait = 1)
                : _queue(queue), _state(std::make_shared<SharedBusyState<T>>())
                , _max_events_per_wait(_max_events_per_wait) {}

        BusyPromise(const BusyPromise<T> &other) {
            this->_state = other._state;
            this->_queue = other._queue;
            this->_max_events_per_wait = other._max_events_per_wait;
        }

        // Move constructor
        BusyPromise(BusyPromise<T> &&other) noexcept {
            this->_state = std::move(other._state);
            this->_queue = other._queue;
            this->_max_events_per_wait = other._max_events_per_wait;
        }

        BusyPromise<T> &operator=(const BusyPromise<T> &other) {
            if(this == &other) return *this;

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
            this->_state->value = value;
            this->_state->is_ready = true;
        }

        /**
         * Set the value of the promise
         * @param value
         */
        void set_value(T &&value) const {
            this->_state->value = std::move(value);
            this->_state->is_ready.store(true);
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
        explicit BusyPromise(ExecutionQueue* queue, const size_t _max_events_per_wait = 1)
                : _queue(queue), _state(std::make_shared<SharedBusyState<void>>())
                , _max_events_per_wait(_max_events_per_wait) {}

        BusyPromise(const BusyPromise<void> &other) {
            this->_state = other._state;
            this->_queue = other._queue;
            this->_max_events_per_wait = other._max_events_per_wait;
        }

        BusyPromise<void> &operator=(const BusyPromise<void> &other) {
            if(this == &other) return *this;

            this->_state = other._state;
            this->_queue = other._queue;
            this->_max_events_per_wait = other._max_events_per_wait;
            return *this;
        }

        BusyPromise<void> &operator=(BusyPromise<void> &&other) noexcept {
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
