// Copyright 2007-2010 Baptiste Lepilleur
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE
/**
 *  @file   value.h
 *  @date   Edited:  2015-01-06 Jochen Neubeck
 */
#pragma once

#include "forwards.h"

/** \brief JSON (JavaScript Object Notation).
 */
namespace Json {

/** \brief Type of the value held by a Value object.
 */
enum ValueType {
  nullValue = 0, ///< 'null' value
  intValue,      ///< signed integer value
  uintValue,     ///< unsigned integer value
  realValue,     ///< double value
  stringValue,   ///< UTF-8 string value
  booleanValue,  ///< bool value
  arrayValue,    ///< array value (ordered list)
  objectValue    ///< object value (collection of name/value pairs).
};

enum CommentPlacement {
  commentBefore = 0,      ///< a comment placed on the line before a value
  commentAfterOnSameLine, ///< a comment just after a value on the same line
  commentAfter, ///< a comment on the line after a value (only make sense for
  /// root value)
  numberOfCommentPlacement
};

/** \brief Represents a <a HREF="http://www.json.org">JSON</a> value.
 *
 * This class is a discriminated union wrapper that can represents a:
 * - signed integer [range: Value::minInt - Value::maxInt]
 * - unsigned integer (range: 0 - Value::maxUInt)
 * - double
 * - UTF-8 string
 * - boolean
 * - 'null'
 * - an ordered list of Value
 * - collection of name/value pairs (javascript object)
 *
 * The type of the held value is represented by a #ValueType and
 * can be obtained using type().
 *
 * values of an #objectValue or #arrayValue can be accessed using operator[]()
 *methods.
 * Non const methods will automatically create the a #nullValue element
 * if it does not exist.
 * The sequence of an #arrayValue will be automatically resize and initialized
 * with #nullValue. resize() can be used to enlarge or truncate an #arrayValue.
 *
 * The get() methods can be used to obtanis default value in the case the
 *required element
 * does not exist.
 *
 * It is possible to iterate over the list of a #objectValue values using
 * the getObjectValues() method.
 */
class JSON_API Value {
public:
  typedef Json::UInt UInt;
  typedef Json::Int Int;
#if defined(JSON_HAS_INT64)
  typedef Json::UInt64 UInt64;
  typedef Json::Int64 Int64;
#endif // defined(JSON_HAS_INT64)
  typedef Json::LargestInt LargestInt;
  typedef Json::LargestUInt LargestUInt;
  typedef Json::ArrayIndex ArrayIndex;

  static const Value& null;
  /// Minimum signed integer value that can be stored in a Json::Value.
  static const LargestInt minLargestInt;
  /// Maximum signed integer value that can be stored in a Json::Value.
  static const LargestInt maxLargestInt;
  /// Maximum unsigned integer value that can be stored in a Json::Value.
  static const LargestUInt maxLargestUInt;

  /// Minimum signed int value that can be stored in a Json::Value.
  static const Int minInt;
  /// Maximum signed int value that can be stored in a Json::Value.
  static const Int maxInt;
  /// Maximum unsigned int value that can be stored in a Json::Value.
  static const UInt maxUInt;

#if defined(JSON_HAS_INT64)
  /// Minimum signed 64 bits int value that can be stored in a Json::Value.
  static const Int64 minInt64;
  /// Maximum signed 64 bits int value that can be stored in a Json::Value.
  static const Int64 maxInt64;
  /// Maximum unsigned 64 bits int value that can be stored in a Json::Value.
  static const UInt64 maxUInt64;
#endif // defined(JSON_HAS_INT64)

private:
  class CZString {
  public:
    enum DuplicationPolicy {
      noDuplication = 0,
      duplicate,
      duplicateOnCopy
    };
    CZString(ArrayIndex index);
    CZString(const char* cstr, DuplicationPolicy allocate);
    CZString(const CZString& other);
    ~CZString();
    CZString& operator=(CZString other);
    bool operator<(const CZString& other) const;
    bool operator==(const CZString& other) const;
    ArrayIndex index() const;
    const char* c_str() const;
    bool isStaticString() const;

  private:
    void swap(CZString& other);
    const char* cstr_;
    ArrayIndex index_;
  };

public:
  typedef std::map<CZString, Value> ObjectValues;

public:
  /** \brief Create a default Value of the given type.

    This is a very useful constructor.
    To create an empty array, pass arrayValue.
    To create an empty object, pass objectValue.
    Another Value can then be set to this one by assignment.
This is useful since clear() and resize() will not alter types.

    Examples:
\code
Json::Value null_value; // null
Json::Value arr_value(Json::arrayValue); // []
Json::Value obj_value(Json::objectValue); // {}
\endcode
  */
  Value(ValueType type = nullValue);
  Value(Int value);
  Value(UInt value);
#if defined(JSON_HAS_INT64)
  Value(Int64 value);
  Value(UInt64 value);
#endif // if defined(JSON_HAS_INT64)
  Value(double value);
  Value(const char* value);
  Value(const char* beginValue, const char* endValue);
  Value(const std::string& value);
  Value(bool value);
  /// Deep copy.
  Value(const Value& other);
  ~Value();

