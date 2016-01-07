/** 
 * @file  DirViewColItems.cpp
 *
 * @brief Code for individual columns in the DirView
 *
 * @date  Created: 2003-08-19
 */
#include "StdAfx.h"
#include "resource.h"
#include "LanguageSelect.h"
#include "DiffContext.h"
#include "DirView.h"
#include "locality.h"
#include "paths.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/**
 * @brief Function to compare two __int64s for a sort
 */
template<class scalar>
int cmp(scalar a, scalar b) { return a > b ? +1 : a < b ? -1 : 0; }

/**
 * @brief Formats a size as a short string.
 */
static String MakeShortSize(__int64 size)
{
	TCHAR buffer[48];
	if (size < 1024)
		_sntprintf(buffer, _countof(buffer), _T("%d B"), static_cast<int>(size));
	else
		StrFormatByteSizeW(size, buffer, _countof(buffer));
	return buffer;
}

/**
 * @name Functions to format content of each type of column.
 * These functions all receive two parameters, a pointer to CDiffContext.
 * which contains general compare information. And a void pointer whose type
 * depends on column to format. Function to call for each column, and
 * parameter for the function are defined in static DirColInfo f_cols table.
 */
/* @{ */
/**
 * @brief Format Filename column data.
 * @param [in] p Pointer to DIFFITEM.
 * @return String to show in the column.
 */
static String ColFileNameGet(const CDiffContext *, const void *p) //sfilename
{
	const DIFFITEM &di = *static_cast<const DIFFITEM*>(p);
	return
	(
		di.left.filename == di.right.filename ||
		di.isSideLeftOnly() ? di.left.filename :
		di.isSideRightOnly() ? di.right.filename :
		di.left.filename + _T('|') + di.right.filename
	);
}

/**
 * @brief Format Extension column data.
 * @param [in] p Pointer to DIFFITEM.
 * @return String to show in the column.
 */
static String ColExtGet(const CDiffContext *, const void *p) //sfilename
{
	const DIFFITEM &di = *static_cast<const DIFFITEM*>(p);
	LPCTSTR filename = di.left.filename.c_str();
	LPCTSTR ext = PathFindExtension(filename);
	LPCTSTR pastext = ext;
	// We don't show extension for folder names
	if (!di.isDirectory())
	{
		if (int len = lstrlen(ext))
		{
			++ext;
			pastext += len;
		}
		else if (LPCTSTR atat = StrStr(filename, _T("@@")))
		{
			if (LPCTSTR backslash = StrRChr(filename, atat, _T('\\')))
				filename = backslash;
			if (LPCTSTR dot = StrRChr(filename, atat, _T('.')))
			{
				ext = dot + 1; // extension excluding dot
				pastext = atat;
			}
		}
	}
	return String(ext, pastext);
}

/**
 * @brief Format Folder column data.
 * @param [in] p Pointer to DIFFITEM.
 * @return String to show in the column.
 */
static String ColPathGet(const CDiffContext *, const void *p)
{
	const DIFFITEM &di = *static_cast<const DIFFITEM*>(p);
	return
	(
		di.left.path == di.right.path ||
		di.isSideLeftOnly() ? di.left.path :
		di.isSideRightOnly() ? di.right.path :
		di.left.path + _T('|') + di.right.path
	);
}

/**
 * @brief Format Result column data.
 * @param [in] pCtxt Pointer to compare context.
 * @param [in] p Pointer to DIFFITEM.
 * @return String to show in the column.
 */
