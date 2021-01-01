/**
 * Memory Pool v1.0.0
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

#ifndef MEMORYPOOL_BLOCK_MAX_SIZE
#define MEMORYPOOL_BLOCK_MAX_SIZE 1024 * 1024
#endif

namespace CPPShift {
    namespace Memory {
        // Simple error collection for memory pool
        enum class EMemoryErrors {
            CANNOT_CREATE_BLOCK,
            OUT_OF_POOL,
            EXCEEDS_MAX_SIZE
        };

        // Header for a single memory block
        struct SMemoryBlockHeader {
            size_t block_size;
            size_t offset;
            SMemoryBlockHeader* next; // The next block
        };

        // Header of a memory unit in the pool holding important metadata
        struct SMemoryUnitHeader {
            size_t length;
#ifdef MEMORYPOOL_REUSE_GARBAGE
            SMemoryUnitHeader* prev_deleted;
#endif
            bool is_deleted; // Later used for garbase collection
        };

        class MemoryPool {
        private:
            // Maximum block size
            size_t maxBlockSize = MEMORYPOOL_BLOCK_MAX_SIZE;
            // The current block at use
            SMemoryBlockHeader* currentBlock;
            // The tail of the block chain
            SMemoryBlockHeader* firstBlock;
#ifdef MEMORYPOOL_REUSE_GARBAGE
            // Holds the last deleted block - used for smart junk memory management
            SMemoryUnitHeader* lastDeletedUnit;
#endif
            SMemoryBlockHeader* createMemoryBlock(size_t block_size = MEMORYPOOL_BLOCK_MAX_SIZE);
        public:
            MemoryPool(size_t max_block_size = MEMORYPOOL_BLOCK_MAX_SIZE);
            ~MemoryPool();

            // Allocate memory in the pool
            template<typename T>
            T* allocate(size_t instances = 1);
            // Reallocate memory in the pool
            template<typename T>
            T* rellocate(T* memory_unit_ptr, size_t instances = 1);
            // Remove memory unit from the pool
            void remove(void* memory_unit_ptr);
        };

        template<typename T>
        inline T* MemoryPool::allocate(size_t instances)
        {
            if (instances <= 0) return nullptr;
            size_t length = instances * sizeof(T);
            size_t full_length = length + sizeof(SMemoryUnitHeader);
#ifndef MEMORYPOOL_IGNORE_MAX_BLOCK_SIZE
            // Check if size exceeds pool size
            if (full_length > this->maxBlockSize) throw EMemoryErrors::EXCEEDS_MAX_SIZE;
#endif
#ifdef MEMORYPOOL_REUSE_GARBAGE
            // Find big enough deleted block before proceeding
            SMemoryUnitHeader* temp_deleted_unit = this->lastDeletedUnit;
            while (temp_deleted_unit != nullptr) {
                if (temp_deleted_unit->length >= length) {
                    temp_deleted_unit->is_deleted = false;
                    this->lastDeletedUnit = temp_deleted_unit->prev_deleted;
                    return reinterpret_cast<T*>(reinterpret_cast<char*>(temp_deleted_unit) + sizeof(SMemoryUnitHeader));
                }
            }
#endif

            // Get the current offest
            size_t offset = this->currentBlock->offset;

            // Find a free or new block if necessary
            if (offset + full_length >= this->currentBlock->block_size - sizeof(SMemoryBlockHeader)) {
                SMemoryBlockHeader* iter = this->firstBlock;
                offset = iter->offset;

                // Run through existing blocks
                while (offset + full_length >= iter->block_size - sizeof(SMemoryBlockHeader)) {
                    // Skip current block
                    if (iter->next == this->currentBlock) iter = iter->next;

                    // Create new block if free space not found
                    if (iter->next == nullptr) {
                        try {
#ifndef MEMORYPOOL_IGNORE_MAX_BLOCK_SIZE
                            iter->next = this->createMemoryBlock(this->maxBlockSize);
#else
                            iter->next = this->createMemoryBlock(
                                full_length + sizeof(SMemoryBlockHeader) > this->maxBlockSize ?
                                full_length + sizeof(SMemoryBlockHeader) : this->maxBlockSize
                            );
#endif
                            this->currentBlock = iter->next;
                            offset = this->currentBlock->offset;
                            break;
                        }
                        catch (EMemoryErrors e) {
                            throw e;
                        }
                    }

                    iter = iter->next;
                }
            }

            // Set memory unit header
            SMemoryUnitHeader* unit = reinterpret_cast<SMemoryUnitHeader*>(
                reinterpret_cast<char*>(this->currentBlock) + sizeof(SMemoryBlockHeader) + offset
                );
            unit->length = length;
            unit->is_deleted = false;
#ifdef MEMORYPOOL_REUSE_GARBAGE
            unit->prev_deleted = nullptr;
#endif
            this->currentBlock->offset += full_length;
            // return offset of new memory unit
            return reinterpret_cast<T*>(reinterpret_cast<char*>(unit) + sizeof(SMemoryUnitHeader));
        }

        template<typename T>
        inline T* MemoryPool::rellocate(T* memory_unit_ptr, size_t instances)
        {
            if (memory_unit_ptr == nullptr) return this->allocate<T>(instances);
            else if (instances <= 0) {
                this->remove(memory_unit_ptr);
                return nullptr;
            }
            size_t length = sizeof(T) * instances;

            // Find unit
            SMemoryUnitHeader* unit = reinterpret_cast<SMemoryUnitHeader*>(
                reinterpret_cast<char*>(memory_unit_ptr) - sizeof(SMemoryUnitHeader)
                );
            // Check for current block
            SMemoryBlockHeader* block = this->currentBlock;
            // Find in other blocks
            if (reinterpret_cast<char*>(unit) > reinterpret_cast<char*>(block) + block->block_size
                || reinterpret_cast<char*>(unit) < reinterpret_cast<char*>(block) + sizeof(SMemoryBlockHeader))
            {
                block = this->firstBlock;
                while (reinterpret_cast<char*>(unit) > reinterpret_cast<char*>(block) + block->block_size
                    || reinterpret_cast<char*>(unit) < reinterpret_cast<char*>(block) + sizeof(SMemoryBlockHeader))
                {
                    if (block->next == nullptr) throw EMemoryErrors::OUT_OF_POOL;
                    block = block->next;
                    if (block == this->currentBlock) block = block->next;
                }
            }
            // If unit is the last in the block
            if (reinterpret_cast<char*>(unit) + sizeof(SMemoryUnitHeader) + unit->length
                == reinterpret_cast<char*>(block) + sizeof(SMemoryBlockHeader) + block->offset)
            {
                block->offset += length - unit->length;
                unit->length = length;
                return memory_unit_ptr;
            }

            if (unit->length > length) return memory_unit_ptr;
            void* new_unit = this->allocate<T>(length);
            memcpy(reinterpret_cast<char*>(new_unit) + sizeof(SMemoryUnitHeader), memory_unit_ptr, unit->length);
            this->remove(memory_unit_ptr);
            return reinterpret_cast<T*>(reinterpret_cast<char*>(new_unit) + sizeof(SMemoryUnitHeader));
        }
    }
}