  // Deep copy, then swap(other).
  Value& operator=(Value other);
  /// Swap everything.
  void swap(Value& other);
  /// Swap values but leave comments and source offsets in place.
  void swapPayload(Value& other);

  ValueType type() const;

  /// Compare payload only, not comments etc.
  bool operator<(const Value& other) const;
  bool operator<=(const Value& other) const;
  bool operator>=(const Value& other) const;
  bool operator>(const Value& other) const;
  bool operator==(const Value& other) const;
  bool operator!=(const Value& other) const;
  int compare(const Value& other) const;

  const char* asCString() const;
  std::string asString() const;
  Int asInt() const;
  UInt asUInt() const;
#if defined(JSON_HAS_INT64)
  Int64 asInt64() const;
  UInt64 asUInt64() const;
#endif // if defined(JSON_HAS_INT64)
  LargestInt asLargestInt() const;
  LargestUInt asLargestUInt() const;
  float asFloat() const;
  double asDouble() const;
  bool asBool() const;

  bool isNull() const;
  bool isBool() const;
  bool isInt() const;
  bool isInt64() const;
  bool isUInt() const;
  bool isUInt64() const;
  bool isIntegral() const;
  bool isDouble() const;
  bool isNumeric() const;
  bool isString() const;
  bool isArray() const;
  bool isObject() const;

  bool isConvertibleTo(ValueType other) const;

  /// Number of values in array or object
  ArrayIndex size() const;

  /// \brief Return true if empty array, empty object, or null;
  /// otherwise, false.
  bool empty() const;

  /// Return isNull()
  bool operator!() const;

  /// Remove all object members and array elements.
  /// \pre type() is arrayValue, objectValue, or nullValue
  /// \post type() is unchanged
  void clear();

  /// Resize the array to size elements.
  /// New elements are initialized to null.
  /// May only be called on nullValue or arrayValue.
  /// \pre type() is arrayValue or nullValue
  /// \post type() is arrayValue
  void resize(ArrayIndex size);

  /// Access an array element (zero based index ).
  /// If the array contains less than index element, then null value are
  /// inserted
  /// in the array so that its size is index+1.
  /// (You may need to say 'value[0u]' to get your compiler to distinguish
  ///  this from the operator[] which takes a string.)
  Value& operator[](ArrayIndex index);
  /// Access an array element (zero based index )
  /// (You may need to say 'value[0u]' to get your compiler to distinguish
  ///  this from the operator[] which takes a string.)
  const Value& operator[](ArrayIndex index) const;
  /// If the array contains at least index+1 elements, returns the element
  /// value,
  /// otherwise returns defaultValue.
  Value get(ArrayIndex index, const Value& defaultValue) const;
  /// Return true if index < size().
  bool isValidIndex(ArrayIndex index) const;
  /// \brief Append value to array at the end.
  ///
  /// Equivalent to jsonvalue[jsonvalue.size()] = value;
  Value& append(const Value& value);

  /// Access an object value by name, create a null member if it does not exist.
  Value& operator[](const char* key);
  /// Access an object value by name, returns null if there is no member with
  /// that name.
  const Value& operator[](const char* key) const;
  /// Access an object value by name, create a null member if it does not exist.
  Value& operator[](const std::string& key);
  /// Access an object value by name, returns null if there is no member with
  /// that name.
  const Value& operator[](const std::string& key) const;
  /// Return the member named key if it exist, defaultValue otherwise.
  Value get(const char* key, const Value& defaultValue) const;
  /// Return the member named key if it exist, defaultValue otherwise.
  Value get(const std::string& key, const Value& defaultValue) const;
  /** \brief Remove the named map member.

      Update 'removed' iff removed.
      \return true iff removed (no exceptions)
  */
  bool removeMember(const char* key, Value* removed);
  /** \brief Remove the indexed array element.

      O(n) expensive operations.
      Update 'removed' iff removed.
      \return true iff removed (no exceptions)
  */
  bool removeIndex(ArrayIndex i, Value* removed);

  /// Return true if the object has a member named key.
  bool isMember(const char* key) const;

  /// Comments must be //... or /* ... */
  void setComment(const char* comment, CommentPlacement placement);
  bool hasComment(CommentPlacement placement) const;
  /// Include delimiters and embedded newlines.
  const char* getComment(CommentPlacement placement) const;

  ObjectValues* getObjectValues();
  const ObjectValues* getObjectValues() const;

private:
  void initBasic(ValueType type);
  struct CommentInfo {
    CommentInfo();
    ~CommentInfo();

    void setComment(const char* text);

    char* comment_;
  };

  union ValueHolder {
    LargestInt int_;
    LargestUInt uint_;
    double real_;
    bool bool_;
    char* string_;
    ObjectValues* map_;
  } value_;
  ValueType type_ : 8;
  CommentInfo* comments_;
};

} // namespace Json

/// Specialize std::swap() for Json::Value. 
template<>
inline void eastl::swap(Json::Value& a, Json::Value& b) { a.swap(b); }
