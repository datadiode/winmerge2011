/////////////////////////////////////////////////////////////////////////////
//    License (GPLv2+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or (at
//    your option) any later version.
//    
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
/////////////////////////////////////////////////////////////////////////////
/** 
 * @file  DiffWrapper.cpp
 *
 * @brief Code for DiffWrapper class
 *
 * @date  Created: 2003-08-22
 */
#include "StdAfx.h"
#include "coretools.h"
#include "MovedLines.h"
#include "Merge.h"
#include "MainFrm.h"
#include "DIFF.H"
#include "LogFile.h"
#include "paths.h"
#include "FileTextStats.h"
#include "FolderCmp.h"
#include "Environment.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static void CopyTextStats(const file_data *, FileTextStats *);

THREAD_LOCAL CDiffWrapper *CDiffWrapper::m_pActiveInstance = NULL;

PATCHOPTIONS::PATCHOPTIONS()
	: outputStyle(OUTPUT_NORMAL)
	, nContext(0)
	, bAddCommandline(true)
	, bAppendFiles(false)
{
}

/**
 * @brief Default constructor.
 * Initializes members and creates new FilterCommentsManager.
 */
CDiffWrapper::CDiffWrapper(DiffList *pDiffList)
: DIFFOPTIONS(NULL)
, m_codepage(0)
, m_pDiffList(pDiffList)
, m_pMovedLines(NULL)
{
	// character that ends a line.  Currently this is always `\n'
	line_end_char = '\n';
}

/**
 * @brief Destructor.
 */
CDiffWrapper::~CDiffWrapper()
{
	delete m_pMovedLines;
	if (m_pActiveInstance == this)
		m_pActiveInstance = NULL;
}

/**
 * @brief Set options to diffutils.
 * Diffutils uses options from global variables we must set. Those variables
 * aren't most logically named, and most are just int's, with some magic
 * values and meanings, with fancy combinations? So not easy to setup. This
 * function maps our easier to handle compare options to diffutils globals.
 */
void CDiffWrapper::SetToDiffUtils()
{
	m_pActiveInstance = this;

	output_style = outputStyle;

	context = nContext;

	if (nIgnoreWhitespace == WHITESPACE_IGNORE_CHANGE)
		ignore_space_change_flag = 1;
	else
		ignore_space_change_flag = 0;

	if (nIgnoreWhitespace == WHITESPACE_IGNORE_ALL)
		ignore_all_space_flag = 1;
	else
		ignore_all_space_flag = 0;

	if (bIgnoreBlankLines)
		ignore_blank_lines_flag = 1;
	else
		ignore_blank_lines_flag = 0;

	if (bIgnoreCase)
		ignore_case_flag = 1;
	else
		ignore_case_flag = 0;

	if (bIgnoreEol)
		ignore_eol_diff = 1;
	else
		ignore_eol_diff = 0;

	if (nIgnoreWhitespace != WHITESPACE_COMPARE_ALL || bIgnoreCase ||
			bIgnoreBlankLines || bIgnoreEol)
		ignore_some_changes = 1;
	else
		ignore_some_changes = 0;

	if (nIgnoreWhitespace != WHITESPACE_COMPARE_ALL)
		length_varies = 1;
	else
		length_varies = 0;

	dbcs_codepage = FileTextEncoding::IsCodepageDBCS(m_codepage);

	// We have no interest changing these values, hard-code them.
	always_text_flag = 0; // diffutils needs to detect binary files
	horizon_lines = 0;
	heuristic = 1;
}

void CDiffWrapper::RefreshFilters()
{
	FilterList::RemoveAllFilters();
	if (bApplyLineFilters)
	{
		String inifile = COptionsMgr::Get(OPT_SUPPLEMENT_FOLDER);
		inifile = paths_ConcatPath(inifile, _T("LineFilters.ini"));
		FilterList::AddFromIniFile(inifile.c_str());
		FilterList::AddFrom(globalLineFilters);
	}
}