static String ColStatusGet(const CDiffContext *pCtxt, const void *p)
{
	const DIFFITEM &di = *static_cast<const DIFFITEM*>(p);
	// Note that order of items does matter. We must check for
	// skipped items before unique items, for example, so that
	// skipped unique items are labeled as skipped, not unique.
	if (di.isResultError())
	{
		return LanguageSelect.LoadString(IDS_CANT_COMPARE_FILES);
	}
	if (di.isResultAbort())
	{
		return LanguageSelect.LoadString(IDS_ABORTED_ITEM);
	}
	if (di.isResultFiltered())
	{
		return LanguageSelect.LoadString(
			di.isDirectory() ? IDS_DIR_SKIPPED :
			IDS_FILE_SKIPPED);
	}
	if (di.isSideLeftOnly())
	{
		String path = di.GetLeftFilepath(pCtxt->GetLeftPath());
		paths_UndoMagic(path);
		return static_cast<LPCTSTR>(
			LanguageSelect.FormatStrings(IDS_LEFT_ONLY_IN_FMT, path.c_str()));
	}
	if (di.isSideRightOnly())
	{
		String path = di.GetRightFilepath(pCtxt->GetRightPath());
		paths_UndoMagic(path);
		return static_cast<LPCTSTR>(
			LanguageSelect.FormatStrings(IDS_RIGHT_ONLY_IN_FMT, path.c_str()));
	}
	if (di.isResultSame())
	{
		return LanguageSelect.LoadString(
			di.isText() ? IDS_TEXT_FILES_SAME :
			di.isBin() ? IDS_BIN_FILES_SAME :
			IDS_IDENTICAL);
	}
	if (di.isResultDiff()) // diff
	{
		return LanguageSelect.LoadString(
			di.isText() ? IDS_TEXT_FILES_DIFF :
			di.isBin() ? IDS_BIN_FILES_DIFF :
			di.isDirectory() ? IDS_FOLDERS_ARE_DIFFERENT :
			IDS_FILES_ARE_DIFFERENT);
	}
	return String();
}

/**
 * @brief Format Date column data.
 * @param [in] p Pointer to integer (seconds since 1.1.1970).
 * @return String to show in the column.
 */
static String ColTimeGet(const CDiffContext *, const void *p)
{
	using locality::TimeString;
	const FileTime &r = *static_cast<const FileTime *>(p);
	FILETIME ft;
	SYSTEMTIME st;
	return r != 0 && FileTimeToLocalFileTime(&r, &ft) &&
		FileTimeToSystemTime(&ft, &st) ? TimeString(st) : String();
}

/**
 * @brief Format Sizw column data.
 * @param [in] p Pointer to integer containing size in bytes.
 * @return String to show in the column.
 */
static String ColSizeGet(const CDiffContext *, const void *p)
{
	const __int64 &r = *static_cast<const __int64*>(p);
	return r != -1 ? NumToStr(r) : String();
}

/**
 * @brief Format Folder column data.
 * @param [in] p Pointer to DIFFITEM.
 * @return String to show in the column.
 */
static String ColSizeShortGet(const CDiffContext *, const void *p)
{
	const __int64 &r = *static_cast<const __int64*>(p);
	return r != -1 ? MakeShortSize(r) : String();
}

/**
 * @brief Format Difference cout column data.
 * @param [in] p Pointer to integer having count of differences.
 * @return String to show in the column.
 */
static String ColDiffsGet(const CDiffContext *, const void *p)
{
	const int &r = *static_cast<const int*>(p);
	switch (r)
	{
	case CDiffContext::DIFFS_UNKNOWN_QUICKCOMPARE:
		// QuickCompare, unknown
		return _T("*");
	case CDiffContext::DIFFS_UNKNOWN:
		// Unique item
		return String();
	}
	return NumToStr(r);
}

/**
 * @brief Format Newer/Older column data.
 * @param [in] p Pointer to DIFFITEM.
 * @return String to show in the column.
 */
static String ColNewerGet(const CDiffContext *, const void *p)
{
	const DIFFITEM &di = *static_cast<const DIFFITEM *>(p);
	if (di.isSideLeftOnly())
	{
		return _T("<*<");
	}
	if (di.isSideRightOnly())
	{
		return _T(">*>");
	}
	if (di.left.mtime != 0 && di.right.mtime != 0)
	{
		if (di.left.mtime > di.right.mtime)
		{
			return _T("<<");
		}
		if (di.left.mtime < di.right.mtime)
		{
			return _T(">>");
		}
		return _T("==");
	}
	return _T("***");
}

/**
 * @brief Format Version info to string.
 * @param [in] pCtxt Pointer to compare context.
 * @param [in] pdi Pointer to DIFFITEM.
 * @param [in] bLeft Is the item left-size item?
 * @return String proper to show in the GUI.
 */
static String GetVersion(const CDiffContext *pCtxt, const DIFFITEM *pdi, BOOL bLeft)
{
	DIFFITEM *di = const_cast<DIFFITEM *>(pdi);
	DiffFileInfo &dfi = bLeft ? di->left : di->right;
	if (dfi.versionChecked == DiffFileInfo::VersionInvalid)
	{
		pCtxt->UpdateVersion(di, bLeft);
	}
	return dfi.versionChecked ? dfi.version.GetVersionString() : String();
}

/**
 * @brief Format Version column data.
 * @param [in] pCtxt Pointer to compare context.
 * @param [in] p Pointer to DIFFITEM.
 * @return String to show in the column.
 */
