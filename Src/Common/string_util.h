/** 
 * @file  string_util.h
 *
 * @brief Char classification routines declarations.
 */
#pragma once

int xisalnum(wint_t c);
int xisspecial(wint_t c);
int xisalpha(wint_t c);
int xisalnum(wint_t c);
int xisspace(wint_t c);

// Convert any negative inputs to negative char equivalents
// This is aimed at correcting any chars mistakenly 
// sign-extended to negative ints.
// This is ok for the UNICODE build because UCS-2LE code bytes
// do not extend as high as 2Gig (actually even full Unicode
// codepoints don't extend that high).
typedef TBYTE normch;

/** @brief Return nonzero if input is outside ASCII or is underline. */
inline int xisspecial(wint_t c)
{
	return normch(c) > (unsigned) _T('\x7f') || c == _T('_');
}

/**
 * @brief Return non-zero if input is alphabetic or "special" (see xisspecial).
 * Also converts any negative inputs to negative char equivalents (see normch).
 */
inline int xisalpha(wint_t c)
{
	return _istalpha(normch(c)) || xisspecial(normch(c));
}

/**
 * @brief Return non-zero if input is alphanumeric or "special" (see xisspecial).
 * Also converts any negative inputs to negative char equivalents (see normch).
 */
inline int xisalnum(wint_t c)
{
	return _istalnum(normch(c)) || xisspecial(normch(c));
}

/**
 * @brief Return non-zero if input is a hex digit.
 * Also converts any negative inputs to negative char equivalents (see normch).
 */
inline int xisxdigit(wint_t c)
{
	return _istxdigit(normch(c));
}

/**
 * @brief Return non-zero if input character is a space.
 * Also converts any negative inputs to negative char equivalents (see normch).
 */
inline int xisspace(wint_t c)
{
	return _istspace(normch(c));
}

template<int (*compare)(LPCTSTR, LPCTSTR, size_t)>
BOOL xiskeyword(LPCTSTR key, UINT len, LPCTSTR const *lower, LPCTSTR const *upper)
{
	while (lower < upper)
	{
		LPCTSTR const *match = lower + ((upper - lower) >> 1);
		int cmp = compare(*match, key, len);
		if (cmp == 0)
			cmp = static_cast<TBYTE>((*match)[len]);
		if (cmp >= 0)
			upper = match;
		if (cmp <= 0)
			lower = match + 1;
	}
	return static_cast<BOOL>(lower - upper);
}

template<int (*compare)(LPCTSTR, LPCTSTR, size_t), typename T, size_t N>
BOOL xiskeyword(LPCTSTR key, UINT len, T (&r)[N])
{
	return xiskeyword<compare>(key, len, std::begin(r), std::end(r));
}

namespace CommonKeywords
{
	BOOL IsNumeric(LPCTSTR pszChars, int nLength);
};

namespace HtmlKeywords
{
	BOOL IsHtmlKeyword(LPCTSTR pszChars, int nLength);
	BOOL IsUser1Keyword(LPCTSTR pszChars, int nLength);
	BOOL IsUser2Keyword(LPCTSTR pszChars, int nLength);
};
