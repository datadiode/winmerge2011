/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997  Dean P. Grimm
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
 * @file  MergeEditView.h
 *
 * @brief Declaration file for CMergeEditView
 */
#pragma once

#include "GhostTextView.h"

/** 
 * @brief Non-diff lines shown above diff when scrolling to it
 */
const UINT CONTEXT_LINES_ABOVE = 5;

/** 
 * @brief Non-diff lines shown below diff when scrolling to it
 */
const UINT CONTEXT_LINES_BELOW = 3;

/////////////////////////////////////////////////////////////////////////////
// CMergeEditView view

class CLocationView;
class CChildFrame;
struct DIFFRANGE;
class CShellContextMenu;

/**
This class is the base class for WinMerge editor panels.
It hooks the painting of ghost lines (GetLineColors), the shared
scrollbar (OnUpdateSibling...).
It offers the UI interface commands to work with diffs 

@todo
If we keep GetLineColors here, we should clear DIFF flag here
and not in CGhostTextBuffer (when insertText/deleteText). 
Small problem... This class doesn't derives from CGhostTextBuffer... 
We could define a new class which derives from CGhostTextBuffer to clear the DIFF flag.
and calls a virtual function for additional things to do on the flag.
Maybe in the future...
*/
class CMergeEditView : public CGhostTextView
{
public:
	CCrystalParser m_xParser; /**< Syntax parser used for syntax highlighting. */
	// Object that displays status line info for one side of a merge view
	HStatusBar *m_pStatusBar;

protected:
	CShellContextMenu *const m_pShellContextMenu; /**< Shell context menu*/
	HMENU m_hShellContextMenu;

public:
	CMergeEditView(HWindow *, CChildFrame *, int);

	// IDropTarget
	STDMETHOD(DragEnter)(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
	STDMETHOD(Drop)(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

	static bool IsShellMenuCmdID(UINT);
	void HandleMenuSelect(WPARAM wParam, LPARAM lParam);
	LRESULT HandleMenuMessage(UINT message, WPARAM wParam, LPARAM lParam);
	void SetLineInfoStatus(LPCTSTR szLine, int nColumn, int nColumns,
		int nChar, int nChars, LPCTSTR szEol, EDITMODE editMode);
	void UpdateLineInfoStatus();
	void SetEncodingStatus(LPCTSTR);
	void SetCRLFModeStatus(enum CRLFSTYLE);
	void RefreshOptions();
	void ShowDiff(bool bScroll);
	virtual void OnEditOperation(int nAction, LPCTSTR pszText);
	virtual void OnUpdateCaret(bool bShowHide = false);
	void UpdateLineLengths();
	virtual bool IsLineInCurrentDiff(int);
	void SelectNone();
	void SelectDiff(int nDiff, bool bScroll = true);
	CDiffTextBuffer *GetTextBuffer() { return static_cast<CDiffTextBuffer *>(m_pTextBuffer); }
	void UpdateResources();
	virtual int RecalcVertScrollBar(bool bPositionOnly = false);
	virtual void GetLineColors(int nLineIndex, COLORREF &crBkgnd, COLORREF &crText);
	void GotoLine(int nLine);
	int GetTopLine() const { return m_nTopLine; }
	int GetTopSubLine() const { return m_nTopSubLine; }
	virtual int GetEmptySubLines(int nLineIndex);
	virtual void InvalidateSubLineIndexCache(int nLineIndex);
	void DocumentsLoaded();

	void OnCurdiff();
	void OnFirstdiff();
	void OnLastdiff();
	void OnNextdiff();
	void OnPrevdiff();
	void OnSelectLineDiff();
	void OnConvertEolTo(UINT);
	void OnEditCopyLineNumbers();
	void OnViewMargin();

	using CGhostTextView::GetFullySelectedLines;
	using CGhostTextView::GetSelection;
	using CCrystalTextView::GetScreenLines;
	using CCrystalTextView::GetSubLines;
	using CCrystalTextView::GetSubLineCount;
	using CCrystalTextView::GetSubLineIndex;
	using CCrystalTextView::GetLineBySubLine;

protected:
	virtual ~CMergeEditView();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	virtual bool IsDiffVisible(int nDiff);
	bool IsDiffVisible(const DIFFRANGE *, int nLinesBelow = 0);
	HMENU ListShellContextMenu();
	HMenu *ApplyPatch(IStream *, int);
	void ApplyPatch(IStream *, const POINTL &);
	void OnNotity(LPARAM);
	void OnLButtonDblClk();
	void OnLButtonUp();
	void OnContextMenu(LPARAM);
	void OnSize();
	void OnTimer(UINT_PTR);
};
