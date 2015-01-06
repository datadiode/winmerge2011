// Copyright 2007-2010 Baptiste Lepilleur
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE
/**
 *  @file   json_tool.h
 *  @date   Edited:  2015-01-06 Jochen Neubeck
 */
#pragma once

/* This header provides common string manipulation support, such as UTF-8,
 * portable conversion from/to string...
 *
 * It is an internal header that must not be exposed.
 */

namespace Json {

/// Converts a unicode code-point to UTF-8.
static inline const char* codePointToUTF8(unsigned int cp, char* result = pod<5, char, char>()) {

  // based on description from http://en.wikipedia.org/wiki/UTF-8

  if (cp <= 0x7f) {
    result[1] = '\0';
    result[0] = static_cast<char>(cp);
  } else if (cp <= 0x7FF) {
    result[2] = '\0';
    result[1] = static_cast<char>(0x80 | (0x3f & cp));
    result[0] = static_cast<char>(0xC0 | (0x1f & (cp >> 6)));
  } else if (cp <= 0xFFFF) {
    result[3] = '\0';
    result[2] = static_cast<char>(0x80 | (0x3f & cp));
    result[1] = 0x80 | static_cast<char>((0x3f & (cp >> 6)));
    result[0] = 0xE0 | static_cast<char>((0xf & (cp >> 12)));
  } else if (cp <= 0x10FFFF) {
    result[4] = '\0';
    result[3] = static_cast<char>(0x80 | (0x3f & cp));
    result[2] = static_cast<char>(0x80 | (0x3f & (cp >> 6)));
    result[1] = static_cast<char>(0x80 | (0x3f & (cp >> 12)));
    result[0] = static_cast<char>(0xF0 | (0x7 & (cp >> 18)));
  } else {
    result[0] = '\0';
  }

  return result;
}

/// Returns true if ch is a control character (in range [0,32[).
static inline bool isControlCharacter(char ch) { return ch > 0 && ch <= 0x1F; }

/** Converts an unsigned integer to string.
 * @param value Unsigned interger to convert to string
 * @param current Input/Output string buffer.
 *        Must have at least uintToStringBufferSize chars free.
 */
static inline char* uintToString(LargestUInt value, char* current) {
  *--current = 0;
  do {
    *--current = char(value % 10) + '0';
    value /= 10;
  } while (value != 0);
  return current;
}

/** Change ',' to '.' everywhere in buffer.
 *
 * We had a sophisticated way, but it did not work in WinCE.
 * @see https://github.com/open-source-parsers/jsoncpp/pull/9
 */
static inline void fixNumericLocale(char* begin, char* end) {
  while (begin < end) {
    if (*begin == ',') {
      *begin = '.';
    }
    ++begin;
  }
}

template
<
	typename type,
	type c0 = 0, type c1 = 0, type c2 = 0, type c3 = 0, type c4 = 0,
	type c5 = 0, type c6 = 0, type c7 = 0, type c8 = 0, type c9 = 0
>
class setof
{
public:
	static bool contains(type c)
	{
		return c0 && c == c0
			|| c1 && c == c1
			|| c2 && c == c2
			|| c3 && c == c3
			|| c4 && c == c4
			|| c5 && c == c5
			|| c6 && c == c6
			|| c7 && c == c7
			|| c8 && c == c8
			|| c9 && c == c9;
	}
};

} // namespace Json {
