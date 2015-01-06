/////////////////////////////////////////////////////////////////////////////
//    License (GPLv2+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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

/** @enum COMPARE_TYPE
 * @brief Different foldercompare methods.
 * These values are the foldercompare methods WinMerge supports.
 */

/** @var CMP_CONTENT
 * @brief Normal by content compare.
 * This compare type is first, default and all-seeing compare type.
 * diffutils is used for producing compare results. So all limitations
 * of diffutils (like buffering) apply to this compare method. But this
 * is also currently only compare method that produces difference lists
 * we can use in file compare.
 */

/** @var CMP_QUICK_CONTENT
 * @brief Faster byte per byte -compare.
 * This type of compare was a response for needing faster compare results
 * in folder compare. It independent from diffutils, and fully customised
 * for WinMerge. It basically does byte-per-byte compare, still implementing
 * different whitespace ignore options.
 *
 * Optionally this compare type can be stopped when first difference is found.
 * Which gets compare as fast as possible. But misses sometimes binary files
 * if zero bytes aren't found before first difference. Also difference counts
 * are not useful with that option.
 */

/** @var CMP_DATE
 * @brief Compare by modified date.
 * This compare type was added after requests and realization that in some
 * situations difference in file's timestamps is enough to judge them
 * different. E.g. when modifying files in local machine, file timestamps
 * are certainly different after modifying them. This method doesn't even
 * open files for reading them. It only reads file's infos for timestamps
 * and compares them.
 *
 * This is no doubt fastest way to compare files.
 */

/** @var CMP_DATE_SIZE
 * @brief Compare by date and then by size.
 * This method is basically same than CMP_DATE, but it adds check for file
 * sizes if timestamps are identical. This is because there are situations
 * timestamps can't be trusted alone, especially with network shares. Adding
 * checking for file sizes adds some more reliability for results with
 * minimal increase in compare time.
 */

/** @var CMP_SIZE
 * @brief Compare by file size.
 * This compare method compares file sizes. This isn't quite accurate method,
 * other than it can detect files that certainly differ. But it can show lot of
 * different files as identical too. Advantage is in some use cases where different
 * size always means files are different. E.g. automatically created logs - when
 * more data is added size increases.
 */
enum COMPARE_TYPE
{
	CMP_CONTENT,
	CMP_QUICK_CONTENT,
	CMP_DATE,
	CMP_DATE_SIZE,
	CMP_SIZE,
};

/**
 * @brief Additional options for creating patch files
 */
struct PATCHOPTIONS
{
	enum output_style outputStyle; /**< Patch file style. */
	int nContext; /**< Number of context lines. */
	bool bAddCommandline; /**< Add diff-style commandline to patch file. */
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
	void SetToDiffUtils();
	void RefreshFilters();
	void RefreshOptions();
	void SetCreatePatchFile(const String &filename);
	void SetDetectMovedBlocks(bool bDetectMovedBlocks);
	void SetPaths(const String &filepath1, const String &filepath2);
	void SetAlternativePaths(const String &altPath1, const String &altPath2);
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
	OP_TYPE PostFilter(
		int LineNumberLeft, int QtyLinesLeft,
		int LineNumberRight, int QtyLinesRight,
		OP_TYPE Op, const TCHAR *FileNameExt);

	static THREAD_LOCAL CDiffWrapper *m_pActiveInstance;
	DIFFSTATUS m_status; /**< Status of last compare */

protected:
	String FormatSwitchString();
	bool Diff2Files(struct change **, file_data *inf, int *bin_status, int *bin_file);
	bool LoadWinMergeDiffsFromDiffUtilsScript(struct change *, const file_data *);
	void WritePatchFile(struct change *script, file_data *inf);
	int RegExpFilter(int StartPos, int EndPos, int FileNo, bool BreakCondition);

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
