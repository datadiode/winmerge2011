/** 
 * @file  ViewableWhitespace.h
 *
 * @brief Repository of character tables used to display whitespace (when View/Whitespace enabled)
 */
// RCS ID line follows -- this is updated by CVS
// $Id$

#ifndef ViewableWhitespace_included_h
#define ViewableWhitespace_included_h

/**
 * @brief Structure containing characters for viewable whitespace chars.
 *
 * These characters are used when user wants to see whitespace characters
 * in editor.
 */
struct ViewableWhitespaceChars
{
	// U+BB: RIGHT POINTING DOUBLE ANGLE QUOTATION MARK
	// U+B7: MIDDLE DOT
	// U+A7: SECTION SIGN
	// U+B6: PILCROW SIGN
	// U+A4: CURRENCY SIGN
	static const TCHAR c_tab	= L'\x00BB'; /**< Visible character for tabs. */
	static const TCHAR c_space	= L'\x00B7'; /**< Visible character for spaces. */
	static const TCHAR c_cr		= L'\x00A7'; /**< Visible character for CR EOL chars. */
	static const TCHAR c_lf		= L'\x00B6'; /**< Visible character for LF EOL chars. */
	static const TCHAR c_eol	= L'\x00A4'; /**< Visible character for general EOL chars. */
};

#endif // ViewableWhitespace_included_h
