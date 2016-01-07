/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997-2000  Thingamahoochie Software
//    Author: Dean Grimm
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
/////////////////////////////////////////////////////////////////////////////
/** 
 * @file  MergeDoc.cpp
 *
 * @brief Implementation file for CMergeDoc
 *
 */
#include "StdAfx.h"
#include "markdown.h"
#include "Merge.h"
#include "LanguageSelect.h"
#include "MainFrm.h"
#include "DiffFileData.h"
#include "DiffTextBuffer.h"
#include "Environment.h"
#include "DiffContext.h"	// FILE_SAME
#include "MovedLines.h"
#include "Common/coretools.h"
#include "Common/defer.h"
#include "MergeEditView.h"
#include "MergeDiffDetailView.h"
#include "LocationView.h"
#include "ChildFrm.h"
#include "DirFrame.h"
#include "FileTransform.h"
#include "UniFile.h"
#include "SaveClosingDlg.h"
#include "codepage.h"
#include "paths.h"
#include "SyntaxColors.h"
#include "ProjectFile.h"
#include "FileOrFolderSelect.h"
#include "LineFiltersList.h"
#include "stream_util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/**
 * @brief Determines currently active view.
 * @return one of MERGEVIEW_INDEX_TYPE values or -1 in error.
 * @todo Detect location pane and return != 1 for it.
 */
int CChildFrame::GetActiveMergeViewIndexType() const
{
	if (GetLeftView()->HasFocus() || m_wndFilePathBar.HasFocus(0))
		return MERGEVIEW_LEFT;
	if (GetRightView()->HasFocus() || m_wndFilePathBar.HasFocus(1))
		return MERGEVIEW_RIGHT;
	if (GetLeftDetailView()->HasFocus())
		return MERGEVIEW_LEFT_DETAIL;
	if (GetRightDetailView()->HasFocus())
		return MERGEVIEW_RIGHT_DETAIL;
	return -1;
}

CGhostTextView *CChildFrame::GetActiveTextView() const
{
	if (GetLeftView()->HasFocus() || m_wndFilePathBar.HasFocus(0))
		return GetLeftView();
	if (GetRightView()->HasFocus() || m_wndFilePathBar.HasFocus(1))
		return GetRightView();
	if (GetLeftDetailView()->HasFocus())
		return GetLeftDetailView();
	if (GetRightDetailView()->HasFocus())
		return GetRightDetailView();
	return NULL;
}

/**
 * @brief Return active merge edit view (or left one if neither active)
 */
CMergeEditView *CChildFrame::GetActiveMergeView() const
{
	int nActiveViewIndexType = GetActiveMergeViewIndexType();
	switch (nActiveViewIndexType)
	{
	case -1:
		return m_pView[0];
	case MERGEVIEW_LEFT_DETAIL:
	case MERGEVIEW_RIGHT_DETAIL:
		nActiveViewIndexType -= MERGEVIEW_LEFT_DETAIL;
		break;
	}
	if (nActiveViewIndexType >= _countof(m_pView))
	{
		ASSERT(FALSE);
		return m_pView[0];
	}
	return m_pView[nActiveViewIndexType];
}

int CChildFrame::GetContextDiff(int &firstDiff, int &lastDiff)
{
	firstDiff = -1;
	lastDiff = -1;

	CMergeEditView *pView = GetActiveMergeView();

	int nDiff = m_nCurDiff;

	// No current diff, but maybe cursor is in diff?
	// Find out diff under cursor
	if (nDiff == -1)
	{
		POINT ptCursor = pView->GetCursorPos();
		nDiff = m_diffList.LineToDiff(ptCursor.y);
	}

	const int nDiffs = m_diffList.GetSignificantDiffs();
	if (nDiffs == 0)
		return nDiff;

	int firstLine, lastLine;
	pView->GetFullySelectedLines(firstLine, lastLine);
	if (lastLine < firstLine)
		return nDiff;

	firstDiff = m_diffList.LineToDiff(firstLine);
	if (firstDiff == -1)
		firstDiff = m_diffList.NextSignificantDiffFromLine(firstLine);
	lastDiff = m_diffList.LineToDiff(lastLine);
	if (lastDiff == -1)
		lastDiff = m_diffList.PrevSignificantDiffFromLine(lastLine);

	if (firstDiff != -1 && lastDiff != -1)
	{
		// Check that first selected line is first diff's first line or above it
		if (const DIFFRANGE *di = m_diffList.DiffRangeAt(firstDiff))
		{
			if ((int)di->dbegin0 < firstLine)
			{
				if (firstDiff < lastDiff)
					++firstDiff;
			}
		}
		// Check that last selected line is last diff's last line or below it
		if (const DIFFRANGE *di = m_diffList.DiffRangeAt(lastDiff))
		{
			if ((int)di->dend0 > lastLine)
			{
				if (firstDiff < lastDiff)
					--lastDiff;
			}
		}
		// Special case: one-line diff is not selected if cursor is in it
		if (firstLine == lastLine)
		{
			firstDiff = -1;
			lastDiff = -1;
		}
	}
	return nDiff;
}

/**
 * @brief Copy diff from left pane to right pane
 *
 * Difference is copied from left to right when
 * - difference is selected
 * - difference is inside selection (allows merging multiple differences).
 * - cursor is inside diff
 *
 * If there is selected diff outside selection, we copy selected
 * difference only.
 */
bool CChildFrame::OnL2r()
{
	bool done = false;
	// Check that right side is not readonly
	if (!m_ptBuf[1]->GetReadOnly()) 
	{
		int firstDiff, lastDiff;
		int currentDiff = GetContextDiff(firstDiff, lastDiff);
		if (firstDiff != -1 && lastDiff != -1 && (lastDiff >= firstDiff))
		{
			WaitStatusCursor waitstatus(IDS_STATUS_COPYL2R);
			if (currentDiff != -1 && m_diffList.IsDiffSignificant(currentDiff))
				ListCopy(0, 1, currentDiff);
			else
				CopyMultipleList(0, 1, firstDiff, lastDiff);
			done = true;
		}
		else if (currentDiff != -1 && m_diffList.IsDiffSignificant(currentDiff))
		{
			WaitStatusCursor waitstatus(IDS_STATUS_COPYL2R);
			ListCopy(0, 1, currentDiff);
			done = true;
		}
	}
	return done;
}

/**
 * @brief Copy diff from right pane to left pane
 *
 * Difference is copied from left to right when
 * - difference is selected
 * - difference is inside selection (allows merging multiple differences).
 * - cursor is inside diff
 *
 * If there is selected diff outside selection, we copy selected
 * difference only.
 */
bool CChildFrame::OnR2l()
{
	bool done = false;
	// Check that left side is not readonly
	if (!m_ptBuf[0]->GetReadOnly())
	{
		int firstDiff, lastDiff;
		int currentDiff = GetContextDiff(firstDiff, lastDiff);
		if (firstDiff != -1 && lastDiff != -1 && (lastDiff >= firstDiff))
		{
			WaitStatusCursor waitstatus(IDS_STATUS_COPYR2L);
			if (currentDiff != -1 && m_diffList.IsDiffSignificant(currentDiff))
				ListCopy(1, 0, currentDiff);
			else
				CopyMultipleList(1, 0, firstDiff, lastDiff);
			done = true;
		}
		else if (currentDiff != -1 && m_diffList.IsDiffSignificant(currentDiff))
		{
			WaitStatusCursor waitstatus(IDS_STATUS_COPYR2L);
			ListCopy(1, 0, currentDiff);
			done = true;
		}
	}
	return done;
}

/**
 * @brief Copy all diffs from right pane to left pane
 */
void CChildFrame::OnAllLeft()
{
	// Check that left side is not readonly
	if (m_ptBuf[0]->GetReadOnly())
		return;
	WaitStatusCursor waitstatus(IDS_STATUS_COPYALL2L);
	CopyAllList(1, 0);
}

/**
 * @brief Copy all diffs from left pane to right pane
 */
void CChildFrame::OnAllRight()
{
	// Check that right side is not readonly
	if (m_ptBuf[1]->GetReadOnly())
		return;
	WaitStatusCursor waitstatus(IDS_STATUS_COPYALL2R);
	CopyAllList(0, 1);
}

