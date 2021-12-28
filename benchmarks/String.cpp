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

#include "String.h"

namespace AppShift {
	String::String(Memory::MemoryPool* mp, const char* str)
	{
		this->mp = mp;
		this->length = strlen(str);
		this->start = new (mp) char[this->length + 1];
		memcpy(this->start, str, this->length);
		this->start[this->length] = '\0';
	}

	String::~String() { this->mp->free(this->start); }

	char* String::data() const { return this->start; }

	size_t String::size() const { return this->length; }

	String& String::operator=(const char* str)
	{
		this->mp->free(this->start);
		this->length = strlen(str);
		this->start = new (this->mp) char[this->length + 1];
		memcpy(this->start, str, this->length);
		this->start[this->length] = '\0';
		return *this;
	}

	String& String::operator=(const String& str)
	{
		this->mp->free(this->start);
		this->length = str.size();
		this->start = new (this->mp) char[this->length + 1];
		memcpy(this->start, str.data(), this->length);
		this->start[this->length] = '\0';
		return *this;
	}

	String& String::operator+=(const char* str)
	{
		int add_length = strlen(str);
		this->start = (char*) this->mp->reallocate(this->start, this->length + add_length + 1);
		memcpy(this->start + this->length, str, add_length);
		this->length += add_length;
		this->start[this->length] = '\0';
		return *this;
	}

	String& String::operator+=(const String& str)
	{
		this->start = (char*) this->mp->reallocate(this->start, this->length + str.size() + 1);
		memcpy(this->start + this->length, str.data(), str.size());
		this->length += str.size();
		this->start[this->length] = '\0';
		return *this;
	}

	std::ostream& operator<<(std::ostream& os, const String& str)
	{
		os << str.data();
		os.flush();
		return os;
	}
}