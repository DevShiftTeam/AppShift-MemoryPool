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

#include "MemoryPool.h"
#include <memory>

namespace CPPShift {
    namespace Memory {
        SMemoryBlockHeader* MemoryPool::createMemoryBlock(SIZE block_size)
        {
#ifndef MEMORYPOOL_IGNORE_MAX_BLOCK_SIZE
            // Check if size exceeds pool size
            if (block_size > MEMORYPOOL_BLOCK_MAX_SIZE) throw EMemoryErrors::EXCEEDS_MAX_SIZE;
#endif
        SMemoryBlockHeader* block = reinterpret_cast<SMemoryBlockHeader*>(malloc(block_size));
        if (block == NULL) throw EMemoryErrors::CANNOT_CREATE_BLOCK;
        // Default metadata
        memset(block, 0, block_size);
        block->block_size = block_size;
        block->offset = 0;
        block->next = nullptr;

        return block;

    }

        MemoryPool::MemoryPool(SIZE max_block_size)
        {
#ifndef MEMORYPOOL_IGNORE_MAX_BLOCK_SIZE
            // Check if size exceeds pool size
            if (max_block_size > MEMORYPOOL_BLOCK_MAX_SIZE) throw EMemoryErrors::EXCEEDS_MAX_SIZE;
#endif
            try {
                this->maxBlockSize = max_block_size;
                this->firstBlock = this->createMemoryBlock(max_block_size);
                this->currentBlock = this->firstBlock;
#ifdef MEMORYPOOL_REUSE_GARBAGE
                this->lastDeletedUnit = nullptr;
#endif
            }
            catch (EMemoryErrors e) {
                throw e;
            }
        }

        MemoryPool::~MemoryPool()
        {
            // Free all blocks
            SMemoryBlockHeader* lastBlock = this->firstBlock;
            while (lastBlock->next != nullptr) {
                SMemoryBlockHeader* tempPtr = lastBlock->next;
                free(lastBlock);
                lastBlock = tempPtr;
            }
            free(lastBlock);
        }

        void MemoryPool::remove(void* memory_unit_ptr)
        {
            // Find unit
            SMemoryUnitHeader* unit = reinterpret_cast<SMemoryUnitHeader*>(reinterpret_cast<char*>(memory_unit_ptr) - sizeof(SMemoryUnitHeader));
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
            // If last get block offset back
            if (reinterpret_cast<char*>(unit) + sizeof(SMemoryUnitHeader) + unit->length
                == reinterpret_cast<char*>(block) + sizeof(SMemoryBlockHeader) + block->offset)
                block->offset -= unit->length + sizeof(SMemoryUnitHeader);
            else {
                unit->is_deleted = true; // Flag as deleted
#ifdef MEMORYPOOL_REUSE_GARBAGE
                unit->prev_deleted = this->lastDeletedUnit;
                this->lastDeletedUnit = unit;
#endif
            }
        }
    }
}