template<BOOL bLeft>
static String ColVersionGet(const CDiffContext *pCtxt, const void *p)
{
	const DIFFITEM &di = *static_cast<const DIFFITEM *>(p);
	return GetVersion(pCtxt, &di, bLeft);
}

/**
 * @brief Format Short Result column data.
 * @param [in] p Pointer to DIFFITEM.
 * @return String to show in the column.
 */
static String ColStatusAbbrGet(const CDiffContext *, const void *p)
{
	const DIFFITEM &di = *static_cast<const DIFFITEM *>(p);
	int id = 0;
	// Note that order of items does matter. We must check for
	// skipped items before unique items, for example, so that
	// skipped unique items are labeled as skipped, not unique.
	if (di.isResultError())
	{
		id = IDS_CMPRES_ERROR;
	}
	else if (di.isResultAbort())
	{
		id = IDS_ABORTED_ITEM;
	}
	else if (di.isResultFiltered())
	{
		if (di.isDirectory())
			id = IDS_DIR_SKIPPED;
		else
			id = IDS_FILE_SKIPPED;
	}
	else if (di.isSideLeftOnly())
	{
		id = IDS_LEFTONLY;
	}
	else if (di.isSideRightOnly())
	{
		id = IDS_RIGHTONLY;
	}
	else if (di.isResultSame())
	{
		id = IDS_IDENTICAL;
	}
	else if (di.isResultDiff())
	{
		id = IDS_DIFFERENT;
	}
	return id ? LanguageSelect.LoadString(id) : String();
}

/**
 * @brief Format Binary column data.
 * @param [in] p Pointer to DIFFITEM.
 * @return String to show in the column.
 */
static String ColBinGet(const CDiffContext *, const void *p)
{
	const DIFFITEM &di = *static_cast<const DIFFITEM *>(p);
	if (di.isBin())
		return _T("*");
	else
		return String();
}

/**
 * @brief Format File Attributes column data.
 * @param [in] p Pointer to file flags class.
 * @return String to show in the column.
 */
static String ColAttrGet(const CDiffContext *, const void *p)
{
	const DiffFileInfo &r = *static_cast<const DiffFileInfo *>(p);
	return r.flags.ToString();
}

/**
 * @brief Format File Encoding column data.
 * @param [in] p Pointer to file information.
 * @return String to show in the column.
 */
static String ColEncodingGet(const CDiffContext *, const void *p)
{
	const DiffFileInfo &r = *static_cast<const DiffFileInfo *>(p);
	return r.encoding.GetName();
}

/**
 * @brief Format EOL type to string.
 * @param [in] p Pointer to DIFFITEM.
 * @param [in] bLeft Are we formatting left-side file's data?
 * @return EOL type as as string.
 */
static String ColEOLTypeGet(const CDiffContext *, const void *p)
{
	const DiffFileInfo &dfi = *static_cast<const DiffFileInfo *>(p);
	const FileTextStats &stats = dfi.m_textStats;
	int id = 0;
	if (stats.nzeros > 0)
	{
		id = IDS_EOL_BIN;
	}
	else if (stats.ncrlfs > 0 && stats.ncrs == 0 && stats.nlfs == 0)
	{
		id = IDS_EOL_DOS;
	}
	else if (stats.ncrlfs == 0 && stats.ncrs > 0 && stats.nlfs == 0)
	{
		id = IDS_EOL_MAC;
	}
	else if (stats.ncrlfs == 0 && stats.ncrs == 0 && stats.nlfs > 0)
	{
		id = IDS_EOL_UNIX;
	}
	else if (stats.ncrlfs == 0 && stats.ncrs == 0 && stats.nlfs == 0)
	{
		return String();
	}
	else
	{
		String s = LanguageSelect.LoadString(IDS_EOL_MIXED);
		TCHAR strstats[40];
		_sntprintf(strstats, _countof(strstats), _T(":%d/%d/%d"), stats.ncrlfs, stats.ncrs, stats.nlfs);
		s += strstats;
		return s;
	}
	return LanguageSelect.LoadString(id);
}

/**
 * @}
 */

/**
 * @name Functions to sort each type of column info.
 * These functions are used to sort information in folder compare GUI. Each
 * column info (type) has its own function to compare the data. Each
 * function receives three parameters:
 * - pointer to compare context
 * - two parameters for data to compare (type varies)
 * Return value is -1, 0, or 1, where 0 means datas are identical.
 */
