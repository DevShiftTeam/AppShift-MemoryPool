/**
 * AppShift Memory Pool v2.0.0
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
 * @author Valasiadis Fotios
 */

#include <iostream>
#include "../../MemoryPoolAllocator.h"
#include "bits/stdc++.h"

using namespace AppShift::Memory;

template<class T, class AllocA, class AllocB>
inline bool operator!=(std::vector<T, AllocA> a, std::vector<T, AllocB> b) {
    return a.size() != b.size() || std::equal(a.begin(), a.end(), b.begin());
}

template<class A, class B>
inline void assert_equals(A a, B b, const std::string& message) {
    if(a != b) {
        std::cerr << message << '\n';
        std::exit(0);
    }
}

int main() {
    MemoryPoolAllocator<int> alloc(1024); // 1kb
    std::vector<int, MemoryPoolAllocator<int>> test_vector(alloc); // alloc is copied
    std::vector<int> vector;

    static_assert(std::is_same<MemoryPoolAllocator<int>::value_type, int>::value);
    static_assert(std::is_same<MemoryPoolAllocator<int>::pointer, int*>::value);
    static_assert(std::is_same<MemoryPoolAllocator<int>::reference, int&>::value);
    static_assert(std::is_same<MemoryPoolAllocator<int>::const_pointer, const int*>::value);
    static_assert(std::is_same<MemoryPoolAllocator<int>::value_type, int>::value);
    static_assert(!MemoryPoolAllocator<int>::propagate_on_container_copy_assignment::value);
    static_assert(MemoryPoolAllocator<int>::propagate_on_container_move_assignment::value);
    static_assert(MemoryPoolAllocator<int>::propagate_on_container_swap::value);

    int dummy;
    assert_equals(alloc.address(dummy), &dummy, "address test");
    assert_equals(alloc.max_size(), 256, "max_size test"); // 2^10 / 2^2 = 2^8 = 256

    // Triggers allocation && object constructions
    test_vector = {4, 6, 43, 92, 543 , 84321, 8543, 134, 95, 0};
    vector = {4, 6, 43, 92, 543 , 84321, 8543, 134, 95, 0};
    assert_equals(test_vector, vector, "Initializer list test");

    // Triggers object destructions
    test_vector.resize(5);
    vector.resize(5);
    assert_equals(test_vector, vector, "resize test");

    // Triggers reallocation
    test_vector.shrink_to_fit();
    vector.shrink_to_fit();
    assert_equals(test_vector, vector, "shrink to fit test");
    return 0;
}