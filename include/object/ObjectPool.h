//
// Created by Sapir Shemer on 24/07/2023.
//

#ifndef APPSHIFTPOOL_OBJECTPOOL_H
#define APPSHIFTPOOL_OBJECTPOOL_H

#include "../PoolArchitectures.h"
#include "../segregated/SegregatedNTSPool.h"
#include "../segregated/SegregatedTSLPool.h"
#include "../segregated/SegregatedTSLFPool.h"

namespace AppShift::Memory{
    template<class BASE_TYPE, size_t ITEM_COUNT, EPoolArchitecture TYPE>
    class ObjectPool {
    public:
        /**
         * Allocate a new item from the pool.
         * @return Pointer to the allocated item.
         */
        BASE_TYPE* allocate() { return reinterpret_cast<BASE_TYPE*>(pool.allocate()); }

        /**
         * Free an item from the pool.
         * @param item Pointer to the item to free.
         */
        void free(BASE_TYPE* item) { pool.free(item); }

        /**
         * Construct a new object pool.
         */
        ObjectPool() : pool() {}
    private:
        SegregatedPool<sizeof(BASE_TYPE), ITEM_COUNT, TYPE> pool;
    };
}

// Override new operators
template<class BASE_TYPE, size_t ITEM_COUNT, EPoolArchitecture TYPE>
void* operator new(size_t size, AppShift::Memory::ObjectPool<BASE_TYPE, ITEM_COUNT, TYPE>& pool) { return pool.allocate(); }
template<class BASE_TYPE, size_t ITEM_COUNT, EPoolArchitecture TYPE>
void* operator new[](size_t size, AppShift::Memory::ObjectPool<BASE_TYPE, ITEM_COUNT, TYPE>& pool) { return pool.allocate(); }
#endif //APPSHIFTPOOL_OBJECTPOOL_H
