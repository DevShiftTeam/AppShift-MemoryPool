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
#include "MemoryPool.h"
#include "String.h"
#include <time.h>

int main() {
    CPPShift::Memory::MemoryPool * mp = CPPShift::Memory::MemoryPoolManager::create();

    clock_t t;
    long double stdavg = 0;
    long double memavg = 0;

    for (long long int j = 0; j < 100; j++) {
        t = clock();
        for (int i = 0; i < 1000000; i++) {
            CPPShift::String strs(mp, "The Big World Is Great And Shit"); // Allocation
            strs += "Some new stuff"; // Re-allocation
        } // Dellocation
        t = clock() - t;
        memavg += (t / (j + 1)) - (memavg / (j + 1));
    }

    std::cout << "CPPShift Library: " << memavg << std::endl;

    for (long long int j = 0; j < 100; j++) {
        t = clock();
        for (int i = 0; i < 1000000; i++) {
            std::string strs("The Big World Is Great And Shit"); // Allocation
            strs += "Some new stuff"; // Rellocation
        } // Dellocation
        t = clock() - t;
        stdavg += (t / (j + 1)) - (stdavg / (j + 1));
    }

    std::cout << "Standard Library: " << stdavg << std::endl;
    return 0;
}
