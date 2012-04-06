//////////////////////////////////////////////////////////////////////
/** 
 * @file  MergeDiffDetailView.h
 *
 * @brief Declaration of CMergeDiffDetailView class
 */
// ID line follows -- this is updated by SVN
// $Id$
//
//////////////////////////////////////////////////////////////////////
#ifndef __MERGEDIFFDETAILVIEW_H__
#define __MERGEDIFFDETAILVIEW_H__


/////////////////////////////////////////////////////////////////////////////
// CMergeDiffDetailView view
#include "edtlib.h"


/**
 * @brief Class for Diff Pane View
 *
 * @note This class must not be used in a vertical scrollable splitter
 * as we want to scroll only in the current diff, but the vertical
 * scrollbar would be for the whole buffer.
 * There are three virtual functions : ScrollToSubLine/EnsureVisible/SetSelection
 * to be sure that the top line and the cursor/selection pos remains in the
 * current diff.
 */
class CMergeDiffDetailView : public CCrystalTextView
{
public:
	CMergeDiffDetailView(CChildFrame *, int);           // protected constructor used by dynamic creation

// Attributes
public:
	int m_nThisPane;
	CChildFrame *const m_pDocument;
protected:
	/// first line of diff (first displayable line)
	int m_lineBegin;
	/// last line of diff (last displayable line)
	int m_lineEnd; 
	/// number of displayed lines
	int m_diffLength;
	/// height (in lines) of the view
	int m_displayLength;

	/// memorize first line of diff
	int m_lineBeginPushed;
	/// memorize cursor position
	POINT m_ptCursorPosPushed;
	/// memorize top line positions
	int m_nTopSubLinePushed;

// Operations
private:
	int GetDiffLineLength();

public:
	virtual CCrystalTextBuffer *LocateTextBuffer ();
	void UpdateResources();
	BOOL IsModified() { return FALSE; }
	BOOL PrimeListWithFile();
	void SetDisplayHeight(int h);
	void OnUpdateCaret(bool bMove);
	void DocumentsLoaded();

	virtual void UpdateSiblingScrollPos (BOOL bHorz);
	virtual void RecalcHorzScrollBar (BOOL bPositionOnly = FALSE );

	virtual void EnsureVisible (POINT pt);
	virtual void SetSelection (const POINT & ptStart, const POINT & ptEnd);

	virtual void OnDisplayDiff(int nDiff=0);

	/* Push cursors before detaching buffer
	 *
	 * @note : laoran 2003/10/03 : don't bother with real lines. 
	 * I tried and it does not work fine
	 */
	void PushCursors();
	/*
	 * @brief Pop cursors after attaching buffer
	 *
	 * @note : also scroll to the old top line
	 */
	void PopCursors();

protected:
	BOOL EnsureInDiff(POINT & pt);
	virtual void ScrollToSubLine (int nNewTopLine, BOOL bNoSmoothScroll = FALSE, BOOL bTrackScrollBar = TRUE);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMergeDiffDetailView)
	public:
	virtual void OnInitialUpdate();
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CMergeDiffDetailView();
	virtual int GetAdditionalTextBlocks (int nLineIndex, TEXTBLOCK *pBuf);
	virtual COLORREF GetColor(int nColorIndex);
	virtual void GetLineColors (int nLineIndex, COLORREF & crBkgnd,
                              COLORREF & crText, BOOL & bDrawWhitespace);
	virtual void GetLineColors2 (int nLineIndex, DWORD ignoreFlags, COLORREF & crBkgnd,
                              COLORREF & crText, BOOL & bDrawWhitespace);
	virtual void OnUpdateSibling (CCrystalTextView * pUpdateSource, BOOL bHorz);

	// Generated message map functions
protected:
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	void OnContextMenu(LPARAM);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.


#endif //__MERGEDIFFDETAILVIEW_H__
