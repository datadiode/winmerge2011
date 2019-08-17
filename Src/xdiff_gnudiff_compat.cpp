#include "StdAfx.h"
#include "CompareOptions.h"
#include "diff.h"
#include "libxdiff/xinclude.h"

// Does the binary layout of struct change match that of xdchange_t?
C_ASSERT(sizeof(change) == sizeof(xdchange_t));
C_ASSERT(offsetof(change, line0) == offsetof(xdchange_t, i1));
C_ASSERT(offsetof(change, line1) == offsetof(xdchange_t, i2));
C_ASSERT(offsetof(change, deleted) == offsetof(xdchange_t, chg1));
C_ASSERT(offsetof(change, inserted) == offsetof(xdchange_t, chg2));
C_ASSERT(offsetof(change, ignore) == offsetof(xdchange_t, ignore));
C_ASSERT(offsetof(change, trivial) == offsetof(xdchange_t, trivial));
C_ASSERT(offsetof(change, match0) == offsetof(xdchange_t, match0));
C_ASSERT(offsetof(change, match1) == offsetof(xdchange_t, match1));

static int hunk_func(long start_a, long count_a, long start_b, long count_b, void *cb_data)
{
	return 0;
}

struct change *diff_2_files_xdiff(struct comparison *cmp, int bMoved_blocks_flag, int *bin_file, int algorithm)
{
	change *script = NULL;
	xdfenv_t xe;
	xdchange_t *xscr;
	xpparam_t xpp = { 0 };
	xdemitconf_t xecfg = { 0 };
	xdemitcb_t ecb = { 0 };

	switch (algorithm)
	{
	case DIFF_ALGORITHM_XDF_PATIENCE:
		xpp.flags = XDF_PATIENCE_DIFF;
		break;
	case DIFF_ALGORITHM_XDF_HISTOGRAM:
		xpp.flags = XDF_HISTOGRAM_DIFF;
		break;
	default:
		break;
	}

	if (cmp->ignore_case)
		xpp.flags |= XDF_IGNORE_CASE;
	if (cmp->ignore_eol_diff)
		xpp.flags |= XDF_IGNORE_CR_AT_EOL;
	if (cmp->ignore_blank_lines)
		xpp.flags |= XDF_IGNORE_BLANK_LINES;
	if (cmp->minimal)
		xpp.flags |= XDF_NEED_MINIMAL;
	if (cmp->indent_heuristic)
		xpp.flags |= XDF_INDENT_HEURISTIC;
	switch (cmp->ignore_white_space)
	{
	case IGNORE_TRAILING_SPACE:
	case IGNORE_TAB_EXPANSION_AND_TRAILING_SPACE:
		xpp.flags |= XDF_IGNORE_WHITESPACE_AT_EOL;
		break;
	case IGNORE_SPACE_CHANGE:
		xpp.flags |= XDF_IGNORE_WHITESPACE_CHANGE;
		break;
	case IGNORE_ALL_SPACE:
		xpp.flags |= XDF_IGNORE_WHITESPACE;
		break;
	default:
		break;
	}

	read_files(cmp, bin_file);

	mmfile_t mmfile1 = {
		const_cast<char *>(cmp->file[0].prefix_end),
		static_cast<long>(cmp->file[0].suffix_begin - cmp->file[0].prefix_end) - cmp->file[0].missing_newline
	};
	mmfile_t mmfile2 = {
		const_cast<char *>(cmp->file[1].prefix_end),
		static_cast<long>(cmp->file[1].suffix_begin - cmp->file[1].prefix_end) - cmp->file[1].missing_newline
	};

	xecfg.hunk_func = hunk_func;
	if (xdl_diff_modified(&mmfile1, &mmfile2, &xpp, &xecfg, &ecb, &xe, &xscr) == 0)
	{
		script = reinterpret_cast<change *>(xscr);
		if (bMoved_blocks_flag)
		{
			void moved_block_analysis(struct change *script, struct file_data fd[]);
			moved_block_analysis(script, cmp->file);
		}
		xdl_free_env(&xe);
	}

	return script;
}
