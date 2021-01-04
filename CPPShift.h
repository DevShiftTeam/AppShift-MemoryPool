#pragma once

/*
 * Architecture independent bit size check
 * Source: https://stackoverflow.com/a/32717129/3761616
 */
#include <cstdint>
#if INTPTR_MAX == INT64_MAX
// 64-bit
#elif INTPTR_MAX == INT32_MAX
// 32-bit
#define CPPSHIFT_32BIT_MODE
#else
#error Unknown pointer size or missing size macros!
#endif

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