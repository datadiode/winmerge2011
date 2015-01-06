// Copyright 2007-2010 Baptiste Lepilleur
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE
/**
 *  @file   assertions.h
 *  @date   Edited:  2015-01-06 Jochen Neubeck
 */
#pragma once

#define JSON_ASSERT(condition)                                                 \
  assert(condition); // @todo <= change this into an exception throw
#define JSON_FAIL_MESSAGE(message) { ThrowJsonException(message); }

#define JSON_ASSERT_MESSAGE(condition, message)                                \
  if (!(condition)) {                                                          \
    JSON_FAIL_MESSAGE(message)                                                 \
  }
