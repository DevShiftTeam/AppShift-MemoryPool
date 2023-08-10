//
// Created by Sapir Shemer on 30/01/2023.
//

#ifndef APPSHIFTPOOL_SEGREGATED_STATICTSLPOOL_H
#define APPSHIFTPOOL_SEGREGATED_STATICTSLPOOL_H

#include "common.h"
#include "../PoolArchitectures.h"

namespace AppShift::Memory {
    template<size_t ITEM_SIZE, size_t ITEM_COUNT>
    struct SSegregatedThreadSafeData;

    template<size_t ITEM_SIZE, size_t ITEM_COUNT>
    struct SSegregatedItemHeader {
        SSegregatedThreadSafeData<ITEM_SIZE, ITEM_COUNT>* parent = nullptr;
    };

    template<size_t SIZE>
    struct SSegregatedStaticThreadSafeData {
        SSegregatedBlockHeader header = {};
        char data[SIZE] = {};
    };

    template<size_t ITEM_SIZE, size_t ITEM_COUNT>
    struct SSegregatedThreadSafeData {
        // SSegregatedBlockHeader
        SSegregatedStaticThreadSafeData<(ITEM_SIZE + sizeof(SSegregatedItemHeader<ITEM_SIZE, ITEM_COUNT>)) * ITEM_COUNT> first_block {};
        // Last block in the chain of available memory blocks
        SSegregatedBlockHeader* last_block = nullptr;
        SSegregatedDeletedItem* last_deleted_item = nullptr;
        std::mutex* mutex = nullptr;

        size_t reference_count = 0;
        const size_t block_size = ITEM_COUNT * (ITEM_SIZE + sizeof(SSegregatedItemHeader<ITEM_SIZE, ITEM_COUNT>));
    };

    /**
     * The templated pool has thread local segregated memory blocks that it manages.
     * The first block is created at compile-time, and other blocks are manages at runtime.
     *
     * Advantages:
     * - "Endless" Memory: If a block is full, it generated a new one.
     * - Very fast, fast initialization since the first block is defined at compile-time
     * - Thread safe :D
     * - No fragmentation!
     * - Different instances with the same size and count will use the same pool blocks.
     * Disadvantages:
     * - Lock-based and does impose performance overhead
     * - Only allocates a single size at a time.
     * - Doesn't remove unused blocks (though they will be chained by the deleted items)
     *
     * @tparam ITEM_SIZE    Size of a segment - allocations return pointers with this size available
     * @tparam ITEM_COUNT   Number of items in each block - a smaller size will be more memory efficient but a less
     * performant.
     *
     * @author Sapir Shemer
     */
    template<size_t ITEM_SIZE, size_t ITEM_COUNT>
    class SegregatedPool<ITEM_SIZE, ITEM_COUNT, EPoolArchitecture::THREAD_SAFE_LOCK_BASED> {
    public:
        SegregatedPool() {
            if(thread_local_data.last_block == nullptr) {
                thread_local_data.mutex = &thread_local_mutex;
                thread_local_data.reference_count = 0;
                thread_local_data.last_block = &thread_local_data.first_block.header;
                thread_local_data.last_deleted_item = nullptr;
            }
            thread_local_data.reference_count++;
        }

        ~SegregatedPool() {
            thread_local_data.reference_count--;
            if(thread_local_data.reference_count == 0) {
                // Remove all blocks
                auto *current_block = thread_local_data.last_block;
                while (current_block != &thread_local_data.first_block.header) {
                    auto *previous_block = current_block->previous;
                    std::free(current_block);
                    current_block = previous_block;
                }
                thread_local_data.last_block = nullptr;
                thread_local_data.last_deleted_item = nullptr;
            }
        }

        void* allocate();
        void free(void* item);
        void dumpPoolData();

    private:
        static thread_local std::mutex thread_local_mutex;
        inline static __thread SSegregatedThreadSafeData<ITEM_SIZE, ITEM_COUNT> thread_local_data = {};
        static void addNewBlock();
    };

    // Mutex init
    template<size_t ITEM_SIZE, size_t ITEM_COUNT>
    thread_local std::mutex SegregatedPool<ITEM_SIZE, ITEM_COUNT, EPoolArchitecture::THREAD_SAFE_LOCK_BASED>::thread_local_mutex;

    template<size_t ITEM_SIZE, size_t ITEM_COUNT>
    void SegregatedPool<ITEM_SIZE, ITEM_COUNT, EPoolArchitecture::THREAD_SAFE_LOCK_BASED>::addNewBlock() {
        // Create next block
        thread_local_data.last_block->next = reinterpret_cast<SSegregatedBlockHeader*>(
                    std::malloc(sizeof(SSegregatedBlockHeader) + thread_local_data.block_size)
        );

        // Initialize data & connect to chain
        thread_local_data.last_block->next->previous = thread_local_data.last_block;
        thread_local_data.last_block = thread_local_data.last_block->next;
        thread_local_data.last_block->offset = 0;
        thread_local_data.last_block->next = nullptr;
    }

