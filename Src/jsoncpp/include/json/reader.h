// Copyright 2007-2010 Baptiste Lepilleur
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE
/**
 *  @file   reader.h
 *  @date   Edited:  2015-01-07 Jochen Neubeck
 */
#pragma once

#include "features.h"
#include "value.h"

namespace Json {

/** \brief Unserialize a <a HREF="http://www.json.org">JSON</a> document into a
 *Value.
 *
 */
class JSON_API Reader {
public:
  typedef char Char;
  typedef const Char* Location;

  /** \brief Constructs a Reader allowing the specified feature set
   * for parsing.
   */
  Reader(FILE*, Features = Features::allFeatures);

  /** \brief Read a Value from a <a HREF="http://www.json.org">JSON</a>
   * document.
   * \param document UTF-8 encoded string containing the document to read.
   * \param root [out] Contains the root value of the document if it was
   *             successfully parsed.
   * \param collectComments \c true to collect comment and allow writing them
   * back during
   *                        serialization, \c false to discard comments.
   *                        This parameter is ignored if
   * Features::allowComments_
   *                        is \c false.
   * \return \c true if the document was successfully parsed, \c false if an
   * error occurred.
   */
  bool parse(const std::string& document, Value& root, bool collectComments = true);

  /** \brief Read a Value from a <a HREF="http://www.json.org">JSON</a>
   document.
   * \param beginDoc Pointer on the beginning of the UTF-8 encoded string of the
   document to read.
   * \param endDoc Pointer on the end of the UTF-8 encoded string of the
   document to read.
   \               Must be >= beginDoc.
   * \param root [out] Contains the root value of the document if it was
   *             successfully parsed.
   * \param collectComments \c true to collect comment and allow writing them
   back during
   *                        serialization, \c false to discard comments.
   *                        This parameter is ignored if
   Features::allowComments_
   *                        is \c false.
   * \return \c true if the document was successfully parsed, \c false if an
   error occurred.
   */
  bool parse(const char* beginDoc,
             const char* endDoc,
             Value& root,
             bool collectComments = true);

  /// \brief Parse from input stream.
  /// \return Whether root value was successfully parsed.
  bool parse(Value& root, bool collectComments = true);

  /** \brief Returns a user friendly string that list errors in the parsed
   * document.
   * \return Formatted error message with the list of errors with their location
   * in
   *         the parsed document. An empty string is returned if no error
   * occurred
   *         during parsing.
   */
  std::string getFormattedErrorMessages() const;

  /** \brief Return whether there are any errors.
   * \return \c true if there are no errors to report \c false if
   * errors have occurred.
   */
  bool good() const;

private:
  enum TokenType {
    tokenEndOfStream = 0,
    tokenObjectBegin,
    tokenObjectEnd,
    tokenArrayBegin,
    tokenArrayEnd,
    tokenString,
    tokenNumber,
    tokenTrue,
    tokenFalse,
    tokenNull,
    tokenArraySeparator,
    tokenMemberSeparator,
    tokenComment,
    tokenError
  };

  class Token {
  public:
    TokenType type_;
    Location start_;
    Location end_;
    int length() const { return static_cast<int>(end_ - start_); }
    std::string asString() const { return std::string(start_, end_); }
  } token_;

  class ErrorInfo {
  public:
    Token token_;
    std::string message_;
    Location extra_;
  };

  typedef std::list<ErrorInfo> Errors;

  bool readToken();
  TokenType match(const Char* pattern, TokenType type);
  TokenType readCStyleComment();
  TokenType readCppStyleComment();
  TokenType readString();
  TokenType readNumber();
  bool readValue(Value&);
  bool readObject(Value&);
  bool readArray(Value&);
  bool decodeNumber(Value&);
  bool decodeString(Value&);
  bool decodeString(std::string&);
  unsigned decodeUnicodeEscapeSequence(Location& current, Location end);
  void addError(const char* message, Location extra = 0);
  Char getNextChar();
  void getLocationLineAndColumn(Location location, int& line, int& column) const;
  std::string getLocationLineAndColumn(Location location) const;
  bool skipCommentTokens(std::string& queuedComments, Value* lastValue = 0);

  Errors errors_;
  FILE* const file_;
  std::string document_;
  Location begin_;
  Location end_;
  Location current_;
  Features features_;
  bool collectComments_;
};

} // namespace Json
