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

#include <stdlib.h>
#define MEMORYPOOL_DEFAULT_BLOCK_SIZE 1024 * 1024

namespace CPPShift::Memory {
    struct MemoryPool;

    // Simple error collection for memory pool
    enum class EMemoryErrors {
        CANNOT_CREATE_MEMORY_POOL,
        CANNOT_CREATE_BLOCK,
        OUT_OF_POOL,
        EXCEEDS_MAX_SIZE,
        CANNOT_CREATE_BLOCK_CHAIN
    };

    // Header for a single memory block
    struct SMemoryBlockHeader {
        size_t blockSize;
        size_t offset;
        MemoryPool* mp_container;
        SMemoryBlockHeader* next;
        SMemoryBlockHeader* prev;
    };

    // Header of a memory unit in the pool holding important metadata
    struct SMemoryUnitHeader {
        size_t length;
        SMemoryBlockHeader* container;
#ifdef MEMORYPOOL_REUSE_GARBAGE
        SMemoryUnitHeader* prevDeleted;
        bool isDeleted;
#endif
    };

    // Header for a scope in memory
    struct SMemoryScopeHeader {
        size_t scopeOffset;
        SMemoryBlockHeader* firstScopeBlock;
        SMemoryScopeHeader* prevScope;
    };

    // A memory representation in memory
    struct MemoryPool {
        // Destructor to guard from memory corruption
        ~MemoryPool() {
            SMemoryBlockHeader* block_iterator = firstBlock;

            while (block_iterator != nullptr) {
                SMemoryBlockHeader* next_iterator = block_iterator->next;
                free(block_iterator);
                block_iterator = next_iterator;
            }
        }
        // Data about the memory pool blocks
        SMemoryBlockHeader* firstBlock;
        SMemoryBlockHeader* currentBlock;
        size_t defaultBlockSize;

        // Data about memory scopes
        SMemoryScopeHeader* currentScope;

#ifdef MEMORYPOOL_REUSE_GARBAGE
        // Holds the last deleted block - used for smart junk memory management
        SMemoryUnitHeader* lastDeletedUnit;
#endif
    };
}