void CChildFrame::SetUnpacker(PackingInfo * infoNewHandler)
{
	if (infoNewHandler)
	{
		*m_pInfoUnpacker = *infoNewHandler;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CMergeDoc commands

/**
 * @brief Save an editor text buffer to a file for prediffing (make UCS-2LE if appropriate)
 *
 * @note 
 * original file is Ansi : 
 *   buffer  -> save as Ansi -> Ansi plugins -> diffutils
 * original file is Unicode (UCS2-LE, UCS2-BE, UTF-8) :
 *   buffer  -> save as UCS2-LE -> Unicode plugins -> convert to UTF-8 -> diffutils
 * (the plugins are optional, not the conversion)
 * @todo Show SaveToFile() errors?
 */
static void SaveBuffForDiff(CDiffTextBuffer &buf, bool bUnicode,
	DiffFileData &diffdata, int i, int nStartLine = 0, int nLines = -1)
{
	int orig_codepage = buf.getCodepage();
	UNICODESET orig_unicoding = buf.getUnicoding();

	size_t alloc_extra = 0;
	// If file was in Unicode
	if (bUnicode)
	{
		// we subvert the buffer's memory of the original file encoding
		buf.setUnicoding(UCS2LE);  // write as UCS-2LE (for preprocessing)
		buf.setCodepage(0); // should not matter
		alloc_extra = ~0U; // allocate extra room for transcoding
	}

	// write buffer out to temporary file
	String sError;
	NulWriteStream nws;
	buf.SaveToFile(NULL, &nws, sError, NULL, CRLF_STYLE_AUTOMATIC, nStartLine, nLines);
	ULONG len = nws.GetSize();
	if (void *buffer = diffdata.AllocBuffer(i, len, alloc_extra))
	{
		MemWriteStream mws(buffer, len);
		buf.SaveToFile(NULL, &mws, sError, NULL, CRLF_STYLE_AUTOMATIC, nStartLine, nLines);
	}

	// restore memory of encoding of original file
	buf.setUnicoding(orig_unicoding);
	buf.setCodepage(orig_codepage);
}

/**
 * @brief Save files to temp files & compare again.
 *
 * [out] If TRUE binary file was detected.
 * @param bIdentical [out] If TRUE files were identical
 * @param bForced [in] If TRUE, suppressing is ignored and rescan
 * is done always
 * @return Tells if rescan was successfully, was suppressed, or
 * error happened
 * If this code is OK, Rescan has detached the views temporarily
 * (positions of cursors have been lost)
 * @note Rescan() ALWAYS compares temp files. Actual user files are not
 * touched by Rescan().
 * @sa CDiffWrapper::RunFileDiff()
 */
int CChildFrame::Rescan(bool &bIdentical, bool bForced)
{
	DiffFileInfo fileInfo;

	if (!bForced)
	{
		if (m_bLockRescan)
			return RESCAN_SUPPRESSED;
	}

	// Check if files have been modified since last rescan
	// Ignore checking in case of scratchpads (empty filenames)
	int nSide = 0;
	do
	{
		const LPCTSTR path = m_strPath[nSide].c_str();
		const FileChange fileChanged = path[0] ?
			IsFileChangedOnDisk(path, fileInfo, m_pRescanFileInfo[nSide]) : FileNoChange;

		if (fileChanged == FileRemoved)
		{
			// Silently ignore removal of files which were not opened for
			// editing. This avoids annoying nag boxes when TortoiseHG or TFS
			// invoke WinMerge to compare a temporarily retrieved source file
			// against its working copy and the latter is being edited.
			if (!m_ptBuf[nSide]->GetReadOnly())
			{
				LanguageSelect.FormatStrings(
					IDS_FILE_DISAPPEARED, paths_UndoMagic(wcsdupa(path))
				).MsgBox(MB_ICONWARNING);
				if (!DoSaveAs(nSide))
					return RESCAN_FILE_ERR;
			}
		}
		else if (fileChanged == FileChanged)
		{
			int response = LanguageSelect.FormatStrings(
				IDS_FILECHANGED_RESCAN, paths_UndoMagic(wcsdupa(path))
			).MsgBox(MB_YESNO | MB_ICONWARNING);
			if (response == IDYES)
			{
				FileLoadResult::FILES_RESULT result = ReloadDoc(nSide);
				if (FileLoadResult::IsError(result))
					return RESCAN_FILE_ERR;
			}
		}
	} while (nSide ^= 1);

	return Rescan2(bIdentical);
}

int CChildFrame::Rescan2(bool &bIdentical)
{
	GetSystemTimeAsFileTime(&m_LastRescan);

	DiffFileData diffdata(getDefaultCodepage());

	// Clear diff list
	m_diffList.Clear();
	m_nCurDiff = -1;
	// Clear moved lines lists
	if (MovedLines *pMovedLines = m_diffWrapper.GetMovedLines())
		pMovedLines->Clear();

	int nBuffer;

	bool bUnicode = false;
	for (nBuffer = 0; nBuffer < m_nBuffers; nBuffer++)
		if (m_ptBuf[nBuffer]->getUnicoding() != NONE)
			bUnicode = true;

	// Set paths for diffing and run diff
	m_diffWrapper.SetCompareFiles(m_strPath[0], m_strPath[1]);
	m_diffWrapper.SetCodepage(bUnicode ? CP_UTF8 : m_ptBuf[0]->m_encoding.m_codepage);

	int nResult = RESCAN_FILE_ERR;
	bool diffSuccess = false;

	if (!HasSyncPoints())
	{
		for (nBuffer = 0; nBuffer < m_nBuffers; nBuffer++)
			SaveBuffForDiff(*m_ptBuf[nBuffer], bUnicode, diffdata, nBuffer);
		diffSuccess = m_diffWrapper.RunFileDiff(diffdata);
	}
	else
	{
		int nSyncPoint[MERGE_VIEW_COUNT];
		int nApparentStartLine[MERGE_VIEW_COUNT];
		int nRealStartLine[MERGE_VIEW_COUNT];
		for (nBuffer = 0; nBuffer < m_nBuffers; nBuffer++)
		{
			nSyncPoint[nBuffer] = -1;
			nApparentStartLine[nBuffer] = 0;
			nRealStartLine[nBuffer] = 0;
		}
		int nDiff = 0;
		for (;;)
		{
			int nApparentLines[MERGE_VIEW_COUNT];
			int nRealLines[MERGE_VIEW_COUNT];

			bool bContinue = false;
			for (nBuffer = 0; nBuffer < m_nBuffers; nBuffer++)
			{
				nApparentLines[nBuffer] = 0;
				nRealLines[nBuffer] = 0;
				const CDiffTextBuffer *const pBuf = m_ptBuf[nBuffer];
				const int nLineCount = pBuf->GetLineCount();
				if (nSyncPoint[nBuffer] < nLineCount)
				{
					do if (nSyncPoint[nBuffer] >= 0)
					{
						++nApparentLines[nBuffer];
						if ((pBuf->GetLineFlags(nSyncPoint[nBuffer]) & LF_GHOST) == 0)
							++nRealLines[nBuffer];
					} while (++nSyncPoint[nBuffer] < nLineCount &&
						(pBuf->GetLineFlags(nSyncPoint[nBuffer]) & LF_INVALID_BREAKPOINT) == 0);
					bContinue = true;
				}
			}

			if (!bContinue)
				break;

			for (nBuffer = 0; nBuffer < m_nBuffers; nBuffer++)
			{
				SaveBuffForDiff(*m_ptBuf[nBuffer], bUnicode, diffdata, nBuffer,
					nApparentStartLine[nBuffer], nApparentLines[nBuffer]);
			}

			diffSuccess = m_diffWrapper.RunFileDiff(diffdata);
			nDiff = m_diffList.FinishSyncPoint(nDiff, nRealStartLine);

			for (nBuffer = 0; nBuffer < m_nBuffers; nBuffer++)
			{
				nApparentStartLine[nBuffer] += nApparentLines[nBuffer];
				nRealStartLine[nBuffer] += nRealLines[nBuffer];
			}
		}
	}
	// set identical/diff result as recorded by diffutils
	bIdentical = m_diffWrapper.m_status.bIdentical;
	// Determine errors and binary file compares
	if (diffSuccess && !m_diffWrapper.m_status.bBinaries)
	{
		nResult = RESCAN_OK;
		// Now update views and buffers for ghost lines

		// Prevent displaying views during this update 
		// BTW, this solves the problem of double asserts
		// (during the display of an assert message box, a second assert in one of the 
		//  display functions happens, and hides the first assert)
		m_pView[0]->DetachFromBuffer();
		m_pView[1]->DetachFromBuffer();
		m_pDetailView[0]->DetachFromBuffer();
		m_pDetailView[1]->DetachFromBuffer();

		// Remove blank lines and clear winmerge flags
		// this operation does not change the modified flag
		m_ptBuf[0]->prepareForRescan();
		m_ptBuf[1]->prepareForRescan();

		// ..lasf DIFFRANGE of file which has EOL must be
		// fixed to contain last line too
		if (!m_diffWrapper.FixLastDiffRange(
				m_ptBuf[0]->GetLineCount(),
				m_ptBuf[1]->GetLineCount(),
				m_diffWrapper.bIgnoreBlankLines))
		{
			nResult = RESCAN_FILE_ERR;
		}

		// Divide diff blocks to match lines.
		if (COptionsMgr::Get(OPT_CMP_MATCH_SIMILAR_LINES))
			AdjustDiffBlocks();

		// Analyze diff-list (updating real line-numbers)
		// this operation does not change the modified flag
		PrimeTextBuffers();

		// Apply flags to lines that moved, to differentiate from appeared/disappeared lines
		if (MovedLines *pMovedLines = m_diffWrapper.GetMovedLines())
			FlagMovedLines(pMovedLines, m_ptBuf[0], m_ptBuf[1]);
		
		// After PrimeTextBuffers() we know amount of real diffs
		// (m_nDiffs) and trivial diffs (m_nTrivialDiffs)

		// Identical files are also updated
		if (m_diffList.FirstSignificantDiff() == -1)
			bIdentical = true;

		// Now buffers data are valid
		m_pView[0]->ReAttachToBuffer(m_ptBuf[0]);
		m_pView[1]->ReAttachToBuffer(m_ptBuf[1]);
		m_pDetailView[0]->ReAttachToBuffer(m_ptBuf[0]);
		m_pDetailView[1]->ReAttachToBuffer(m_ptBuf[1]);

		m_bEditAfterRescan[0] = false;
		m_bEditAfterRescan[1] = false;
	}

	bool bAllowmixedEOL = COptionsMgr::Get(OPT_ALLOW_MIXED_EOL);
	m_pView[0]->SetCRLFModeStatus(
		bAllowmixedEOL && m_ptBuf[0]->IsMixedEOL() ?
		CRLF_STYLE_MIXED : m_ptBuf[0]->GetCRLFMode());
	m_pView[1]->SetCRLFModeStatus(
		bAllowmixedEOL && m_ptBuf[1]->IsMixedEOL() ?
		CRLF_STYLE_MIXED : m_ptBuf[1]->GetCRLFMode());

	SetLastCompareResult(m_diffList.GetSignificantDiffs());

	m_pFileTextStats[0] = diffdata.m_textStats[0];
	m_pFileTextStats[1] = diffdata.m_textStats[1];

	m_pRescanFileInfo[0].Update(m_strPath[0].c_str());
	m_pRescanFileInfo[1].Update(m_strPath[1].c_str());

	return nResult;
}

/** @brief Adjust all different lines that were detected as actually matching moved lines */
void CChildFrame::FlagMovedLines(MovedLines *pMovedLines,
	CDiffTextBuffer *pBuffer1, CDiffTextBuffer *pBuffer2)
{
	int i;
	for (i = 0; i < pBuffer1->GetLineCount(); ++i)
	{
		int j = pMovedLines->LineInBlock(i, MovedLines::SIDE_LEFT);
		if (j != -1)
		{
			TRACE("%d->%d\n", i, j);
			ASSERT(j>=0);
			// We only flag lines that are already marked as being different
			int apparent = pBuffer1->ComputeApparentLine(i);
			LineInfo &info = pBuffer1->m_aLines[apparent];
			if (info.m_dwFlags & LF_DIFF)
			{
				info.m_dwFlags |= LF_MOVED;
			}
		}
	}

	for (i = 0; i < pBuffer2->GetLineCount(); ++i)
	{
		int j = pMovedLines->LineInBlock(i, MovedLines::SIDE_RIGHT);
		if (j != -1)
		{
			TRACE("%d->%d\n", i, j);
			ASSERT(j>=0);
			// We only flag lines that are already marked as being different
			int apparent = pBuffer2->ComputeApparentLine(i);
			LineInfo &info = pBuffer2->m_aLines[apparent];
			if (info.m_dwFlags & LF_DIFF)
			{
				info.m_dwFlags |= LF_MOVED;
			}
		}
	}

	// todo: Need to record actual moved information
}

/**
 * @brief Prints (error) message by rescan status.
 *
 * @param nRescanResult [in] Resultcocode from rescan().
 * @param bIdentical [in] Were files identical?.
 * @sa CChildFrame::Rescan()
 */
void CChildFrame::ShowRescanError(int nRescanResult, BOOL bIdentical)
{
	// Rescan was suppressed, there is no sensible status
	switch (nRescanResult)
	{
	case RESCAN_SUPPRESSED:
		return;
	case RESCAN_FILE_ERR:
		LanguageSelect.MsgBox(IDS_FILEERROR, MB_ICONSTOP);
		return;
	case RESCAN_TEMP_ERR:
		LanguageSelect.MsgBox(IDS_TEMP_FILEERROR, MB_ICONSTOP);
		return;
	}
	// Files are not binaries, but they are identical
	if (bIdentical)
		AlertFilesIdentical();
}

/**
 * @brief Copy all diffs from one side to side.
 * @param [in] srcPane Source side from which diff is copied
 * @param [in] dstPane Destination side
 */
void CChildFrame::CopyAllList(int srcPane, int dstPane)
{
	CopyMultipleList(srcPane, dstPane, 0, m_diffList.GetSize() - 1);
}

/**
 * @brief Copy range of diffs from one side to side.
 * This function copies given range of differences from side to another.
 * Ignored differences are skipped, and not copied.
 * @param [in] srcPane Source side from which diff is copied
 * @param [in] dstPane Destination side
 * @param [in] firstDiff First diff copied (0-based index)
 * @param [in] lastDiff Last diff copied (0-based index)
 */
void CChildFrame::CopyMultipleList(int srcPane, int dstPane, int firstDiff, int lastDiff)
{
#ifdef _DEBUG
	if (firstDiff > lastDiff)
		_RPTF0(_CRT_ERROR, "Invalid diff range (firstDiff > lastDiff)!");
	if (firstDiff < 0)
		_RPTF0(_CRT_ERROR, "Invalid diff range (firstDiff < 0)!");
	if (lastDiff > m_diffList.GetSize() - 1)
		_RPTF0(_CRT_ERROR, "Invalid diff range (lastDiff < diffcount)!");
#endif

	lastDiff = min(m_diffList.GetSize() - 1, lastDiff);
	firstDiff = max(0, firstDiff);
	if (firstDiff > lastDiff)
		return;
	
	// Note we don't care about m_nDiffs count to become zero,
	// because we don't rescan() so it does not change

	bool bGroupWithPrevious = false;
	// copy from bottom up is more efficient
	for (int i = lastDiff; i >= firstDiff; --i)
	{
		if (m_diffList.IsDiffSignificant(i))
		{
			defer::Decrement<BYTE> Decrement = { &++m_bLockRescan };
			if (!ListCopy(srcPane, dstPane, i, bGroupWithPrevious))
				return; // sync failure
			// Group merge with previous (merge undo data to one action)
			bGroupWithPrevious = true;
		}
	}

	FlushAndRescan();
}

/**
 * @brief Sanity check difference.
 *
 * Checks that lines in difference are inside difference in both files.
 * If file is edited, lines added or removed diff lines get out of sync and
 * merging fails miserably.
 *
 * @param [in] dr Difference to check.
 * @return TRUE if difference lines match, FALSE otherwise.
 */
bool CChildFrame::SanityCheckDiff(const DIFFRANGE *pdr) const
{
	const int cd_dbegin = pdr->dbegin0;
	const int cd_dend = pdr->dend0;

	// Must ensure line number is in range before getting line flags
	if (cd_dend >= m_ptBuf[0]->GetLineCount())
		return false;
	DWORD dwLeftFlags = m_ptBuf[0]->GetLineFlags(cd_dend);

	// Must ensure line number is in range before getting line flags
	if (cd_dend >= m_ptBuf[1]->GetLineCount())
		return false;
	DWORD dwRightFlags = m_ptBuf[1]->GetLineFlags(cd_dend);

	// Optimization - check last line first so we don't need to
	// check whole diff for obvious cases
	if (!(dwLeftFlags & LF_WINMERGE_FLAGS) ||
		!(dwRightFlags & LF_WINMERGE_FLAGS))
	{
		return false;
	}

	for (int line = cd_dbegin; line < cd_dend; line++)
	{
		dwLeftFlags = m_ptBuf[0]->GetLineFlags(cd_dend);
		dwRightFlags = m_ptBuf[1]->GetLineFlags(cd_dend);
		if (!(dwLeftFlags & LF_WINMERGE_FLAGS) ||
			!(dwRightFlags & LF_WINMERGE_FLAGS))
		{
			return false;
		}
	}
	return true;
}

/**
 * @brief Copy selected (=current) difference from from side to side.
 * @param [in] srcPane Source side from which diff is copied
 * @param [in] dstPane Destination side
 * @param [in] nDiff Diff to copy, if -1 function determines it.
 * @param [in] bGroupWithPrevious Adds diff to same undo group with
 * @return true if ok, false if sync failure & need to abort copy
 * previous action (allows one undo for copy all)
 */
bool CChildFrame::ListCopy(int srcPane, int dstPane, int nDiff /* = -1*/,
		bool bGroupWithPrevious /*= false*/)
{
	int firstDiff, lastDiff;
	// If diff-number not given, determine it from active view
	if (nDiff != -1 || (nDiff = GetContextDiff(firstDiff, lastDiff)) != -1)
	{
		// suppress Rescan during this method
		// (Not only do we not want to rescan a lot of times, but
		// it will wreck the line status array to rescan as we merge)
		defer::Decrement<BYTE> Decrement = { &++m_bLockRescan };
		const DIFFRANGE *const pcd = m_diffList.DiffRangeAt(nDiff);
		CDiffTextBuffer *const sbuf = m_ptBuf[srcPane];
		CDiffTextBuffer *const dbuf = m_ptBuf[dstPane];
		const bool bSrcWasMod = sbuf->IsModified();
		const int cd_dbegin = srcPane == 0 ? pcd->dbegin0 : pcd->dbegin1;
		int cd_dend = srcPane == 0 ? pcd->dend0 : pcd->dend1;
		bool bInSync = SanityCheckDiff(pcd);

		if (!bInSync)
		{
			LanguageSelect.MsgBox(IDS_VIEWS_OUTOFSYNC, MB_ICONSTOP);
			return false; // abort copying
		}

		// curView is the view which is changed, so the opposite of the source view
		CMergeEditView *const dstView = m_pView[dstPane];

		// If we remove whole diff from current view, we must fix cursor
		// position first. Normally we would move to end of previous line,
		// but we want to move to begin of that line for usability.
		if ((pcd->op == OP_LEFTONLY && dstPane == 0) ||
			(pcd->op == OP_RIGHTONLY && dstPane == 1))
		{
			POINT currentPos = dstView->GetCursorPos();
			currentPos.x = 0;
			if (currentPos.y > 0)
				--currentPos.y;
			dstView->SetCursorPos(currentPos);
		}

		dbuf->BeginUndoGroup(bGroupWithPrevious);

		ASSERT(cd_dend >= cd_dbegin);

		const CRLFSTYLE nCrlfStyle = dbuf->GetCRLFMode();
		const LPCTSTR pszCRLF = nCrlfStyle != CRLF_STYLE_MIXED ?
			CCrystalTextBuffer::GetStringEol(nCrlfStyle) : NULL;

		String strLine;
		int cd_dpastend = cd_dend + 1;
		sbuf->GetText(cd_dbegin, 0,
			cd_dpastend < sbuf->GetLineCount() ? cd_dpastend : cd_dend,
			cd_dpastend < sbuf->GetLineCount() ? 0 : sbuf->GetLineLength(cd_dend),
			strLine, pszCRLF);
		dbuf->DeleteText(dstView, cd_dbegin, 0,
			cd_dpastend < dbuf->GetLineCount() ? cd_dpastend : cd_dend,
			cd_dpastend < dbuf->GetLineCount() ? 0 : dbuf->GetLineLength(cd_dend),
			CE_ACTION_MERGE);
		if (int cchText = strLine.length())
			dbuf->InsertText(dstView, cd_dbegin, 0, strLine.c_str(), cchText, CE_ACTION_MERGE);

		dbuf->FlushUndoGroup(dstView);

		// remove the diff
		SetCurrentDiff(-1);

		// reset the mod status of the source view because we do make some
		// changes, but none that concern the source text
		sbuf->SetModified(bSrcWasMod);
	}

	FlushAndRescan();
	return true;
}

/**
 * @brief Save file with new filename.
 *
 * This function is called by CChildFrame::DoSave() or CChildFrame::DoSaveAs()
 * to save file with new filename. CChildFrame::DoSave() calls if saving with
 * normal filename fails, to let user choose another filename/location.
 * Also, if file is unnamed file (e.g. scratchpad) then it must be saved
 * using this function.
 * @param [in, out] strPath 
 * - [in] : Initial path shown to user
 * - [out] : Path to new filename if saving succeeds
 * @param [in, out] nSaveResult 
 * - [in] : Statuscode telling why we ended up here. Maybe the result of
 * previous save.
 * - [out] : Statuscode of this saving try
 * @param [in, out] sError Error string from lower level saving code
 * @param [in] nBuffer Buffer we are saving
 * @return FALSE as long as the user is not satisfied. Calling function
 * should not continue until TRUE is returned.
 * @sa CChildFrame::DoSave()
 * @sa CChildFrame::DoSaveAs()
 * @sa CChildFrame::CDiffTextBuffer::SaveToFile()
 */
bool CChildFrame::TrySaveAs(String &strPath, int &nSaveResult, String & sError,
	int nBuffer, PackingInfo * pInfoTempUnpacker)
{
	String strSavePath = strPath; // New path for next saving try
	bool result = true;
	int response = IDOK; // Set default we use for scratchpads
	int nActiveViewIndexType = GetActiveMergeViewIndexType();

	if (nActiveViewIndexType == -1)
	{
		// We don't have valid view active, but still tried to save.
		// We don't know which file to save, so just cancel.
		// Possible origin in location pane?
		_RPTF0(_CRT_ERROR, "Save request from invalid view!");
		nSaveResult = SAVE_CANCELLED;
		return true;
	}

	HWND parent = m_pView[nActiveViewIndexType]->m_hWnd;

	// We shouldn't get here if saving is succeed before
	ASSERT(nSaveResult != SAVE_DONE);

	// SAVE_NO_FILENAME is temporarily used for scratchpad.
	// So don't ask about saving in that case.
	if (nSaveResult != SAVE_NO_FILENAME)
	{
		response = LanguageSelect.FormatStrings(IDS_FILESAVE_FAILED,
			paths_UndoMagic(wcsdupa(strPath.c_str())), sError.c_str()
		).MsgBox(MB_OKCANCEL | MB_ICONWARNING);
	}

	switch (response)
	{
	case IDOK:
		if (SelectFile(parent, strSavePath,
			nBuffer == 0 ? IDS_SAVE_LEFT_AS : IDS_SAVE_RIGHT_AS,
			NULL, FALSE))
		{
			nSaveResult = m_ptBuf[nBuffer]->SaveToFile(
				strSavePath.c_str(), NULL, sError, pInfoTempUnpacker);

			if (nSaveResult == SAVE_DONE)
			{
				// We are saving scratchpad (unnamed file)
				if (strPath.empty())
				{
					m_nBufferType[nBuffer] = BUFFER_UNNAMED_SAVED;
					m_strDesc[nBuffer].clear();
				}
				strPath = strSavePath;
				UpdateHeaderPath(nBuffer);
			}
			else
				result = false;
		}
		else
			nSaveResult = SAVE_CANCELLED;
		break;

	case IDCANCEL:
		nSaveResult = SAVE_CANCELLED;
		break;
	}
	return result;
}

/**
 * @brief Save file creating backups etc.
 *
 * Safe top-level file saving function. Checks validity of given path.
 * Creates backup file if wanted to. And if saving to given path fails,
 * allows user to select new location/name for file.
 * @param [in] szPath Path where to save including filename. Can be
 * empty/NULL if new file is created (scratchpad) without filename.
 * @param [out] bSaveSuccess Will contain information about save success with
 * the original name (to determine if file statuses should be changed)
 * @param [in] nBuffer Index (0-based) of buffer to save
 * @return Tells if caller can continue (no errors happened)
 * @note Return value does not tell if SAVING succeeded. Caller must
 * Check value of bSaveSuccess parameter after calling this function!
 * @note If CMainFrame::m_strSaveAsPath is non-empty, file is saved
 * to directory it points to. If m_strSaveAsPath contains filename,
 * that filename is used.
 * @sa CChildFrame::TrySaveAs()
 * @sa CMainFrame::CheckSavePath()
 * @sa CChildFrame::CDiffTextBuffer::SaveToFile()
 */
bool CChildFrame::DoSave(bool &bSaveSuccess, int nBuffer)
{
	if (m_pOpener && m_nBufferType[nBuffer] == BUFFER_UNNAMED)
	{
		CMergeEditView *const pView = m_pView[nBuffer];
		const int nEndLine = pView->GetLineCount() - 1;
		const int nEndChar = pView->GetLineLength(nEndLine);
		String text;
		pView->GetText(0, 0, nEndLine, nEndChar, text);
		m_pOpener->m_pView[nBuffer]->ReplaceSelection(text.c_str(), text.length());
		CDiffTextBuffer *const pBuf = m_ptBuf[nBuffer];
		pBuf->m_nSyncPosition = pBuf->m_nUndoPosition;
		pBuf->SetModified(false);
		// remember revision number on save
		pBuf->m_dwRevisionNumberOnSave = pBuf->m_dwCurrentRevisionNumber;
		// redraw line revision marks
		pBuf->UpdateViews(NULL, NULL, UPDATE_FLAGSONLY);
		// remove asterisk from header
		UpdateHeaderPath(nBuffer);
		return true;
	}

	DiffFileInfo fileInfo;
	LPCTSTR szPath = m_strPath[nBuffer].c_str();
	String strSavePath = szPath;

	FileChange fileChanged = IsFileChangedOnDisk(szPath, fileInfo, m_pSaveFileInfo[nBuffer]);
	if (fileChanged == FileChanged)
	{
		int response = LanguageSelect.FormatStrings(
			IDS_FILECHANGED_ONDISK, paths_UndoMagic(wcsdupa(szPath))
		).MsgBox(MB_ICONWARNING | MB_YESNO);
		if (response == IDNO)
		{
			bSaveSuccess = false;
			return true;
		}		
	}

	// use a temp packer
	// first copy the m_pInfoUnpacker
	// if an error arises during packing, change and take a "do nothing" packer
	PackingInfo infoTempUnpacker = *m_pInfoUnpacker;

	bSaveSuccess = false;
	
	// Check third arg possibly given from command-line
	if (!m_pInfoUnpacker->saveAsPath.empty())
	{
		strSavePath = m_pInfoUnpacker->saveAsPath;
		if (paths_DoesPathExist(strSavePath.c_str()) == IS_EXISTING_DIR)
		{
			// third arg was a directory, so get append the filename
			LPCTSTR name = PathFindFileName(szPath);
			strSavePath = paths_ConcatPath(strSavePath, name);
		}
	}

	if (DWORD attr = paths_IsReadonlyFile(strSavePath.c_str()))
		if (m_pMDIFrame->HandleReadonlySave(attr, strSavePath) == IDCANCEL)
			return false;

	if (!m_pMDIFrame->CreateBackup(false, strSavePath.c_str()))
		return false;

	// FALSE as long as the user is not satisfied
	// TRUE if saving succeeds, even with another filename, or if the user cancels
	bool result = false;
	// the error code from the latest save operation, 
	// or SAVE_DONE when the save succeeds
	// TODO: Shall we return this code in addition to bSaveSuccess ?
	int nSaveResult = SAVE_NO_FILENAME;

	String sError;
	if (m_nBufferType[nBuffer] != BUFFER_UNNAMED)
	{
		// We have a filename, just try to save
		nSaveResult = m_ptBuf[nBuffer]->SaveToFile(
			strSavePath.c_str(), NULL, sError, &infoTempUnpacker);
	}

	if (nSaveResult != SAVE_DONE)
	{
		// Saving failed, user may save to another location if wants to
		do
			result = TrySaveAs(strSavePath, nSaveResult, sError, nBuffer, &infoTempUnpacker);
		while (!result);
	}

	// Saving succeeded with given/selected filename
	if (nSaveResult == SAVE_DONE)
	{
		// Preserve file times if user wants to
		if (COptionsMgr::Get(OPT_PRESERVE_FILETIMES))
		{
			fileInfo.ApplyFileTimeTo(strSavePath.c_str());
		}
		m_ptBuf[nBuffer]->SetModified(false);
		m_pSaveFileInfo[nBuffer].Update(strSavePath.c_str());
		m_strPath[nBuffer] = strSavePath;
		m_pRescanFileInfo[nBuffer].Update(strSavePath.c_str());
		UpdateHeaderPath(nBuffer);
		m_pInfoUnpacker->saveAsPath.clear();
		bSaveSuccess = true;
		result = true;
	}
	else if (nSaveResult == SAVE_CANCELLED)
	{
		// User cancelled current operation, lets do what user wanted to do
		result = false;
	}
	return result;
}

/**
 * @brief Save file with different filename.
 *
 * Safe top-level file saving function. Asks user to select filename
 * and path. Does not create backups.
 * @param [in] szPath Path where to save including filename. Can be
 * empty/NULL if new file is created (scratchpad) without filename.
 * @param [out] bSaveSuccess Will contain information about save success with
 * the original name (to determine if file statuses should be changed)
 * @param [in] nBuffer Index (0-based) of buffer to save
 * @return Tells if caller can continue (no errors happened)
 * @note Return value does not tell if SAVING succeeded. Caller must
 * Check value of bSaveSuccess parameter after calling this function!
 * @sa CChildFrame::TrySaveAs()
 * @sa CMainFrame::CheckSavePath()
 * @sa CChildFrame::CDiffTextBuffer::SaveToFile()
 */
bool CChildFrame::DoSaveAs(int nBuffer)
{
	String strSavePath = m_strPath[nBuffer];

	// use a temp packer
	// first copy the m_pInfoUnpacker
	// if an error arises during packing, change and take a "do nothing" packer
	PackingInfo infoTempUnpacker = *m_pInfoUnpacker;

	bool bSaveSuccess = false;
	// FALSE as long as the user is not satisfied
	// TRUE if saving succeeds, even with another filename, or if the user cancels
	bool result = false;
	// the error code from the latest save operation, 
	// or SAVE_DONE when the save succeeds

	// Use SAVE_NO_FILENAME to prevent asking about error
	int nSaveResult = SAVE_NO_FILENAME;

	// Loop until user succeeds saving or cancels
	String sError;
	do
		result = TrySaveAs(strSavePath, nSaveResult, sError, nBuffer, &infoTempUnpacker);
	while (!result);

	// Saving succeeded with given/selected filename
	if (nSaveResult == SAVE_DONE)
	{
		m_pSaveFileInfo[nBuffer].Update(strSavePath.c_str());
		m_strPath[nBuffer] = strSavePath;
		m_pRescanFileInfo[nBuffer].Update(strSavePath.c_str());
		UpdateHeaderPath(nBuffer);
		bSaveSuccess = true;
	}
	return bSaveSuccess;
}

/**
 * @brief Get left->right info for a moved line (apparent line number)
 */
int CChildFrame::RightLineInMovedBlock(int apparentLine)
{
	int movedLine = -1;
	if (m_ptBuf[0]->GetLineFlags(apparentLine) & LF_MOVED)
	{
		if (MovedLines *pMovedLines = m_diffWrapper.GetMovedLines())
		{
			int realLine = m_ptBuf[0]->ComputeRealLine(apparentLine);
			movedLine = pMovedLines->LineInBlock(realLine, MovedLines::SIDE_LEFT);
			if (movedLine != -1)
				movedLine = m_ptBuf[1]->ComputeApparentLine(movedLine);
		}
	}
	return movedLine;
}

/**
 * @brief Get right->left info for a moved line (apparent line number)
 */
int CChildFrame::LeftLineInMovedBlock(int apparentLine)
{
	int movedLine = -1;
	if (m_ptBuf[1]->GetLineFlags(apparentLine) & LF_MOVED)
	{
		if (MovedLines *pMovedLines = m_diffWrapper.GetMovedLines())
		{
			int realLine = m_ptBuf[1]->ComputeRealLine(apparentLine);
			movedLine = pMovedLines->LineInBlock(realLine, MovedLines::SIDE_RIGHT);
			if (movedLine != -1)
				movedLine = m_ptBuf[0]->ComputeApparentLine(movedLine);
		}
	}
	return movedLine;
}

/**
 * @brief Save modified documents.
 * This function asks if user wants to save modified documents. We also
 * allow user to cancel the closing.
 *
 * There is a special trick avoiding showing two save-dialogs, as MFC framework
 * sometimes calls this function twice. We use static counter for these calls
 * and if we already have saving in progress (counter == 1) we skip the new
 * saving dialog.
 *
 * @return TRUE if docs are closed, FALSE if closing is cancelled.
 */
bool CChildFrame::SaveModified()
{
	return PromptAndSaveIfNeeded(true);
}

/**
 * @brief Sets the current difference.
 * @param [in] nDiff Difference to set as current difference.
 */
void CChildFrame::SetCurrentDiff(int nDiff)
{
	if (nDiff >= 0 && nDiff <= m_diffList.LastSignificantDiff())
		m_nCurDiff = nDiff;
	else
		m_nCurDiff = -1;
	UpdateCmdUI();
	OnUpdateStatusNum();
}

/**
 * @brief Take care of rescanning document.
 * 
 * Update view and restore cursor and scroll position after
 * rescanning document.
 * @param [in] bForced If TRUE rescan cannot be suppressed
 */
void CChildFrame::FlushAndRescan(bool bForced)
{
	if (m_idContextLines < ID_VIEW_CONTEXT_UNLIMITED)
		return;

	// Ignore suppressing when forced rescan
	if (!bForced && m_bLockRescan)
		return;

	WaitStatusCursor waitstatus(IDS_STATUS_RESCANNING);

	CGhostTextView *const pActiveView = GetActiveTextView();

	// store cursors and hide caret
	m_pView[0]->PushCursors();
	m_pView[1]->PushCursors();
	m_pDetailView[0]->PushCursors();
	m_pDetailView[1]->PushCursors();
	if (pActiveView)
		pActiveView->HideCursor();

	bool bIdentical = false;
	int nRescanResult = Rescan(bIdentical, bForced);

	// restore cursors and caret
	m_pView[0]->PopCursors();
	m_pView[1]->PopCursors();
	m_pDetailView[0]->PopCursors();
	m_pDetailView[1]->PopCursors();
	if (pActiveView)
		pActiveView->ShowCursor();

	// because of ghostlines, m_nTopLine may differ just after Rescan
	// scroll both views to the same top line
	AlignScrollPositions();

	// Refresh display
	UpdateAllViews();
	UpdateCmdUI();

	// Show possible error after updating screen
	if (nRescanResult != RESCAN_SUPPRESSED)
		ShowRescanError(nRescanResult, bIdentical);

	GetSystemTimeAsFileTime(&m_LastRescan);
}

/**
 * @brief Saves both files
 */
void CChildFrame::OnFileSave()
{
	// We will need to know if either of the originals actually changed
	// so we know whether to update the diff status
	bool bLChangedOriginal = false;
	bool bRChangedOriginal = false;

	const bool bROLeft = m_ptBuf[0]->GetReadOnly();
	const bool bRORight = m_ptBuf[1]->GetReadOnly();

	if (!m_pInfoUnpacker->saveAsPath.empty() && bRORight > bROLeft || m_ptBuf[0]->IsSaveable())
	{
		// (why we don't use return value of DoSave)
		// DoSave will return TRUE if it wrote to something successfully
		// but we have to know if it overwrote the original file
		DoSave(bLChangedOriginal, 0);
	}

	if (!m_pInfoUnpacker->saveAsPath.empty() && bROLeft > bRORight || m_ptBuf[1]->IsSaveable())
	{
		// See comments above for left case
		DoSave(bRChangedOriginal, 1);
	}

	// If either of the actual source files being compared was changed
	// we need to update status in the dir view
	if (bLChangedOriginal || bRChangedOriginal)
	{
		// If DirDoc contains diffs
		if (m_pDirDoc)
		{
			if (m_bEditAfterRescan[0] || m_bEditAfterRescan[1])
				FlushAndRescan(false);
			m_pDirDoc->UpdateChangedItem(this);
		}
	}
}

/**
 * @brief Saves left-side file
 */
void CChildFrame::OnFileSaveLeft()
{
	bool bLSaveSuccess = false;
	bool bLModified = false;

	if (m_ptBuf[0]->IsModified() && !m_ptBuf[0]->GetReadOnly())
	{
		bLModified = true;
		DoSave(bLSaveSuccess, 0);
	}

	// If file were modified and saving succeeded,
	// update status on dir view
	if (bLModified && bLSaveSuccess)
	{
		// If DirDoc contains compare results
		if (m_pDirDoc)
		{
			if (m_bEditAfterRescan[0] || m_bEditAfterRescan[1])
				FlushAndRescan(false);
			m_pDirDoc->UpdateChangedItem(this);
		}
	}
}

/**
 * @brief Saves right-side file
 */
void CChildFrame::OnFileSaveRight()
{
	bool bRSaveSuccess = false;
	bool bRModified = false;

	if (m_ptBuf[1]->IsModified() && !m_ptBuf[1]->GetReadOnly())
	{
		bRModified = true;
		DoSave(bRSaveSuccess, 1);
	}

	// If file were modified and saving succeeded,
	// update status on dir view
	if (bRModified && bRSaveSuccess)
	{
		// If DirDoc contains compare results
		if (m_pDirDoc)
		{
			if (m_bEditAfterRescan[0] || m_bEditAfterRescan[1])
				FlushAndRescan(false);
			m_pDirDoc->UpdateChangedItem(this);
		}
	}
}

/**
 * @brief Saves left-side file with name asked
 */
void CChildFrame::OnFileSaveAsLeft()
{
	DoSaveAs(0);
}

/**
 * @brief Saves right-side file with name asked
 */
void CChildFrame::OnFileSaveAsRight()
{
	DoSaveAs(1);
}

/**
 * @brief Update diff-number pane text in file compare.
 * The diff number pane shows selected difference/amount of differences when
 * there is difference selected. If there is no difference selected, then
 * the panel shows amount of differences. If there are ignored differences,
 * those are not count into numbers.
 * @param [in] pCmdUI UI component to update.
 */
void CChildFrame::OnUpdateStatusNum()
{
	if (m_pMDIFrame->GetActiveDocFrame() != this)
		return;

	String s;
	const int nDiffs = m_diffList.GetSignificantDiffs();
	
	// Files are identical - show text "Identical"
	if (nDiffs <= 0)
		s = LanguageSelect.LoadString(IDS_IDENTICAL);
	
	// There are differences, but no selected diff
	// - show amount of diffs
	else if (GetCurrentDiff() < 0)
	{
		s = LanguageSelect.LoadString(nDiffs == 1 ? IDS_1_DIFF_FOUND : IDS_NO_DIFF_SEL_FMT);
		string_replace(s, _T("%1"), NumToStr(nDiffs, 10).c_str());
	}
	
	// There are differences and diff selected
	// - show diff number and amount of diffs
	else
	{
		s = LanguageSelect.LoadString(IDS_DIFF_NUMBER_STATUS_FMT);
		const int signInd = m_diffList.GetSignificantIndex(GetCurrentDiff());
		string_replace(s, _T("%1"), NumToStr(signInd + 1, 10).c_str());
		string_replace(s, _T("%2"), NumToStr(nDiffs, 10).c_str());
	}
	m_pMDIFrame->GetStatusBar()->SetPartText(2, s.c_str());
}

/**
 * @brief Build the diff array and prepare buffers accordingly (insert ghost lines, set WinMerge flags)
 *
 * @note Buffers may have different length after PrimeTextBuffers. Indeed, no
 * synchronization is needed after the last line. So no ghost line will be created
 * to face an ignored difference in the last line (typically : 'ignore blank lines' 
 * + empty last line on one side).
 * If you feel that different length buffers are really strange, CHANGE FIRST
 * the last diff to take into account the empty last line.
 */
void CChildFrame::PrimeTextBuffers()
{
	const UINT nContextLines =
		m_idContextLines < ID_VIEW_CONTEXT_UNLIMITED ?
		m_idContextLines - ID_VIEW_CONTEXT_0 : UINT_MAX;
	SetCurrentDiff(-1);
	m_nTrivialDiffs = 0;
	const int nDiffCount = m_diffList.GetSize();
	// resize m_aLines once for each view
	UINT lcount0new = m_ptBuf[0]->GetLineCount();
	UINT lcount1new = m_ptBuf[1]->GetLineCount();
	// walk the diff list and calculate numbers of extra lines to add
	m_diffList.AddExtraLinesCounts(lcount0new, lcount1new, m_ptBuf, nContextLines);
	m_ptBuf[0]->RemoveAllGhostLines(LF_SKIPPED);
	m_ptBuf[1]->RemoveAllGhostLines(LF_SKIPPED);
	UINT lcount0 = m_ptBuf[0]->GetLineCount();
	UINT lcount1 = m_ptBuf[1]->GetLineCount();
// this ASSERT may be false because of empty last line (see function's note)
//	ASSERT(lcount0new == lcount1new);
	m_ptBuf[0]->m_aLines.resize(lcount0new);
	m_ptBuf[1]->m_aLines.resize(lcount1new);

	LineInfo GhostLineInfo;
	GhostLineInfo.m_dwFlags = LF_GHOST;

	// walk the diff list backward, move existing lines to proper place,
	// add ghost lines, and set flags
	int nDiff = nDiffCount;
	while (nDiff)
	{
		DIFFRANGE *curDiff = m_diffList.DiffRangeAt(--nDiff);
		// move matched lines after curDiff
		UINT nline0 = lcount0 - curDiff->end0 - 1; // #lines on left after current diff
		UINT nline1 = lcount1 - curDiff->end1 - 1; // #lines on right after current diff
		// Matched lines should really match...
		// But matched lines after last diff may differ because of empty last line (see function's note)
		if (nDiff < nDiffCount - 1)
			ASSERT(nline0 == nline1);
		// Move all lines after current diff down as far as needed
		// for any ghost lines we're about to insert
		m_ptBuf[0]->MoveLine(curDiff->end0 + 1, lcount0 - 1, lcount0new - nline0);
		m_ptBuf[1]->MoveLine(curDiff->end1 + 1, lcount1 - 1, lcount1new - nline1);
		lcount0 -= nline0;
		lcount1 -= nline1;
		lcount0new -= nline0;
		lcount1new -= nline1;
		// move unmatched lines and add ghost lines
		nline0 = curDiff->end0 - curDiff->begin0 + 1; // #lines in diff on left
		nline1 = curDiff->end1 - curDiff->begin1 + 1; // #lines in diff on right
		lcount0 -= nline0;
		lcount1 -= nline1;
		UINT nline = max(nline0, nline1);
		lcount0new -= nline;
		lcount1new -= nline;
		m_ptBuf[0]->MoveLine(curDiff->begin0, curDiff->end0, lcount0new);
		m_ptBuf[1]->MoveLine(curDiff->begin1, curDiff->end1, lcount1new);
		UINT i;
		for (i = nline1 ; i < nline0 ; ++i)
		{
			m_ptBuf[1]->m_aLines[lcount1new + i] = GhostLineInfo;
		}
		for (i = nline0 ; i < nline1 ; ++i)
		{
			m_ptBuf[0]->m_aLines[lcount0new + i] = GhostLineInfo;
		}

		// set dbegin, dend, blank, and line flags
		curDiff->dbegin0 = lcount0new;
		curDiff->dbegin1 = lcount1new;

		switch (curDiff->op)
		{
		case OP_LEFTONLY:
			// set curdiff
			// left side
			curDiff->dend0 = lcount0new + nline0 - 1;
			curDiff->blank0 = 0;
			// right side
			curDiff->dend1 = lcount1new + nline0 - 1;
			curDiff->blank1 = curDiff->dbegin1;
			// flag lines
			for (i = curDiff->dbegin0 ; i <= curDiff->dend0; i++)
				m_ptBuf[0]->m_aLines[i].m_dwFlags |= LF_DIFF;
			// blanks are already inserted (and flagged) to compensate for diff on other side
			break;
		case OP_RIGHTONLY:
			// set curdiff
			// left side
			curDiff->dend0 = lcount0new + nline1 - 1;
			curDiff->blank0 = curDiff->dbegin0;
			// right side
			curDiff->dend1 = lcount1new + nline1 - 1;
			curDiff->blank1 = 0;
			// flag lines
			for (i = curDiff->dbegin1 ; i <= curDiff->dend1 ; i++)
				m_ptBuf[1]->m_aLines[i].m_dwFlags |= LF_DIFF;
			// blanks are already inserted (and flagged) to compensate for diff on other side
			break;
		case OP_TRIVIAL:
			++m_nTrivialDiffs;
			// fall through and handle as diff
		case OP_DIFF:
			// set curdiff
			// left side
			curDiff->dend0 = lcount0new + nline - 1;
			curDiff->blank0 = 0;
			// right side
			curDiff->dend1 = lcount1new + nline - 1;
			curDiff->blank1 = 0;
			if (nline0 > nline1)
				// more lines on left, ghost lines on right side
				curDiff->blank1 = curDiff->dend1 + 1 - (nline0 - nline1);
			else if (nline0 < nline1)
				// more lines on right, ghost lines on left side
				curDiff->blank0 = curDiff->dend0 + 1 - (nline1 - nline0);
			// flag lines
			// left side
			for (i = curDiff->dbegin0 ; i <= curDiff->dend0 ; ++i)
			{
				if (curDiff->blank0 == 0 || i < curDiff->blank0)
				{
					// set diff or trivial flag
					DWORD dflag = curDiff->op == OP_DIFF ? LF_DIFF : LF_TRIVIAL;
					m_ptBuf[0]->m_aLines[i].m_dwFlags |= dflag;
				}
				else if (curDiff->op == OP_TRIVIAL)
				{
					// ghost lines are already inserted (and flagged)
					// ghost lines opposite to trivial lines are ghost and trivial
					m_ptBuf[0]->m_aLines[i].m_dwFlags |= LF_TRIVIAL;
				}
			}
			// right side
			for (i = curDiff->dbegin1 ; i <= curDiff->dend1 ; ++i)
			{
				if (curDiff->blank1 == 0 || i < curDiff->blank1)
				{
					// set diff or trivial flag
					DWORD dflag = curDiff->op == OP_DIFF ? LF_DIFF : LF_TRIVIAL;
					m_ptBuf[1]->m_aLines[i].m_dwFlags |= dflag;
				}
				else if (curDiff->op == OP_TRIVIAL)
				{
					// ghost lines are already inserted (and flagged)
					// ghost lines opposite to trivial lines are ghost and trivial
					m_ptBuf[1]->m_aLines[i].m_dwFlags |= LF_TRIVIAL;
				}
			}
			break;
		}
	}

	// Flag any remaining differences which were trivialized by prediffing,
	// and hence did not make it into the DiffList produced by DiffUtils.
	UINT n = min(m_ptBuf[0]->GetLineCount(), m_ptBuf[1]->GetLineCount());
	UINT i;
	for (i = 0; i < n; ++i)
	{
		LineInfo &info0 = m_ptBuf[0]->m_aLines[i];
		LineInfo &info1 = m_ptBuf[1]->m_aLines[i];
		if (((info0.m_dwFlags | info1.m_dwFlags) & LF_WINMERGE_FLAGS) == 0)
		{
			int len = info0.Length();
			if (len != info1.Length() ||
				memcmp(info0.GetLine(), info1.GetLine(), len * sizeof(TCHAR)) != 0)
			{
				info0.m_dwFlags |= LF_TRIVIAL;
				info1.m_dwFlags |= LF_TRIVIAL;
			}
		}
	}

	m_diffList.ConstructSignificantChain();

	// Used to strip trivial diffs out of the diff chain
	// if m_nTrivialDiffs
	// via copying them all to a new chain, then copying only non-trivials back
	// but now we keep all diffs, including trivial diffs
	m_ptBuf[0]->FinishLoading();
	m_ptBuf[1]->FinishLoading();
}

/**
 * @brief Checks if file has changed since last update (save or rescan).
 * @param [in] szPath File to check
 * @param [in] dfi Previous fileinfo of file
 * @param [in] bSave If TRUE Compare to last save-info, else to rescan-info
 * @param [in] nBuffer Index (0-based) of buffer
 * @return TRUE if file is changed.
 */
CChildFrame::FileChange CChildFrame::IsFileChangedOnDisk(LPCTSTR szPath,
	DiffFileInfo &dfi, const DiffFileInfo &fileInfo)
{
	// We assume file existed, so disappearing means removal
	if (!dfi.Update(szPath))
		return FileRemoved;

	bool bIgnoreSmallDiff = COptionsMgr::Get(OPT_IGNORE_SMALL_FILETIME);
	UINT64 lower = min(dfi.mtime.castTo<UINT64>(), fileInfo.mtime.castTo<UINT64>());
	UINT64 upper = max(dfi.mtime.castTo<UINT64>(), fileInfo.mtime.castTo<UINT64>());
	UINT64 tolerance = bIgnoreSmallDiff ? SmallTimeDiff * FileTime::TicksPerSecond : 0;
	if ((upper - lower > tolerance) || (dfi.size.int64 != fileInfo.size.int64))
		return FileChanged;

	return FileNoChange;
}

/**
 * @brief Asks and then saves modified files.
 *
 * This function saves modified files. Dialog is shown for user to select
 * modified file(s) one wants to save or discard changed. Cancelling of
 * save operation is allowed unless denied by parameter. After successfully
 * save operation file statuses are updated to directory compare.
 * @param [in] bAllowCancel If FALSE "Cancel" button is disabled.
 * @return TRUE if user selected "OK" so next operation can be
 * executed. If FALSE user choosed "Cancel".
 * @note If filename is empty, we assume scratchpads are saved,
 * so instead of filename, description is shown.
 * @todo If we have filename and description for file, what should
 * we do after saving to different filename? Empty description?
 * @todo Parameter @p bAllowCancel is always true in callers - can be removed.
 */
bool CChildFrame::PromptAndSaveIfNeeded(bool bAllowCancel)
{
	const BOOL bLModified = m_ptBuf[0]->IsModified();
	const BOOL bRModified = m_ptBuf[1]->IsModified();

	if (!bLModified && !bRModified) //Both files unmodified
		return true;

	const String &pathLeft = m_strPath[0];
	const String &pathRight = m_strPath[1];

	SaveClosingDlg dlg;
	dlg.m_bAskForLeft = bLModified;
	dlg.m_bAskForRight = bRModified;
	dlg.m_sLeftFile = pathLeft.empty() ? m_strDesc[0] : pathLeft;
	dlg.m_sRightFile = pathRight.empty() ? m_strDesc[1] : pathRight;
	if (!bAllowCancel)
		dlg.m_bDisableCancel = TRUE;

	bool result = true;
	bool bLSaveSuccess = false;
	bool bRSaveSuccess = false;

	if (LanguageSelect.DoModal(dlg) == IDOK)
	{
		if (bLModified && dlg.m_leftSave == SaveClosingDlg::SAVECLOSING_SAVE)
		{
			if (!DoSave(bLSaveSuccess, 0))
				result = false;
		}

		if (bRModified && dlg.m_rightSave == SaveClosingDlg::SAVECLOSING_SAVE)
		{
			if (!DoSave(bRSaveSuccess, 1))
				result = false;
		}
	}
	else
	{	
		result = false;
	}

	// If file were modified and saving was successfull,
	// update status on dir view
	if (bLSaveSuccess || bRSaveSuccess)
	{
		// If directory compare has results
		if (m_pDirDoc)
		{
			if (m_bEditAfterRescan[0] || m_bEditAfterRescan[1])
				FlushAndRescan(false);
			m_pDirDoc->UpdateChangedItem(this);
		}
	}

	return result;
}

/** Rescan only if we did not Rescan during the last timeOutInSecond seconds*/
void CChildFrame::RescanIfNeeded(DWORD timeout)
{
	// if we did not rescan during the request timeOut, Rescan
	// else we did Rescan after the request, so do nothing
	FileTime now;
	GetSystemTimeAsFileTime(&now);
	if ((now.castTo<UINT64>() - m_LastRescan.castTo<UINT64>()) >= timeout * (FileTime::TicksPerSecond / 1000))
		// (laoran 08-01-2003) maybe should be FlushAndRescan(TRUE) ??
		FlushAndRescan();
}

/**
 * @brief Loads file to buffer and shows load-errors
 * @param [in] sFileName File to open
 * @param [in] nBuffer Index (0-based) of buffer to load
 * @param [out] readOnly whether file is read-only
 * @param [in] encoding encoding used
 * @return Tells if files were loaded successfully
 * @sa CChildFrame::OpenDocs()
 **/
FileLoadResult::FILES_RESULT CChildFrame::LoadFile(int nBuffer, bool &readOnly, FileLocation &fileinfo)
{
	CDiffTextBuffer *pBuf = m_ptBuf[nBuffer];
	m_strPath[nBuffer] = fileinfo.filepath;

	m_pInfoUnpacker->textType = CMainFrame::GetFileExt(m_strPath[nBuffer].c_str(), m_strDesc[nBuffer].c_str());

	String sOpenError;
	FileLoadResult::FILES_RESULT retVal = pBuf->LoadFromFile(fileinfo.filepath.c_str(),
		m_pInfoUnpacker.get(), readOnly, CRLF_STYLE_AUTOMATIC, fileinfo.encoding, sOpenError);

	// if CChildFrame::CDiffTextBuffer::LoadFromFile failed,
	// it left the pBuf in a valid (but empty) state via a call to InitNew

	if (FileLoadResult::IsOkImpure(retVal))
	{
		// File loaded, and multiple EOL types in this file
		FileLoadResult::SetMainOk(retVal);

		// If mixed EOLs are not enabled, enable them for this doc.
		if (!COptionsMgr::Get(OPT_ALLOW_MIXED_EOL))
		{
			pBuf->SetMixedEOL(true);
		}
	}

	if (FileLoadResult::IsError(retVal))
	{
		// Error from Unifile/system
		LanguageSelect.FormatStrings(
			sOpenError.empty() ? IDS_ERROR_FILE_NOT_FOUND : IDS_ERROR_FILEOPEN,
			paths_UndoMagic(wcsdupa(fileinfo.filepath.c_str())),
			sOpenError.c_str()
		).MsgBox(MB_ICONSTOP);
	}
	return retVal;
}

/**
 * @brief Check if specified codepage number is valid for WinMerge Editor.
 * @param [in] cp Codepage number to check.
 * @return true if codepage is valid, false otherwise.
 */
bool CChildFrame::IsValidCodepageForMergeEditor(unsigned cp)
{
	if (!cp) // 0 is our signal value for invalid
		return false;
	// Codepage must be actually installed on system
	// for us to be able to use it
	// We accept whatever codepages that codepage module says are installed
	return isCodepageInstalled(cp);
}

/**
 * @brief Sanity check file's specified codepage.
 * This function checks if file's specified codepage is valid for WinMerge
 * editor and if not resets the codepage to default.
 * @param [in,out] fileinfo Class containing file's codepage.
 */
void CChildFrame::SanityCheckCodepage(FileLocation & fileinfo)
{
	if (fileinfo.encoding.m_unicoding == NONE
		&& !IsValidCodepageForMergeEditor(fileinfo.encoding.m_codepage))
	{
		int cp = getDefaultCodepage();
		if (!IsValidCodepageForMergeEditor(cp))
			cp = CP_ACP;
		fileinfo.encoding.SetCodepage(cp);
	}
}

/**
 * @brief Loads one file from disk and updates file infos.
 * @param [in] index Index of file in internal buffers.
 * @param [in] filename File's name.
 * @param [in] readOnly Is file read-only?
 * @param [in] encoding File's encoding.
 * @return One of FileLoadResult values.
 */
FileLoadResult::FILES_RESULT CChildFrame::LoadOneFile(
	int index, bool &readOnly, FileLocation &fileinfo)
{
	FileLoadResult::FILES_RESULT loadSuccess = FileLoadResult::FRESULT_ERROR;
	if (!fileinfo.filepath.empty())
	{
		if (fileinfo.description.empty())
			m_nBufferType[index] = BUFFER_NORMAL;
		else
		{
			m_nBufferType[index] = BUFFER_NORMAL_NAMED;
			m_strDesc[index] = fileinfo.description;
		}
		m_pSaveFileInfo[index].Update(fileinfo.filepath.c_str());
		m_pRescanFileInfo[index].Update(fileinfo.filepath.c_str());
		loadSuccess = LoadFile(index, readOnly, fileinfo);
	}
	else
	{
		m_nBufferType[index] = BUFFER_UNNAMED;
		m_ptBuf[index]->InitNew();
		m_strDesc[index] = fileinfo.description;
		m_ptBuf[index]->m_encoding = fileinfo.encoding;
		loadSuccess = FileLoadResult::FRESULT_OK;
	}
	UpdateHeaderPath(index);
	return loadSuccess;
}

/**
 * @brief Loads files and does initial rescan.
 * @param filelocLeft [in] File to open to left side (path & encoding info)
 * @param fileLocRight [in] File to open to right side (path & encoding info)
 * @param bROLeft [in] Is left file read-only
 * @param bRORight [in] Is right file read-only
 * @todo Options are still read from CMainFrame, this will change
 * @sa CMainFrame::ShowMergeDoc()
 */
void CChildFrame::OpenDocs(
	FileLocation &filelocLeft,
	FileLocation &filelocRight,
	bool bROLeft, bool bRORight)
{
	bool bIdentical = false;

	// Filter out invalid codepages, or editor will display all blank
	SanityCheckCodepage(filelocLeft);
	SanityCheckCodepage(filelocRight);

	// clear undo stack
	undoTgt.clear();
	curUndo = undoTgt.begin();

	// Prevent displaying views during LoadFile
	// Note : attach buffer again only if both loads succeed
	m_pView[0]->DetachFromBuffer();
	m_pView[1]->DetachFromBuffer();
	m_pDetailView[0]->DetachFromBuffer();
	m_pDetailView[1]->DetachFromBuffer();

	// free the buffers
	m_ptBuf[0]->FreeAll();
	m_ptBuf[1]->FreeAll();

	String sLeftFile = filelocLeft.filepath;
	String sRightFile = filelocRight.filepath;

	// Load files
	FileLoadResult::FILES_RESULT nLeftSuccess = LoadOneFile(0, bROLeft, filelocLeft);
	String sextL = m_pInfoUnpacker->textType;
	FileLoadResult::FILES_RESULT nRightSuccess = LoadOneFile(1, bRORight, filelocRight);
	String sextR = m_pInfoUnpacker->textType;

	// Bail out if either side failed
	if (!FileLoadResult::IsOk(nLeftSuccess) || !FileLoadResult::IsOk(nRightSuccess))
	{
		DestroyFrame();
		return;
	}

	// Warn user if file load was lossy (bad encoding)
	// TODO: It would be nice to report how many lines were lossy
	// we did calculate those numbers when we loaded the files, in the text stats
	if (int idres = FileLoadResult::IsLossy(nLeftSuccess) &&
		FileLoadResult::IsLossy(nRightSuccess) ? IDS_LOSSY_TRANSCODING_BOTH :
		FileLoadResult::IsLossy(nLeftSuccess) ? IDS_LOSSY_TRANSCODING_LEFT :
		FileLoadResult::IsLossy(nRightSuccess) ? IDS_LOSSY_TRANSCODING_RIGHT : 0)
	{
		LanguageSelect.MsgBox(idres, MB_ICONSTOP);
	}

	// Now buffers data are valid
	m_pView[0]->AttachToBuffer(m_ptBuf[0]);
	m_pView[1]->AttachToBuffer(m_ptBuf[1]);
	m_pDetailView[0]->AttachToBuffer(m_ptBuf[0]);
	m_pDetailView[1]->AttachToBuffer(m_ptBuf[1]);

	// Currently there is only one set of syntax colors, which all documents & views share
	m_pView[0]->SetColorContext(&SyntaxColors);
	m_pView[1]->SetColorContext(&SyntaxColors);
	m_pDetailView[0]->SetColorContext(&SyntaxColors);
	m_pDetailView[1]->SetColorContext(&SyntaxColors);

	// Set read-only etc. statuses.
	// CRLF mode is volatile, so update it upon Rescan().
	m_bInitialReadOnly[0] = bROLeft;
	m_ptBuf[0]->SetReadOnly(bROLeft);
	m_pView[0]->SetEncodingStatus(filelocLeft.encoding.GetName().c_str());

	m_bInitialReadOnly[1] = bRORight;
	m_ptBuf[1]->SetReadOnly(bRORight);
	m_pView[1]->SetEncodingStatus(filelocRight.encoding.GetName().c_str());

	// prepare the four views
	CMergeEditView *const pLeft = GetLeftView();
	CMergeEditView *const pRight = GetRightView();
	CMergeDiffDetailView *const pLeftDetail = GetLeftDetailView();
	CMergeDiffDetailView *const pRightDetail = GetRightDetailView();
	
	// set the document types (do it before Rescan)
	// Warning : it is the first thing to do (must be done before UpdateView,
	// or any function that calls UpdateView, like SelectDiff)
	// Note: If option enabled, and another side type is not recognized,
	// we use recognized type for unrecognized side too.
	
	bool syntaxHLEnabled = COptionsMgr::Get(OPT_SYNTAX_HIGHLIGHT);
	CCrystalTextView::TextDefinition *bLeftTyped = NULL;
	CCrystalTextView::TextDefinition *bRightTyped = NULL;
	
	if (syntaxHLEnabled)
	{
		bLeftTyped = pLeft->SetTextType(sextL.c_str());
		pLeftDetail->SetTextType(bLeftTyped);
		bRightTyped = pRight->SetTextType(sextR.c_str());
		pRightDetail->SetTextType(bRightTyped);
	}

	// If textypes of the files aren't recogzined by their extentions,
	// try to recognize them using their first lines 
	if (bLeftTyped == NULL && bRightTyped == NULL)
	{
		if (LPCTSTR sFirstLine = m_ptBuf[0]->GetLineChars(0))
			bLeftTyped = pLeft->SetTextTypeByContent(sFirstLine);
		if (LPCTSTR sFirstLine = m_ptBuf[1]->GetLineChars(0))
			bRightTyped = pRight->SetTextTypeByContent(sFirstLine);
	}

	// If other side didn't have recognized texttype, apply recognized
	// type to unrecognized one. (comparing file.cpp and file.bak applies
	// cpp file type to .bak file.
	if (bRightTyped == NULL)
	{
		pRight->SetTextType(bLeftTyped);
		pRightDetail->SetTextType(bLeftTyped);
	}
	if (bLeftTyped == NULL)
	{
		pLeft->SetTextType(bRightTyped);
		pLeftDetail->SetTextType(bRightTyped);
	}

	// Check the EOL sensitivity option (do it before Rescan)
	if (m_ptBuf[0]->GetCRLFMode() != m_ptBuf[1]->GetCRLFMode() &&
		!COptionsMgr::Get(OPT_ALLOW_MIXED_EOL) && !m_diffWrapper.bIgnoreEol)
	{
		// Options and files not are not compatible :
		// Sensitive to EOL on, allow mixing EOL off, and files have a different EOL style.
		// All lines will differ, that is not very interesting and probably not wanted.
		// Propose to turn off the option 'sensitive to EOL'
		if (LanguageSelect.MsgBox(IDS_SUGGEST_IGNOREEOL, MB_YESNO | MB_ICONWARNING | MB_DONT_ASK_AGAIN | MB_IGNORE_IF_SILENCED) == IDYES)
		{
			m_diffWrapper.bIgnoreEol = true;
		}
	}

	int nRescanResult = Rescan(bIdentical);

	// Open filed if rescan succeed and files are not binaries
	if (nRescanResult != RESCAN_OK)
	{
		// CChildFrame::Rescan fails if files do not exist on both sides 
		// or the really arcane case that the temp files couldn't be created, 
		// which is too obscure to bother reporting if you can't write to 
		// your temp directory, doing nothing is graceful enough for that).
		ShowRescanError(nRescanResult, bIdentical);
		DestroyFrame();
		return;
	}

	pLeft->DocumentsLoaded();
	pRight->DocumentsLoaded();
	pLeftDetail->DocumentsLoaded();
	pRightDetail->DocumentsLoaded();

	// Activate yourself to have panes resized before auto-scrolling to 1st diff
	ActivateFrame();

	// Inform user that files are identical
	// Don't show message if new buffers created
	if (bIdentical &&
		((m_nBufferType[0] == BUFFER_NORMAL) ||
		 (m_nBufferType[0] == BUFFER_NORMAL_NAMED) ||
		 (m_nBufferType[1] == BUFFER_NORMAL) ||
		 (m_nBufferType[1] == BUFFER_NORMAL_NAMED)))
	{
		ShowRescanError(nRescanResult, bIdentical);
	}

	// scroll to first diff
	if (COptionsMgr::Get(OPT_SCROLL_TO_FIRST))
	{
		int nDiff = m_diffList.FirstSignificantDiff();
		if (nDiff != -1)
			pLeft->SelectDiff(nDiff);
	}

	// Exit if files are identical should only work for the first
	// comparison and must be disabled afterward.
	m_pMDIFrame->m_bExitIfNoDiff = MergeCmdLineInfo::Disabled;

	// Force repaint of location pane to update it in case we had some warning
	// dialog visible and it got painted before files were loaded
	m_wndLocationView.ForceRecalculate();
}

/**
 * @brief Re-load a document.
 * This methods re-loads the file compare document. The re-loaded document is
 * one side of the file compare.
 * @param [in] index The document to re-load.
 * @return Open result code.
 */
FileLoadResult::FILES_RESULT CChildFrame::ReloadDoc(int index)
{
	// clear undo stack
	undoTgt.clear();
	curUndo = undoTgt.begin();

	FileLocation fileinfo;
	fileinfo.filepath = m_strPath[index];
	fileinfo.encoding = m_ptBuf[index]->getEncoding();
	fileinfo.description = m_strDesc[index];

	bool readOnly = m_ptBuf[index]->GetReadOnly();

	// Prevent displaying views during LoadFile
	// Note : attach buffer again only if both loads succeed
	// clear undo buffers
	// free the buffers
	m_pView[index]->DetachFromBuffer();
	m_pDetailView[index]->DetachFromBuffer();
	m_ptBuf[index]->FreeAll();

	// Load file
	return LoadOneFile(index, readOnly, fileinfo);
}

/**
 * @brief Refresh cached options.
 *
 * For compare speed, we have to cache some frequently needed options,
 * instead of getting option value every time from COptionsMgr:: This
 * function must be called every time options are changed to COptionsMgr::
 */
void CChildFrame::RefreshOptions()
{
	m_diffWrapper.RefreshOptions();
	// Refresh view options
	m_pView[0]->RefreshOptions();
	m_pView[1]->RefreshOptions();
	m_pDetailView[0]->RefreshOptions();
	m_pDetailView[1]->RefreshOptions();
}

/**
 * @brief Write path and filename to headerbar
 * @note SetText() does not repaint unchanged text
 */
void CChildFrame::UpdateHeaderPath(int pane)
{
	String sText;
	if (m_nBufferType[pane] == BUFFER_UNNAMED ||
		m_nBufferType[pane] == BUFFER_NORMAL_NAMED)
	{
		sText = m_strDesc[pane];
	}
	else
	{
		sText = m_strPath[pane];
		if (m_pDirDoc)
		{
			if (pane == 0)
				m_pDirDoc->ApplyLeftDisplayRoot(sText);
			else
				m_pDirDoc->ApplyRightDisplayRoot(sText);
		}
	}

	m_wndFilePathBar.SetText(pane, sText.c_str(), m_ptBuf[pane]->IsModified(), m_nBufferType[pane]);

	SetTitle();
}

/**
 * @brief Paint differently the headerbar of the active view
 */
void CChildFrame::UpdateHeaderActivity(int pane, bool bActivate)
{
	m_wndFilePathBar.SetActive(pane, bActivate);
}

/**
 * @brief Return if doc is in Merging/Editing mode
 */
bool CChildFrame::GetMergingMode() const
{
	return m_bMergingMode;
}

/**
 * @brief Set doc to Merging/Editing mode
 */
void CChildFrame::SetMergingMode(bool bMergingMode)
{
	m_bMergingMode = bMergingMode;
	COptionsMgr::SaveOption(OPT_MERGE_MODE, m_bMergingMode);
}

/**
 * @brief Set detect/not detect Moved Blocks
 */
void CChildFrame::SetDetectMovedBlocks(bool bDetectMovedBlocks)
{
	m_diffWrapper.SetDetectMovedBlocks(bDetectMovedBlocks);
	COptionsMgr::SaveOption(OPT_CMP_MOVED_BLOCKS, bDetectMovedBlocks);
	FlushAndRescan();
}

/**
 * @brief Check if given buffer has mixed EOL style.
 * @param [in] nBuffer Buffer to check.
 * @return true if buffer's EOL style is mixed, false otherwise.
 */
bool CChildFrame::IsMixedEOL(int nBuffer) const
{
	CDiffTextBuffer *pBuf = m_ptBuf[nBuffer];
	return pBuf->IsMixedEOL();
}

void CChildFrame::SetEditedAfterRescan(int nBuffer)
{
	m_bEditAfterRescan[nBuffer] = true;
}

/**
 * @brief Update document filenames to title
 */
void CChildFrame::SetTitle()
{
	string_format sTitle(_T("%s%s - %s%s"),
		&_T("\0* ")[m_ptBuf[0]->IsModified()],
		!m_strDesc[0].empty() ? m_strDesc[0].c_str() : PathFindFileName(m_strPath[0].c_str()),
		&_T("\0* ")[m_ptBuf[1]->IsModified()],
		!m_strDesc[1].empty() ? m_strDesc[1].c_str() : PathFindFileName(m_strPath[1].c_str()));
	SetWindowText(sTitle.c_str());
}

// Return current word breaking break type setting (whitespace only or include punctuation)
int CChildFrame::GetBreakType()
{
	return COptionsMgr::Get(OPT_BREAK_TYPE);
}

// Return true to do line diff colors at the byte level (false to do them at word level)
bool CChildFrame::GetByteColoringOption()
{
	// color at byte level if 'break_on_words' option not set
	return !COptionsMgr::Get(OPT_BREAK_ON_WORDS);
}

/// Swap files and update views
void CChildFrame::SwapFiles()
{
	// Swap views
	m_wndFilePathBar.SwapPanes(0x1000, 0x1001);
	m_wndFilePathBar.SwapPanes(0x6000, 0x6001);

	m_wndDiffViewBar.SwapPanes(
		m_pDetailView[0]->GetDlgCtrlID(), m_pDetailView[1]->GetDlgCtrlID());

	// Swap buffers and so on
	std::swap(m_ptBuf[0], m_ptBuf[1]);
	std::swap(m_pView[0], m_pView[1]);
	std::swap(m_pDetailView[0], m_pDetailView[1]);
	std::swap(m_pSaveFileInfo[0], m_pSaveFileInfo[1]);
	std::swap(m_pRescanFileInfo[0], m_pRescanFileInfo[1]);
	std::swap(m_pFileTextStats[0], m_pFileTextStats[1]);
	std::swap(m_nBufferType[0], m_nBufferType[1]);
	std::swap(m_bEditAfterRescan[0], m_bEditAfterRescan[1]);
	m_strDesc[0].swap(m_strDesc[1]);

	m_strPath[0].swap(m_strPath[1]);
	m_diffList.SwapSides();

	m_ptBuf[0]->m_nThisPane = 0;
	m_pView[0]->m_nThisPane = 0;
	m_pDetailView[0]->m_nThisPane = 0;
	m_ptBuf[1]->m_nThisPane = 1;
	m_pView[1]->m_nThisPane = 1;
	m_pDetailView[1]->m_nThisPane = 1;

	// Update views
	HWindow *pWndFocus = HWindow::GetFocus();
	UpdateHeaderPath(0);
	m_wndFilePathBar.SetActive(0, pWndFocus->m_hWnd == m_pView[0]->m_hWnd);
	UpdateHeaderPath(1);
	m_wndFilePathBar.SetActive(1, pWndFocus->m_hWnd == m_pView[1]->m_hWnd);

	if (MovedLines *pMovedLines = m_diffWrapper.GetMovedLines())
	{
		pMovedLines->SwapSides();
	}

	m_pDetailView[1]->RecalcHorzScrollBar();
	UpdateAllViews();
}

/**
 * @brief Display encodings to user
 */
void CChildFrame::OnFileEncoding()
{
	DoFileEncodingDialog();
}

void CChildFrame::OnToolsCompareSelection()
{
	FileLocation filelocLeft, filelocRight;
	filelocLeft.encoding = m_ptBuf[0]->getEncoding();
	filelocLeft.description = LanguageSelect.LoadString(IDS_SELECTION_LEFT);
	filelocRight.encoding = m_ptBuf[1]->getEncoding();
	filelocRight.description = LanguageSelect.LoadString(IDS_SELECTION_RIGHT);
	CChildFrame *const pMergeDoc = new CChildFrame(m_pMDIFrame, NULL, this);
	pMergeDoc->OpenDocs(filelocLeft, filelocRight, FALSE, FALSE);
	int nSide = 0;
	do
	{
		CMergeEditView *const pView = m_pView[nSide];
		CDiffTextBuffer *const pTargetBuf = pMergeDoc->m_ptBuf[nSide];
		CMergeEditView *const pTargetView = pMergeDoc->m_pView[nSide];
		// Have the modal MergeDoc inherit the current syntax coloring rules.
		pTargetView->SetTextType(pView->m_CurSourceDef);
		POINT ptStart, ptEnd;
		pView->GetSelection(ptStart, ptEnd);
		String text;
		pView->GetText(ptStart.y, ptStart.x, ptEnd.y, ptEnd.x, text);
		// Trick the TextBuffer to flag the to-be-inserted text as revision 0.
		--pTargetBuf->m_dwCurrentRevisionNumber;
		pTargetBuf->InsertText(pTargetView, 0, 0,
			text.c_str(), text.length(), CE_ACTION_UNKNOWN, FALSE);
		pTargetBuf->SetModified(false);
	} while (nSide ^= 1);
	pMergeDoc->FlushAndRescan();
	pMergeDoc->ActivateFrame();
}

/**
 * @brief Generate report from file compare results.
 */
void CChildFrame::OnToolsGenerateReport()
{
	String s;
	if (!SelectFile(m_pMDIFrame->m_hWnd, s, IDS_SAVE_AS_TITLE, IDS_HTML_REPORT_FILES, FALSE, _T("htm")))
		return;
	// create HTML report
	UniStdioFile file;
	if (!file.Open(s.c_str(), _T("wt")))
	{
		String errMsg = GetSysError(GetLastError());
		LanguageSelect.MsgBox(IDS_REPORT_ERROR, errMsg.c_str(), MB_ICONSTOP);
		return;
	}
	WriteReport(file);
	LanguageSelect.MsgBox(IDS_REPORT_SUCCESS, MB_ICONINFORMATION);
}

void CChildFrame::WriteReport(UniStdioFile &file)
{
	WaitStatusCursor waitstatus;
	int nFontSize = 8;
	// calculate HTML font size
	if (HSurface *pDC = GetDC())
	{
		LOGFONT lf;
		m_pView[0]->GetFont(lf);
		nFontSize = -MulDiv(lf.lfHeight, 72, pDC->GetDeviceCaps(LOGPIXELSY));
		ReleaseDC(pDC);
	}
	file.SetCodepage(CP_UTF8);
	// For the purpose of printing, the preferable orientation is landscape.
	// An occasional proposal to mimic landscape orientation without requiring
	// the user to call up the page setup dialog is to apply a style that reads
	// { filter: progid:DXImageTransform.Microsoft.BasicImage(Rotation=3); }
	// WinMerge takes programmatic control over print settings by means of a
	// print template, and hence gets away without following the above proposal.
	String styles;
	m_pView[0]->GetHTMLStyles(styles);
	string_format header(
		_T("<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.01//EN' 'http://www.w3.org/TR/html4/strict.dtd'>\n")
		_T("<html>\n")
		_T("<head>\n")
		_T("<meta http-equiv='Content-Type' content='text/html; charset=UTF-8'>\n")
		_T("<title>WinMerge File Compare Report</title>\n")
		_T("<style type='text/css'>\n")
		_T("body { margin: 0; }\n")
		_T("@media print { thead { display: table-header-group; } }\n")
		_T("table { font-size: %dpt; font-family: sans-serif; word-wrap: break-word; width: 100%%; margin: 0; border: none; table-layout: fixed; }\n")
		_T("tr { vertical-align: top; height: 2.4ex; }\n")
		_T("th { color: white; background-color: blue; text-align: center; }\n")
		_T("code { line-height: 2.4ex; }\n")
		_T("%s")
		_T("</style>\n")
		_T("</head>\n")
		_T("<body>\n")
		_T("<table cellspacing='0' cellpadding='0'>\n")
		_T("<col style='width: 2.4em; background-color: lightgrey;'>\n")
		_T("<col style='width: 50%%; background-color: white;'>\n")
		_T("<col style='width: 2.4em; background-color: lightgrey;'>\n")
		_T("<col style='width: 50%%; background-color: white;'>\n")
		_T("<thead>\n")
		_T("<tr>\n")
		_T("<th colspan='2'>%s</th>\n")
		_T("<th colspan='2'>%s</th>\n")
		_T("</tr>\n")
		_T("</thead>\n")
		_T("<tbody>\n"),
		nFontSize,
		styles.c_str(),
		OString(CMarkdown::Entitify(m_wndFilePathBar.GetTitle(0).c_str())).W,
		OString(CMarkdown::Entitify(m_wndFilePathBar.GetTitle(1).c_str())).W);
	file.WriteString(header);
	// write the body of the report
	int nLineCount[2] = { m_ptBuf[0]->GetLineCount(), m_ptBuf[1]->GetLineCount() };
	for (int nLine = 0 ; nLine < nLineCount[0] || nLine < nLineCount[1] ; ++nLine)
	{
		file.WriteString(_T("<tr>"));
		int nSide = 0;
		do
		{
			if (nLine < nLineCount[nSide])
			{
				const LineInfo &li = m_ptBuf[nSide]->GetLineInfo(nLine);
				if ((li.m_dwFlags & LF_GHOST) == 0)
				{
					String line;
					m_pView[nSide]->GetHTMLLine(nLine, line);
					string_format data(_T("<td align='right'>%d</td><td>%s</td>"),
						m_ptBuf[nSide]->ComputeRealLine(nLine) + 1,
						line.c_str());
					file.WriteString(data);
				}
				else if (li.m_nSkippedLines != 0)
				{
					string_format data(
						_T("<td colspan='2' align='center'><cite>%s</cite></td>"),
						FormatAmount<IDS_LINE_EXCLUDED, IDS_LINES_EXCLUDED>(li.m_nSkippedLines).c_str());
					file.WriteString(data);
				}
				else
				{
					file.WriteString(_T("<td colspan='2'></td>"));
				}
			}
			else
			{
				file.WriteString(_T("<td colspan='2'></td>"));
			}
		} while (nSide ^= 1);
		file.WriteString(_T("</tr>\n"));
	}
	file.WriteString(
		_T("</tbody>\n")
		_T("</table>\n")
		_T("</body>\n")
		_T("</html>\n"));
	file.Close();
}

// IMergeDoc
HRESULT CChildFrame::GetFilePath(long nSide, BSTR *pbsPath)
{
	if (nSide < 0 || nSide > 1)
		return E_INVALIDARG;
	if (!SysReAllocString(pbsPath, m_wndFilePathBar.GetTitle(nSide).c_str()))
		return E_OUTOFMEMORY;
	return S_OK;
}

HRESULT CChildFrame::GetStyleRules(long nSide, BSTR *pbsRules)
{
	if (nSide < 0 || nSide > 1)
		return E_INVALIDARG;
	String styles;
	m_pView[nSide]->GetHTMLStyles(styles);
	if (!SysReAllocString(pbsRules, styles.c_str()))
		return E_OUTOFMEMORY;
	return S_OK;
}

HRESULT CChildFrame::GetLineCount(long nSide, long *pnLines)
{
	if (nSide < 0 || nSide > 1)
		return E_INVALIDARG;
	*pnLines = m_ptBuf[nSide]->GetLineCount();
	return S_OK;
}

HRESULT CChildFrame::GetMarginHTML(long nSide, long nLine, BSTR *pbsHTML)
{
	if (nSide < 0 || nSide > 1)
		return E_INVALIDARG;
	if (nLine < m_ptBuf[nSide]->GetLineCount() &&
		!(m_ptBuf[nSide]->GetLineFlags(nLine) & LF_GHOST))
	{
		// line number
		TCHAR numbuff[24];
		_ltot(m_ptBuf[nSide]->ComputeRealLine(nLine) + 1, numbuff, 10);
		if (!SysReAllocString(pbsHTML, numbuff))
			return E_OUTOFMEMORY;
	}
	else
	{
		SysReAllocStringLen(pbsHTML, NULL, 0);
	}
	return S_OK;
}

HRESULT CChildFrame::GetContentHTML(long nSide, long nLine, BSTR *pbsHTML)
{
	if (nSide < 0 || nSide > 1)
		return E_INVALIDARG;
	if (nLine < m_ptBuf[nSide]->GetLineCount())
	{
		String line;
		const LineInfo &li = m_ptBuf[nSide]->GetLineInfo(nLine);
		if ((li.m_dwFlags & LF_GHOST) && (li.m_nSkippedLines != 0))
		{
			line.append_sprintf(_T("<CITE>%s</CITE>"),
				FormatAmount<IDS_LINE_EXCLUDED, IDS_LINES_EXCLUDED>(li.m_nSkippedLines).c_str());
		}
		else
		{
			m_pView[nSide]->GetHTMLLine(nLine, line);
		}
		if (!SysReAllocString(pbsHTML, line.c_str()))
			return E_OUTOFMEMORY;
	}
	else
	{
		SysReAllocStringLen(pbsHTML, NULL, 0);
	}
	return S_OK;
}

HRESULT CChildFrame::PrepareHTML(long nLine, long nStop, IDispatch *pFrame, long *pnLine)
{
	long nDirection = nStop < nLine ? 1 : -1; // Caution: 1 is up and -1 is down!
	CMyComPtr<IHTMLWindow2> spWindow2;
	OException::Check(pFrame->QueryInterface(&spWindow2));
	CMyComPtr<IHTMLWindow4> spWindow4;
	OException::Check(pFrame->QueryInterface(&spWindow4));
	CMyComPtr<IHTMLDocument2> spDocument2;
	spWindow2->get_document(&spDocument2);
	CMyComPtr<IHTMLElement> spBody;
	spDocument2->get_body(&spBody);
	CMyComPtr<IDispatch> spChildrenDispatch;
	spBody->get_children(&spChildrenDispatch);
	CMyComPtr<IHTMLElementCollection> spChildren;
	OException::Check(spChildrenDispatch->QueryInterface(&spChildren));
	VARIANT varEmpty;
	::VariantInit(&varEmpty);
	VARIANT varZero;
	V_VT(&varZero) = VT_I4;
	V_I4(&varZero) = 0;
	CMyComPtr<IDispatch> spTableDispatch;
	OException::Check(spChildren->item(varEmpty, varZero, &spTableDispatch));
	CMyComPtr<IHTMLTable> spTable;
	OException::Check(spTableDispatch->QueryInterface(&spTable));
	CMyComPtr<IHTMLElementCollection> spRows;
	spTable->get_rows(&spRows);
	CMyComPtr<IHTMLFrameBase> spFrameBase;
	OException::Check(spWindow4->get_frameElement(&spFrameBase));
	CMyComPtr<IHTMLElement> spElement;
	OException::Check(spFrameBase->QueryInterface(&spElement));
	long offsetHeight = 0;
	spElement->get_offsetHeight(&offsetHeight);

	CMyComBSTR bstrHTML;
	while (nLine != nStop)
	{
		CMyComPtr<IDispatch> spRowDispatch;
		spTable->insertRow(nDirection, &spRowDispatch);
		CMyComPtr<IHTMLTableRow> spRow;
		spRowDispatch->QueryInterface(&spRow);

		long nSide = 0;
		do
		{
			CMyComPtr<IDispatch> spCellDispatch;
			CMyComPtr<IHTMLElement> spCellElement;
			GetMarginHTML(nSide, nLine, &bstrHTML);
			spRow->insertCell(-1, &spCellDispatch);
			spCellDispatch->QueryInterface(&spCellElement);
			if (bstrHTML.Length())
			{
				spCellElement->put_innerHTML(bstrHTML);
				spCellElement.Release();
				spCellDispatch.Release();
				spRow->insertCell(-1, &spCellDispatch);
				spCellDispatch->QueryInterface(&spCellElement);
			}
			else
			{
				CMyComPtr<IHTMLTableCell> spCell;
				spCellDispatch->QueryInterface(&spCell);
				spCell->put_colSpan(2);
				spCell->put_align(CMyComBSTR(L"center"));
			}
			GetContentHTML(nSide, nLine, &bstrHTML);
			spCellElement->put_innerHTML(bstrHTML);
		} while (nSide ^= 1);

		long rowCount = 0;
		spRows->get_length(&rowCount);
		long offsetHeightBody = 0;
		spBody->get_offsetHeight(&offsetHeightBody);
		if (offsetHeightBody > offsetHeight && rowCount > 2)
		{
			long rowIndex = 0;
			spRow->get_rowIndex(&rowIndex);
			spTable->deleteRow(rowIndex);
			break;
		}
		nLine -= nDirection;
	}

	*pnLine = nLine;
	return S_OK;
}

HRESULT CChildFrame::WriteReport(BSTR bsPath)
{
	UniStdioFile file;
	if (!file.Open(bsPath, _T("wt")))
		return HRESULT_FROM_WIN32(GetLastError());
	WriteReport(file);
	return S_OK;
}

HRESULT CChildFrame::LimitContext(long nLines)
{
	if ((nLines < -1) || (nLines > 5))
		return E_INVALIDARG;
	SendMessage(WM_COMMAND, nLines >= 0 ?
		ID_VIEW_CONTEXT_0 + nLines : ID_VIEW_CONTEXT_UNLIMITED);
	return S_OK;
}


/**
 * @brief Add or move synchronization point
 */
void CChildFrame::SetSyncPoint()
{
	int nBuffer;
	int nMove = -1; // zero-based index of synchronization point to move
	for (nBuffer = 0; nBuffer < m_nBuffers; nBuffer++)
	{
		int y = m_pView[nBuffer]->GetCursorPos().y;
		int nLine = m_ptBuf[nBuffer]->ComputeApparentLine(m_ptBuf[nBuffer]->ComputeRealLine(y));
		if (m_ptBuf[nBuffer]->GetLineFlags(nLine) & LF_INVALID_BREAKPOINT)
		{
			int nFound = -1;
			do
			{
				++nMove;
				nFound = m_ptBuf[nBuffer]->FindLineWithFlag(LF_INVALID_BREAKPOINT, nFound);
			} while (nFound != -1 && nFound != nLine);
			break;
		}
	}

	for (nBuffer = 0; nBuffer < m_nBuffers; nBuffer++)
	{
		int y = m_pView[nBuffer]->GetCursorPos().y;
		int nLine = m_ptBuf[nBuffer]->ComputeApparentLine(m_ptBuf[nBuffer]->ComputeRealLine(y));
		if (nMove != -1 && (m_ptBuf[nBuffer]->GetLineFlags(nLine) & LF_INVALID_BREAKPOINT) == 0)
		{
			int nLoop = -1;
			int nFound = -1;
			do
			{
				++nLoop;
				nFound = m_ptBuf[nBuffer]->FindLineWithFlag(LF_INVALID_BREAKPOINT, nFound);
			} while (nFound != -1 && nLoop < nMove);
			if (nFound != -1)
			{
				DWORD dwFlags = m_ptBuf[nBuffer]->GetLineFlags(nFound);
				m_ptBuf[nBuffer]->SetLineFlags(nFound, dwFlags & ~LF_INVALID_BREAKPOINT);
			}
		}
		m_ptBuf[nBuffer]->SetLineFlags(nLine, LF_INVALID_BREAKPOINT);
	}

	m_bHasSyncPoints = true;

	for (int nBuffer = 0; nBuffer < m_nBuffers; ++nBuffer)
		m_pView[nBuffer]->SetSelectionMargin(true);

	FlushAndRescan(true);
}

bool CChildFrame::IsCursorAtSyncPoint()
{
	int nBuffer;
	int nCount = 0;
	for (nBuffer = 0; nBuffer < m_nBuffers; nBuffer++)
	{
		int y = m_pView[nBuffer]->GetCursorPos().y;
		int nLine = m_ptBuf[nBuffer]->ComputeApparentLine(m_ptBuf[nBuffer]->ComputeRealLine(y));
		if (m_ptBuf[nBuffer]->GetLineFlags(nLine) & LF_INVALID_BREAKPOINT)
		{
			++nCount;
		}
	}
	return nCount == m_nBuffers;
}

/**
 * @brief Clear the synchronization point at which the cursor is positioned
 */
void CChildFrame::ClearSyncPoint()
{
	m_bHasSyncPoints = false;
	for (int nBuffer = 0; nBuffer < m_nBuffers; nBuffer++)
	{
		int y = m_pView[nBuffer]->GetCursorPos().y;
		int nLine = m_ptBuf[nBuffer]->ComputeApparentLine(m_ptBuf[nBuffer]->ComputeRealLine(y));
		DWORD dwFlags = m_ptBuf[nBuffer]->GetLineFlags(nLine);
		m_ptBuf[nBuffer]->SetLineFlags(nLine, dwFlags & ~LF_INVALID_BREAKPOINT);
		if (m_ptBuf[nBuffer]->FindLineWithFlag(LF_INVALID_BREAKPOINT) != -1)
			m_bHasSyncPoints = true;
	}
	FlushAndRescan(true);
}

/**
 * @brief Clear synchronization points
 */
void CChildFrame::ClearSyncPoints()
{
	m_bHasSyncPoints = false;
	for (int nBuffer = 0; nBuffer < m_nBuffers; ++nBuffer)
	{
		int nLineCount = m_ptBuf[nBuffer]->GetLineCount();
		for (int nLine = 0; nLine < nLineCount; ++nLine)
		{
			DWORD dwFlags = m_ptBuf[nBuffer]->GetLineFlags(nLine);
			m_ptBuf[nBuffer]->SetLineFlags(nLine, dwFlags & ~LF_INVALID_BREAKPOINT);
		}
	}
	FlushAndRescan(true);
}

void CChildFrame::UpdateAllViews()
{
	m_pView[0]->Invalidate();
	m_pView[1]->Invalidate();
	m_pDetailView[0]->Invalidate();
	m_pDetailView[1]->Invalidate();
	m_wndLocationView.ForceRecalculate();
	m_pView[0]->UpdateLineInfoStatus();
	m_pView[1]->UpdateLineInfoStatus();
}
