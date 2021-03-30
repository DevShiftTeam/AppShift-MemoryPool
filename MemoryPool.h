/**
 * CPPShift Memory Pool v2.0.0
 *
 * Copyright 2020-present Sapir Shemer, DevShift (devshift.biz)
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

#include "MemoryPoolData.h"
#include <cstring>
#include <cstddef>
#include <memory>

namespace CPPShift::Memory {
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

        // Data about the memory pool blocks
        SMemoryBlockHeader* firstBlock;
        SMemoryBlockHeader* currentBlock;
        size_t defaultBlockSize;

        // Data about memory scopes
        SMemoryScopeHeader* currentScope;

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

		// Templated allocation
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

		/**
		 * Dump memory pool meta data of blocks unit to stream. 
		 * Might be useful for debugging and analyzing memory usage
		 * 
		 * @param MemoryPool* mp Memory pool to dump data from
		 */
		void dumpPoolData();

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

// Override new operators to create with memory pool
extern void* operator new(size_t size, CPPShift::Memory::MemoryPool* mp);
extern void* operator new[](size_t size, CPPShift::Memory::MemoryPool* mp);