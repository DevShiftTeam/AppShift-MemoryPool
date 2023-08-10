//
// Created by Sapir Shemer on 26/07/2023.
//

#ifndef APPSHIFTPOOL_STACKTSLFPOOL_H
#define APPSHIFTPOOL_STATICTSLFPOOL_H

#include "../../include/stack/common.h"

#include <cstdlib>
#include <memory>

void* AppShift::Memory::allocateFromOffset_unsafe(AppShift::Memory::SStackPoolBlockHeader *block, size_t size) {
    /// @note Unsafe since it ignores if the new allocated memory overflows the block
    auto* new_item = reinterpret_cast<SStackPoolItemHeader*>(
            reinterpret_cast<char*>(block)
            + sizeof(SStackPoolBlockHeader)
            + block->offset
    );

    block->offset += size + sizeof(SStackPoolItemHeader);
    block->number_of_allocated++;
    new_item->size = size;
    new_item->container = block;
    return reinterpret_cast<char*>(new_item) + sizeof(SStackPoolItemHeader);
}

void* AppShift::Memory::allocateFromDeleted_unsafe(AppShift::Memory::SStackPoolBlockHeader *block, size_t size) {
    SStackPoolDeletedHeader *current_deleted_check = nullptr;

    do {
        // First fit algorithm
        current_deleted_check = block->last_deleted_item;
        // Keeps the next deleted unit for fixing the deleted objects chain late
        auto *next_deleted_unit = current_deleted_check;
        while (current_deleted_check != nullptr && current_deleted_check->item_data.size < size) {
            next_deleted_unit = current_deleted_check;
            current_deleted_check = current_deleted_check->previous;
        }

        // Remove deleted item from the deleted items list
        if (current_deleted_check != nullptr) {
            if (current_deleted_check == next_deleted_unit)
                block->last_deleted_item = block->last_deleted_item->previous;
            else next_deleted_unit->previous = current_deleted_check->previous;

            block->number_of_allocated++;
            break;
        }

        // Move to next block
        block = block->previous;
    } while(block != nullptr);

    /// @note Unsafe because it can return a nullptr.
    /// Simply interpreting the deleted header as a unit header is enough since the structure
    /// is the same and contains the same data (item_data is SStackPoolItemHeader).
    return reinterpret_cast<char*>(current_deleted_check);
}

void AppShift::Memory::removeBlock_unsafe(AppShift::Memory::SStackPoolBlockHeader* &current_last, AppShift::Memory::SStackPoolBlockHeader *block) {
    // Use sorounding blocks
    auto* previous_block = block->previous;
    auto* next_block = block->next;

    // If there is more than one block, then modify the block list
    /// @note Equal only when both are nullptr
    if(previous_block != next_block) {
        if(next_block != nullptr) next_block->previous = previous_block;
        else current_last = previous_block; // If no next block then we are at the last
        if(previous_block != nullptr) {
            previous_block->next = next_block;
            std::free(block); // Delete only if not first block
        }
    } else {
        block->offset = 0;
        block->last_deleted_item = nullptr;
    }
}

void AppShift::Memory::addNewBlock(AppShift::Memory::SStackPoolBlockHeader *&current_last, size_t size) {
    // Create new block
    auto* new_block = reinterpret_cast<SStackPoolBlockHeader*>(
            std::malloc(size + sizeof(SStackPoolBlockHeader))
    );

    // Connect previous block to this block
    current_last->next = new_block;
    new_block->previous = current_last;
    new_block->next = nullptr;
    current_last = new_block;

    // Initialize block
    current_last->last_deleted_item = nullptr;
    current_last->offset = 0;
    current_last->number_of_allocated = 0;
    current_last->size = size;
}

#endif