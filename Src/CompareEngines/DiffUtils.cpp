/**
 * @file  DiffUtils.cpp
 *
 * @brief Implementation file for DiffUtils class.
 */
// ID line follows -- this is updated by SVN
// $Id$


#include "StdAfx.h"
#include "DiffContext.h"
#include "DIFF.H"
#include "coretools.h"
#include "DiffList.h"

namespace CompareEngines
{

/**
 * @brief Default constructor.
 */
DiffUtils::DiffUtils(const CDiffContext *context)
{
	DIFFOPTIONS::operator=(context->m_options);
	RefreshFilters();
	SetToDiffUtils();
}

/**
 * @brief Compare two files (as earlier specified).
 * @return DIFFCODE as a result of compare.
 */
int DiffUtils::diffutils_compare_files(file_data *data)
{
	int bin_flag = 0;
	int bin_file = 0; // bitmap for binary files

	// Do the actual comparison (generating a change script)
	struct change *script = NULL;
	if (!Diff2Files(&script, data, &bin_flag, &bin_file))
		return DIFFCODE::CMPERR;

	// DIFFCODE::TEXTFLAGS leaves text vs. binary classification to caller
	UINT code = DIFFCODE::SAME | DIFFCODE::FILE | DIFFCODE::TEXTFLAGS;

	// make sure to start counting diffs at 0
	// (usually it is -1 at this point, for unknown)
	m_ndiffs = 0;
	m_ntrivialdiffs = 0;

	const bool bPostFilter = bFilterCommentsLines || bIgnoreBlankLines || bIgnoreCase;
	if (script && (FilterList::HasRegExps() || bPostFilter))
	{
		struct change *next = script;
		String asLwrCaseExt = A2T(data[0].name);
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
				translate_range(&data[0], first0, last0, &trans_a0, &trans_b0);
				translate_range(&data[1], first1, last1, &trans_a1, &trans_b1);

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
					int line0 = thisob->line0;
					int line1 = thisob->line1;
					int end0 = line0 + QtyLinesLeft;
					int end1 = line1 + QtyLinesRight;
					if (line0 + RegExpFilter(line0, end0, 0, false) > end0 &&
						line1 + RegExpFilter(line1, end1, 1, false) > end1)
					{
						thisob->trivial = 1;
					}
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

} // namespace CompareEngines