void CDiffWrapper::RefreshOptions()
{
	nIgnoreWhitespace = COptionsMgr::Get(OPT_CMP_IGNORE_WHITESPACE);
	bIgnoreBlankLines = COptionsMgr::Get(OPT_CMP_IGNORE_BLANKLINES);
	bFilterCommentsLines = COptionsMgr::Get(OPT_CMP_FILTER_COMMENTLINES);
	bIgnoreCase = COptionsMgr::Get(OPT_CMP_IGNORE_CASE);
	bIgnoreEol = COptionsMgr::Get(OPT_CMP_IGNORE_EOL);
	bApplyLineFilters = COptionsMgr::Get(OPT_LINEFILTER_ENABLED);
	RefreshFilters();
	SetDetectMovedBlocks(COptionsMgr::Get(OPT_CMP_MOVED_BLOCKS));
}

/**
 * @brief Enables/disables patch-file creation and sets filename.
 * This function enables or disables patch file creation. When
 * @p filename is empty, patch files are disabled.
 * @param [in] filename Filename for patch file, or empty string.
 */
void CDiffWrapper::SetCreatePatchFile(const String &filename)
{
	m_sPatchFile = filename;
	string_replace(m_sPatchFile, _T('/'), _T('\\'));
}

/**
 * @brief Enables/disables moved block detection.
 * @param [in] bDetectMovedBlocks If TRUE moved blocks are detected.
 */
void CDiffWrapper::SetDetectMovedBlocks(bool bDetectMovedBlocks)
{
	if (bDetectMovedBlocks)
	{
		if (m_pMovedLines == NULL)
			m_pMovedLines = new MovedLines;
	}
	else
	{
		delete m_pMovedLines;
		m_pMovedLines = NULL;
	}
}

/**
 * @brief Replace spaces in a string
 * @param [in] str - String to search
 * @param [in] rep - String to replace
 */
static void ReplaceSpaces(std::string &str, const char *rep)
{
	std::string::size_type pos = 0;
	std::string::size_type replen = static_cast<std::string::size_type>(strlen(rep));
	while ((pos = str.find_first_of(" \t", pos)) != std::string::npos)
	{
		std::string::size_type posend = str.find_first_not_of(" \t", pos);
		if (posend != std::string::npos)
			str.replace(pos, posend - pos, rep);
		else
			str.replace(pos, 1, rep);
		pos += replen;
	}
}

/**
 * @brief The main entry for post filtering.  Performs post-filtering, by setting comment blocks to trivial
 * @param [in] LineNumberLeft	- First line number to read from left file
 * @param [in] QtyLinesLeft		- Number of lines in the block for left file
 * @param [in] LineNumberRight	- First line number to read from right file
 * @param [in] QtyLinesRight	- Number of lines in the block for right file
 * @param [in] Op				- OP_TYPE of the block as determined by caller
 * @param [in] FileNameExt		- The file name extension.  Needs to be lower case string ("cpp", "java", "c")
 * @return Revised OP_TYPE of the block, with OP_TRIVIAL indicating that the block should be ignored
 */
