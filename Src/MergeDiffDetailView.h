/** 
 * @file  MergeDiffDetailView.h
 *
 * @brief Declaration of CMergeDiffDetailView class
 */
#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMergeDiffDetailView view
#include "GhostTextView.h"

/**
 * @brief Class for Diff Pane View
 *
 * @note This class must not be used in a vertical scrollable splitter
 * as we want to scroll only in the current diff, but the vertical
 * scrollbar would be for the whole buffer.
 * There are two virtual functions : ScrollToSubLine/SetSelection
 * to be sure that the top line and the cursor/selection pos remains in the
 * current diff.
 */
class CMergeDiffDetailView : public CGhostTextView
{
public:
	CMergeDiffDetailView(HWindow *, CChildFrame *, int);
protected:
	/// first line of diff (first displayable line)
	int m_lineBegin;
	/// last line of diff (last displayable line)
	int m_lineEnd; 
	/// memorize first line of diff
	int m_lineBeginPushed;
	/// memorize last line of diff
	int m_lineEndPushed;

// Operations
private:
	int GetDiffLineLength();
	bool EnsureInDiff(POINT &);

public:
	void RefreshOptions();
	void OnUpdateCaret(bool bShowHide);
	void DocumentsLoaded();

	virtual bool QueryEditable() { return false; }
	virtual bool IsLineInCurrentDiff(int) { return false; }

	virtual int RecalcHorzScrollBar(bool bPositionOnly = false);
	virtual int RecalcVertScrollBar(bool bPositionOnly = false);

	virtual void SetSelection(const POINT &ptStart, const POINT &ptEnd);

	void OnDisplayDiff(int nDiff);

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

// Implementation
protected:
	virtual void ScrollToSubLine(int nNewTopLine);
	virtual ~CMergeDiffDetailView();
	virtual void DrawSingleLine(HSurface *, const RECT &, int nLineIndex);
	virtual void GetLineColors(int nLineIndex, COLORREF &crBkgnd, COLORREF &crText);
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	void OnContextMenu(LPARAM);
};
