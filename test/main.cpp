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

#include <iostream>
#include "../MemoryPool.h"
#include "String.h"
#include "STDString.h"
#include <time.h>

int main() {
    CPPShift::Memory::MemoryPool * mp = CPPShift::Memory::MemoryPoolManager::create();

    clock_t t;
    long double benchavg = 0;
    long double benchavg_new = 0;

    for (long long int j = 0; j < 100; j++) {
        t = clock();
        for (int i = 0; i < 1000000; i++) {
            CPPShift::String strs(mp, "The Big World Is Great And Shit"); // Allocation
            strs += "Some new stuff"; // Re-allocation
        } // Dellocation
        t = clock() - t;
        benchavg += (t / (j + 1)) - (benchavg / (j + 1));
    }

    std::cout << "CPPShift Library: " << (benchavg * 1000) / CLOCKS_PER_SEC << std::endl;

    for (long long int j = 0; j < 100; j++) {
        t = clock();
        for (int i = 0; i < 1000000; i++) {
            CPPShift::STDString strs("The Big World Is Great And Shit"); // Allocation
            strs += "Some new stuff"; // Re-allocation
        } // Dellocation
        t = clock() - t;
        benchavg_new += (t / (j + 1)) - (benchavg_new / (j + 1));
    }

    std::cout << "CPPShift Library with regular new/delete: " << (benchavg_new * 1000) / CLOCKS_PER_SEC << std::endl;
    std::cout << "MemoryPool is " << benchavg_new / benchavg << " Times faster than new/delete" << std::endl;

    benchavg = 0;
    for (long long int j = 0; j < 100; j++) {
        t = clock();
        for (int i = 0; i < 1000000; i++) {
            std::string strs("The Big World Is Great And Shit"); // Allocation
            strs += "Some new stuff"; // Rellocation
        } // Dellocation
        t = clock() - t;
        benchavg += (t / (j + 1)) - (benchavg / (j + 1));
    }

    std::cout << "Standard Library: " << (benchavg * 1000) / CLOCKS_PER_SEC << std::endl;
    return 0;
}
