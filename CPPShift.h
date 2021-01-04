#pragma once

#ifndef CPPSHIFT_32BIT_MODE
#include <cstddef>
#endif

namespace CPPShift {
#ifdef CPPSHIFT_32BIT_MODE
	typedef unsigned int SIZE;
#else
	typedef size_t SIZE;
#endif // CPPSHIFT_32BIT_MODE
}