/* @{ */
/**
 * @brief Compare file names.
 * @param [in] pCtxt Pointer to compare context.
 * @param [in] p Pointer to DIFFITEM having first name to compare.
 * @param [in] q Pointer to DIFFITEM having second name to compare.
 * @return Compare result.
 */
static int ColFileNameSort(const CDiffContext *pCtxt, const void *p, const void *q)
{
	const DIFFITEM &ldi = *static_cast<const DIFFITEM *>(p);
	const DIFFITEM &rdi = *static_cast<const DIFFITEM *>(q);
	if (int cmp = rdi.isDirectory() - ldi.isDirectory())
		return cmp;
	return lstrcmpi(ColFileNameGet(pCtxt, p).c_str(), ColFileNameGet(pCtxt, q).c_str());
}

/**
 * @brief Compare file name extensions.
 * @param [in] pCtxt Pointer to compare context.
 * @param [in] p First file name extension to compare.
 * @param [in] q Second file name extension to compare.
 * @return Compare result.
 */
static int ColExtSort(const CDiffContext *pCtxt, const void *p, const void *q)
{
	const DIFFITEM &ldi = *static_cast<const DIFFITEM *>(p);
	const DIFFITEM &rdi = *static_cast<const DIFFITEM *>(q);
	if (int cmp = rdi.isDirectory() - ldi.isDirectory())
		return cmp;
	return lstrcmpi(ColExtGet(pCtxt, p).c_str(), ColExtGet(pCtxt, q).c_str());
}

/**
 * @brief Compare folder names.
 * @param [in] pCtxt Pointer to compare context.
 * @param [in] p Pointer to DIFFITEM having first folder name to compare.
 * @param [in] q Pointer to DIFFITEM having second folder name to compare.
 * @return Compare result.
 */
static int ColPathSort(const CDiffContext *pCtxt, const void *p, const void *q)
{
	return lstrcmpi(ColPathGet(pCtxt, p).c_str(), ColPathGet(pCtxt, q).c_str());
}

/**
 * @brief Compare compare results.
 * @param [in] p Pointer to DIFFITEM having first result to compare.
 * @param [in] q Pointer to DIFFITEM having second result to compare.
 * @return Compare result.
 */
static int ColStatusSort(const CDiffContext *, const void *p, const void *q)
{
	const DIFFITEM &ldi = *static_cast<const DIFFITEM *>(p);
	const DIFFITEM &rdi = *static_cast<const DIFFITEM *>(q);
	return cmp(rdi.diffcode, ldi.diffcode);
}

/**
 * @brief Compare file times.
 * @param [in] p First time to compare.
 * @param [in] q Second time to compare.
 * @return Compare result.
 */
static int ColTimeSort(const CDiffContext *, const void *p, const void *q)
{
	C_ASSERT(sizeof(FILETIME) == sizeof(UINT64));
	const UINT64 &r = *static_cast<const UINT64 *>(p);
	const UINT64 &s = *static_cast<const UINT64 *>(q);
	return cmp(r, s);
}

/**
 * @brief Compare file sizes.
 * @param [in] p First size to compare.
 * @param [in] q Second size to compare.
 * @return Compare result.
 */
static int ColSizeSort(const CDiffContext *, const void *p, const void *q)
{
	const __int64 &r = *static_cast<const __int64*>(p);
	const __int64 &s = *static_cast<const __int64*>(q);
	return cmp(r, s);
}

/**
 * @brief Compare difference counts.
 * @param [in] p First count to compare.
 * @param [in] q Second count to compare.
 * @return Compare result.
 */
static int ColDiffsSort(const CDiffContext *, const void *p, const void *q)
{
	const int &r = *static_cast<const int*>(p);
	const int &s = *static_cast<const int*>(q);
	return r - s;
}

/**
 * @brief Compare newer/older statuses.
 * @param [in] pCtxt Pointer to compare context.
 * @param [in] p Pointer to DIFFITEM having first status to compare.
 * @param [in] q Pointer to DIFFITEM having second status to compare.
 * @return Compare result.
 */
static int ColNewerSort(const CDiffContext *pCtxt, const void *p, const void *q)
{
	return ColNewerGet(pCtxt, p).compare(ColNewerGet(pCtxt, q));
}

/**
 * @brief Compare file versions.
 * @param [in] pCtxt Pointer to compare context.
 * @param [in] p Pointer to DIFFITEM having first version to compare.
 * @param [in] q Pointer to DIFFITEM having second version to compare.
 * @return Compare result.
 */
