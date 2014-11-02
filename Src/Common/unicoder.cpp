/**
 *  @file   unicoder.cpp
 *  @author Perry Rapp, Creator, 2003-2006
 *  @date   Created: 2003-10
 *  @date   Edited:  2013-10-03 (Jochen Neubeck)
 *
 *  @brief  Implementation of utility unicode conversion routines
 */

/* The MIT License
Copyright (c) 2003 Perry Rapp
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "StdAfx.h"
#include "unicoder.h"
#include "codepage.h"
#include "Utf8FileDetect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// This is not in older platform sdk versions
#ifndef WC_NO_BEST_FIT_CHARS
#define WC_NO_BEST_FIT_CHARS        0x00000400
#endif

namespace ucr
{

/**
 * @brief convert 8-bit character input to Unicode codepoint and return it
 */
unsigned int byteToUnicode(unsigned char ch, unsigned int codepage)
{
	if (ch < 0x80)
		return ch;
	wchar_t wbuff;
	int n = MultiByteToWideChar(codepage, 0, (const char*) & ch, 1, &wbuff, 1);
	if (n > 0)
		return wbuff;
	else
		return '?';
}

/**
 * @brief Extract character from pointer, handling UCS-2 codesets
 *  This does not handle MBCS or UTF-8 codepages correctly!
 *  Client should not use this except for Unicode or SBCS codepages.
 */
unsigned int get_unicode_char(unsigned char * ptr, UNICODESET codeset, int codepage)
{
	unsigned int ch;
	switch (codeset)
	{
	case UCS2LE:
		ch = *((WORD *)ptr);
		break;
	case UCS2BE:
		ch = (ptr[0] << 8) + ptr[1];
		break;
	default:
		// TODO: How do we recognize valid codepage ?
		// if not, use byteToUnicode(*ptr)
		ch = byteToUnicode(*ptr, codepage);
		break;
	}
	return ch;
}

/**
 * @brief Convert series of bytes (8-bit chars) to TCHARs.
 *
 * @param [out] str String returned.
 * @param [in] lpd Original byte array to convert.
 * @param [in] len Length of the original byte array.
 * @param [in] codepage Codepage used.
 * @param [out] lossy Was conversion lossy?
 * @return true if conversion succeeds, false otherwise.
 * @todo This doesn't inform the caller whether translation was lossy
 *  In fact, this doesn't even know. Probably going to have to make
 *  two passes, the first with MB_ERR_INVALID_CHARS. Ugh. :(
 */
bool maketstring(String &str, const char *lpd, int len, int codepage, bool *lossy)
{
	if (!len)
	{
		str.clear();
		return true;
	}

	// 0 is a valid value (CP_ACP)!
	if (codepage == -1)
		codepage = getDefaultCodepage();

	// Convert input to Unicode, using specified codepage
	// TCHAR is wchar_t, so convert into String (str)
	DWORD flags = MB_ERR_INVALID_CHARS;
	int wlen = len * 2 + 6;

	str.resize(wlen);

	LPWSTR wbuff = &str.front();
	do
	{
		if (int n = MultiByteToWideChar(codepage, flags, lpd, len, wbuff, wlen - 1))
		{
			/*
			NB: MultiByteToWideChar is documented as only zero-terminating
			if input was zero-terminated, but it appears that it can
			zero-terminate even if input wasn't.
			So we check if it zero-terminated and adjust count accordingly.
			*/
			//>2007-01-11 jtuc: We must preserve an embedded zero even if it is
			// the last input character. As we don't expect MultiByteToWideChar to
			// add a zero that does not originate from the input string, it is a
			// good idea to ASSERT that the assumption holds.
			if (wbuff[n - 1] == 0 && lpd[len - 1] != 0)
			{
				ASSERT(FALSE);
				--n;
			}
			str.resize(n);
			return true;
		}
		*lossy = true;
		flags ^= MB_ERR_INVALID_CHARS;
	} while (flags == 0 && GetLastError() == ERROR_NO_UNICODE_TRANSLATION);
	str = _T('?');
	return true;
}

