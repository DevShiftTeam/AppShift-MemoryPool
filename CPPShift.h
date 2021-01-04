#pragma once

#ifndef CPPSHIFT_32BIT_MODE
#include <cstddef>
#endif

// Check windows
#if _WIN32 || _WIN64
#if !_WIN64
#define CPPSHIFT_32BIT_MODE
#endif
#endif

// Check GCC
#if __GNUC__
#if !(__x86_64__ && __ppc64__)
#define CPPSHIFT_32BIT_MODE
#endif
#endif

namespace CPPShift {
#ifdef CPPSHIFT_32BIT_MODE
	typedef unsigned int SIZE;
#else
	typedef size_t SIZE;
#endif // CPPSHIFT_32BIT_MODE
}