template<BOOL bLeft>
static int ColVersionSort(const CDiffContext *pCtxt, const void *p, const void *q)
{
	return ColVersionGet<bLeft>(pCtxt, p).compare(ColVersionGet<bLeft>(pCtxt, q));
}

/**
 * @brief Compare binary statuses.
 * This function returns a comparison of binary results.
 * @param [in] p Pointer to DIFFITEM having first status to compare.
 * @param [in] q Pointer to DIFFITEM having second status to compare.
 * @return Compare result:
 * - if both items are text files or binary files: 0
 * - if left is text and right is binary: -1
 * - if left is binary and right is text: 1
 */
static int ColBinSort(const CDiffContext *, const void *p, const void *q)
{
	const DIFFITEM &ldi = *static_cast<const DIFFITEM *>(p);
	const DIFFITEM &rdi = *static_cast<const DIFFITEM *>(q);
	return ldi.isBin() - rdi.isBin();
}

/**
 * @brief Compare file flags.
 * @param [in] p Pointer to first flag structure to compare.
 * @param [in] q Pointer to second flag structure to compare.
 * @return Compare result.
 */
static int ColAttrSort(const CDiffContext *, const void *p, const void *q)
{
	const DiffFileInfo &r = *static_cast<const DiffFileInfo *>(p);
	const DiffFileInfo &s = *static_cast<const DiffFileInfo *>(q);
	return r.flags.ToString().compare(s.flags.ToString());
}

/**
 * @brief Compare file encodings.
 * @param [in] p Pointer to first structure to compare.
 * @param [in] q Pointer to second structure to compare.
 * @return Compare result.
 */
static int ColEncodingSort(const CDiffContext *, const void *p, const void *q)
{
	const DiffFileInfo &r = *static_cast<const DiffFileInfo *>(p);
	const DiffFileInfo &s = *static_cast<const DiffFileInfo *>(q);
	return FileTextEncoding::Collate(r.encoding, s.encoding);
}
/* @} */

/**
 * @brief All existing folder compare columns.
 *
 * This table has information for all folder compare columns. Fields are
 * (in this order):
 *  - internal name
 *  - name resource ID: column's name shown in header
 *  - description resource ID: columns description text
 *  - custom function for getting column data
 *  - custom function for sorting column data
 *  - parameter for custom functions: DIFFITEM (if NULL) or one of its fields
 *  - default column order number, -1 if not shown by default
 *  - ascending (TRUE) or descending (FALSE) default sort order
 *  - alignment of column contents: numbers are usually right-aligned
 */
