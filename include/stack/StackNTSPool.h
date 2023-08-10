/**
 * Architectural Styles (For further explanation on each style read the README.md file):
 * - Stack
 * - Non-Thread Safe
 * - Static First Block
 *
 * @author Sapir Shemer
 */

#ifndef APPSHIFTPOOL_STACK_STATICNTSPOOL_H
#define APPSHIFTPOOL_STACK_STATICNTSPOOL_H

#include "common.h"

namespace AppShift::Memory {
    /**
     * A stack memory pool class that manages pre-allocated blocks for dynamic memory
     * allocations that act like an application's stack.
     *
     * Advantages:
     * - Faster than OS memory allocations.
     * - Take control of anti-fragmentation level and algorithms.
     * - Can easily analyze and collect data on software allocations.
     * - Can generate any size of allocation unlike a segregated pool.
     * - First block is created statically.
     *
     * Disadvantages:
     * - Slower that a segregated pool as more actions need to take place.
     * - Fragmentation problems can occur and induce memory corruption if an incorrect
     * fragmentation is selected, or if memory allocations on developer side are not linear.
     * - Non-thread safe
     *
     * @tparam SIZE The size of each block in the stack - The bigger the size the less system calls are made.
     *
     * @author Sapir Shemer
     */
    template<size_t SIZE>
    class StackPool<SIZE, EPoolArchitecture::NON_THREAD_SAFE> : IStackPool {
    public:
        explicit StackPool() = default;
        ~StackPool() {
            auto* current_block = last_block;

            while(current_block != &first_block.header) {
                auto* previous_block = current_block->previous;
                std::free(current_block);
                current_block = previous_block;
            }
        };

        // Allocate/Free pool interface
        void* allocate(size_t size) override;
        void* reallocate(void* item, size_t size) override;
        void free(void* item) override;

        // Scope allocations together
        void startScope() override;
        // Free all allocation after the scope
        void endScope() override;

        void dumpPoolData();
    private:
        SStackPoolStaticData<SIZE> first_block = {};
        SStackPoolBlockHeader* last_block = reinterpret_cast<SStackPoolBlockHeader*>(&first_block);

        // Chain of all deleted items that are present before the offsets
        SStackPoolDeletedHeader* last_deleted_item = nullptr;

        // Scoping mechanism data
        SStackPoolScopeHeader* current_scope = nullptr;
    };

    template<size_t SIZE>
    void *StackPool<SIZE, EPoolArchitecture::NON_THREAD_SAFE>::allocate(size_t size) {
        // If enough space in block, allocate from last offset
        /// @note This check keeps the allocation safe
        if(this->last_block->offset + size + sizeof(SStackPoolItemHeader) <= this->last_block->size)
            return allocateFromOffset_unsafe(this->last_block, size);

        // Allocate from garbage
        auto* from_deleted = allocateFromDeleted_unsafe(this->last_block ,size);
        if(from_deleted != nullptr) return reinterpret_cast<char*>(from_deleted) + sizeof(SStackPoolItemHeader);

        // Allocate from a new block
        addNewBlock(this->last_block, size > SIZE ? size : SIZE);
        return allocateFromOffset_unsafe(this->last_block, size);
    }

    template<size_t SIZE>
    void *StackPool<SIZE, EPoolArchitecture::NON_THREAD_SAFE>::reallocate(void *item, size_t size) {
        auto* item_header = reinterpret_cast<SStackPoolItemHeader*>(
                reinterpret_cast<char*>(item)
                - sizeof(SStackPoolItemHeader)
        );

        // If there is enough space in the item itself do nothing
        if(item_header->size >= size) return item;
        // If item is last in block, and there is enough space in offset, then expand offset and item size
        if(
                reinterpret_cast<char*>(item_header->container)
                + sizeof(SStackPoolBlockHeader)
                + item_header->container->offset
                == reinterpret_cast<char*>(item) + item_header->size
                && size <= item_header->size + item_header->container->size - item_header->container->offset
                ) {
            item_header->container->offset += size - item_header->size;
            item_header->size = size;
            return item;
        }

        // Allocate & copy
        void* new_item = this->allocate(size);
        std::memcpy(new_item, item, item_header->size);
        this->free(item);

        return new_item;
    }