OP_TYPE CDiffWrapper::PostFilter(int LineNumberLeft, int QtyLinesLeft,
	int LineNumberRight, int QtyLinesRight, OP_TYPE Op, const TCHAR *FileNameExt)
{
	ASSERT(Op != OP_TRIVIAL);
	
	//First of all get the comment marker set used to indicate comment blocks
	FilterCommentsSet filtercommentsset = GetSetForFileType(FileNameExt);
	if (filtercommentsset.StartMarker.empty() && 
		filtercommentsset.EndMarker.empty() &&
		filtercommentsset.InlineMarker.empty())
	{
		return Op;
	}

	OP_TYPE LeftOp = OP_NONE;
	if (Op != OP_RIGHTONLY)
		LeftOp = filtercommentsset.PostFilter(LineNumberLeft, QtyLinesLeft, &files[0]);

	OP_TYPE RightOp = OP_NONE;
	if (Op != OP_LEFTONLY)
		RightOp = filtercommentsset.PostFilter(LineNumberRight, QtyLinesRight, &files[1]);

	for (int i = 0; (i < QtyLinesLeft) || (i < QtyLinesRight); i++)
	{
		//Lets test  all lines if only a comment is different.
		const char *LineStrLeft = "";
		const char *EndLineLeft = LineStrLeft;
		const char *LineStrRight = "";
		const char *EndLineRight = LineStrRight;
		if (i < QtyLinesLeft)
		{
			LineStrLeft = files[0].linbuf[LineNumberLeft + i];
			EndLineLeft = files[0].linbuf[LineNumberLeft + i + 1];
		}
		if (i < QtyLinesRight)
		{
			LineStrRight = files[1].linbuf[LineNumberRight + i];
			EndLineRight = files[1].linbuf[LineNumberRight + i + 1];
		}
			
		if (EndLineLeft && EndLineRight)
		{	
			std::string LineDataLeft(LineStrLeft, EndLineLeft);
			std::string LineDataRight(LineStrRight, EndLineRight);

			if (!filtercommentsset.StartMarker.empty() && !filtercommentsset.EndMarker.empty())
			{
				//Lets remove block comments, and see if lines are equal
				const char *CommentStrStart;
				const char *CommentStrEnd;
				do
				{
					CommentStrStart = FilterCommentsSet::FindCommentMarker(LineDataLeft.c_str(), filtercommentsset.StartMarker);
					CommentStrEnd = FilterCommentsSet::FindCommentMarker(LineDataLeft.c_str(), filtercommentsset.EndMarker);
					if (CommentStrStart < CommentStrEnd)
					{
						LineDataLeft.erase(static_cast<std::string::size_type>(CommentStrStart - LineDataLeft.c_str()),
							static_cast<std::string::size_type>(CommentStrEnd + filtercommentsset.EndMarker.size() - CommentStrStart));
					}
					else if (LeftOp == OP_TRIVIAL)
					{
						if (CommentStrEnd < CommentStrStart)
							CommentStrEnd += filtercommentsset.EndMarker.size();
						LineDataLeft.erase(0, static_cast<std::string::size_type>(CommentStrEnd - LineDataLeft.c_str()));
					}
				} while (CommentStrStart < CommentStrEnd);
				do
				{
					CommentStrStart = FilterCommentsSet::FindCommentMarker(LineDataRight.c_str(), filtercommentsset.StartMarker);
					CommentStrEnd = FilterCommentsSet::FindCommentMarker(LineDataRight.c_str(), filtercommentsset.EndMarker);
					if (CommentStrStart < CommentStrEnd)
					{
						LineDataRight.erase(static_cast<std::string::size_type>(CommentStrStart - LineDataRight.c_str()),
							static_cast<std::string::size_type>(CommentStrEnd + filtercommentsset.EndMarker.size() - CommentStrStart));
					}
					else if (RightOp == OP_TRIVIAL)
					{
						if (CommentStrEnd < CommentStrStart)
							CommentStrEnd += filtercommentsset.EndMarker.size();
						LineDataRight.erase(0, static_cast<std::string::size_type>(CommentStrEnd - LineDataRight.c_str()));
					}
				} while (CommentStrStart < CommentStrEnd);
			}

			if (!filtercommentsset.InlineMarker.empty())
			{
				//Lets remove line comments
				LineDataLeft.erase(static_cast<std::string::size_type>(
					FilterCommentsSet::FindCommentMarker(
						LineDataLeft.c_str(), filtercommentsset.InlineMarker
					) - LineDataLeft.c_str()
				));
				LineDataRight.erase(static_cast<std::string::size_type>(
					FilterCommentsSet::FindCommentMarker(
						LineDataRight.c_str(), filtercommentsset.InlineMarker
					) - LineDataRight.c_str()
				));
			}

			if (nIgnoreWhitespace == WHITESPACE_IGNORE_ALL)
			{
				//Ignore character case
				ReplaceSpaces(LineDataLeft, "");
				ReplaceSpaces(LineDataRight, "");
			}
			else if (nIgnoreWhitespace == WHITESPACE_IGNORE_CHANGE)
			{
				//Ignore change in whitespace char count
				ReplaceSpaces(LineDataLeft, " ");
				ReplaceSpaces(LineDataRight, " ");
			}

			if (bIgnoreCase)
			{
				//ignore case
				std::transform(LineDataLeft.begin(), LineDataLeft.end(), LineDataLeft.begin(), ::toupper);
				std::transform(LineDataRight.begin(), LineDataRight.end(), LineDataRight.begin(), ::toupper);
			}

			if (LineDataLeft != LineDataRight)
			{	
				return Op;
			}
		}
	}
	//only difference is trival
	return OP_TRIVIAL;
}

