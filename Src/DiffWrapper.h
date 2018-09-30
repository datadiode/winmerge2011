/////////////////////////////////////////////////////////////////////////////
//    License (GPLv3+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
/////////////////////////////////////////////////////////////////////////////
/**
 * @file  DiffWrapper.h
 *
 * @brief Declaration file for CDiffWrapper.
 *
 * @date  Created: 2003-08-22
 */
#pragma once

#include "FileLocation.h"
#include "FileTextStats.h"
#include "CompareOptions.h"
#include "FilterCommentsManager.h"
#include "FilterList.h"
#include "LineFiltersList.h"
#include "diffutils/config.h"

class CDiffContext;
struct DIFFRANGE;
class DiffList;
struct DiffFileData;
struct file_data;
struct FilterCommentsSet;
class MovedLines;
enum OP_TYPE;

/**
 * @brief Additional options for creating patch files
 */
struct PATCHOPTIONS
{
	enum output_style outputStyle; /**< Patch file style. */
	int nContext; /**< Number of context lines. */
	bool bAddCommandline; /**< Add diff-style commandline to patch file. */
	bool bOmitTimestamps; /**< Omit timestamps from patch file. */
	bool bAppendFiles; /**< Do we append to existing patch file? */
protected:
	PATCHOPTIONS();
};

/**
 * @brief Diffutils returns this statusdata about files compared
 */
struct DIFFSTATUS
{
	BOOL bLeftMissingNL; /**< Left file is missing EOL before EOF */
	BOOL bRightMissingNL; /**< Right file is missing EOL before EOF */
	bool bBinaries; /**< Files are binaries */
	bool bIdentical; /**< diffutils said files are identical */
	bool bPatchFileFailed; /**< Creating patch file failed */

	DIFFSTATUS() { memset(this, 0, sizeof *this); } // start out with all flags clear
};

/**
 * @brief Wrapper class for diffengine (diffutils and ByteComparator).
 * Diffwappre class is used to run selected diffengine. For folder compare
 * there are several methods (COMPARE_TYPE), but for file compare diffutils
 * is used always. For file compare diffutils can output results to external
 * DiffList or to patch file. Output type must be selected with member
 * functions SetCreatePatchFile() and SetCreateDiffList().
 */
class CDiffWrapper
	: public DIFFOPTIONS
	, public PATCHOPTIONS
	, public FilterCommentsManager
	, public FilterList /**< Filter list for line filters. */
{
public:
	CDiffWrapper(DiffList * = NULL);
	~CDiffWrapper();
	void SetToDiffUtils(DiffFileData &);
	void RefreshFilters();
	void RefreshOptions();
	void SetCreatePatchFile(const String &filename);
	void SetDetectMovedBlocks(bool bDetectMovedBlocks);
	void SetPaths(const String &filepath1, const String &filepath2);
	void SetAlternativePaths(const String &altPath1, const String &altPath2, bool bAddCommonSuffix = false);
	void SetCodepage(int codepage) { m_codepage = codepage; }
	bool RunFileDiff();
	bool RunFileDiff(DiffFileData &diffdata);
	bool AddDiffRange(UINT begin0, UINT end0, UINT begin1, UINT end1, OP_TYPE op);
	bool FixLastDiffRange(int leftBufferLines, int rightBufferLines, bool bIgnoreBlankLines);
	MovedLines *GetMovedLines() { return m_pMovedLines; }
	void SetCompareFiles(const String &OriginalFile1, const String &OriginalFile2);

	const String &GetCompareFile(int side) const
	{
		return side == 0 ? m_sOriginalFile1 : m_sOriginalFile2;
	}

	// Postfiltering
	OP_TYPE PostFilter(struct comparison *,
		int LineNumberLeft, int QtyLinesLeft,
		int LineNumberRight, int QtyLinesRight,
		OP_TYPE Op, const TCHAR *FileNameExt);

	DIFFSTATUS m_status; /**< Status of last compare */

protected:
	void FormatSwitchString(struct comparison const *, char *);
	bool Diff2Files(struct change **, struct comparison *, int *bin_status, int *bin_file);
	bool LoadWinMergeDiffsFromDiffUtilsScript(struct change *, struct comparison *);
	void WritePatchFile(struct change *script, struct comparison *);
	int RegExpFilter(struct comparison *, int StartPos, int EndPos, int FileNo, bool BreakCondition);

	int m_codepage; /**< Codepage used in line filter */

private:
	String m_s1File; /**< Full path to first diff'ed file. */
	String m_s2File; /**< Full path to second diff'ed file. */
	String m_s1AlternativePath; /**< First file's alternative path (may be relative). */
	String m_s2AlternativePath; /**< Second file's alternative path (may be relative). */
	String m_sOriginalFile1; /**< First file's original (NON-TEMP) path. */
	String m_sOriginalFile2; /**< Second file's original (NON-TEMP) path. */
	String m_sPatchFile; /**< Full path to created patch file. */
	DiffList *const m_pDiffList; /**< Pointer to external DiffList */
	MovedLines *m_pMovedLines;
};