    template<size_t ITEM_SIZE, size_t ITEM_COUNT>
    void *SegregatedPool<ITEM_SIZE, ITEM_COUNT, EPoolArchitecture::THREAD_SAFE_LOCK_BASED>::allocate() {
        if(thread_local_data.last_block == NULL) {
            thread_local_data.mutex = &thread_local_mutex;
            thread_local_data.reference_count = 0;
            thread_local_data.last_block = &thread_local_data.first_block.header;
            thread_local_data.last_deleted_item = nullptr;
        }

        SSegregatedItemHeader<ITEM_SIZE, ITEM_COUNT>* new_item;

        std::lock_guard<std::mutex> lock(*thread_local_data.mutex);
        // Allocate from last deleted block
        if(thread_local_data.last_deleted_item != nullptr) {
            new_item = reinterpret_cast<SSegregatedItemHeader<ITEM_SIZE, ITEM_COUNT>*>(thread_local_data.last_deleted_item);
            thread_local_data.last_deleted_item = thread_local_data.last_deleted_item->previous;
            new_item->parent = &thread_local_data;

            return reinterpret_cast<char*>(new_item) + sizeof(SSegregatedItemHeader<ITEM_SIZE, ITEM_COUNT>);
        }

        // Allocate from offset
        if(thread_local_data.last_block->offset < thread_local_data.block_size) {
            new_item = reinterpret_cast<SSegregatedItemHeader<ITEM_SIZE, ITEM_COUNT>*>(
                    reinterpret_cast<char*>(thread_local_data.last_block)
                    + sizeof(SSegregatedBlockHeader)
                    + thread_local_data.last_block->offset
            );
            new_item->parent = &thread_local_data;
            thread_local_data.last_block->offset += ITEM_SIZE + sizeof(SSegregatedItemHeader<ITEM_SIZE, ITEM_COUNT>);

            return reinterpret_cast<char*>(new_item) + sizeof(SSegregatedItemHeader<ITEM_SIZE, ITEM_COUNT>);
        }

        // Add new block & allocate from it
        this->addNewBlock();
        thread_local_data.last_block->offset += ITEM_SIZE + sizeof(SSegregatedItemHeader<ITEM_SIZE, ITEM_COUNT>);
        new_item = reinterpret_cast<SSegregatedItemHeader<ITEM_SIZE, ITEM_COUNT>*>(reinterpret_cast<char*>(thread_local_data.last_block) + sizeof(SSegregatedBlockHeader));
        new_item->parent = reinterpret_cast<SSegregatedThreadSafeData<ITEM_SIZE, ITEM_COUNT>*>(&thread_local_data);

        return reinterpret_cast<char*>(new_item) + sizeof(SSegregatedItemHeader<ITEM_SIZE, ITEM_COUNT>);
    }

    template<size_t ITEM_SIZE, size_t ITEM_COUNT>
    void SegregatedPool<ITEM_SIZE, ITEM_COUNT, EPoolArchitecture::THREAD_SAFE_LOCK_BASED>::free(void *item) {
        auto* item_header = reinterpret_cast<SSegregatedItemHeader<ITEM_SIZE, ITEM_COUNT>*>(reinterpret_cast<char*>(item) - sizeof(SSegregatedItemHeader<ITEM_SIZE, ITEM_COUNT>));
        auto* item_parent = item_header->parent;

        std::lock_guard<std::mutex> lock(*item_parent->mutex);
        auto* new_deleted_item = reinterpret_cast<SSegregatedDeletedItem*>(item_header);
        new_deleted_item->previous = thread_local_data.last_deleted_item;
        thread_local_data.last_deleted_item = new_deleted_item;
    }

    template<size_t ITEM_SIZE, size_t ITEM_COUNT>
    void SegregatedPool<ITEM_SIZE, ITEM_COUNT, EPoolArchitecture::THREAD_SAFE_LOCK_BASED>::dumpPoolData() {
        size_t block_count = 0;
        auto* current_block = reinterpret_cast<SSegregatedBlockHeader*>(&thread_local_data.first_block);

        while (current_block != nullptr) {
            block_count++;
            std::cout << "Block Number: " << block_count << std::endl;
            current_block = current_block->next;
        }
    }
}

template<size_t ITEM_SIZE, size_t ITEM_COUNT>
inline void* operator new(size_t size, AppShift::Memory::SegregatedPool<ITEM_SIZE, ITEM_COUNT, EPoolArchitecture::THREAD_SAFE_LOCK_BASED>& pool) { return pool.allocate(); }
template<size_t ITEM_SIZE, size_t ITEM_COUNT>
inline void* operator new[](size_t size, AppShift::Memory::SegregatedPool<ITEM_SIZE, ITEM_COUNT, EPoolArchitecture::THREAD_SAFE_LOCK_BASED>& pool) { return pool.allocate(); }
#endif //APPSHIFTPOOL_SEGREGATED_STATICTSLPOOL_H