/**
 * @brief Set source paths for diffing two files.
 * Sets full paths to two files we are diffing. Paths can be actual user files
 * or temporary copies of user files. Parameter @p tempPaths tells if paths
 * are temporary paths that can be deleted.
 * @param [in] filepath1 First file to compare "original file".
 * @param [in] filepath2 Second file to compare "changed file".
 */
void CDiffWrapper::SetPaths(const String &filepath1, const String &filepath2)
{
	m_s1File = filepath1;
	m_s2File = filepath2;
}

/**
 * @brief Set source paths for original (NON-TEMP) diffing two files.
 * Sets full paths to two (NON-TEMP) files we are diffing.
 * @param [in] OriginalFile1 First file to compare "(NON-TEMP) file".
 * @param [in] OriginalFile2 Second file to compare "(NON-TEMP) file".
 */
void CDiffWrapper::SetCompareFiles(const String &OriginalFile1, const String &OriginalFile2)
{
	m_sOriginalFile1 = OriginalFile1;
	m_sOriginalFile2 = OriginalFile2;
}

/**
 * @brief Set alternative paths for compared files.
 * Sets alternative paths for diff'ed files. These alternative paths might not
 * be real paths. For example when creating a patch file from folder compare
 * we want to use relative paths.
 * @param [in] altPath1 Alternative file path of first file.
 * @param [in] altPath2 Alternative file path of second file.
 */
void CDiffWrapper::SetAlternativePaths(const String &altPath1, const String &altPath2)
{
	m_s1AlternativePath = altPath1;
	m_s2AlternativePath = altPath2;
}

bool CDiffWrapper::RunFileDiff(DiffFileData &diffdata)
{
	SetToDiffUtils();

	// Compare the files, if no error was found.
	// Last param (bin_file) is NULL since we don't
	// (yet) need info about binary sides.
	int bin_flag = 0;
	struct change *script = NULL;
	bool bRet = Diff2Files(&script, diffdata.m_inf, &bin_flag, NULL);
	if (bRet)
	{
		CopyTextStats(&diffdata.m_inf[0], &diffdata.m_textStats[0]);
		CopyTextStats(&diffdata.m_inf[1], &diffdata.m_textStats[1]);
	}

	// We don't anymore create diff-files for every rescan.
	// User can create patch-file whenever one wants to.
	// We don't need to waste time. But lets keep this as
	// debugging aid. Sometimes it is very useful to see
	// what differences diff-engine sees!
#ifdef _DEBUG
	// throw the diff into a temp file
	String sTempPath = env_GetTempPath(); // get path to Temp folder
	String path = paths_ConcatPath(sTempPath, _T("Diff.txt"));

	outfile = _tfopen(path.c_str(), _T("w+"));
	if (outfile != NULL)
	{
		print_normal_script(script);
		fclose(outfile);
		outfile = NULL;
	}
#endif

	// First determine what happened during comparison
	// If there were errors or files were binaries, don't bother
	// creating diff-lists or patches
	
	// diff_2_files set bin_flag to -1 if different binary
	// diff_2_files set bin_flag to +1 if same binary
	if (bin_flag != 0)
	{
		m_status.bBinaries = true;
		m_status.bIdentical = bin_flag != -1;
	}
	else
	{ // text files according to diffutils, so change script exists
		m_status.bBinaries = false;
		m_status.bIdentical = script == NULL;
	}
	file_data *inf = diffdata.m_inf;
	m_status.bLeftMissingNL = inf[0].missing_newline;
	m_status.bRightMissingNL = inf[1].missing_newline;

	// Create patch file
	if (!m_status.bBinaries && !m_sPatchFile.empty())
	{
		WritePatchFile(script, &inf[0]);
	}
	
	// Go through diffs adding them to WinMerge's diff list
	// This is done on every WinMerge's doc rescan!
	if (!m_status.bBinaries && m_pDiffList)
	{
		bRet = LoadWinMergeDiffsFromDiffUtilsScript(script, diffdata.m_inf);
	}			

	// cleanup the script
	while (script != NULL)
	{
		struct change *link = script->link;
		free(script);
		script = link;
	}

	// Done with diffutils filedata
	diffdata.Close();

	return bRet;
}

