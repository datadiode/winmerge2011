/**
 * @file  ByteCompare.cpp
 *
 * @brief Implementation file for ByteCompare
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include <io.h>
#include "FileLocation.h"
#include "IAbortable.h"
#include "CompareOptions.h"
#include "DiffContext.h"
#include "DIFF.H"
#include "ByteCompare.h"

using namespace CompareEngines;

/** @brief Quick contents compare's file buffer size. */
static const size_t CMPBUFF = 0x8000;
static const size_t PADDING = 0x0008;

/**
 * @brief Default constructor.
 */
ByteCompare::ByteCompare(const DIFFOPTIONS &options)
	: m_piAbortable(NULL)
{
	SetFromDiffOptions(options);
	m_osfhandle[0] = NULL;
	m_osfhandle[1] = NULL;
}

/**
 * @brief Default destructor.
 */
ByteCompare::~ByteCompare()
{
}

/**
 * @brief Set compare-type specific options.
 * @param [in] stopAfterFirstDiff Do we stop compare after first found diff.
 */
void ByteCompare::SetAdditionalOptions(bool stopAfterFirstDiff)
{
	m_bStopAfterFirstDiff = stopAfterFirstDiff;
}

/**
 * @brief Set Abortable-interface.
 * @param [in] piAbortable Pointer to abortable interface.
 */
void ByteCompare::SetAbortable(const IAbortable * piAbortable)
{
	m_piAbortable = piAbortable;
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
}

enum BLANKNESS_TYPE
{
	EOF_0 = 0,
	EOF_1 = 1,
	NEITHER = 2,
//	ZERO = 4,
	BLANK = 8,
	CR = 16,
	LF = 32,
	CRLF = 64,
	START = 128
};

inline bool is_single_bit(unsigned mask)
{
	return mask > (mask & mask - 1);
}

