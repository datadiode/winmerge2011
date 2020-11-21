/**
 * @file  ByteCompare.cpp
 *
 * @brief Implementation file for ByteCompare
 */
#include "StdAfx.h"
#include <io.h>
#include "FileLocation.h"
#include "CompareOptions.h"
#include "DiffContext.h"
#include "ByteCompare.h"

using namespace CompareEngines;

/** @brief Quick contents compare's file buffer size. */
static const size_t CMPBUFF = 0x040000;
static const size_t PADDING = 0x000008;

/**
 * @brief Default constructor.
 */
ByteCompare::ByteCompare(const CDiffContext *pCtxt)
	: DIFFOPTIONS(pCtxt->m_options), m_pCtxt(pCtxt)
{
	m_osfhandle[0] = NULL;
	m_osfhandle[1] = NULL;
}

/**
 * @brief Set filedata.
 * @param [in] items Count of filedata items to set.
 * @param [in] data File data.
 * @note Path names are set by SetPaths() function.
 */
void ByteCompare::SetFileData(int items, file_data *data)
{
	// We support only two files currently!
	ASSERT(items == 2);
	m_osfhandle[0] = reinterpret_cast<HANDLE>(_get_osfhandle(data[0].desc));
	m_osfhandle[1] = reinterpret_cast<HANDLE>(_get_osfhandle(data[1].desc));
	m_st_size[0] = data[0].stat.st_size;
	m_st_size[1] = data[1].stat.st_size;
}

enum ByteCompare::BLANKNESS_TYPE
{
	EOF_0 = 0,
	EOF_1 = 1,
	NEITHER = 2,
	LEADBYTE = 4,
	BLANK = 8,
	CR = 16,
	LF = 32,
	CRLF = 64,
	START = 128
};

inline unsigned has_multiple_bits(unsigned mask)
{
	return mask & mask - 1;
}

template<>
BOOL ByteCompare::IsDBCSLeadByteEx<BYTE>(int codepage, BYTE byte)
{
	return ::IsDBCSLeadByteEx(codepage, byte);
}

