/**
 * Architectural Styles (For further explanation on each style read the README.md file):
 * - Stack
 * - Thread Safe Lock-Based
 * - Static First Block
 *
 * @author Sapir Shemer
 */

#ifndef APPSHIFTPOOL_STACKTSLPOOL_H
#define APPSHIFTPOOL_STACKTSLPOOL_H

#include "common.h"
#include <iostream>
#include <mutex>
#include "../PoolArchitectures.h"
#include "../execution/EventLoop.h"

namespace AppShift::Memory {
    /**
     * A thread safe templated stack memory pool class that manages pre-allocated blocks for dynamic memory
     * allocations that act like an application's stack.
     *
     * Advantages:
     * - Faster than OS memory allocations.
     * - Take control of anti-fragmentation level and algorithms.
     * - Can easily analyze and collect data on software allocations.
     * - Can generate any size of allocation unlike a segregated pool.
     * - Thread safe allocations.
     * - First block is created statically.
     * - Memory Efficiency: Will use the same blocks for similar sized pools.
     *
     * Disadvantages:
     * - Slower that a segregated pool as more actions need to take place.
     * - Fragmentation problems can occur and induce memory corruption if an incorrect
     * fragmentation is selected, or if memory allocations on developer side are not linear.
     * - Thread safety is lock-based thus induces performance penalty.
     *
     * @note Pool object can pass through different threads and still be thread safe. Another pool
     * of the same size can be created and will use the same blocks.
     *
     * @tparam SIZE The size of each block in the stack - The bigger the size the less system calls are made.
     *
     * @author Sapir Shemer
     */
    template<size_t SIZE>
    class StackPool<SIZE, EPoolArchitecture::THREAD_SAFE_LOCK_BASED> : IStackPool {
    public:
        explicit StackPool() {
            if(first_block == nullptr) {
                first_block = reinterpret_cast<SStackPoolStaticData<SIZE>*>(
                        std::malloc(sizeof(SStackPoolStaticData<SIZE>))
                );
                last_block = &first_block->header;
                INIT_STACK_BLOCK(last_block, SIZE)
            }

            reference_count++;
        };

        ~StackPool() {
            reference_count--;
            if(reference_count == 0) {
                auto* current_block = last_block;
                while(current_block != nullptr) {
                    auto* next_block = current_block->previous;
                    std::free(current_block);
                    current_block = next_block;
                }

                first_block = nullptr;
                last_block = nullptr;
                current_scope = nullptr;
                last_deleted_item = nullptr;
            }
        }

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
        inline static SStackPoolStaticData<SIZE>* first_block = nullptr;
        inline static SStackPoolBlockHeader* last_block = nullptr;
        inline static std::atomic<size_t> reference_count = 0;

        // Chain of all deleted items that are present before the offsets
        inline static SStackPoolDeletedHeader* last_deleted_item = nullptr;

        // Scoping mechanism data
        inline static SStackPoolScopeHeader* current_scope = nullptr;

        // Functions for separating allocations & de-allocations into a set of different algorithms
        void addNewBlock(size_t size);
        void removeBlock_unsafe(SStackPoolBlockHeader* block);
        void* allocateFromOffset_unsafe(size_t size);
        void* allocateFromDeleted_unsafe(size_t size);

        // Locking mechanism
        static std::mutex pool_mutex;
    };

    template<size_t SIZE>
    std::mutex StackPool<SIZE, EPoolArchitecture::THREAD_SAFE_LOCK_BASED>::pool_mutex;

    template<size_t SIZE>
    void StackPool<SIZE, EPoolArchitecture::THREAD_SAFE_LOCK_BASED>::addNewBlock(size_t size) {
        // Create new block
        auto* new_block = reinterpret_cast<SStackPoolBlockHeader*>(
                std::malloc(size + sizeof(SStackPoolBlockHeader))
        );

        // Connect previous block to this block
        this->last_block->next = new_block;
        new_block->previous = this->last_block;
        new_block->next = nullptr;
        this->last_block = new_block;

        // Initialize block
        this->last_block->last_deleted_item = nullptr;
        this->last_block->offset = 0;
        this->last_block->number_of_allocated = 0;
        this->last_block->size = size;
    }

    template<size_t SIZE>
    void StackPool<SIZE, EPoolArchitecture::THREAD_SAFE_LOCK_BASED>::removeBlock_unsafe(SStackPoolBlockHeader *block) {
        // Use sorounding blocks
        auto* previous_block = block->previous;
        auto* next_block = block->next;

        // If there is more than one block, then modify the block list
        /// @note Equal only when both are nullptr
        if(previous_block != next_block) {
            if(next_block != nullptr) next_block->previous = previous_block;
            else this->last_block = previous_block; // If no next block then we are at the last
            if(previous_block != nullptr) {
                previous_block->next = next_block;
                std::free(block); // Delete only if not first block
            }
        } else {
            block->offset = 0;
            block->last_deleted_item = nullptr;
        }
    }

    template<size_t SIZE>
    void *StackPool<SIZE, EPoolArchitecture::THREAD_SAFE_LOCK_BASED>::allocateFromOffset_unsafe(size_t size) {
        /// @note Unsafe since it ignores if the new allocated memory overflows the block
        auto* new_item = reinterpret_cast<SStackPoolItemHeader*>(
                reinterpret_cast<char*>(this->last_block)
                + sizeof(SStackPoolBlockHeader)
                + this->last_block->offset
        );

        this->last_block->offset += size + sizeof(SStackPoolItemHeader);
        this->last_block->number_of_allocated++;
        new_item->size = size;
        new_item->container = this->last_block;
        return reinterpret_cast<char*>(new_item) + sizeof(SStackPoolItemHeader);
    }

