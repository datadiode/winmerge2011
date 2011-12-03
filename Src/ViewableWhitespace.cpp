/** 
 * @file  ViewableWhitespace.cpp
 *
 * @brief Repository of character tables used to display whitespace (when View/Whitespace enabled)
 */
// RCS ID line follows -- this is updated by CVS
// $Id$

#include "StdAfx.h" 
#include "ViewableWhitespace.h"

/** @brief Structure for whitespace characters. */

// U+BB: RIGHT POINTING DOUBLE ANGLE QUOTATION MARK
// U+B7: MIDDLE DOT
// U+A7: SECTION SIGN
// U+B6: PILCROW SIGN
// U+A4: CURRENCY SIGN

const TCHAR ViewableWhitespaceChars::c_tab[] = L"\x00BB"; /**< Visible character for tabs. */
const TCHAR ViewableWhitespaceChars::c_space[] = L"\x00B7"; /**< Visible character for spaces. */
const TCHAR ViewableWhitespaceChars::c_cr[] = L"\x00A7"; /**< Visible character for CR EOL chars. */
const TCHAR ViewableWhitespaceChars::c_lf[] = L"\x00B6"; /**< Visible character for LF EOL chars. */
const TCHAR ViewableWhitespaceChars::c_eol[] = L"\x00A4"; /**< Visible character for general EOL chars. */
