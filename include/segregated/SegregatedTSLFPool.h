//
// Created by Sapir Shemer on 31/07/2023.
//

#ifndef APPSHIFTPOOL_SEGREGATEDTSLFPOOL_H
#define APPSHIFTPOOL_SEGREGATEDTSLFPOOL_H

#include "common.h"
#include "../PoolArchitectures.h"
#include "SegregatedNTSPool.h"

namespace AppShift::Memory {
/**
 * The templated pool has thread local segregated memory blocks that it manages.
 * The first block is created at compile-time, and other blocks are manages at runtime.
 *
 * Advantages:
 * - "Endless" Memory: If a block is full, it generated a new one.
 * - Very fast, fast initialization since the first block is defined at compile-time
 * - Thread safe allocations with no performance overhead! :D
 * - No fragmentation!
 * - Different instances with the same size and count will use the same pool blocks.
 * Disadvantages:
 * - Only allocates a single size at a time.
 * - Doesn't remove unused blocks
 *
 * @tparam ITEM_SIZE    Size of a segment - allocations return pointers with this size available
 * @tparam ITEM_COUNT   Number of items in each block - a smaller size will be more memory efficient but a less
 * performant.
 *
 * @author Sapir Shemer
 */
template<size_t ITEM_SIZE, size_t ITEM_COUNT>
class SegregatedPool<ITEM_SIZE, ITEM_COUNT, EPoolArchitecture::THREAD_SAFE_LOCK_FREE> {
public:
    SegregatedPool() {
        reference_count++;
        this->initializePool();
    }

    ~SegregatedPool() {
        reference_count--;

        // If no pools reference the blocks then release all data
        if(reference_count == 0) {
            delete local_pool;
        }
    }

    void* allocate() {
        this->initializePool();
        return local_pool->allocate();
    }
    void free(void* item) {
        this->initializePool();
        local_pool->free(item);
    }

    void dumpPoolData() {
        this->initializePool();
        local_pool->dumpPoolData();
    }

private:
    inline static __thread SegregatedPool<ITEM_SIZE, ITEM_COUNT, EPoolArchitecture::NON_THREAD_SAFE>*
            local_pool = nullptr;
    inline static __thread size_t reference_count = 0;

    void initializePool() {
        if(local_pool == nullptr)
            local_pool = new SegregatedPool<ITEM_SIZE, ITEM_COUNT, EPoolArchitecture::NON_THREAD_SAFE>();
    }
};

}

#endif //APPSHIFTPOOL_SEGREGATEDTSLFPOOL_H
