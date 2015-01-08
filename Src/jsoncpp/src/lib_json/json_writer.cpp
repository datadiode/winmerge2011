// Copyright 2011 Baptiste Lepilleur
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE
/**
 *  @file   json_writer.cpp
 *  @date   Edited:  2015-01-06 Jochen Neubeck
 */
#include <json/writer.h>
#include "json_tool.h"

#include <utility>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#if defined(_MSC_VER)
#include <float.h>
#define isfinite _finite
#define snprintf _snprintf
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1400 // VC++ 8.0
// Disable warning about strdup being deprecated.
#pragma warning(disable : 4996)
#endif

#if defined(__sun) && defined(__SVR4) //Solaris
#include <ieeefp.h>
#define isfinite finite
#endif

namespace Json {

static bool containsControlCharacter(const char* str) {
  while (*str) {
    if (isControlCharacter(*(str++)))
      return true;
  }
  return false;
}

const char* valueToString(LargestInt value, char* buffer) {
  char* current = buffer + sizeof(UIntToStringBuffer);
  bool isNegative = value < 0;
  if (isNegative)
    value = -value;
  current = uintToString(LargestUInt(value), current);
  if (isNegative)
    *--current = '-';
  assert(current >= buffer);
  return current;
}

const char* valueToString(LargestUInt value, char* buffer) {
  char* current = buffer + sizeof(UIntToStringBuffer);
  current = uintToString(value, current);
  assert(current >= buffer);
  return current;
}

#if defined(JSON_HAS_INT64)

const char* valueToString(Int value, char* buffer) {
  return valueToString(LargestInt(value), buffer);
}

const char* valueToString(UInt value, char* buffer) {
  return valueToString(LargestUInt(value), buffer);
}

#endif // # if defined(JSON_HAS_INT64)

const char* valueToString(double value, char* buffer) {
  // Print into the buffer. We need not request the alternative representation
  // that always has a decimal point because JSON doesn't distingish the
  // concepts of reals and integers.
  const char *ret = buffer;
  if (isfinite(value)) {
    int len = sprintf(buffer, "%.17g", value);
    assert(len >= 0);
    // assume "C" locale for now
    // fixNumericLocale(buffer, buffer + len);
  } else {
    // IEEE standard states that NaN values will not compare to themselves
    ret = value != value ? "null" : &"-1e+9999"[value >= 0];
  }
  return ret;
}

const char* valueToString(bool value) { return value ? "true" : "false"; }

std::string valueToQuotedString(const char* value) {
  // We have to walk value and escape any special characters.
  // Appending to std::string is not efficient, but this should be rare.
  // (Note: forward slashes are *not* rare, but I am not escaping them.)
  std::string::size_type maxsize = static_cast
    <std::string::size_type>(strlen(value) * 2 + 3); // allescaped+quotes+NULL
  std::string result;
  result.reserve(maxsize); // to avoid lots of mallocs
  result.push_back('"');
  while (char c = *value++) {
    switch (c) {
    case '"':
    case '\\':
      break;
    case '\b':
      c = 'b';
      break;
    case '\f':
      c = 'f';
      break;
    case '\n':
      c = 'n';
      break;
    case '\r':
      c = 'r';
      break;
    case '\t':
      c = 't';
      break;
    // case '/':
    // Even though \/ is considered a legal escape in JSON, a bare
    // slash is also legal, so I see no reason to escape it.
    // (I hope I am not misunderstanding something.
    // blep notes: actually escaping \/ may be useful in javascript to avoid </
    // sequence.
    // Should add a flag to allow this compatibility mode and prevent this
    // sequence from occurring.
    default:
      if (isControlCharacter(c)) {
        char buffer[8];
        sprintf(buffer, "\\u04x", c);
        result += buffer;
      } else {
        result.push_back(c);
      }
      continue;
    }
    result.push_back('\\');
    result.push_back(c);
  }
  result.push_back('"');
  return result;
}

// Class Writer
// //////////////////////////////////////////////////////////////////
Writer::Writer(FILE* file, const char* indentation)
  : file_(file)
  , indentation_(indentation) {}

void Writer::write(const char* text) { fputs(text, file_); }

void Writer::write(const Value& root) {
  indentString_.resize(0);
  write("\xEF\xBB\xBF");
  writeCommentBefore(root);
  writeValue(root);
  writeCommentAfter(root);
  writeIndent();
}

void Writer::writeValue(const Value& value) {
  switch (value.type()) {
  case nullValue:
    write("null");
    break;
  case intValue:
    write(valueToString(value.asLargestInt()));
    break;
  case uintValue:
    write(valueToString(value.asLargestUInt()));
    break;
  case realValue:
    write(valueToString(value.asDouble()));
    break;
  case stringValue:
    write(valueToQuotedString(value.asCString()).c_str());
    break;
  case booleanValue:
    write(valueToString(value.asBool()));
    break;
  case arrayValue:
    writeArrayValue(value);
    break;
  case objectValue:
    writeObjectValue(value);
    break;
  }
}

void Writer::writeArrayValue(const Value& value) {
  if (unsigned size = value.size()) {
    writeWithIndent("[");
    indent();
    unsigned index = 0;
    for (;;) {
      const Value &childValue = value[index];
      writeCommentBefore(childValue);
      writeIndent();
      writeValue(childValue);
      if (++index == size) {
        writeCommentAfter(childValue);
        break;
      }
      write(",");
      writeCommentAfter(childValue);
    }
    unindent();
    writeWithIndent("]");
  } else {
    write("[]");
  }
}

void Writer::writeObjectValue(const Value& value) {
  const Value::ObjectValues *const values = value.getObjectValues();
  Value::ObjectValues::const_iterator it = values->begin();
  if (it != values->end()) {
    writeWithIndent("{");
    indent();
    for (;;) {
      const Value &childValue = it->second;
      writeCommentBefore(childValue);
      writeWithIndent(valueToQuotedString(it->first.c_str()).c_str());
      write(" : ");
      writeValue(childValue);
      if (++it == values->end()) {
        writeCommentAfter(childValue);
        break;
      }
      write(",");
      writeCommentAfter(childValue);
    }
    unindent();
    writeWithIndent("}");
  } else {
    write("{}");
  }
}

void Writer::writeIndent() {
  if (indentString_.empty())
    indentString_ = "\n";
  else
    write(indentString_.c_str());
}

void Writer::writeWithIndent(const char* text) {
  writeIndent();
  write(text);
}

void Writer::indent() { indentString_ += indentation_; }

void Writer::unindent() {
  assert(indentString_.size() >= indentation_.size());
  indentString_.resize(indentString_.size() - indentation_.size());
}

void Writer::writeComment(const char* q) {
  while (const char *p = strchr(q, '/')) {
    q = p;
    bool block = false;
    switch (char c = *++q) {
    case '*':
      block = true;
      do {
      case '/':
        do {
          c = *++q;
        } while (c != '\0' && c != '\r' && c != '\n');
      } while (block && (q[-1] != '/' || q[-2] != '*'));
    }
    writeWithIndent(std::string(p, q).c_str());
  }
}

void Writer::writeCommentBefore(const Value& value) {
  writeComment(value.getComment(commentBefore));
}

void Writer::writeCommentAfter(const Value& value) {
  if (value.hasComment(commentAfterOnSameLine)) {
    write(" ");
    write(value.getComment(commentAfterOnSameLine));
  }
  writeComment(value.getComment(commentAfter));
}

} // namespace Json
