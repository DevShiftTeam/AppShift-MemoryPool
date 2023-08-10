//
// Created by Sapir Shemer on 24/07/2023.
//

#ifndef APPSHIFTPOOL_SEGREGATED_STATICNTSPOOL_H
#define APPSHIFTPOOL_SEGREGATED_STATICNTSPOOL_H

#include "common.h"
#include "../PoolArchitectures.h"
#include <iostream>

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
    class SegregatedPool<ITEM_SIZE, ITEM_COUNT, EPoolArchitecture::NON_THREAD_SAFE> {
    public:
        ~SegregatedPool() {
            // Remove all blocks
            auto *current_block = last_block;
            while (current_block != &first_block.header) {
                auto *previous_block = current_block->previous;
                std::free(current_block);
                current_block = previous_block;
            }
        }

        void* allocate();
        void free(void* item);
        void dumpPoolData();

    private:
        // First block for managing segregated memory
        // SSegregatedBlockHeader
        SSegregatedStaticData<ITEM_SIZE * ITEM_COUNT> first_block = {};
        // Last block in the chain of available memory blocks
        SSegregatedBlockHeader* last_block = &first_block.header;

        SSegregatedDeletedItem* last_deleted_item = nullptr;
        const size_t block_size = ITEM_COUNT * ITEM_SIZE;

        void addNewBlock();
    };

    template<size_t ITEM_SIZE, size_t ITEM_COUNT>
    void SegregatedPool<ITEM_SIZE, ITEM_COUNT, EPoolArchitecture::NON_THREAD_SAFE>::addNewBlock() {
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
    void *SegregatedPool<ITEM_SIZE, ITEM_COUNT, EPoolArchitecture::NON_THREAD_SAFE>::allocate() {
        // Allocate from last deleted block
        if(last_deleted_item != nullptr) {
            auto* new_item = last_deleted_item;
            last_deleted_item = last_deleted_item->previous;

            return new_item;
        }

        // Allocate from offset
        if(last_block->offset < block_size) {
            auto* new_item = reinterpret_cast<char*>(last_block) + sizeof(SSegregatedBlockHeader) + last_block->offset;
            last_block->offset += ITEM_SIZE;

            return new_item;
        }

        // Add new block & allocate from it
        this->addNewBlock();
        last_block->offset += ITEM_SIZE;
        return reinterpret_cast<char*>(last_block) + sizeof(SSegregatedBlockHeader);
    }

    template<size_t ITEM_SIZE, size_t ITEM_COUNT>
    void SegregatedPool<ITEM_SIZE, ITEM_COUNT, EPoolArchitecture::NON_THREAD_SAFE>::free(void *item) {
        auto* new_deleted_item = reinterpret_cast<SSegregatedDeletedItem*>(item);
        new_deleted_item->previous = last_deleted_item;
        last_deleted_item = new_deleted_item;
    }

    template<size_t ITEM_SIZE, size_t ITEM_COUNT>
    void SegregatedPool<ITEM_SIZE, ITEM_COUNT, EPoolArchitecture::NON_THREAD_SAFE>::dumpPoolData() {
        size_t block_count = 0;
        auto* current_block = reinterpret_cast<SSegregatedBlockHeader*>(&first_block);

        while (current_block != nullptr) {
            block_count++;
            std::cout << "Block Number: " << block_count << std::endl;
            current_block = current_block->next;
        }
    }
}

template<size_t ITEM_SIZE, size_t ITEM_COUNT>
inline void* operator new(size_t size, AppShift::Memory::SegregatedPool<ITEM_SIZE, ITEM_COUNT, EPoolArchitecture::NON_THREAD_SAFE>& pool) { return pool.allocate(); }
template<size_t ITEM_SIZE, size_t ITEM_COUNT>
inline void* operator new[](size_t size, AppShift::Memory::SegregatedPool<ITEM_SIZE, ITEM_COUNT, EPoolArchitecture::NON_THREAD_SAFE>& pool) { return pool.allocate(); }

#endif //APPSHIFTPOOL_SEGREGATED_STATICNTSPOOL_H
