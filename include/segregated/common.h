//
// Created by Sapir Shemer on 26/01/2023.
//

#ifndef APPSHIFTPOOL_SEGREGATED_COMMON_H
#define APPSHIFTPOOL_SEGREGATED_COMMON_H

#include <memory>
#include <cstdlib>

namespace AppShift::Memory {
    /**
     * Structure representing a deleted item in a segregated block/pool
     *
     * @author Sapir Shemer
     */
    struct SSegregatedDeletedItem {
        SSegregatedDeletedItem* previous = nullptr;
    };

    /**
     * A header for the block of segregated memory storage used by the pool
     *
     * @note The size of the block and size of a segregated item is handled by the segregated pool class
     *
     * @author Sapir Shemer
     */
    struct SSegregatedBlockHeader {
        // Next & previous blocks
        SSegregatedBlockHeader* previous = nullptr;
        SSegregatedBlockHeader* next = nullptr;

        // Current offset of allocations
        size_t offset = 0;

#ifdef MP_DEBUG_MODE
        // Analysis helpers
        size_t number_of_allocated = 0;
#endif
    };

    template<size_t SIZE>
    struct SSegregatedStaticData {
        SSegregatedBlockHeader header = {};
        char data[SIZE] = {};
    };
    /**
     * Interface of a segregated pool containing all the data
     */
    class ISegregatedPool {
    public:
        virtual void* allocate() = 0;
        virtual void free(void* item) = 0;

    protected:
        SSegregatedDeletedItem* last_deleted_item = nullptr;
        size_t block_size{};
        size_t item_size{};
    };
}

#endif //APPSHIFTPOOL_SEGREGATED_COMMON_H
