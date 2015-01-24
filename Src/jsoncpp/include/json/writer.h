// Copyright 2007-2010 Baptiste Lepilleur
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE
/**
 *  @file   writer.cpp
 *  @date   Edited:  2015-01-06 Jochen Neubeck
 */
#pragma once

#include "value.h"

namespace Json {

class Value;

/** \brief Writes a Value in <a HREF="http://www.json.org">JSON</a> format in a
 human friendly way,
     to a stream rather than to a string.
 *
 * The rules for line break and indent are as follow:
 * - Object value:
 *     - if empty then print {} without indent and line break
 *     - if not empty the print '{', line break & indent, print one value per
 line
 *       and then unindent and line break and print '}'.
 * - Array value:
 *     - if empty then print [] without indent and line break
 *     - if the array contains no object value, empty array or some other value
 types,
 *       and all the values fit on one lines, then print the array on a single
 line.
 *     - otherwise, it the values do not fit on one line, or the array contains
 *       object or non empty array, then print one value per line.
 *
 * If the Value have comments then they are outputed according to their
 #CommentPlacement.
 *
 * \param indentation Each level will be indented by this amount extra.
 * \sa Reader, Value, Value::setComment()
 */
class JSON_API Writer {
public:
  Writer(FILE* , const char* indentation = "\t");

public:
  /** \brief Serialize a Value in <a HREF="http://www.json.org">JSON</a> format.
   * \param out Stream to write to. (Can be ostringstream, e.g.)
   * \param root Value to serialize.
   * \note There is no point in deriving from Writer, since write() should not
   * return a value.
   */
  void write(const Value& root);

private:
  void write(const char*);
  void writeValue(const Value&);
  void writeArrayValue(const Value&);
  void writeObjectValue(const Value&);
  void writeIndent();
  void indent();
  void unindent();
  void writeComment(const char*);
  void writeCommentBefore(const Value&);
  void writeCommentAfter(const Value&);

  FILE* const file_;
  std::string indentString_;
  std::string indentation_;
};

#if defined(JSON_HAS_INT64)
const char* JSON_API valueToString(Int value, char* buffer = UIntToStringBuffer());
const char* JSON_API valueToString(UInt value, char* buffer = UIntToStringBuffer());
#endif // if defined(JSON_HAS_INT64)
const char* JSON_API valueToString(LargestInt value, char* buffer = UIntToStringBuffer());
const char* JSON_API valueToString(LargestUInt value, char* buffer = UIntToStringBuffer());
const char* JSON_API valueToString(double value, char* buffer = pod<32, char>());
const char* JSON_API valueToString(bool value);
std::string JSON_API valueToQuotedString(const char* value);

} // namespace Json
