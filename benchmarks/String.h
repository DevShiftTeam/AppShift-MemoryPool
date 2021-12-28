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
 * @author Sapir Shemer
 */

#pragma once

#include "../MemoryPool.h"
#include <ostream>
#include <cstring>

namespace AppShift {
	class String {
	public:
		String(Memory::MemoryPool* mp, const char * str = "");
		~String();

		char* data() const;
		size_t size() const;

		String& operator=(const char* str);
		friend std::ostream& operator<<(std::ostream& os, const String& dt);
		String& operator=(const String& str);
		String& operator+=(const char * str);
		String& operator+=(const String& str);

	private:
		char* start;
		size_t length;
		Memory::MemoryPool * mp;
	};
}