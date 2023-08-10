//
// Created by Sapir Shemer on 27/01/2023.
//

#ifndef APPSHIFTPOOL_STACK_COMMON_H
#define APPSHIFTPOOL_STACK_COMMON_H

#include <cstring>
#include <cstddef>
#include <memory>
#include <vector>
#include <thread>
#include <iostream>

#define INIT_STACK_BLOCK(block_to_init, SIZE) \
    block_to_init->previous = nullptr; \
    block_to_init->next = nullptr; \
    block_to_init->offset = 0; \
    block_to_init->size = SIZE; \
    block_to_init->number_of_allocated = 0; \
    block_to_init->last_deleted_item = nullptr; \

namespace AppShift::Memory {
    struct SStackPoolDeletedHeader;

    struct SStackPoolBlockHeader {
        // Links to previous & next blocks in the chain
        SStackPoolBlockHeader* previous = nullptr;
        SStackPoolBlockHeader* next = nullptr;

        // Current offset of allocations
        size_t offset = 0;
        // size of the block
        size_t size = 0;
        // Number of items allocated that reside before the offset
        size_t number_of_allocated = 0;

        // Holds a chain of deleted items
        SStackPoolDeletedHeader* last_deleted_item = nullptr;
    };

    struct SStackPoolScopeHeader {
        SStackPoolBlockHeader* container = nullptr;
        size_t offset = 0;

        // Pointer to previous scope to create the effect of nested scopes
        SStackPoolScopeHeader* previous = nullptr;
    };

    struct SStackPoolItemHeader {
        SStackPoolBlockHeader* container = nullptr;
        size_t size = 0;
    };

    struct SStackPoolDeletedHeader {
        SStackPoolItemHeader item_data = {};
        SStackPoolDeletedHeader* previous = nullptr;
    };

    /**
     * A static block that exists in the stack and represents a stack memory pool block.
     * The block is used to statically initialize block for a templated thread safe memory pool.
     *
     * @tparam SIZE The size of the block
     *
     * @author Sapir Shemer
     */
    template<size_t SIZE>
    struct SStackPoolStaticData {
        SStackPoolBlockHeader header = {
                nullptr,
                nullptr,
                0,
                SIZE,
                0,
                nullptr
        };
        char data[SIZE] = {};
    };

    class IStackPool {
    public:
        // Allocation/De-allocation
        virtual void* allocate(size_t size) = 0;
        virtual void* reallocate(void* item, size_t size) = 0;
        virtual void free(void* item) = 0;

        // Scoping the heap
        virtual void startScope() = 0;
        virtual void endScope() = 0;
    };

    /**
     * Allows sharing the same dynamic pool blocks for same size pool.
     * Provides the ability to make thread safe and lock free pool by implement the list of all
     * dynamic pools as thread local.
     */
    struct SDynamicPoolData {
        size_t block_size;
        // Block chain head & tail
        SStackPoolBlockHeader* first_block{};
        SStackPoolBlockHeader* last_block{};

        // Chain of all deleted items that are present before the offsets
        SStackPoolDeletedHeader* last_deleted_item = nullptr;

        // Scoping mechanism data
        SStackPoolScopeHeader* current_scope = nullptr;

        // Thread id to keep track if the pool exists in the desired thread
        std::thread::id thread_id = std::this_thread::get_id();
    };

    /**
     *
     */
    inline thread_local std::vector<SDynamicPoolData> dynamic_pools = std::vector<SDynamicPoolData>();

    void* allocateFromDeleted_unsafe(SStackPoolBlockHeader* block, size_t size);
    void* allocateFromOffset_unsafe(SStackPoolBlockHeader* block, size_t size);
    void removeBlock_unsafe(SStackPoolBlockHeader* &current_last, SStackPoolBlockHeader* block);
    void addNewBlock(SStackPoolBlockHeader* &current_last, size_t size);
}

#endif //APPSHIFTPOOL_STACK_COMMON_H