template<class CodePoint, int CodeShift, int x, int j>
unsigned ByteCompare::CompareFiles(FileLocation *location)
{
	char buff[2][CMPBUFF + PADDING];
	stl_size_t bytes_ahead[2] = { 0, 0 };
	stl_size_t read_ahead_size[2] = { CMPBUFF, CMPBUFF };
	stl_size_t advancement[2];
	unsigned blankness_type[2] = { START, START };
	unsigned blankness_type_prev[2];
	int const codepage[2] =
	{
		FileTextEncoding::IsCodepageDBCS(location[0].encoding.m_codepage),
		FileTextEncoding::IsCodepageDBCS(location[1].encoding.m_codepage)
	};
	unsigned nocount[2] = { 0, 0 };
	unsigned *count[2] = { NULL, NULL };
	CodePoint c[2] = { '?', '?' };
	CodePoint *cursor[2] = { c, c };
	unsigned diffcode = DIFFCODE::SAME;
	BOOL failed = FALSE;
	BOOL const both = m_osfhandle[j ^ x] != m_osfhandle[j];
	for (;;) switch (both)
	{
	case TRUE:
		{
			enum { i = j };
			if (unsigned *pcount = count[i])
			{
				++*pcount;
				cursor[i] += advancement[i];
				bytes_ahead[i] -= advancement[i] * sizeof(CodePoint);
				blankness_type_prev[i] = blankness_type[i];
			}
			if (bytes_ahead[i] < 8)
			{
				if (m_pCtxt->ShouldAbort())
					return DIFFCODE::CMPABORT;
				if (m_pCtxt->m_bStopAfterFirstDiff && (diffcode == DIFFCODE::DIFF))
				{
					// Do not read more data, but continue to process what is
					// already in memory.
					bytes_ahead[i] = 0;
				}
				else if (read_ahead_size[i] == CMPBUFF)
				{
					// Pad for alignment.
					stl_size_t const padding = PADDING - bytes_ahead[i];
					cursor[i] = static_cast<CodePoint *>(memcpy(
						buff[i] + padding, cursor[i], bytes_ahead[i]));
					DWORD bytes_read = 0;
					BOOL const ok = ReadFile(m_osfhandle[i],
						reinterpret_cast<char *>(cursor[i]) + bytes_ahead[i],
						read_ahead_size[i], &bytes_read, NULL);
					if (!ok || (bytes_read & sizeof(CodePoint) - 1) != 0)
						failed = TRUE;
					bytes_ahead[i] += read_ahead_size[i] = bytes_read;
				}
			}
			count[i] = &nocount[i];
			advancement[i] = 0;
			blankness_type[i] = i;
			if (bytes_ahead[i] >= sizeof(CodePoint))
			{
				advancement[i] = 1;
				blankness_type[i] = NEITHER;
				c[i] = cursor[i][0];
				switch (c[i])
				{
				case 0x20 << CodeShift:
				case '\t' << CodeShift:
					blankness_type[i] = BLANK;
					break;
				case '\0' << CodeShift:
					//blankness_type[i] = ZERO;
					count[i] = &m_textStats[i].nzeros;
					break;
				case '\r' << CodeShift:
					if (bytes_ahead[i] < 2 * sizeof(CodePoint) || cursor[i][1] != ('\n' << CodeShift))
					{
						blankness_type[i] = CR;
						count[i] = &m_textStats[i].ncrs;
					}
					else
					{
						advancement[i] = 2;
						blankness_type[i] = CRLF;
						count[i] = &m_textStats[i].ncrlfs;
					}
					blankness_type_prev[i] = BLANK; // Simulate a preceding BLANK.
					break;
				case '\n' << CodeShift:
					blankness_type[i] = LF;
					count[i] = &m_textStats[i].nlfs;
					blankness_type_prev[i] = BLANK; // Simulate a preceding BLANK.
					break;
				default:
					if (codepage[i] != 0 &&
						(blankness_type_prev[i] & LEADBYTE) == 0 &&
						IsDBCSLeadByteEx(codepage[i], c[i]))
					{
						blankness_type[i] = LEADBYTE;
					}
					break;
				}
			}
		}
	default:
		{
			enum { i = j ^ x };
			if (unsigned *pcount = count[i])
			{
				++*pcount;
				cursor[i] += advancement[i];
				bytes_ahead[i] -= advancement[i] * sizeof(CodePoint);
				blankness_type_prev[i] = blankness_type[i];
			}
			if (bytes_ahead[i] < 8)
			{
				if (m_pCtxt->ShouldAbort())
					return DIFFCODE::CMPABORT;
				if (m_pCtxt->m_bStopAfterFirstDiff && (diffcode == DIFFCODE::DIFF))
				{
					// Do not read more data, but continue to process what is
					// already in memory.
					bytes_ahead[i] = 0;
				}
				else if (read_ahead_size[i] == CMPBUFF)
				{
					// Pad for alignment.
					stl_size_t const padding = PADDING - bytes_ahead[i];
					cursor[i] = static_cast<CodePoint *>(memcpy(
						buff[i] + padding, cursor[i], bytes_ahead[i]));
					DWORD bytes_read = 0;
					BOOL const ok = ReadFile(m_osfhandle[i],
						reinterpret_cast<char *>(cursor[i]) + bytes_ahead[i],
						read_ahead_size[i], &bytes_read, NULL);
					if (!ok || (bytes_read & sizeof(CodePoint) - 1) != 0)
						failed = TRUE;
					bytes_ahead[i] += read_ahead_size[i] = bytes_read;
				}
			}
			count[i] = &nocount[i];
			advancement[i] = 0;
			blankness_type[i] = i;
			if (bytes_ahead[i] >= sizeof(CodePoint))
			{
				advancement[i] = 1;
				blankness_type[i] = NEITHER;
				c[i] = cursor[i][0];
				switch (c[i])
				{
				case 0x20 << CodeShift:
				case '\t' << CodeShift:
					blankness_type[i] = BLANK;
					break;
				case '\0' << CodeShift:
					//blankness_type[i] = ZERO;
					count[i] = &m_textStats[i].nzeros;
					break;
				case '\r' << CodeShift:
					if (bytes_ahead[i] < 2 * sizeof(CodePoint) || cursor[i][1] != ('\n' << CodeShift))
					{
						blankness_type[i] = CR;
						count[i] = &m_textStats[i].ncrs;
					}
					else
					{
						advancement[i] = 2;
						blankness_type[i] = CRLF;
						count[i] = &m_textStats[i].ncrlfs;
					}
					blankness_type_prev[i] = BLANK; // Simulate a preceding BLANK.
					break;
				case '\n' << CodeShift:
					blankness_type[i] = LF;
					count[i] = &m_textStats[i].nlfs;
					blankness_type_prev[i] = BLANK; // Simulate a preceding BLANK.
					break;
				default:
					if (codepage[i] != 0 &&
						(blankness_type_prev[i] & LEADBYTE) == 0 &&
						IsDBCSLeadByteEx(codepage[i], c[i]))
					{
						blankness_type[i] = LEADBYTE;
					}
					break;
				}
			}
		}
		int mask = blankness_type[0] | blankness_type[1];
		if (mask & ~(NEITHER | LEADBYTE))
		{
			// Check for possible end-of-file conditions.
			switch (mask)
			{
			case EOF_0 | START:
				m_textStats[1] = m_textStats[0];
				return failed ? DIFFCODE::CMPERR : DIFFCODE::SAME;
			case EOF_1 | START:
				m_textStats[0] = m_textStats[1];
				return failed ? DIFFCODE::CMPERR : DIFFCODE::SAME;
			case EOF_0 | EOF_1:
				return failed ? DIFFCODE::CMPERR : diffcode;
			}
			if (bIgnoreBlankLines)
			{
				if (blankness_type[0] & (CR | LF | CRLF))
				{
					// EOL on left side only
					count[1] = NULL;
					blankness_type[0] = BLANK; // Simulate a BLANK.
					continue;
				}
				if (blankness_type[1] & (CR | LF | CRLF))
				{
					// EOL on right side only
					count[0] = NULL;
					blankness_type[1] = BLANK; // Simulate a BLANK.
					continue;
				}
			}
			if (mask & BLANK)
			{
				// There is a blank on one or both sides.
				switch (nIgnoreWhitespace)
				{
				case WHITESPACE_IGNORE_ALL:
					blankness_type_prev[0] = BLANK; // Simulate a preceding BLANK.
					blankness_type_prev[1] = BLANK; // Simulate a preceding BLANK.
					// fall through
				case WHITESPACE_IGNORE_CHANGE:
					if (blankness_type[0] == blankness_type[1])
					{
						// Blank on both sides
						continue;
					}
					if (blankness_type[0] & blankness_type_prev[1] & BLANK)
					{
						// Blank on left side
						count[1] = NULL;
						continue;
					}
					if (blankness_type[1] & blankness_type_prev[0] & BLANK)
					{
						// Blank on right side
						count[0] = NULL;
						continue;
					}
					break;
				}
			}
			else if (has_multiple_bits(mask & (CR | LF | CRLF)))
			{
				// EOL on both sides, but different in style. Force bytes ahead
				// to (un)equalness, depending on bIgnoreEol.
				c[0] = true;
				c[1] = bIgnoreEol;
			}
			else if ((blankness_type[0] == EOF_0) || (blankness_type[1] == EOF_1))
			{
				// EOF on either side
				diffcode = DIFFCODE::DIFF;
				continue;
			}
		}
		if (c[0] == c[1] ||
			bIgnoreCase &&
			((blankness_type_prev[0] | blankness_type_prev[1]) & LEADBYTE) == 0 &&
			tolower(c[0]) == tolower(c[1]))
			continue;
		diffcode = DIFFCODE::DIFF;
	}
}

