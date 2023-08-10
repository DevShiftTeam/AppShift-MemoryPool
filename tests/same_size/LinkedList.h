//
// Created by Sapir Shemer on 30/01/2023.
//

#ifndef APPSHIFTPOOL_LINKEDLIST_H
#define APPSHIFTPOOL_LINKEDLIST_H


#include <random>
#include <algorithm>
#include "../../include/MemoryPool.h"

// Generate a hundred randoms indices
void generateRandoms(int* arr) {
    // Seed with a real random value, if available
    std::random_device r;
    std::mt19937 mt(r());
    std::uniform_int_distribution<int> uniform_dist(0, 999);

    for(int i = 0; i < 100; i++) {
        arr[i] = uniform_dist(mt);
    }
}

struct LinkedItemTest {
    LinkedItemTest* previous;
    LinkedItemTest* next;

    size_t number;
};

template<class NODE_TYPE>
NODE_TYPE* createLinkedList(size_t size) {
    if(size == 0) return nullptr;

    auto* current = new NODE_TYPE;
    size_t count = 1;
    current->number = count;
    current->previous = nullptr;
    current->next = nullptr;

    while(count < size) {
        count++;

        current->next = new NODE_TYPE;
        current->next->previous = current;
        current = current->next;
        current->next = nullptr;
        current->number = count;
    }

    return current;
}

template<class NODE_TYPE>
LinkedItemTest* removeIndices(NODE_TYPE* item, const int* arr, int size) {
    auto* current_head = item;
    int current = 0;
    for(int i = 0; i < size; i++) {
        // Skip the same indicies
        while(arr[i] == arr[i + 1]) i++;

        // Find item
        int jumps = arr[i] - current;
        while(jumps > 0) {
            item = item->previous;
            jumps--;
        }

        // If the current head changes, then save the new head
        if(current_head == item) current_head = item->previous;

        // Remove from chain
        auto* temp = item;
        if(item->next != nullptr) item->next->previous = item->previous;
        if(item->previous != nullptr) item->previous->next = item->next;
        item = item->previous;
        delete temp;
        current = arr[i] + 1;
    }

    return current_head;
}

template<class NODE_TYPE>
void removeLinkedList(NODE_TYPE* item) {
    if(item == nullptr) return;

    // Get last
    while(item->next != nullptr) item = item->next;

    // Start removing
    while(item != nullptr) {
        auto* temp = item->previous;
        delete item;
        item = temp;
    }
}



template<class NODE_TYPE, class POOL>
NODE_TYPE* createLinkedListInPool(POOL& pool, size_t size) {
    if(size == 0) return nullptr;

    auto* current = new (pool) NODE_TYPE;
    size_t count = 1;
    current->number = count;
    current->previous = nullptr;
    current->next = nullptr;

    while(count < size) {
        count++;

        current->next = new (pool) NODE_TYPE;
        current->next->previous = current;
        current = current->next;
        current->next = nullptr;
        current->number = count;
    }

    return current;
}

template<class NODE_TYPE, class POOL>
LinkedItemTest* removeIndicesInPool(POOL& pool, NODE_TYPE* item, const int* arr, int size) {
    auto* current_head = item;
    int current = 0;
    for(int i = 0; i < size; i++) {
        // Skip the same indicies
        while(arr[i] == arr[i + 1]) i++;

        // Find item
        int jumps = arr[i] - current;
        while(jumps > 0) {
            item = item->previous;
            jumps--;
        }

        // If the current head changes, then save the new head
        if(current_head == item) current_head = item->previous;

        // Remove from chain
        auto* temp = item;
        if(item->next != nullptr) item->next->previous = item->previous;
        if(item->previous != nullptr) item->previous->next = item->next;
        item = item->previous;
        pool.free(temp);
        current = arr[i] + 1;
    }

    return current_head;
}

template<class NODE_TYPE, class POOL>
void removeLinkedListInPool(POOL& pool, NODE_TYPE* item) {
    if(item == nullptr) return;

    // Get last
    while(item->next != nullptr) item = item->next;

    // Start removing
    while(item != nullptr) {
        auto* temp = item->previous;
        pool.free(item);
        item = temp;
    }
}
#endif //APPSHIFTPOOL_LINKEDLIST_H