template<class CodePoint, int CodeShift>
unsigned ByteCompare::CompareFiles(const size_t x, const size_t j)
{
	char buff[2][CMPBUFF + PADDING];
	size_t bytes_ahead[2] = { 0, 0 };
	size_t read_ahead_size[2] = { CMPBUFF, CMPBUFF };
	unsigned blankness_type[2] = { START, START };
	unsigned blankness_type_prev[2];
	unsigned nocount[2] = { 0, 0 };
	unsigned *count[2] = { NULL, NULL };
	size_t advancement[2];
	CodePoint c[2] = { '?', '?' };
	CodePoint *cursor[2] = { c, c };
	unsigned diffcode = DIFFCODE::SAME;
	BOOL ok = TRUE;

	while (true)
	{
		size_t i = j;
		do
		{
			if (unsigned *pcount = count[i])
			{
				++*pcount;
				cursor[i] += advancement[i];
				bytes_ahead[i] -= advancement[i] * sizeof(CodePoint);
				blankness_type_prev[i] = blankness_type[i];
			}
			if (bytes_ahead[i] < 8)
			{
				if (m_piAbortable && m_piAbortable->ShouldAbort())
					return DIFFCODE::CMPABORT;
				if (m_bStopAfterFirstDiff && (diffcode == DIFFCODE::DIFF))
				{
					// Do not read more data, but continue to process what is
					// already in memory.
					bytes_ahead[i] = 0;
				}
				else if (read_ahead_size[i] == CMPBUFF)
				{
					size_t const bytes_left = bytes_ahead[i];
					// Pad for alignment.
					size_t const padding = PADDING - bytes_left;
					char *const left = buff[i] + padding;
					memcpy(left, cursor[i], bytes_left);
					cursor[i] = reinterpret_cast<CodePoint *>(left);
					DWORD bytes_read = 0;
					ok = ReadFile(m_osfhandle[i], left + bytes_left,
						read_ahead_size[i], &bytes_read, NULL);
					bytes_ahead[i] += read_ahead_size[i] = bytes_read;
					if (ok && (bytes_read & sizeof(CodePoint) - 1) != 0)
						ok = FALSE;
				}
			}
			count[i] = &nocount[i];
			advancement[i] = 0;
			blankness_type[i] = i;
			if (size_t n = bytes_ahead[i] / sizeof(CodePoint))
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
					if (n < 2 || cursor[i][1] != ('\n' << CodeShift))
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
				}
			}
		} while (m_osfhandle[i ^= x] != m_osfhandle[j]);
		int mask = blankness_type[0] | blankness_type[1];
		if (mask != NEITHER)
		{
			// Check for possible end-of-file conditions.
			switch (mask)
			{
			case EOF_0 | START:
				m_textStats[1] = m_textStats[0];
				return ok ? DIFFCODE::SAME : DIFFCODE::CMPERR;
			case EOF_1 | START:
				m_textStats[0] = m_textStats[1];
				return ok ? DIFFCODE::SAME : DIFFCODE::CMPERR;
			case EOF_0 | EOF_1:
				return ok ? diffcode : DIFFCODE::CMPERR;
			}
			if (m_bIgnoreBlankLines)
			{
				if (blankness_type[0] & (CR | LF | CRLF))
				{
					// EOL on left side only
					count[1] = NULL;
					continue;
				}
				if (blankness_type[1] & (CR | LF | CRLF))
				{
					// EOL on right side only
					count[0] = NULL;
					continue;
				}
			}
			if (mask & BLANK)
			{
				// There is a blank on one or both sides.
				switch (m_ignoreWhitespace)
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
			else if (!is_single_bit(mask & (CR | LF | CRLF)))
			{
				// EOL on both sides, but different in style. Force bytes ahead
				// to (un)equalness, depending on bIgnoreEOLDifference.
				c[0] = true;
				c[1] = m_bIgnoreEOLDifference;
			}
			else if ((blankness_type[0] == EOF_0) || (blankness_type[1] == EOF_1))
			{
				// EOF on either side
				diffcode = DIFFCODE::DIFF;
				continue;
			}
		}
		if (c[0] == c[1] || m_bIgnoreCase && tolower(c[0]) == tolower(c[1]))
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
 * @param [in] bStopAfterFirstDiff Stop compare after we find first difference?
 * @param [in] piAbortable Interface allowing to abort compare
 * @return DIFFCODE
 */
unsigned ByteCompare::CompareFiles(FileLocation *location)
{
	m_textStats[0].clear();
	m_textStats[1].clear();

	if (m_bStopAfterFirstDiff && (m_osfhandle[0] == m_osfhandle[1]))
		return DIFFCODE::SAME;

	unsigned code = DIFFCODE::DIFF;

	switch ((location[0].encoding.m_unicoding) |
			(location[1].encoding.m_unicoding << CaseLabel<>::Shift))
	{
	case CaseLabel<NONE, NONE>::Value:
	case CaseLabel<NONE, UTF8>::Value:
	case CaseLabel<UTF8, NONE>::Value:
	case CaseLabel<UTF8, UTF8>::Value:
		code = CompareFiles<BYTE, 0>();
		break;
	case CaseLabel<UCS2LE, UCS2LE>::Value:
		code = CompareFiles<WCHAR, 0>();
		break;
	case CaseLabel<UCS2BE, UCS2BE>::Value:
		code = CompareFiles<WCHAR, 8>();
		break;
	case CaseLabel<UCS4LE, UCS4LE>::Value:
		code = CompareFiles<ULONG, 0>();
		break;
	case CaseLabel<UCS4BE, UCS4BE>::Value:
		code = CompareFiles<ULONG, 16>();
		break;
	default:
		size_t i = 0;
		do
		{
			switch (location[i].encoding.m_unicoding)
			{
			case NONE:
			case UTF8:
				code = CompareFiles<BYTE, 0>(0, i);
				break;
			case UCS2LE:
				code = CompareFiles<WCHAR, 0>(0, i);
				break;
			case UCS2BE:
				code = CompareFiles<WCHAR, 8>(0, i);
				break;
			case UCS4LE:
				code = CompareFiles<ULONG, 0>(0, i);
				break;
			case UCS4BE:
				code = CompareFiles<ULONG, 16>(0, i);
				break;
			}
			if (code == DIFFCODE::CMPERR || code == DIFFCODE::CMPABORT)
				break;
			// Files differ in character wideness, so flag them as different.
			// Note that since full compare operates on transcoded content, it
			// may flag files as equal which byte compare considers different.
			code = DIFFCODE::DIFF;
		} while (m_osfhandle[i ^= 1] != m_osfhandle[0]);
		break;
	}
	// Indicate that text vs. binary classification is up to caller.
	return code | DIFFCODE::FILE | DIFFCODE::TEXTFLAGS;
}

/**
 * @brief Return text statistics for last compare.
 * @param [in] side For which file to return statistics.
 * @param [out] stats Stats as asked.
 */
void ByteCompare::GetTextStats(int side, FileTextStats *stats)
{
	stats->ncrlfs = m_textStats[side].ncrlfs;
	stats->ncrs = m_textStats[side].ncrs;
	stats->nlfs = m_textStats[side].nlfs;
	stats->nzeros = m_textStats[side].nzeros;
}
