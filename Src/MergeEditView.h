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
 *
 */
// ID line follows -- this is updated by SVN
// $Id$

#if !defined(AFX_MERGEEDITVIEW_H__0CE31CFD_4BEE_4378_ADB4_B7C9F50A9F53__INCLUDED_)
#define AFX_MERGEEDITVIEW_H__0CE31CFD_4BEE_4378_ADB4_B7C9F50A9F53__INCLUDED_

/** 
 * @brief Color settings.
 */
struct COLORSETTINGS
{
	COLORREF	clrDiff;			/**< Difference color */
	COLORREF	clrSelDiff;			/**< Selected difference color */
	COLORREF	clrDiffDeleted;		/**< Difference deleted color */
	COLORREF	clrSelDiffDeleted;	/**< Selected difference deleted color */
	COLORREF	clrDiffText;		/**< Difference text color */
	COLORREF	clrSelDiffText;		/**< Selected difference text color */
	COLORREF	clrTrivial;			/**< Ignored difference color */
	COLORREF	clrTrivialDeleted;	/**< Ignored difference deleted color */
	COLORREF	clrTrivialText;		/**< Ignored difference text color */
	COLORREF	clrMoved;			/**< Moved block color */
	COLORREF	clrMovedDeleted;	/**< Moved block deleted color */
	COLORREF	clrMovedText;		/**< Moved block text color */
	COLORREF	clrSelMoved;		/**< Selected moved block color */
	COLORREF	clrSelMovedDeleted;	/**< Selected moved block deleted color */
	COLORREF	clrSelMovedText;	/**< Selected moved block text color */
	COLORREF	clrWordDiff;		/**< Word difference color */
	COLORREF	clrWordDiffDeleted;	/**< Word differenceDeleted color */
	COLORREF	clrWordDiffText;	/**< Word difference text color */
	COLORREF	clrSelWordDiff;		/**< Selected word difference color */
	COLORREF	clrSelWordDiffDeleted;	/**< Selected word difference deleted color */
	COLORREF	clrSelWordDiffText;	/**< Selected word difference text color */
};

/** 
 * @brief Non-diff lines shown above diff when scrolling to it
 */
const UINT CONTEXT_LINES_ABOVE = 5;

/** 
 * @brief Non-diff lines shown below diff when scrolling to it
 */
const UINT CONTEXT_LINES_BELOW = 3;


#define FLAG_RESCAN_WAITS_FOR_IDLE   1


/////////////////////////////////////////////////////////////////////////////
// CMergeEditView view
#include "edtlib.h"
#include "GhostTextView.h"

class CLocationView;
class CChildFrame;
struct DIFFRANGE;

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
	CMergeEditView(HWindow *, CChildFrame *, int);

	STDMETHOD(DragEnter)(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
	STDMETHOD(Drop)(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

	CCrystalParser m_xParser; /**< Syntax parser used for syntax highlighting. */

	// Object that displays status line info for one side of a merge view
	HStatusBar *m_pStatusBar;
	void SetLineInfoStatus(LPCTSTR szLine, int nColumn, int nColumns,
		int nChar, int nChars, LPCTSTR szEol, EDITMODE editMode);
	void UpdateLineInfoStatus();
	void SetEncodingStatus(LPCTSTR);
	void SetCRLFModeStatus(enum CRLFSTYLE);
protected:
	/**
	 * Are automatic rescans enabled?
	 * If automatic rescans are enabled then we rescan files after edit
	 * events, unless timer suppresses rescan. We suppress rescans within
	 * certain time from previous rescan.
	 */
	bool m_bAutomaticRescan;

private:
	COLORSETTINGS m_cachedColors; /**< Cached color settings */

// Operations
public:
	using CGhostTextView::GetFullySelectedLines;
	using CGhostTextView::GetSelection;
	void RefreshOptions();
	bool EnableRescan(bool bEnable);
	void ShowDiff(bool bScroll);
	virtual void OnEditOperation(int nAction, LPCTSTR pszText);
	virtual void OnUpdateCaret(bool bShowHide = false);
	void UpdateLineLengths();
	bool IsLineInCurrentDiff(int nLine);
	void SelectNone();
	void SelectDiff(int nDiff, bool bScroll = true);
	CDiffTextBuffer *GetTextBuffer() { return static_cast<CDiffTextBuffer *>(m_pTextBuffer); }
	void UpdateResources();
	virtual int RecalcVertScrollBar(bool bPositionOnly = false);
	virtual int GetAdditionalTextBlocks(int nLineIndex, TEXTBLOCK *&pBuf);
	virtual COLORREF GetColor(int nColorIndex);
	virtual void GetLineColors(int nLineIndex, COLORREF &crBkgnd, COLORREF &crText);
	void GotoLine(int nLine);
	int GetTopLine() { return m_nTopLine; }
	int GetTopSubLine() { return m_nTopSubLine; }
	using CCrystalTextView::GetScreenLines;
	using CCrystalTextView::GetSubLines;
	using CCrystalTextView::GetSubLineCount;
	using CCrystalTextView::GetSubLineIndex;
	using CCrystalTextView::GetLineBySubLine;
	virtual int GetEmptySubLines( int nLineIndex );
	virtual void InvalidateSubLineIndexCache( int nLineIndex );
	void DocumentsLoaded();

	bool IsDiffVisible(int nDiff);

// Implementation
protected:
	virtual ~CMergeEditView();
	bool IsDiffVisible(const DIFFRANGE *, int nLinesBelow = 0);

	HMenu *ApplyPatch(IStream *, int);
	void ApplyPatch(IStream *, const POINTL &);

	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	void OnNotity(LPARAM);
	void OnLButtonDblClk();
	void OnLButtonUp();
	void OnContextMenu(LPARAM);
	void OnSize();
	void OnTimer(UINT_PTR);

public:
	void OnCurdiff();
	void OnFirstdiff();
	void OnLastdiff();
	void OnNextdiff();
	void OnPrevdiff();
	void OnSelectLineDiff();
	void OnConvertEolTo(UINT);
	void OnEditCopyLineNumbers();
	void OnViewMargin();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MERGEEDITVIEW_H__0CE31CFD_4BEE_4378_ADB4_B7C9F50A9F53__INCLUDED_)
