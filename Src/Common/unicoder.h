/**
 *  @file   unicoder.h
 *  @author Perry Rapp, Creator, 2003-2004
 *  @date   Created: 2003-10
 *  @date   Edited:  2006-02-20 (Perry Rapp)
 *
 *  @brief  Declaration of utility unicode conversion routines
 */
#pragma once

/** @brief Known Unicode encodings. */
typedef enum
{
	NONE,      /**< No unicode. */
	UCS2LE,    /**< UCS-2 / UTF-16 little endian. */
	UCS2BE,    /**< UCS-2 / UTF-16 big endian. */
	UTF8,      /**< UTF-8. */
	UCS4LE,    /**< UTF-32 little endian */
	UCS4BE,    /**< UTF-32 big-endian */
} UNICODESET;

#ifdef __cplusplus

namespace ucr
{
	/**
	 * @brief A simple buffer struct.
	 */
	struct buffer
	{
		unsigned char * ptr; /**< Pointer to a buffer. */
		unsigned int capacity; /**< Buffer's size in bytes. */
		unsigned int size; /**< Size of the data in the buffer, <= capacity. */

		buffer(unsigned int initialSize);
		~buffer();
		void resize(unsigned int newSize);
	};

	unsigned int get_unicode_char(unsigned char * ptr, UNICODESET unicoding, int codepage = 0);
	void maketstring(String &str, const char *lpd, int len, int codepage, bool *lossy);
	unsigned int byteToUnicode(unsigned char ch, unsigned int codepage);

	// generic function to do all conversions
	void convert(UNICODESET unicoding1, int codepage1, const unsigned char * src, int srcbytes, UNICODESET unicoding2, int codepage2, buffer * dest, BOOL *loss = NULL);

} // namespace ucr

#endif

EXTERN_C UNICODESET DetermineEncoding(unsigned char *pBuffer, size_t size, unsigned *pBom);
EXTERN_C int EqualCodepages(int cp1, int cp2);
