/**
 * AppShift Memory Pool v2.0.0
 *
 * Copyright 2020-present DevShift (devshift.biz)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Sapir Shemer
 */
#pragma once
#define MEMORYPOOL_DEFAULT_BLOCK_SIZE 1024 * 1024

#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <memory>
#include <mutex>

namespace AppShift::Memory {
    /// Simple error collection for memory pool
    enum class EMemoryErrors {
        CANNOT_CREATE_MEMORY_POOL,
        CANNOT_CREATE_BLOCK,
        OUT_OF_POOL,
        EXCEEDS_MAX_SIZE,
        CANNOT_CREATE_BLOCK_CHAIN
    };

    /// Header of a deleted unit
    struct SMemoryDeletedHeader {
        size_t length;
        SMemoryDeletedHeader* prev;
    };

    /// Header for a single memory block
    struct SMemoryBlockHeader {
        // Block metadata
        size_t blockSize;
        size_t offset;

        // Movement to other blocks in chain
        SMemoryBlockHeader* next;
        SMemoryBlockHeader* prev;

        // Basic Garbage
        size_t numberOfAllocated;
        size_t numberOfDeleted;

        // Advanced Garbage
        SMemoryDeletedHeader* lastDeletedUnit;
        size_t biggestDeletedUnitSize;
    };

    /// Header of a memory unit in the pool
    struct SMemoryUnitHeader {
        size_t length;
        SMemoryBlockHeader* container;
    };

    /// Header for a scope in memory pool
    struct SMemoryScopeHeader {
        size_t scopeOffset;
        SMemoryBlockHeader* firstScopeBlock;
        SMemoryScopeHeader* prevScope;
    };

    class MemoryPool {
    public:
        /**
         * Creates a memory pool structure and initializes it
         *
         * @param size_t block_size Defines the default size of a block in the pool, by default uses MEMORYPOOL_DEFAULT_BLOCK_SIZE
         */
        MemoryPool(size_t block_size = MEMORYPOOL_DEFAULT_BLOCK_SIZE);
        // Destructor
        ~MemoryPool();

        /// @section Data

        /// Data about the memory pool blocks
        SMemoryBlockHeader* firstBlock;
        SMemoryBlockHeader* currentBlock;
        size_t defaultBlockSize;

        /// Data about memory scope/s
        SMemoryScopeHeader* currentScope;

        /// Pool mutex for thread safety support
        std::mutex poolMutex;

        /// @section Default Functionality

        /**
         * Create a new standalone memory block unattached to any memory pool
         *
         * @param size_t block_size Defines the default size of a block in the pool, by default uses MEMORYPOOL_DEFAULT_BLOCK_SIZE
         *
         * @returns SMemoryBlockHeader* Pointer to the header of the memory block
         */
        void createMemoryBlock(size_t block_size = MEMORYPOOL_DEFAULT_BLOCK_SIZE);

        /**
         * Allocates memory in a pool
         *
         * @param MemoryPool* mp Memory pool to allocate memory in
         * @param size_t size Size to allocate in memory pool
         *
         * @returns void* Pointer to the newly allocate space
         */
        void* allocate(size_t size);

        /// Templated allocation
        template<typename T>
        T* allocate(size_t instances);

        /**
         * Re-allocates memory in a pool
         *
         * @param void* unit_pointer_start Pointer to the object to re-allocate
         * @param size_t new_size New size to allocate in memory pool
         *
         * @returns void* Pointer to the newly allocate space
         */
        void* reallocate(void* unit_pointer_start, size_t new_size);

        // Templated re-allocation
        template<typename T>
        T* reallocate(T* unit_pointer_start, size_t new_size);

        /**
         * Frees memory in a pool
         *
         * @param void* unit_pointer_start Pointer to the object to free
         */
        void free(void* unit_pointer_start);

        /// @section Scoping the Heap

        /**
         * Start a scope in the memory pool.
         * All the allocations between startScope and andScope will be freed.
         * It is a very efficient way to free multiple allocations
         *
         * @param MemoryPool* mp Memory pool to start the scope in
         */
        void startScope();

        /**
         *
         */
        void endScope();

        /// @section Garbage Collection

        /**
         * Get and allocate new memory unit from garbage
         *
         * @param size Size of allocation to make
         * @return void* or nullptr if no size fit found
         */
        void* getFromGarbage(size_t size);

        /**
         * Adds a memory unit to garbage
         *
         * @param unit
         */
        void addToGarbage(SMemoryUnitHeader* unit);

        /// @section Debugging helper function

        /**
         * Dump memory pool meta data of blocks unit to stream.
         * Might be useful for debugging and analyzing memory usage.
         */
        void dumpPoolData();

        /**
         * Dump memory pool blocks meta-data: size, allocated, deleted, occupied
         */
        void dumpMiniPoolData();
	};

	template<typename T>
	inline T* MemoryPool::allocate(size_t instances) {
		return reinterpret_cast<T*>(this->allocate(instances * sizeof(T)));
	}

	template<typename T>
	inline T* MemoryPool::reallocate(T* unit_pointer_start, size_t instances) {
		return reinterpret_cast<T*>(this->reallocate(reinterpret_cast<void*>(unit_pointer_start), instances * sizeof(T)));
	}
}

/// Override new operators to assign memory from the memory pool
extern void* operator new(size_t size, AppShift::Memory::MemoryPool* mp);
extern void* operator new[](size_t size, AppShift::Memory::MemoryPool* mp);