/**
 * @brief Runs diff-engine.
 */
bool CDiffWrapper::RunFileDiff()
{
	DiffFileData diffdata(FileTextEncoding::GetDefaultCodepage());
	diffdata.SetDisplayFilepaths(m_s1File.c_str(), m_s2File.c_str()); // store true names for diff utils patch file
	if (!diffdata.OpenFiles(m_s1File.c_str(), m_s2File.c_str()))
		return false;
	return RunFileDiff(diffdata);
}

/**
 * @brief Add diff to external diff-list
 */
bool CDiffWrapper::AddDiffRange(UINT begin0, UINT end0, UINT begin1, UINT end1, OP_TYPE op)
{
	bool ok = false;
	try
	{
		DIFFRANGE dr;
		dr.begin0 = begin0;
		dr.end0 = end0;
		dr.begin1 = begin1;
		dr.end1 = end1;
		dr.op = op;
		m_pDiffList->AddDiff(dr);
		ok = true;
	}
	catch (OException *e)
	{
		e->ReportError(theApp.m_pMainWnd->m_hWnd, MB_ICONSTOP);
		delete e;
	}
	TRACE("left=%d,%d   right=%d,%d   op=%d\n",
		begin0, end0, begin1, end1, op);
	return ok;
}

/**
 * @brief Expand last DIFFRANGE of file by one line to contain last line after EOL.
 * @param [in] leftBufferLines size of array pane left
 * @param [in] rightBufferLines size of array pane right
 * @param [in] bIgnoreBlankLines, if true we allways add a new diff and make as trivial
 */
bool CDiffWrapper::FixLastDiffRange(int leftBufferLines, int rightBufferLines, bool bIgnoreBlankLines)
{
	// If one file has EOL before EOF and other not...
	if (m_status.bLeftMissingNL != m_status.bRightMissingNL)
	{
		const int count = m_pDiffList->GetSize();
		if (count > 0 && !bIgnoreBlankLines)
		{
			DIFFRANGE *dr = m_pDiffList->DiffRangeAt(count - 1);

			if (m_status.bRightMissingNL)
			{
				if (dr->op == OP_RIGHTONLY)
					dr->op = OP_DIFF;
				++dr->end0;
			}
			else
			{
				if (dr->op == OP_LEFTONLY)
					dr->op = OP_DIFF;
				++dr->end1;
			}
		}
		else 
		{
			// we have to create the DIFF
			DIFFRANGE dr;
			dr.end0 = leftBufferLines - 1;
			dr.end1 = rightBufferLines - 1;
			if (m_status.bRightMissingNL)
			{
				dr.begin0 = dr.end0;
				dr.begin1 = dr.end1 + 1;
				dr.op = OP_LEFTONLY;
			}
			else
			{
				dr.begin0 = dr.end0 + 1;
				dr.begin1 = dr.end1;
				dr.op = OP_RIGHTONLY;
			}
			if (bIgnoreBlankLines)
				dr.op = OP_TRIVIAL;
			ASSERT(dr.begin0 == dr.begin1);

			if (!AddDiffRange(dr.begin0, dr.end0, dr.begin1, dr.end1, dr.op))
				return false;
		}
	}
	return true;
}

