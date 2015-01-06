// Copyright 2007-2010 Baptiste Lepilleur
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE
/**
 *  @file   forwards.h
 *  @date   Edited:  2015-01-06 Jochen Neubeck
 */
#pragma once

// Don't #include "config.h" but rather maintain settings here for simplicity
#define JSON_API
#define JSON_NO_INT64

#include <stdlib.h>

#ifndef stl
#include <Common/stl.h>
#endif

#include stl(vector)
#include stl(string)
#include stl(list)
#include stl(map)

#include <math.h>

// trivialized modf() implementation to avoid library overhead
static inline double modf(double d, double *integral_part) {
  return d - (*integral_part = double(__int64(d)));
}

namespace H2O {

class OException;

inline void __declspec(noreturn) ThrowJsonException(const char* msg) {
  wchar_t buf[1024];
  buf[mbstowcs(buf, msg, 1023)] = L'\0';
  throw reinterpret_cast<OException*>(buf);
}

} // namespace H2O

namespace Json {

using H2O::ThrowJsonException;

template<size_t size, typename type = char, typename alignment = int>
struct pod {
  alignment data[ size * sizeof(type) % sizeof(alignment) == 0 ?
                  size * sizeof(type) / sizeof(alignment) : -1 ];
  operator type* () { return reinterpret_cast<type*>(data); }
  static void swap(void* p, void* q) {
    std::swap(*static_cast<pod*>(p), *static_cast<pod*>(q));
  }
};

typedef int Int;
typedef int LargestInt;
typedef unsigned int UInt;
typedef unsigned int LargestUInt;
typedef unsigned int ArrayIndex;

// Defines a char buffer for use with uintToString().
typedef pod<3 * sizeof(LargestUInt) + 1, char, char> UIntToStringBuffer;

} // namespace Json
