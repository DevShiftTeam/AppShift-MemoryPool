/**
 * Memory Pool v1.0.0
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

#include "String.h"

namespace CPPShift {
	String::String(Memory::MemoryPool& mp, const char* str)
	{
		this->mp = &mp;
		this->length = 0;
		this->start = nullptr;
		while (*(str + this->length) != '\0') this->length++;
		this->start = this->mp->allocate<char>(this->length);
		memcpy(this->start, str, this->length);
	}

	String::~String() { /*this->mp->remove(this->start);*/ }

	char* String::data() const { return this->start; }

	size_t String::size() const { return this->length; }

	String& String::operator=(const char* str)
	{
		this->mp->remove(this->start);
		this->length = 0;
		this->start = nullptr;
		while (str[this->length] != '\0') this->length++;
		this->start = this->mp->allocate<char>(this->length);
		memcpy(this->start, str, this->length);
		return *this;
	}

	String& String::operator=(const String& str)
	{
		this->mp->remove(this->start);
		this->length = str.size();
		this->start = this->mp->allocate<char>(this->length);
		memcpy(this->start, str.data(), this->length);
		return *this;
	}

	String& String::operator+=(const char* str)
	{
		int add_length = 0;
		while (str[add_length] != '\0') add_length++;
		this->start = this->mp->reallocate<char>(this->start, this->length + add_length);
		memcpy(this->start + this->length, str, add_length);
		this->length += add_length;
		return *this;
	}

	String& String::operator+=(const String& str)
	{
		this->start = this->mp->reallocate<char>(this->start, this->length + str.size());
		memcpy(this->start + this->length, str.data(), str.size());
		this->length += str.size();
		return *this;
	}

	std::ostream& operator<<(std::ostream& os, const String& str)
	{
		os << str.data();
		os.flush();
		return os;
	}
}