    template<size_t SIZE>
    void *StackPool<SIZE, EPoolArchitecture::THREAD_SAFE_LOCK_BASED>::allocateFromDeleted_unsafe(size_t size) {
        auto* current_block = this->last_block;
        SStackPoolDeletedHeader *current_deleted_check = nullptr;

        do {
            // First fit algorithm
            current_deleted_check = current_block->last_deleted_item;
            // Keeps the next deleted unit for fixing the deleted objects chain late
            auto *next_deleted_unit = current_deleted_check;
            while (current_deleted_check != nullptr && current_deleted_check->item_data.size < size) {
                next_deleted_unit = current_deleted_check;
                current_deleted_check = current_deleted_check->previous;
            }

            // Remove deleted item from the deleted items list
            if (current_deleted_check != nullptr) {
                if (current_deleted_check == next_deleted_unit)
                    current_block->last_deleted_item = current_block->last_deleted_item->previous;
                else next_deleted_unit->previous = current_deleted_check->previous;

                current_block->number_of_allocated++;
                break;
            }

            // Move to next block
            current_block = current_block->previous;
        } while(current_block != nullptr);

        /// @note Unsafe because it can return a nullptr.
        /// Simply interpreting the deleted header as a unit header is enough since the structure
        /// is the same and contains the same data (item_data is SStackPoolItemHeader).
        return reinterpret_cast<char*>(current_deleted_check);
    }

    template<size_t SIZE>
    void *StackPool<SIZE, EPoolArchitecture::THREAD_SAFE_LOCK_BASED>::allocate(size_t size) {
        std::lock_guard<std::mutex> lock(this->pool_mutex);

        // Initialize last block
        if(this->last_block == nullptr) {
            this->last_block = reinterpret_cast<SStackPoolBlockHeader*>(&this->first_block);
        }

        // If enough space in block, allocate from last offset
        /// @note This check keeps the allocation safe
        if(this->last_block->offset + size + sizeof(SStackPoolItemHeader) <= this->last_block->size)
            return this->allocateFromOffset_unsafe(size);

        // Allocate from garbage
        auto* from_deleted = this->allocateFromDeleted_unsafe(size);
        if(from_deleted != nullptr) return reinterpret_cast<char*>(from_deleted) + sizeof(SStackPoolItemHeader);

        // Allocate from a new block
        this->addNewBlock(size > SIZE ? size : SIZE);
        return this->allocateFromOffset_unsafe(size);
    }

    template<size_t SIZE>
    void *StackPool<SIZE, EPoolArchitecture::THREAD_SAFE_LOCK_BASED>::reallocate(void *item, size_t size) {
        this->pool_mutex.lock();
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
        this->pool_mutex.unlock();

        // Allocate & copy
        void* new_item = this->allocate(size);
        std::memcpy(new_item, item, item_header->size);
        this->free(item);

        return new_item;
    }

    template<size_t SIZE>
    void StackPool<SIZE, EPoolArchitecture::THREAD_SAFE_LOCK_BASED>::free(void *item) {
        std::lock_guard<std::mutex> lock(this->pool_mutex);

        auto* item_header = reinterpret_cast<SStackPoolItemHeader*>(
                reinterpret_cast<char*>(item)
                - sizeof(SStackPoolItemHeader)
        );

        // Change the allocated numbers count
        item_header->container->number_of_allocated--;

        // Remove block if empty and not the only block
        if(item_header->container->number_of_allocated == 0) {
            this->removeBlock_unsafe(item_header->container);
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
    void StackPool<SIZE, EPoolArchitecture::THREAD_SAFE_LOCK_BASED>::startScope() {
        std::lock_guard<std::mutex> lock(this->pool_mutex);

        // Add scope to current pool
        if(this->last_block->size - this->last_block->offset < sizeof(SStackPoolScopeHeader))
            this->addNewBlock(SIZE);

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
    void StackPool<SIZE, EPoolArchitecture::THREAD_SAFE_LOCK_BASED>::endScope() {
        std::lock_guard<std::mutex> lock(this->pool_mutex);

        /// @note Unsafe if a scope is not present (this->current_scope = nullptr)
        // Remove blocks until we get to the current scope's container
        while(this->last_block != this->current_scope->container)
            this->removeBlock_unsafe(this->last_block);

        // Remove offset beyond current scope
        this->last_block->offset = this->current_scope->offset;
        // Go back to previous scope
        this->current_scope = this->current_scope->previous;
    }

    template<size_t SIZE>
    void StackPool<SIZE, EPoolArchitecture::THREAD_SAFE_LOCK_BASED>::dumpPoolData() {
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
inline void* operator new(size_t size, AppShift::Memory::StackPool<SIZE, EPoolArchitecture::THREAD_SAFE_LOCK_BASED>& pool) { return pool.allocate(size); }
template<size_t SIZE>
inline void* operator new[](size_t size, AppShift::Memory::StackPool<SIZE, EPoolArchitecture::THREAD_SAFE_LOCK_BASED>& pool) { return pool.allocate(size); }

#endif //APPSHIFTPOOL_STACKTSLPOOL_H
