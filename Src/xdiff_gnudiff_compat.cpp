#include "StdAfx.h"
#include <io.h>
#include <sys/stat.h>
#include "CompareOptions.h"
#include "diff.h"
#include "xalloc.h"
#include "libxdiff/xinclude.h"

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
		static_cast<long>(cmp->file[0].suffix_begin - cmp->file[0].prefix_end)
	};
	mmfile_t mmfile2 = {
		const_cast<char *>(cmp->file[1].prefix_end),
		static_cast<long>(cmp->file[1].suffix_begin - cmp->file[1].prefix_end)
	};

	xecfg.hunk_func = hunk_func;
	if (xdl_diff_modified(&mmfile1, &mmfile2, &xpp, &xecfg, &ecb, &xe, &xscr) == 0)
	{
		change *prev = NULL;
		for (xdchange_t *xcur = xscr; xcur; xcur = xcur->next)
		{
			change *const e = static_cast<change *>(xmalloc(sizeof(change)));
			e->line0 = xcur->i1;
			e->line1 = xcur->i2;
			e->deleted = xcur->chg1;
			e->inserted = xcur->chg2;
			e->match0 = -1;
			e->match1 = -1;
			e->trivial = xcur->ignore != 0;
			e->link = NULL;
			prev = (prev ? prev->link : script) = e;
		}
		if (bMoved_blocks_flag)
		{
			void moved_block_analysis(struct change *script, struct file_data fd[]);
			moved_block_analysis(script, cmp->file);
		}
		xdl_free_script(xscr);
		xdl_free_env(&xe);
	}

	return script;
}
