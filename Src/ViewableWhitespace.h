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
	static const TCHAR c_tab[]; /**< Visible character for tabs. */
	static const TCHAR c_space[]; /**< Visible character for spaces. */
	static const TCHAR c_cr[]; /**< Visible character for CR EOL chars. */
	static const TCHAR c_lf[]; /**< Visible character for LF EOL chars. */
	static const TCHAR c_eol[]; /**< Visible character for general EOL chars. */
};

inline const ViewableWhitespaceChars *GetViewableWhitespaceChars(int codepage) { return NULL; }

#endif // ViewableWhitespace_included_h
