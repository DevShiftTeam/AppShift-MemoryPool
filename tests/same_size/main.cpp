#include <iostream>
#include <chrono>
#include "LinkedList.h"
#include "../../include/execution/ThreadPool.h"
#include "../include/tabulate/table.hpp"

using namespace AppShift::Execution;

auto stdTest(size_t iteration_count, int* randoms_to_remove) {
    auto start = std::chrono::steady_clock::now();
    for (size_t i = 0; i < iteration_count; i++) {
        auto *my_item = createLinkedList<LinkedItemTest>(1000);
        my_item = removeIndices(my_item, randoms_to_remove, 100);
        auto *new_list = createLinkedList<LinkedItemTest>(10);
        removeLinkedList(my_item);
        removeLinkedList(new_list);
    }
    auto end = std::chrono::steady_clock::now();

    return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

template<class POOL>
auto testPool(POOL& pool, size_t iteration_count, int* randoms_to_remove) {
    auto start = std::chrono::steady_clock::now();
    for (size_t i = 0; i < iteration_count; i++) {
        auto *my_item = createLinkedListInPool<LinkedItemTest>(pool, 1000);
        my_item = removeIndicesInPool(pool, my_item, randoms_to_remove, 100);
        auto *new_list = createLinkedListInPool<LinkedItemTest>(pool, 10);
        removeLinkedListInPool(pool, my_item);
        removeLinkedListInPool(pool, new_list);
    }
    auto end = std::chrono::steady_clock::now();

    return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

auto stdTestMultiThread(size_t iteration_count, int* randoms_to_remove) {
    auto start = std::chrono::steady_clock::now();
    {
        ThreadPool event_loop = ThreadPool();
        for (size_t i = 0; i < iteration_count; i++) {
            event_loop.addEvent([randoms_to_remove]() {
                auto *my_item = createLinkedList<LinkedItemTest>(1000);
                my_item = removeIndices(my_item, randoms_to_remove, 100);
                auto *new_list = createLinkedList<LinkedItemTest>(10);
                removeLinkedList(my_item);
                removeLinkedList(new_list);
            });
        }
    }
    auto end = std::chrono::steady_clock::now();

    return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

template<class POOL>
auto testPoolMultiThread(POOL& pool, size_t iteration_count, int* randoms_to_remove) {
    auto start = std::chrono::steady_clock::now();
    {
        ThreadPool event_loop = ThreadPool();
        for (size_t i = 0; i < iteration_count; i++) {
            event_loop.addEvent([randoms_to_remove, &pool]() {
                auto *my_item = createLinkedListInPool<LinkedItemTest>(pool, 1000);
                my_item = removeIndicesInPool(pool, my_item, randoms_to_remove, 100);
                auto *new_list = createLinkedListInPool<LinkedItemTest>(pool, 10);
                removeLinkedListInPool(pool, my_item);
                removeLinkedListInPool(pool, new_list);
            });
        }
    }
    auto end = std::chrono::steady_clock::now();

    return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

int main() {
    // Pools to test
    auto seg_pool = AppShift::Memory::ObjectPool<LinkedItemTest>();
    auto stack_pool = AppShift::Memory::StackPool<1024 * 40>();
    auto stack_lb_pool = AppShift::Memory::StackPool<1024 * 40, EPoolArchitecture::THREAD_SAFE_LOCK_BASED>();

    // Test variables
    size_t iteration_count = 30000;

    // Generate random indices
    int randoms_to_remove[100];
    generateRandoms(randoms_to_remove);
    std::sort(std::begin(randoms_to_remove), std::end(randoms_to_remove));

    tabulate::Table results_single_thread;
    tabulate::Table results_multi_thread;
    results_single_thread.add_row({"Test Name", "Time", "Relative Performance"});
    results_multi_thread.add_row({"Test Name", "Time", "Relative Performance"});
    results_single_thread.format().multi_byte_characters(true);
    results_multi_thread.format().multi_byte_characters(true);

    long long std_time = stdTest(iteration_count, randoms_to_remove);

    results_single_thread.add_row({"STD", std::to_string(std_time) + " μs", "100%"});
    // Stack Pool:
    long long stack_pool_time = testPool(stack_pool, iteration_count, randoms_to_remove);
    results_single_thread.add_row({
        "Stack Pool",
        std::to_string(stack_pool_time) + " μs",
        std::to_string(static_cast<double>(std_time) / static_cast<double>(stack_pool_time) * 100) + "%"});

    // Segregated Pool:
    long long seg_pool_time = testPool(seg_pool, iteration_count, randoms_to_remove);
    results_single_thread.add_row({
        "Segregated Pool",
        std::to_string(seg_pool_time) + " μs",
        std::to_string(static_cast<double>(std_time) / static_cast<double>(seg_pool_time) * 100) + "%"});

    // Stack Lock Based Pool:
    long long stack_lb_pool_time = testPool(stack_lb_pool, iteration_count, randoms_to_remove);
    results_single_thread.add_row({
        "Stack Lock Based Pool",
        std::to_string(stack_lb_pool_time) + " μs",
        std::to_string(static_cast<double>(std_time) / static_cast<double>(stack_lb_pool_time) * 100) + "%"});

    // Multi Threaded:
    long long std_time_multi_thread = stdTestMultiThread(iteration_count, randoms_to_remove);
    results_multi_thread.add_row({
        "STD MT",
        std::to_string(std_time_multi_thread) + " μs",
        std::to_string(static_cast<double>(std_time_multi_thread) / static_cast<double>(std_time_multi_thread) * 100) + "%"});

    // Stack Pool Multi Threaded:
    long long stack_pool_time_multi_thread = testPoolMultiThread(stack_pool, iteration_count, randoms_to_remove);
    results_multi_thread.add_row({
        "Stack Pool MT",
        std::to_string(stack_pool_time_multi_thread) + " μs",
        std::to_string(static_cast<double>(std_time_multi_thread) / static_cast<double>(stack_pool_time_multi_thread) * 100) + "%"});

    // Segregated Pool Multi Threaded:
    long long seg_pool_time_multi_thread = testPoolMultiThread(seg_pool, iteration_count, randoms_to_remove);
    results_multi_thread.add_row({
        "Segregated Pool MT",
        std::to_string(seg_pool_time_multi_thread) + " μs",
        std::to_string(static_cast<double>(std_time_multi_thread) / static_cast<double>(seg_pool_time_multi_thread) * 100) + "%"});

    // Stack Lock Based Pool Multi Threaded:
    long long stack_lb_pool_time_multi_thread = testPoolMultiThread(stack_lb_pool, iteration_count, randoms_to_remove);
    results_multi_thread.add_row({
        "Stack Lock Based Pool MT",
        std::to_string(stack_lb_pool_time_multi_thread) + " μs",
        std::to_string(static_cast<double>(std_time_multi_thread) / static_cast<double>(stack_lb_pool_time_multi_thread) * 100) + "%"});

    // Align center all cells
    results_single_thread.format()
            .font_align(tabulate::FontAlign::center)
            .width(20);
    results_multi_thread.format()
            .font_align(tabulate::FontAlign::center)
            .width(20);

    // center-align and color header cells
    results_single_thread.row(0).format()
            .font_color(tabulate::Color::yellow)
            .font_style({tabulate::FontStyle::bold});
    results_multi_thread.row(0).format()
            .font_color(tabulate::Color::yellow)
            .font_style({tabulate::FontStyle::bold});

    std::cout << "Single Threaded Results:" << std::endl;
    std::cout << results_single_thread << std::endl;
    std::cout << "Multi Threaded Results:" << std::endl;
    std::cout << results_multi_thread << std::endl;
    return 0;
}