const CDirView::DirColInfo CDirView::f_cols[] =
{
	{ _T("Name"), IDS_COLHDR_FILENAME, IDS_COLDESC_FILENAME, &ColFileNameGet, &ColFileNameSort, 0, 0, true, LVCFMT_LEFT },
	{ _T("Path"), IDS_COLHDR_DIR, IDS_COLDESC_DIR, &ColPathGet, &ColPathSort, 0, 1, true, LVCFMT_LEFT },
	{ _T("Status"), IDS_COLHDR_RESULT, IDS_COLDESC_RESULT, &ColStatusGet, &ColStatusSort, 0, 2, true, LVCFMT_LEFT },
	{ _T("Lmtime"), IDS_COLHDR_LTIMEM, IDS_COLDESC_LTIMEM, &ColTimeGet, &ColTimeSort, FIELD_OFFSET(DIFFITEM, left.mtime), 3, false, LVCFMT_LEFT },
	{ _T("Rmtime"), IDS_COLHDR_RTIMEM, IDS_COLDESC_RTIMEM, &ColTimeGet, &ColTimeSort, FIELD_OFFSET(DIFFITEM, right.mtime), 4, false, LVCFMT_LEFT },
	{ _T("Lctime"), IDS_COLHDR_LTIMEC, IDS_COLDESC_LTIMEC, &ColTimeGet, &ColTimeSort, FIELD_OFFSET(DIFFITEM, left.ctime), -1, false, LVCFMT_LEFT },
	{ _T("Rctime"), IDS_COLHDR_RTIMEC, IDS_COLDESC_RTIMEC, &ColTimeGet, &ColTimeSort, FIELD_OFFSET(DIFFITEM, right.ctime), -1, false, LVCFMT_LEFT },
	{ _T("Ext"), IDS_COLHDR_EXTENSION, IDS_COLDESC_EXTENSION, &ColExtGet, &ColExtSort, 0, 5, true, LVCFMT_LEFT },
	{ _T("Lsize"), IDS_COLHDR_LSIZE, IDS_COLDESC_LSIZE, &ColSizeGet, &ColSizeSort, FIELD_OFFSET(DIFFITEM, left.size), -1, false, LVCFMT_RIGHT },
	{ _T("Rsize"), IDS_COLHDR_RSIZE, IDS_COLDESC_RSIZE, &ColSizeGet, &ColSizeSort, FIELD_OFFSET(DIFFITEM, right.size), -1, false, LVCFMT_RIGHT },
	{ _T("LsizeShort"), IDS_COLHDR_LSIZE_SHORT, IDS_COLDESC_LSIZE_SHORT, &ColSizeShortGet, &ColSizeSort, FIELD_OFFSET(DIFFITEM, left.size), -1, false, LVCFMT_RIGHT },
	{ _T("RsizeShort"), IDS_COLHDR_RSIZE_SHORT, IDS_COLDESC_RSIZE_SHORT, &ColSizeShortGet, &ColSizeSort, FIELD_OFFSET(DIFFITEM, right.size), -1, false, LVCFMT_RIGHT },
	{ _T("Newer"), IDS_COLHDR_NEWER, IDS_COLDESC_NEWER, &ColNewerGet, &ColNewerSort, 0, -1, true, LVCFMT_LEFT },
	{ _T("Lversion"), IDS_COLHDR_LVERSION, IDS_COLDESC_LVERSION, &ColVersionGet<TRUE>, &ColVersionSort<TRUE>, 0, -1, true, LVCFMT_LEFT },
	{ _T("Rversion"), IDS_COLHDR_RVERSION, IDS_COLDESC_RVERSION, &ColVersionGet<FALSE>, &ColVersionSort<FALSE>, 0, -1, true, LVCFMT_LEFT },
	{ _T("StatusAbbr"), IDS_COLHDR_RESULT_ABBR, IDS_COLDESC_RESULT_ABBR, &ColStatusAbbrGet, &ColStatusSort, 0, -1, true, LVCFMT_LEFT },
	{ _T("Binary"), IDS_COLHDR_BINARY, IDS_COLDESC_BINARY, &ColBinGet, &ColBinSort, 0, -1, true, LVCFMT_LEFT },
	{ _T("Lattr"), IDS_COLHDR_LATTRIBUTES, IDS_COLDESC_LATTRIBUTES, &ColAttrGet, &ColAttrSort, FIELD_OFFSET(DIFFITEM, left), -1, true, LVCFMT_LEFT },
	{ _T("Rattr"), IDS_COLHDR_RATTRIBUTES, IDS_COLDESC_RATTRIBUTES, &ColAttrGet, &ColAttrSort, FIELD_OFFSET(DIFFITEM, right), -1, true, LVCFMT_LEFT },
	{ _T("Lencoding"), IDS_COLHDR_LENCODING, IDS_COLDESC_LENCODING, &ColEncodingGet, &ColEncodingSort, FIELD_OFFSET(DIFFITEM, left), -1, true, LVCFMT_LEFT },
	{ _T("Rencoding"), IDS_COLHDR_RENCODING, IDS_COLDESC_RENCODING, &ColEncodingGet, &ColEncodingSort, FIELD_OFFSET(DIFFITEM, right), -1, true, LVCFMT_LEFT },
	{ _T("Snsdiffs"), IDS_COLHDR_NSDIFFS, IDS_COLDESC_NSDIFFS, ColDiffsGet, ColDiffsSort, FIELD_OFFSET(DIFFITEM, nsdiffs), -1, false, LVCFMT_RIGHT },
	{ _T("Snidiffs"), IDS_COLHDR_NIDIFFS, IDS_COLDESC_NIDIFFS, ColDiffsGet, ColDiffsSort, FIELD_OFFSET(DIFFITEM, nidiffs), -1, false, LVCFMT_RIGHT },
	{ _T("Leoltype"), IDS_COLHDR_LEOL_TYPE, IDS_COLDESC_LEOL_TYPE, &ColEOLTypeGet, 0, FIELD_OFFSET(DIFFITEM, left), -1, true, LVCFMT_LEFT },
	{ _T("Reoltype"), IDS_COLHDR_REOL_TYPE, IDS_COLDESC_REOL_TYPE, &ColEOLTypeGet, 0, FIELD_OFFSET(DIFFITEM, right), -1, true, LVCFMT_LEFT },
};

/**
 * @brief Count of all known columns
 */
const int CDirView::g_ncols = _countof(CDirView::f_cols);
