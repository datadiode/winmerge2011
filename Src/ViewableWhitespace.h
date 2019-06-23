/**
 * @file  ViewableWhitespace.h
 *
 * @brief Repository of character tables used to display whitespace (when View/Whitespace enabled)
 */
#pragma once

/**
 * @brief Structure containing characters for viewable whitespace chars.
 *
 * These characters are used when user wants to see whitespace characters
 * in editor.
 */
struct ViewableWhitespaceChars
{
	// U+00BB: RIGHT POINTING DOUBLE ANGLE QUOTATION MARK
	// U+00B7: MIDDLE DOT
	// U+00AC: NOT SIGN
	// U+2193: DOWN ARROW
	static const TCHAR c_tab	= L'\x00BB'; /**< Visible character for tabs. */
	static const TCHAR c_space	= L'\x00B7'; /**< Visible character for spaces. */
	static const TCHAR c_cr		= L'\x00AC'; /**< Visible character for CR EOL chars. */
	static const TCHAR c_lf		= L'\x2193'; /**< Visible character for LF EOL chars. */
	static const TCHAR c_eol	= L'\x00AC'; /**< Visible character for general EOL chars. */
};