/**
 * @brief Formats command-line for diff-engine last run (like it was called from command-line)
 */
String CDiffWrapper::FormatSwitchString()
{
	String switches;
	switch (outputStyle)
	{
	case OUTPUT_CONTEXT:
		switches = _T(" C");
		break;
	case OUTPUT_UNIFIED:
		switches = _T(" U");
		break;
	case OUTPUT_ED:
		switches = _T(" e");
		break;
	case OUTPUT_FORWARD_ED:
		switches = _T(" f");
		break;
	case OUTPUT_RCS:
		switches = _T(" n");
		break;
	case OUTPUT_NORMAL:
		switches = _T(" ");
		break;
	case OUTPUT_IFDEF:
		switches = _T(" D");
		break;
	case OUTPUT_SDIFF:
		switches = _T(" y");
		break;
	}

	if (nContext > 0)
		switches += NumToStr(nContext, 10).c_str();

	if (ignore_all_space_flag > 0)
		switches += _T("w");

	if (ignore_blank_lines_flag > 0)
		switches += _T("B");

	if (ignore_case_flag > 0)
		switches += _T("i");

	if (ignore_space_change_flag > 0)
		switches += _T("b");

	return switches;
}

/**
 * @brief Compare two files using diffutils.
 *
 * Compare two files (in DiffFileData param) using diffutils. Run diffutils
 * inside SEH so we can trap possible error and exceptions. If error or
 * execption is trapped, return compare failure.
 * @param [out] diffs Pointer to list of change structs where diffdata is stored.
 * @param [in] diffData files to compare.
 * @param [out] bin_status used to return binary status from compare.
 * @param [out] bin_file Returns which file was binary file as bitmap.
    So if first file is binary, first bit is set etc. Can be NULL if binary file
    info is not needed (faster compare since diffutils don't bother checking
    second file if first is binary).
 * @return TRUE when compare succeeds, FALSE if error happened during compare.
 * @note This function is used in file compare, not folder compare. Similar
 * folder compare function is in DiffFileData.cpp.
 */
bool CDiffWrapper::Diff2Files(struct change **diffs, file_data *inf, int *bin_status, int *bin_file)
{
	bool bRet = true;
	try
	{
		// Diff files. depth is zero because we are not 6comparing dirs
		*diffs = diff_2_files(inf, 0, bin_status, m_pMovedLines != NULL, bin_file);
	}
	catch (OException *e)
	{
		delete e;
		*diffs = NULL;
		bRet = false;
	}
	return bRet;
}

/**
 * @brief Match regular expression list against given difference.
 * This function matches the regular expression list against the difference
 * (given as start line and end line). Matching the diff requires that all
 * lines in difference match.
 * @param [in] StartPos First line of the difference.
 * @param [in] endPos Last line of the difference.
 * @param [in] FileNo File to match.
 * return number of lines matching any of the expressions.
 */
int CDiffWrapper::RegExpFilter(int StartPos, int EndPos, int FileNo, bool BreakCondition)
{
	int line = StartPos;
	while (line <= EndPos)
	{
		const char *string = files[FileNo].linbuf[line];
		int stringlen = linelen(string);
		if (FilterList::Match(stringlen, string, m_codepage) == BreakCondition)
			break;
		++line;
	}
	return line - StartPos;
}

/**
 * @brief Walk the diff utils change script, building the WinMerge list of diff blocks
 */
