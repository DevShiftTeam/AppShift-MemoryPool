//
// Created by Sapir Shemer on 31/07/2023.
//

#ifndef APPSHIFTPOOL_SEGREGATEDTSLFPOOL_H
#define APPSHIFTPOOL_SEGREGATEDTSLFPOOL_H

#include "common.h"
#include "../PoolArchitectures.h"

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

        if(last_block == nullptr) {
            first_block = new SSegregatedStaticData<ITEM_SIZE * ITEM_COUNT>();
            last_block = &first_block->header;
            last_deleted_item = nullptr;
        }
    }

    ~SegregatedPool() {
        reference_count--;

        // If no pools reference the blocks then release all data
        if(reference_count == 0) {
            // Remove all blocks
            auto *current_block = last_block;
            while (current_block != &first_block->header) {
                auto *previous_block = current_block->previous;
                std::free(current_block);
                current_block = previous_block;
            }
            last_block = nullptr;
            last_deleted_item = nullptr;
            std::free(first_block);
        }
    }

    void* allocate();
    void free(void* item);
    void dumpPoolData();

private:
    // SSegregatedBlockHeader
    inline static __thread SSegregatedStaticData<ITEM_SIZE * ITEM_COUNT>* first_block = nullptr;
    // Last block in the chain of available memory blocks
    inline static __thread SSegregatedBlockHeader* last_block = nullptr;
    inline static __thread SSegregatedDeletedItem* last_deleted_item = nullptr;
    inline static __thread size_t reference_count = 0;

    static const size_t block_size = ITEM_COUNT * ITEM_SIZE;

    static void addNewBlock();
};

template<size_t ITEM_SIZE, size_t ITEM_COUNT>
void SegregatedPool<ITEM_SIZE, ITEM_COUNT, EPoolArchitecture::THREAD_SAFE_LOCK_FREE>::addNewBlock() {
    // Create next block
    last_block->next = reinterpret_cast<SSegregatedBlockHeader*>(
            std::malloc(sizeof(SSegregatedBlockHeader) + block_size)
    );

    // Initialize data & connect to chain
    last_block->next->previous = last_block;
    last_block = last_block->next;
    last_block->offset = 0;
    last_block->next = nullptr;
}

template<size_t ITEM_SIZE, size_t ITEM_COUNT>
void *SegregatedPool<ITEM_SIZE, ITEM_COUNT, EPoolArchitecture::THREAD_SAFE_LOCK_FREE>::allocate() {
    if(last_block == nullptr) {
        first_block = new SSegregatedStaticData<ITEM_SIZE * ITEM_COUNT>();
        last_block = &first_block->header;
        last_deleted_item = nullptr;
    }

    // Allocate from last deleted block
    if(last_deleted_item != nullptr) {
        void* new_item = last_deleted_item;
        last_deleted_item = last_deleted_item->previous;

        return new_item;
    }

    // Allocate from offset
    if(last_block->offset < block_size) {
        void* new_item = reinterpret_cast<char*>(last_block)
                + sizeof(SSegregatedBlockHeader)
                + last_block->offset;
        last_block->offset += ITEM_SIZE;

        return new_item;
    }

    // Add new block & allocate from it
    addNewBlock();
    last_block->offset += ITEM_SIZE;
    void* new_item = reinterpret_cast<char*>(last_block) + sizeof(SSegregatedBlockHeader);

    return new_item;
}

template<size_t ITEM_SIZE, size_t ITEM_COUNT>
void SegregatedPool<ITEM_SIZE, ITEM_COUNT, EPoolArchitecture::THREAD_SAFE_LOCK_FREE>::free(void *item) {
    auto* new_deleted_item = reinterpret_cast<SSegregatedDeletedItem*>(item);
    new_deleted_item->previous = last_deleted_item;
    last_deleted_item = new_deleted_item;
}

template<size_t ITEM_SIZE, size_t ITEM_COUNT>
void SegregatedPool<ITEM_SIZE, ITEM_COUNT, EPoolArchitecture::THREAD_SAFE_LOCK_FREE>::dumpPoolData() {
    size_t block_count = 0;
    auto* current_block = reinterpret_cast<SSegregatedBlockHeader*>(&first_block);

    while (current_block != nullptr) {
        block_count++;
        std::cout << "Block Number: " << block_count << std::endl;
        current_block = current_block->next;
    }
}
}

#endif //APPSHIFTPOOL_SEGREGATEDTSLFPOOL_H
