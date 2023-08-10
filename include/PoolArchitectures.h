//
// Created by Sapir Shemer on 15/02/2023.
//

#ifndef APPSHIFTPOOL_POOLARCHITECTURES_H
#define APPSHIFTPOOL_POOLARCHITECTURES_H

enum class EPoolArchitecture {
    THREAD_SAFE_LOCK_FREE,
    THREAD_SAFE_LOCK_BASED,
    NON_THREAD_SAFE
};

namespace AppShift::Memory {
    template<size_t SIZE = 0, EPoolArchitecture TYPE = EPoolArchitecture::THREAD_SAFE_LOCK_FREE>
    class StackPool;

    template<size_t ITEM_SIZE = 0, size_t ITEM_COUNT = 0, EPoolArchitecture TYPE = EPoolArchitecture::THREAD_SAFE_LOCK_FREE>
    class SegregatedPool;

    template<class BASE_TYPE, size_t ITEM_COUNT = 128, EPoolArchitecture TYPE = EPoolArchitecture::THREAD_SAFE_LOCK_FREE>
    class ObjectPool;
}

#endif //APPSHIFTPOOL_POOLARCHITECTURES_H