/**
 * @brief Buffer constructor.
 * The constructor creates buffer with given size.
 * @param [in] initialSize Buffer's size.
 */
buffer::buffer(unsigned int initialSize)
{
	size = 0;
	capacity = initialSize;
	ptr = capacity ? (unsigned char *)calloc(capacity, 1) : NULL;
}

/**
 * @brief Buffer destructor.
 * Frees the reserved buffer.
 */
buffer::~buffer()
{
	free(ptr);
}

/**
 * @brief Resize the buffer.
 * @param [in] newSize New size of the buffer.
 */
void buffer::resize(unsigned int newSize)
{
	if (capacity < newSize)
	{
		capacity = newSize;
		unsigned char *tmp = static_cast<unsigned char *>(realloc(ptr, capacity));
		if (tmp == NULL)
			OException::Throw(ERROR_OUTOFMEMORY);
		ptr = tmp;
	}
}

/**
 * @brief Convert from one text encoding to another; return false if any lossing conversions
 */
void convert(UNICODESET unicoding1, int codepage1, const unsigned char * src, int srcbytes, UNICODESET unicoding2, int codepage2, buffer * dest, BOOL *loss)
{
	if (unicoding1 == unicoding2 && (unicoding1 || EqualCodepages(codepage1, codepage2)))
	{
		// simple byte copy
		dest->resize(srcbytes);
		CopyMemory(dest->ptr, src, srcbytes);
		dest->size = srcbytes;
	}
	else if ((unicoding1 == UCS2LE && unicoding2 == UCS2BE)
			|| (unicoding1 == UCS2BE && unicoding2 == UCS2LE))
	{
		// simple byte swap
		dest->resize(srcbytes);
		for (int i = 0; i < srcbytes; i += 2)
		{
			// Byte-swap into destination
			dest->ptr[i] = src[i+1];
			dest->ptr[i+1] = src[i];
		}
		dest->size = srcbytes;
	}
	else if (unicoding1 == UCS2LE)
	{
		// From UCS-2LE to 8-bit (or UTF-8)

		// WideCharToMultiByte: lpDefaultChar & lpUsedDefaultChar must be NULL when using UTF-8

		int destcp = (unicoding2 == UTF8 ? CP_UTF8 : codepage2);
		DWORD flags = 0;
		int bytes = WideCharToMultiByte(destcp, flags, (const wchar_t*)src, srcbytes / 2, 0, 0, NULL, NULL);
		dest->resize(bytes);
		bytes = WideCharToMultiByte(destcp, flags,
			(const wchar_t*)src, srcbytes / 2,
			(char *)dest->ptr, dest->capacity,
			NULL, destcp != CP_UTF8 ? loss : NULL);
		dest->size = bytes;
	}
	else if (unicoding2 == UCS2LE)
	{
		// From 8-bit (or UTF-8) to UCS-2LE
		int srccp = (unicoding1 == UTF8 ? CP_UTF8 : codepage1);
		DWORD flags = 0;
		int wchars = MultiByteToWideChar(srccp, flags, (const char*)src, srcbytes, 0, 0);
		dest->resize(wchars*2);
		wchars = MultiByteToWideChar(srccp, flags, (const char*)src, srcbytes, (LPWSTR)dest->ptr, dest->capacity / 2);
		dest->size = wchars * 2;
	}
	else
	{
		// Break problem into two simpler pieces by converting through UCS-2LE
		buffer intermed(dest->capacity);
		convert(unicoding1, codepage1, src, srcbytes, UCS2LE, 0, &intermed, loss);
		convert(UCS2LE, 0, intermed.ptr, intermed.size, unicoding2, codepage2, dest, loss);
	}
}

} // namespace ucr

inline int CoincidenceOf(int mask) { return mask & mask - 1; }

/**
 * @brief Determine encoding from byte buffer.
 * @param [in] pBuffer Pointer to the begin of the buffer.
 * @param [in] size Size of the buffer.
 * @param [out] pBom Returns true if buffer had BOM bytes, false otherwise.
 * @return One of UNICODESET values as encoding.
 * EF BB BF UTF-8
 * FF FE UTF-16, little endian
 * FE FF UTF-16, big endian
 * FF FE 00 00 UTF-32, little endian
 * 00 00 FE FF UTF-32, big-endian
 */
