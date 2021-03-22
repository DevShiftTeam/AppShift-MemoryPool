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

#include "STDString.h"

namespace CPPShift {
	STDString::STDString(const char* str)
	{
		this->length = strlen(str);
		this->start = new char[this->length];
		memcpy(this->start, str, this->length);
	}

	STDString::~STDString() { delete this->start; }

	char* STDString::data() const { return this->start; }

	size_t STDString::size() const { return this->length; }

	STDString& STDString::operator=(const char* str)
	{
		delete this->start;
		this->length = strlen(str);
		this->start = new char[this->length];
		memcpy(this->start, str, this->length);
		return *this;
	}

	STDString& STDString::operator=(const STDString& str)
	{
		delete this->start;
		this->length = str.size();
		this->start = new char[this->length];
		memcpy(this->start, str.data(), this->length);
		return *this;
	}

	STDString& STDString::operator+=(const char* str)
	{
		int add_length = strlen(str);
		char* str_holder = new char[this->length + add_length];
		memcpy(str_holder, this->start, this->length);
		memcpy(str_holder + this->length, str, add_length);
		delete this->start;
		this->start = str_holder;
		this->length += add_length;
		return *this;
	}

	STDString& STDString::operator+=(const STDString& str)
	{
		char* prev = this->start;
		this->start = (char*) realloc(this->start, this->length + str.size());
		if (start == NULL) {
			this->start = prev;
			return *this;
		}
		memcpy(this->start + this->length, str.data(), str.size());
		this->length += str.size();
		return *this;
	}

	std::ostream& operator<<(std::ostream& os, const STDString& str)
	{
		os << str.data();
		os.flush();
		return os;
	}
}