template<UNICODESET L = NONE, UNICODESET R = NONE>
class CaseLabel
{
public:
	enum { Shift = 3, Value = (L) | (R << Shift) };
};

/**
 * @brief Compare two specified files, byte-by-byte
 * @param [in] location FileLocation
 * @return DIFFCODE
 */
unsigned ByteCompare::CompareFiles(FileLocation *location)
{
	m_textStats[0].clear();
	m_textStats[1].clear();

	if (m_pCtxt->m_bStopAfterFirstDiff)
	{
		if (m_osfhandle[0] == m_osfhandle[1])
		{
			m_textStats[0].nzeros = location[0].encoding.m_binary;
			m_textStats[1].nzeros = location[1].encoding.m_binary;
			return DIFFCODE::SAME | DIFFCODE::FILE | DIFFCODE::TEXTFLAGS;
		}
		// If file sizes differ and none of the ignore options are effective,
		// then we know files are different without reading file contents.
		if (m_st_size[0] != m_st_size[1] &&
			nIgnoreWhitespace == WHITESPACE_COMPARE_ALL &&
			!bIgnoreCase && !bIgnoreBlankLines && !bIgnoreEol)
		{
			m_textStats[0].nzeros = location[0].encoding.m_binary;
			m_textStats[1].nzeros = location[1].encoding.m_binary;
			return DIFFCODE::DIFF | DIFFCODE::FILE | DIFFCODE::TEXTFLAGS;
		}
	}

	unsigned code = DIFFCODE::DIFF;

	switch ((location[0].encoding.m_unicoding) |
			(location[1].encoding.m_unicoding << CaseLabel<>::Shift))
	{
	case CaseLabel<NONE, NONE>::Value:
	case CaseLabel<NONE, UTF8>::Value:
	case CaseLabel<UTF8, NONE>::Value:
	case CaseLabel<UTF8, UTF8>::Value:
		code = CompareFiles<BYTE, 0, 1, 0>(location);
		break;
	case CaseLabel<UCS2LE, UCS2LE>::Value:
		code = CompareFiles<WCHAR, 0, 1, 0>(location);
		break;
	case CaseLabel<UCS2BE, UCS2BE>::Value:
		code = CompareFiles<WCHAR, 8, 1, 0>(location);
		break;
	case CaseLabel<UCS4LE, UCS4LE>::Value:
		code = CompareFiles<ULONG, 0, 1, 0>(location);
		break;
	case CaseLabel<UCS4BE, UCS4BE>::Value:
		code = CompareFiles<ULONG, 16, 1, 0>(location);
		break;
	default:
		BOOL const both = m_osfhandle[1] != m_osfhandle[0];
		switch (both)
		{
		case TRUE:
			switch (location[1].encoding.m_unicoding)
			{
			case NONE:
			case UTF8:
				code = CompareFiles<BYTE, 0, 0, 1>(location);
				break;
			case UCS2LE:
				code = CompareFiles<WCHAR, 0, 0, 1>(location);
				break;
			case UCS2BE:
				code = CompareFiles<WCHAR, 8, 0, 1>(location);
				break;
			case UCS4LE:
				code = CompareFiles<ULONG, 0, 0, 1>(location);
				break;
			case UCS4BE:
				code = CompareFiles<ULONG, 16, 0, 1>(location);
				break;
			}
			if (code == DIFFCODE::CMPERR || code == DIFFCODE::CMPABORT)
				break;
		default:
			switch (location[0].encoding.m_unicoding)
			{
			case NONE:
			case UTF8:
				code = CompareFiles<BYTE, 0, 0, 0>(location);
				break;
			case UCS2LE:
				code = CompareFiles<WCHAR, 0, 0, 0>(location);
				break;
			case UCS2BE:
				code = CompareFiles<WCHAR, 8, 0, 0>(location);
				break;
			case UCS4LE:
				code = CompareFiles<ULONG, 0, 0, 0>(location);
				break;
			case UCS4BE:
				code = CompareFiles<ULONG, 16, 0, 0>(location);
				break;
			}
			if (code == DIFFCODE::CMPERR || code == DIFFCODE::CMPABORT)
				break;
			// Files differ in character wideness, so flag them as different.
			// Note that since full compare operates on transcoded content, it
			// may flag files as equal which byte compare considers different.
			code = DIFFCODE::DIFF;
		}
		break;
	}
	// Indicate that text vs. binary classification is up to caller.
	return code | DIFFCODE::FILE | DIFFCODE::TEXTFLAGS;
}