UNICODESET DetermineEncoding(unsigned char *pBuffer, size_t size, unsigned *pBom)
{
	if (size == 0)
		return NONE;
	// initialize to a pattern that differs everywhere from all possible unicode signatures
	unsigned long sig = 0x3F3F3F3F;
	// copy at most 4 bytes from buffer
	memcpy(&sig, pBuffer, min(size, sizeof sig));
	// check for the two possible 4 bytes signatures
	*pBom = sizeof sig;
	if (sig == 0x0000FEFF)
		return UCS4LE;
	if (sig == 0xFFFE0000)
		return UCS4BE;
	// check for the only possible 3 bytes signature
	reinterpret_cast<BYTE *>(&sig)[--*pBom] = 0x00;
	if (sig == 0xBFBBEF)
		return UTF8;
	// check for UCS2 the two possible 2 bytes signatures
	int bufSize = static_cast<int>(min<size_t>(size, 8 * 1024));
	if (wine_version &&
		wmemchr(reinterpret_cast<wchar_t *>(pBuffer), L'\0', bufSize / 2))
	{
		return NONE;
	}
	if (memchr(pBuffer, 0, bufSize))
	{
		int icheck = IS_TEXT_UNICODE_NOT_UNICODE_MASK ^
			IS_TEXT_UNICODE_UNICODE_MASK ^ IS_TEXT_UNICODE_CONTROLS ^
			IS_TEXT_UNICODE_REVERSE_MASK ^ IS_TEXT_UNICODE_REVERSE_CONTROLS;
		if (IsTextUnicode(pBuffer, bufSize, &icheck)
		||	(
				icheck &
				(
					IS_TEXT_UNICODE_UNICODE_MASK ^
					IS_TEXT_UNICODE_REVERSE_MASK ^
					IS_TEXT_UNICODE_SIGNATURE ^
					IS_TEXT_UNICODE_REVERSE_SIGNATURE
				)
			) > (icheck & IS_TEXT_UNICODE_NOT_UNICODE_MASK)
		||	CoincidenceOf
			(
				icheck &
				(
					IS_TEXT_UNICODE_SIGNATURE |
					IS_TEXT_UNICODE_STATISTICS
				)
			)
		||	CoincidenceOf
			(
				icheck &
				(
					IS_TEXT_UNICODE_REVERSE_SIGNATURE |
					IS_TEXT_UNICODE_REVERSE_STATISTICS
				)
			))
		{
			*pBom = icheck &
			(
				IS_TEXT_UNICODE_SIGNATURE | IS_TEXT_UNICODE_REVERSE_SIGNATURE
			) ? 2 : 0;
			return (icheck & 0xF) >= ((icheck >> 4) & 0xF) ? UCS2LE : UCS2BE;
		}
		return NONE;
	}
	*pBom = 0;
	return CheckForInvalidUtf8(pBuffer, bufSize) ? NONE : UTF8;
}

/**
 * @brief Change any special codepage constants into real codepage numbers
 */
static int NormalizeCodepage(int cp)
{
	if (cp == CP_THREAD_ACP) // should only happen on Win2000+
	{
		TCHAR buff[32];
		if (GetLocaleInfo(GetThreadLocale(), LOCALE_IDEFAULTANSICODEPAGE, buff, sizeof(buff) / sizeof(buff[0])))
			cp = _ttol(buff);
		else
			// a valid codepage is better than no codepage
			cp = GetACP();
	}
	if (cp == CP_ACP) cp = GetACP();
	if (cp == CP_OEMCP) cp = GetOEMCP();
	return cp;
}

/**
 * @brief Compare two codepages for equality
 */
BOOL EqualCodepages(int cp1, int cp2)
{
	return (cp1 == cp2)
			|| (NormalizeCodepage(cp1) == NormalizeCodepage(cp2));
}