    template<size_t SIZE>
    void StackPool<SIZE, EPoolArchitecture::NON_THREAD_SAFE>::free(void *item) {
        auto* item_header = reinterpret_cast<SStackPoolItemHeader*>(
                reinterpret_cast<char*>(item)
                - sizeof(SStackPoolItemHeader)
        );

        // Change the allocated numbers count
        item_header->container->number_of_allocated--;

        // Remove block if empty and not the only block
        if(item_header->container->number_of_allocated == 0) {
            removeBlock_unsafe(this->last_block, item_header->container);
        }
            // Remove from offset
        else if(
                reinterpret_cast<char*>(item_header->container)
                + sizeof(SStackPoolBlockHeader)
                + item_header->container->offset
                == reinterpret_cast<char*>(item) + item_header->size
                ) {
            item_header->container->offset -= item_header->size + sizeof(SStackPoolItemHeader);
        }
            // Mark as a deleted item
        else {
            reinterpret_cast<SStackPoolDeletedHeader*>(item_header)->previous = item_header->container->last_deleted_item;
            item_header->container->last_deleted_item = reinterpret_cast<SStackPoolDeletedHeader*>(item_header);
        }
    }

    template<size_t SIZE>
    void StackPool<SIZE, EPoolArchitecture::NON_THREAD_SAFE>::startScope() {
        // Add scope to current pool
        if(this->last_block->size - this->last_block->offset < sizeof(SStackPoolScopeHeader))
            addNewBlock(this->last_block, SIZE);

        // Create the scope object
        auto* new_scope = reinterpret_cast<SStackPoolScopeHeader*>(
                reinterpret_cast<char*>(this->last_block)
                + sizeof(SStackPoolBlockHeader)
                + this->last_block->offset
        );

        // Set scope data
        new_scope->offset = this->last_block->offset;
        new_scope->container = this->last_block;
        new_scope->previous = this->current_scope;
        this->current_scope = new_scope;

        // Update block offset
        this->last_block->offset += sizeof(SStackPoolScopeHeader);
    }

    template<size_t SIZE>
    void StackPool<SIZE, EPoolArchitecture::NON_THREAD_SAFE>::endScope() {
        /// @note Unsafe if a scope is not present (this->current_scope = nullptr)
        // Remove blocks until we get to the current scope's container
        while(this->last_block != this->current_scope->container)
            removeBlock_unsafe(this->last_block, this->last_block);

        // Remove offset beyond current scope
        this->last_block->offset = this->current_scope->offset;
        // Go back to previous scope
        this->current_scope = this->current_scope->previous;
    }

    template<size_t SIZE>
    void StackPool<SIZE, EPoolArchitecture::NON_THREAD_SAFE>::dumpPoolData() {
        auto* current_block = reinterpret_cast<SStackPoolBlockHeader*>(&first_block);
        size_t block_count = 0;

        while(current_block != nullptr) {
            block_count++;
            std::cout << "Block Number: " << block_count << std::endl;
            std::cout << "Block Size: " << current_block->size << std::endl;
            std::cout << "Block Offset: " << current_block->offset << std::endl;
            std::cout << "Fullness: " << (float) ((float)current_block->offset / (float)current_block->size) * 100 << "%" << std::endl;

            size_t total_deleted = 0;
            size_t total_deleted_count = 0;
            auto* current_deleted = current_block->last_deleted_item;
            while(current_deleted != nullptr) {
                total_deleted_count++;
                total_deleted += current_deleted->item_data.size + sizeof(SStackPoolItemHeader);
                current_deleted = current_deleted->previous;
            }
            std::cout << "Total deleted items: " << total_deleted_count << std::endl;
            std::cout << "Total Free Space: " << current_block->size - current_block->offset + total_deleted << std::endl;
            std::cout << "Fullness (Without Deleted): " << (float) ((float)(current_block->offset - total_deleted) / (float)current_block->size) * 100 << "%" << std::endl;

            current_block = current_block->next;
        }
    }
}

template<size_t SIZE>
inline void* operator new(size_t size, AppShift::Memory::StackPool<SIZE, EPoolArchitecture::NON_THREAD_SAFE>& pool) { return pool.allocate(size); }
template<size_t SIZE>
inline void* operator new[](size_t size, AppShift::Memory::StackPool<SIZE, EPoolArchitecture::NON_THREAD_SAFE>& pool) { return pool.allocate(size); }

#endif //APPSHIFTPOOL_STACK_STATICNTSPOOL_H