bool CDiffWrapper::LoadWinMergeDiffsFromDiffUtilsScript(struct change *script, const file_data *inf)
{
	//Logic needed for Ignore comment option
	String asLwrCaseExt;
	if (bFilterCommentsLines)
	{
		asLwrCaseExt = m_sOriginalFile1;
		String::size_type PosOfDot = asLwrCaseExt.rfind('.');
		if (PosOfDot != String::npos)
		{
			asLwrCaseExt.erase(0, PosOfDot + 1);
			CharLower(&asLwrCaseExt.front());
		}
	}

	struct change *next = script;
	while (next)
	{
		/* Find a set of changes that belong together.  */
		struct change *thisob = next;
		struct change *end = find_change(next);
		
#ifdef DEBUG
		debug_script(thisob);
#endif

		/* Print thisob hunk.  */
		//(*printfun) (thisob);
		/* Determine range of line numbers involved in each file.  */
		int first0 = 0, last0 = 0, first1 = 0, last1 = 0, deletes = 0, inserts = 0;

		/* Disconnect them from the rest of the changes,
		making them a hunk, and remember the rest for next iteration.  */
		next = end->link;
		end->link = 0;
		analyze_hunk(thisob, &first0, &last0, &first1, &last1, &deletes, &inserts);
		/* Reconnect the script so it will all be freed properly.  */
		end->link = next;

		if (deletes || inserts || thisob->trivial)
		{
			OP_TYPE op = deletes && inserts ? OP_DIFF :
				deletes ? OP_LEFTONLY : inserts ? OP_RIGHTONLY : OP_TRIVIAL;
				
			/* Print the lines that the first file has.  */
			int trans_a0 = 0, trans_b0 = 0, trans_a1 = 0, trans_b1 = 0;
			translate_range(&inf[0], first0, last0, &trans_a0, &trans_b0);
			translate_range(&inf[1], first1, last1, &trans_a1, &trans_b1);

			// Store information about these blocks in moved line info
			if (m_pMovedLines != NULL)
			{
				if (thisob->match0 >= 0)
				{
					ASSERT(thisob->inserted);
					for (int i = 0; i < thisob->inserted; ++i)
					{
						int line0 = i + thisob->match0 + (trans_a0 - first0 - 1);
						int line1 = i + thisob->line1 + (trans_a1 - first1 - 1);
						m_pMovedLines->Add(MovedLines::SIDE_RIGHT, line1, line0);
					}
				}
				if (thisob->match1 >= 0)
				{
					ASSERT(thisob->deleted);
					for (int i = 0; i < thisob->deleted; ++i)
					{
						int line0 = i + thisob->line0 + (trans_a0 - first0 - 1);
						int line1 = i + thisob->match1 + (trans_a1 - first1 - 1);
						m_pMovedLines->Add(MovedLines::SIDE_LEFT, line0, line1);
					}
				}
			}

			//Determine quantity of lines in this block for both sides
			int QtyLinesLeft = trans_b0 - trans_a0;
			int QtyLinesRight = trans_b1 - trans_a1;
			if (bFilterCommentsLines && (op != OP_TRIVIAL))
			{
				op = PostFilter(
					thisob->line0, QtyLinesLeft + 1,
					thisob->line1, QtyLinesRight + 1,
					op, asLwrCaseExt.c_str());
			}

			if (FilterList::HasRegExps())
			{
				int line0 = thisob->line0;
				int line1 = thisob->line1;
				int end0 = line0 + QtyLinesLeft;
				int end1 = line1 + QtyLinesRight;
				if (line0 + RegExpFilter(line0, end0, 0, false) > end0 &&
					line1 + RegExpFilter(line1, end1, 1, false) > end1)
				{
					op = OP_TRIVIAL;
				}
			}
			/*{
				// Split into slices of trivial and non-trivial diff ranges
				// (This follows a proposed but rejected patch and is left here
				// as comment for academic reference.)
				int line0 = thisob->line0;
				int line1 = thisob->line1;
				do
				{
					// Determine end positions in this block for both sides
					int end0 = line0 + (trans_b0 - trans_a0);
					int end1 = line1 + (trans_b1 - trans_a1);
					// Match lines against regular expression filters
					// Our strategy is that every line in both sides must
					// match regexp before we mark difference as ignored.
					OP_TYPE op_slice = op;
					int match0 = RegExpFilter(line0, end0, 0, true);
					int match1 = RegExpFilter(line1, end1, 1, true);
					if (match0 == 0 && match1 == 0)
					{
						op_slice = OP_TRIVIAL;
						// Optimization: Above results tell us enough about current line, so continue matching on next line
						if (end0 >= line0)
							match0 = 1 + RegExpFilter(line0 + 1, end0, 0, false);
						if (end1 >= line1)
							match1 = 1 + RegExpFilter(line1 + 1, end1, 1, false);
					}
					ASSERT(match0 != 0 || match1 != 0);
					if (!AddDiffRange(trans_a0 - 1, trans_a0 - 1 + match0 - 1, trans_a1 - 1, trans_a1 - 1 + match1 - 1, op_slice))
						return FALSE;
					trans_a0 += match0;
					trans_a1 += match1;
					line0 += match0;
					line1 += match1;
				} while (trans_a0 <= trans_b0 || trans_a1 <= trans_b1);
			}
			else*/
			{
				if (!AddDiffRange(trans_a0 - 1, trans_b0 - 1, trans_a1 - 1, trans_b1 - 1, op))
					return false;
			}
		}
	}
	return true;
}

