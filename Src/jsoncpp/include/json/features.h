// Copyright 2007-2010 Baptiste Lepilleur
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE
/**
 *  @file   features.h
 *  @date   Edited:  2015-01-06 Jochen Neubeck
 */
#pragma once

#include "forwards.h"

namespace Json {

/** \brief Configuration passed to reader and writer.
 * This configuration object can be used to force the Reader or Writer
 * to behave in a standard conforming way.
 */
enum Features {
  /// indicates whether comments are allowed.
  allowComments                 = 1 << 0,
  /// indicates whether root must be either an array or an object value.
  strictRoot                    = 1 << 1,
  /// indicates whether dropped null placeholders are allowed.
  allowDroppedNullPlaceholders  = 1 << 2,
  /// \c true if numeric object key are allowed. Default: \c false.
  allowNumericKeys              = 1 << 3,
  /** \brief A configuration that allows all features and assumes all strings
   * are UTF-8.
   * - C & C++ comments are allowed
   * - Root object can be any JSON value
   * - Assumes Value strings are encoded in UTF-8
   */
  allFeatures = allowComments | allowDroppedNullPlaceholders | allowNumericKeys,
  /** \brief A configuration that is strictly compatible with the JSON
   * specification.
   * - Comments are forbidden.
   * - Root object must be either an array or an object value.
   * - Assumes Value strings are encoded in UTF-8
   */
  strictModeFeatures = 0
};

} // namespace Json
