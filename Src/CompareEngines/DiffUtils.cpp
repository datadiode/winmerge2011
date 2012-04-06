/**
 * @file  DiffUtils.cpp
 *
 * @brief Implementation file for DiffUtils class.
 */
// ID line follows -- this is updated by SVN
// $Id$


#include "StdAfx.h"
#include "CompareOptions.h"
#include "FilterList.h"
#include "DiffContext.h"
#include "DIFF.H"
#include "DiffUtils.h"
#include "coretools.h"
#include "DiffList.h"
#include "FilterCommentsManager.h"

namespace CompareEngines
{
static void CopyTextStats(const file_data * inf, FileTextStats * myTextStats);

/**
 * @brief Default constructor.
 */
DiffUtils::DiffUtils(const DIFFOPTIONS &options)
	: m_inf(NULL)
{
	SetFromDiffOptions(options);
	SetToDiffUtils();
}

/**
 * @brief Default destructor.
 */
DiffUtils::~DiffUtils()
{
}

/**
 * @brief Set filedata.
 * @param [in] items Count of filedata items to set.
 * @param [in] data File data.
 */
void DiffUtils::SetFileData(int items, file_data *data)
{
	// We support only two files currently!
	ASSERT(items == 2);
	m_inf = data;
}

/**
 * @brief Compare two files (as earlier specified).
 * @return DIFFCODE as a result of compare.
 */
int DiffUtils::diffutils_compare_files()
{
	int bin_flag = 0;
	int bin_file = 0; // bitmap for binary files

	// Do the actual comparison (generating a change script)
	struct change *script = NULL;
	bool success = Diff2Files(&script, 0, &bin_flag, false, &bin_file);
	if (!success)
		return DIFFCODE::CMPERR;

	// DIFFCODE::TEXTFLAGS leaves text vs. binary classification to caller
	UINT code = DIFFCODE::SAME | DIFFCODE::FILE | DIFFCODE::TEXTFLAGS;

	// make sure to start counting diffs at 0
	// (usually it is -1 at this point, for unknown)
	m_ndiffs = 0;
	m_ntrivialdiffs = 0;

	const bool bPostFilter = m_filterCommentsLines || m_bIgnoreBlankLines || m_bIgnoreCase;
	if (script && (FilterList::HasRegExps() || bPostFilter))
	{
		struct change *next = script;
		String asLwrCaseExt = A2T(m_inf[0].name);
		String::size_type PosOfDot = asLwrCaseExt.rfind('.');
		if (PosOfDot != String::npos)
		{
			asLwrCaseExt.erase(0, PosOfDot + 1);
			CharLower(&asLwrCaseExt.front());
		}

		while (next)
		{
			/* Find a set of changes that belong together.  */
			struct change *thisob = next;
			struct change *end = find_change(next);
			/* Disconnect them from the rest of the changes,
			making them a hunk, and remember the rest for next iteration.  */
			next = end->link;
			end->link = 0;
#ifdef _DEBUG
			debug_script(thisob);
#endif
			/* Determine range of line numbers involved in each file.  */
			int first0 = 0, last0 = 0, first1 = 0, last1 = 0, deletes = 0, inserts = 0;
			analyze_hunk(thisob, &first0, &last0, &first1, &last1, &deletes, &inserts);
			if (deletes || inserts || thisob->trivial)
			{
				/* Print the lines that the first file has.  */
				int trans_a0 = 0, trans_b0 = 0, trans_a1 = 0, trans_b1 = 0;
				translate_range(&m_inf[0], first0, last0, &trans_a0, &trans_b0);
				translate_range(&m_inf[1], first1, last1, &trans_a1, &trans_b1);

				//Determine quantity of lines in this block for both sides
				int QtyLinesLeft = trans_b0 - trans_a0;
				int QtyLinesRight = trans_b1 - trans_a1;

				if (bPostFilter && (deletes || inserts))
				{
					OP_TYPE op = PostFilter(
						thisob->line0, QtyLinesLeft + 1,
						thisob->line1, QtyLinesRight + 1,
						OP_DIFF, asLwrCaseExt.c_str());
					if (op == OP_TRIVIAL)
						thisob->trivial = 1;
				}

				// Match lines against regular expression filters
				// Our strategy is that every line in both sides must
				// match regexp before we mark difference as ignored.
				if (FilterList::HasRegExps())
				{
					bool match2 = false;
					bool match1 = RegExpFilter(thisob->line0, thisob->line0 + QtyLinesLeft, 0);
					if (match1)
						match2 = RegExpFilter(thisob->line1, thisob->line1 + QtyLinesRight, 1);
					if (match1 && match2)
						thisob->trivial = 1;
				}

			}
			/* Reconnect the script so it will all be freed properly.  */
			end->link = next;
		}
	}
	// Free change script (which we don't want)

	while (script != NULL)
	{
		struct change *link = script->link;
		if (!script->trivial)
			++m_ndiffs;
		else
			++m_ntrivialdiffs;
		free(script);
		script = link;
	}

	// diff_2_files set bin_flag to -1 if different binary
	// diff_2_files set bin_flag to +1 if same binary
	if ((m_ndiffs > 0) || (bin_flag < 0))
	{
		// DIFFCODE::TEXTFLAGS leaves text vs. binary classification to caller
		code = DIFFCODE::DIFF | DIFFCODE::FILE | DIFFCODE::TEXTFLAGS;
	}
	if (bin_flag != 0)
	{
		// We don't know diff counts for binary files
		m_ndiffs = CDiffContext::DIFFS_UNKNOWN;
	}

	return code;
}

/**
 * @brief Match regular expression list against given difference.
 * This function matches the regular expression list against the difference
 * (given as start line and end line). Matching the diff requires that all
 * lines in difference match.
 * @param [in] StartPos First line of the difference.
 * @param [in] endPos Last line of the difference.
 * @param [in] FileNo File to match.
 * return true if any of the expressions matches.
 */
bool DiffUtils::RegExpFilter(int StartPos, int EndPos, int FileNo)
{
	bool linesMatch = true; // set to false when non-matching line is found.
	int line = StartPos;

	while (line <= EndPos && linesMatch == true)
	{
		const char *string = files[FileNo].linbuf[line];
		size_t stringlen = linelen(string);
		if (!FilterList::Match(stringlen, string, m_codepage))
		{
			linesMatch = false;
		}
		++line;
	}
	return linesMatch;
}

/**
 * @brief Compare two files using diffutils.
 *
 * Compare two files (in DiffFileData param) using diffutils. Run diffutils
 * inside SEH so we can trap possible error and exceptions. If error or
 * execption is trapped, return compare failure.
 * @param [out] diffs Pointer to list of change structs where diffdata is stored.
 * @param [in] depth Depth in folder compare (we use 0).
 * @param [out] bin_status used to return binary status from compare.
 * @param [in] bMovedBlocks If TRUE moved blocks are analyzed.
 * @param [out] bin_file Returns which file was binary file as bitmap.
    So if first file is binary, first bit is set etc. Can be NULL if binary file
    info is not needed (faster compare since diffutils don't bother checking
    second file if first is binary).
 * @return TRUE when compare succeeds, FALSE if error happened during compare.
 */
bool DiffUtils::Diff2Files(struct change ** diffs, int depth,
		int * bin_status, bool bMovedBlocks, int * bin_file)
{
	bool bRet = true;
	try
	{
		*diffs = diff_2_files(m_inf, depth, bin_status, bMovedBlocks, bin_file);
	}
	catch (OException *)
	{
		*diffs = NULL;
		bRet = false;
	}
	return bRet;
}

/**
 * @brief Copy text stat results from diffutils back into the FileTextStats structure
 */
static void CopyTextStats(const file_data * inf, FileTextStats * myTextStats)
{
	myTextStats->ncrlfs = inf->count_crlfs;
	myTextStats->ncrs = inf->count_crs;
	myTextStats->nlfs = inf->count_lfs;
	myTextStats->nzeros = inf->count_zeros;
}

/**
 * @brief Return diff counts for last compare.
 * @param [out] diffs Count of real differences.
 * @param [out] trivialDiffs Count of ignored differences.
 */
void DiffUtils::GetDiffCounts(int & diffs, int & trivialDiffs)
{
	diffs = m_ndiffs;
	trivialDiffs = m_ntrivialdiffs;
}

/**
 * @brief Return text statistics for last compare.
 * @param [in] side For which file to return statistics.
 * @param [out] stats Stats as asked.
 */
void DiffUtils::GetTextStats(int side, FileTextStats *stats)
{
	CopyTextStats(&m_inf[side], stats);
}

} // namespace CompareEngines