/**
 * @brief Write out a patch file.
 * Writes patch file using already computed diffutils script. Converts path
 * delimiters from \ to / since we want to keep compatibility with patch-tools.
 * @param [in] script list of changes.
 * @param [in] inf file_data table containing filenames
 */
void CDiffWrapper::WritePatchFile(struct change * script, file_data * inf)
{
	file_data inf_patch[2];
	CopyMemory(&inf_patch, inf, sizeof inf_patch);
	
	// Get paths, primarily use alternative paths, only if they are empty
	// use full filepaths
	OString path1 = HString::Uni(!m_s1AlternativePath.empty() ?
		m_s1AlternativePath.c_str() : m_s1File.c_str())->Oct(CP_THREAD_ACP);
	OString path2 = HString::Uni(!m_s2AlternativePath.empty() ?
		m_s2AlternativePath.c_str() : m_s2File.c_str())->Oct(CP_THREAD_ACP);
	inf_patch[0].name = path1.A;
	inf_patch[1].name = path2.A;

	_tstati64(m_s1File.c_str(), &inf_patch[0].stat);
	_tstati64(m_s2File.c_str(), &inf_patch[1].stat);

	outfile = NULL;
	if (!m_sPatchFile.empty())
	{
		LPCTSTR mode = bAppendFiles ? _T("a+") : _T("w+");
		outfile = _tfopen(m_sPatchFile.c_str(), mode);
	}

	if (!outfile)
	{
		m_status.bPatchFileFailed = true;
		return;
	}

	// Print "command line"
	if (bAddCommandline)
	{
		String switches = FormatSwitchString();
		_ftprintf(outfile, _T("diff%s %s %s\n"),
			switches.c_str(), path1.A, path2.A);
	}

	// Output patchfile
	switch (output_style)
	{
	case OUTPUT_CONTEXT:
		print_context_header(inf_patch, 0);
		print_context_script(script, 0);
		break;
	case OUTPUT_UNIFIED:
		print_context_header(inf_patch, 1);
		print_context_script(script, 1);
		break;
	case OUTPUT_ED:
		print_ed_script(script);
		break;
	case OUTPUT_FORWARD_ED:
		pr_forward_ed_script(script);
		break;
	case OUTPUT_RCS:
		print_rcs_script(script);
		break;
	case OUTPUT_NORMAL:
		print_normal_script(script);
		break;
	case OUTPUT_IFDEF:
		print_ifdef_script(script);
		break;
	case OUTPUT_SDIFF:
		print_sdiff_script(script);
	}
	
	fclose(outfile);
	outfile = NULL;
}

/**
 * @brief Copy text stat results from diffutils back into the FileTextStats structure
 */
void CopyTextStats(const file_data *inf, FileTextStats *myTextStats)
{
	myTextStats->ncrlfs = inf->count_crlfs;
	myTextStats->ncrs = inf->count_crs;
	myTextStats->nlfs = inf->count_lfs;
	myTextStats->nzeros = inf->count_zeros;
}
