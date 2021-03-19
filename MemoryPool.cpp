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
#include <iostream>

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
	mp->currentScope = nullptr;

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
	block->prev = nullptr;

	return block;
}

void* CPPShift::Memory::MemoryPoolManager::allocate(MemoryPool* mp, size_t size)
{
	if (mp == NULL) return nullptr;// If there is enough space in current block then use the current block
	if (size + sizeof(SMemoryUnitHeader) < mp->currentBlock->blockSize - mp->currentBlock->offset);
	// Create new block if not enough space
	else if (size + sizeof(SMemoryUnitHeader) >= mp->defaultBlockSize) {
		mp->currentBlock->next = createMemoryBlock(size + sizeof(SMemoryUnitHeader));
		mp->currentBlock->next->mp_container = mp;
		mp->currentBlock->next->prev = mp->currentBlock;
		mp->currentBlock = mp->currentBlock->next;
	}
	else {
		mp->currentBlock->next = createMemoryBlock(mp->defaultBlockSize);
		mp->currentBlock->next->mp_container = mp;
		mp->currentBlock->next->prev = mp->currentBlock;
		mp->currentBlock = mp->currentBlock->next;
	}

	// Add unit
	SMemoryUnitHeader* unit = reinterpret_cast<SMemoryUnitHeader*>(reinterpret_cast<char*>(mp->currentBlock) + sizeof(SMemoryBlockHeader) + mp->currentBlock->offset);
	unit->length = size;
	unit->container = mp->currentBlock;
	mp->currentBlock->offset += sizeof(SMemoryUnitHeader) + size;

	return reinterpret_cast<char*>(unit) + sizeof(SMemoryUnitHeader);
}

void* CPPShift::Memory::MemoryPoolManager::allocate_unsafe(MemoryPool* mp, size_t size)
{
	if (size + sizeof(SMemoryUnitHeader) < mp->currentBlock->blockSize - mp->currentBlock->offset);
	// Create new block if not enough space
	else if (size + sizeof(SMemoryUnitHeader) >= mp->defaultBlockSize) {
		mp->currentBlock->next = createMemoryBlock(size + sizeof(SMemoryUnitHeader));
		mp->currentBlock->next->mp_container = mp;
		mp->currentBlock = mp->currentBlock->next;
	}
	else {
		mp->currentBlock->next = createMemoryBlock(mp->defaultBlockSize);
		mp->currentBlock->next->mp_container = mp;
		mp->currentBlock = mp->currentBlock->next;
	}

	// Add unit
	SMemoryUnitHeader* unit = reinterpret_cast<SMemoryUnitHeader*>(reinterpret_cast<char*>(mp->currentBlock) + sizeof(SMemoryBlockHeader) + mp->currentBlock->offset);
	unit->length = size;
	unit->container = mp->currentBlock;
	mp->currentBlock->offset += sizeof(SMemoryUnitHeader) + size;

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
	MemoryPool* mp = block->mp_container;

	// If last in block, then reset offset
	if (reinterpret_cast<char*>(block) + sizeof(SMemoryBlockHeader) + block->offset == reinterpret_cast<char*>(unit) + sizeof(SMemoryUnitHeader) + unit->length) {
		block->offset -= sizeof(SMemoryUnitHeader) + unit->length;
		return;
	}
	
	// If block offset is 0 remove block if not the only one left
	if (mp->currentBlock != mp->firstBlock && block->offset == 0) {
		SMemoryBlockHeader* prev = block->prev;
		SMemoryBlockHeader* next = block->next;
		if (block == mp->currentBlock) mp->currentBlock = block->prev;
		prev->next = next;
		next->prev = prev;
	}
}


void CPPShift::Memory::MemoryPoolManager::dumpPoolData(MemoryPool* mp)
{
	if (mp == NULL) return;

	SMemoryBlockHeader* block = mp->firstBlock;
	SMemoryUnitHeader* unit;
	size_t current_unit_offset;

	size_t block_counter = 1;
	size_t unit_counter = 1;

	while (block != nullptr) {
		// Dump block data
		std::cout << "Block " << block_counter << ": " << std::endl;
		std::cout << "\t" << "Used: " << (float) (block->offset) / (float)(block->blockSize) * 100 << "% " << "(" << block->offset << "/" << block->blockSize << ")" << std::endl;

		if (block->offset == 0) {
			block = block->next;
			block_counter++;
			continue;
		}

		std::cout << "\t" << "Units: ========================" << std::endl;
		current_unit_offset = 0;
		unit_counter = 1;
		while (current_unit_offset < block->offset) {
			unit = reinterpret_cast<SMemoryUnitHeader*>(reinterpret_cast<char*>(block + 1) + current_unit_offset);
			std::cout << "\t\t" << "Unit " << unit_counter << ": " << unit->length + sizeof(SMemoryUnitHeader) << std::endl;
			current_unit_offset += sizeof(SMemoryUnitHeader) + unit->length;
			unit_counter++;
		}

		std::cout << "\t" << "===============================" << std::endl;

		block = block->next;
		block_counter++;
	}
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