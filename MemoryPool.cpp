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

#include "MemoryPool.h"

CPPShift::Memory::MemoryPool* CPPShift::Memory::MemoryPoolManager::create(size_t block_size)
{
	// Create memory pool
	MemoryPool* mp = reinterpret_cast<MemoryPool*>(malloc(sizeof(MemoryPool)));
	if (mp == NULL) throw EMemoryErrors::CANNOT_CREATE_MEMORY_POOL;

	// Add first block to memory pool
	mp->firstBlock = createMemoryBlock(block_size);
	mp->firstBlock->mp_container = mp;
	mp->currentBlock = mp->firstBlock;
	mp->defaultBlockSize = block_size;

	return mp;
}

CPPShift::Memory::SMemoryBlockHeader* CPPShift::Memory::MemoryPoolManager::createMemoryBlock(size_t block_size)
{
	// Create the block
	SMemoryBlockHeader* block = reinterpret_cast<SMemoryBlockHeader*>(malloc(sizeof(block) + block_size));
	if (block == NULL) throw EMemoryErrors::CANNOT_CREATE_BLOCK;

	// Initalize block data
	block->blockSize = block_size;
	block->offset = 0;
	block->mp_container = nullptr;
	block->next = nullptr;

	return block;
}

void* CPPShift::Memory::MemoryPoolManager::allocate(MemoryPool* mp, size_t size)
{
	if (mp == NULL) return nullptr;
	SMemoryBlockHeader* block_to_use;

	// If there is enough space in current block add to current and return pointer
	if (size + sizeof(SMemoryUnitHeader) < mp->currentBlock->blockSize - mp->currentBlock->offset) block_to_use = mp->currentBlock;
	// Create new block if not enough space
	else if (size + sizeof(SMemoryUnitHeader) >= mp->defaultBlockSize) {
		block_to_use = createMemoryBlock(size);
		mp->firstBlock->mp_container = mp;
		mp->currentBlock->next = block_to_use;
	}
	else {
		block_to_use = createMemoryBlock(mp->defaultBlockSize);
		mp->firstBlock->mp_container = mp;
		mp->currentBlock->next = block_to_use;
	}

	// Add unit
	SMemoryUnitHeader* unit = reinterpret_cast<SMemoryUnitHeader*>(reinterpret_cast<char*>(block_to_use) + sizeof(SMemoryBlockHeader) + block_to_use->offset);
	unit->length = size;
	unit->container = block_to_use;
	block_to_use->offset += sizeof(SMemoryUnitHeader) + size;

	return reinterpret_cast<char*>(unit) + sizeof(SMemoryUnitHeader);
}

void* CPPShift::Memory::MemoryPoolManager::allocate_unsafe(MemoryPool* mp, size_t size)
{
	SMemoryBlockHeader* block_to_use;

	// If there is enough space in current block add to current and return pointer
	if (size + sizeof(SMemoryUnitHeader) < mp->currentBlock->blockSize - mp->currentBlock->offset) block_to_use = mp->currentBlock;
	// Create new block if not enough space
	else if (size + sizeof(SMemoryUnitHeader) >= mp->defaultBlockSize) {
		block_to_use = createMemoryBlock(size);
		mp->firstBlock->mp_container = mp;
		mp->currentBlock->next = block_to_use;
	}
	else {
		block_to_use = createMemoryBlock(mp->defaultBlockSize);
		mp->firstBlock->mp_container = mp;
		mp->currentBlock->next = block_to_use;
	}

	// Add unit
	SMemoryUnitHeader* unit = reinterpret_cast<SMemoryUnitHeader*>(reinterpret_cast<char*>(block_to_use) + sizeof(SMemoryBlockHeader) + block_to_use->offset);
	unit->length = size;
	unit->container = block_to_use;
	block_to_use->offset += sizeof(SMemoryUnitHeader) + size;

	return reinterpret_cast<char*>(unit) + sizeof(SMemoryUnitHeader);
}

void* CPPShift::Memory::MemoryPoolManager::reallocate(void* unit_pointer_start, size_t new_size)
{
	if (unit_pointer_start == NULL) return nullptr;

	// Find unit
	SMemoryUnitHeader* unit = reinterpret_cast<SMemoryUnitHeader*>(reinterpret_cast<char*>(unit_pointer_start) - sizeof(SMemoryUnitHeader));
	SMemoryBlockHeader* block = unit->container;

	// If last in block && enough space in block, then reset length
	if (reinterpret_cast<char*>(block) + sizeof(SMemoryBlockHeader) + block->offset == reinterpret_cast<char*>(unit) + sizeof(SMemoryUnitHeader) + unit->length
		&& block->blockSize > block->offset + new_size - unit->length) {
		block->offset += new_size - unit->length;
		unit->length = new_size;

		return unit_pointer_start;
	}

#ifdef MEMORYPOOL_REUSE_GARBAGE
	// Loop through deleted units and find a unit with enough space
	SMemoryUnitHeader* unit_iterator = block->mp_container->lastDeletedUnit;
	while (unit_iterator != NULL) {
		if (unit_iterator->length >= new_size) return reinetrpret_cast<char*>(unit_iterator) + sizeof(SMemoryUnitHeader);
		unit_iterator = unit_iterator->prevDeleted;
	}
#endif // MEMORYPOOL_REUSE_GARBAGE


	// Allocate new and free previous
	void* temp_point = allocate_unsafe(block->mp_container, new_size);
	std::memcpy(temp_point, unit_pointer_start, unit->length);
	free(unit_pointer_start);

	return temp_point;
}

void CPPShift::Memory::MemoryPoolManager::free(void* unit_pointer_start)
{
	if (unit_pointer_start == nullptr) return;

	// Find unit
	SMemoryUnitHeader* unit = reinterpret_cast<SMemoryUnitHeader*>(reinterpret_cast<char*>(unit_pointer_start) - sizeof(SMemoryUnitHeader));
	SMemoryBlockHeader* block = unit->container;

	// If last in block, then reset offset
	if (reinterpret_cast<char*>(block) + sizeof(SMemoryBlockHeader) + block->offset == reinterpret_cast<char*>(unit) + sizeof(SMemoryUnitHeader) + unit->length) {
		block->offset -= sizeof(SMemoryUnitHeader) + unit->length;
		return;
	}

#ifdef MEMORYPOOL_REUSE_GARBAGE
	// Set unit deleted flag as true and add to deleted chain of units
	unit->isDeleted = true;
	MemoryPool container = reinterpret_cast<MemoryPool*>(block->container);
	unit->prev_deleted = container->lastDeletedUnit;
	container->lastDeletedUnit = unit;
#endif // MEMORYPOOL_REUSE_GARBAGE
}

void* operator new(size_t size, CPPShift::Memory::MemoryPool* mp) {
#ifndef MEMORYPOOL_SAFE_ALLOCATION
	return CPPShift::Memory::MemoryPoolManager::allocate_unsafe(mp, size);
#else
	return CPPShift::Memory::MemoryPoolManager::allocate(mp, size);
#endif // !MEMORYPOOL_SAFE_ALLOCATION
}

void* operator new[](size_t size, CPPShift::Memory::MemoryPool* mp) {
#ifndef MEMORYPOOL_SAFE_ALLOCATION
	return CPPShift::Memory::MemoryPoolManager::allocate_unsafe(mp, size);
#else
	return CPPShift::Memory::MemoryPoolManager::allocate(mp, size);
#endif // !MEMORYPOOL_SAFE_ALLOCATION
}