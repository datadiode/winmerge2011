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
 * @file  EditorFilePathBar.h
 *
 * @brief Interface of the CEditorFilePathBar class.
 *
 */
#pragma once

#include <Common/FloatState.h>
#include <Common/SplitState.h>

/**
 * @brief Types for buffer. Buffer's type defines behavior
 * of buffer when saving etc.
 * 
 * Difference between BUFFER_NORMAL and BUFFER_NORMAL_NAMED is
 * that _NAMED has description text given and which is shown
 * instead of filename.
 *
 * BUFFER_UNNAMED is created empty buffer (scratchpad), it has
 * no filename, and default description is given for it. After
 * this buffer is saved it becomes _SAVED. It is not equal to
 * NORMAL_NAMED, since scratchpads don't have plugins etc.
 */
enum BUFFERTYPE
{
	BUFFER_NORMAL, /**< Normal, file loaded from disk */
	BUFFER_NORMAL_NAMED, /**< Normal, description given */
	BUFFER_UNNAMED, /**< Empty, created buffer */
	BUFFER_UNNAMED_SAVED, /**< Empty buffer saved with filename */
};

/**
 * @brief A dialog bar with two controls for left/right path.
 * This class is a dialog bar for the both files path in the editor. 
 * The bar looks like a statusBar (font, height). The control
 * displays a tip for each path (as a tooltip). 
 */
class CEditorFilePathBar
	: ZeroInit<CEditorFilePathBar>
	, public ODialog
	, public CFloatState
	, public CSplitState
{
public : 
	CEditorFilePathBar(const LONG *FloatScript = NULL, const LONG *SplitScript = NULL);
	~CEditorFilePathBar();

	BOOL Create(HWND);

	// Implement IFilepathHeaders
	void SetText(int pane, LPCTSTR, BOOL bDirty, BUFFERTYPE = BUFFER_NORMAL);
	BOOL GetModify(int pane);
	void SetModify(int pane, BOOL bDirty);
	void SetActive(int pane, bool bActive);
	bool HasFocus(int pane) const { return m_rgFocused[pane]; }
	HEdit *GetControlRect(int pane, LPRECT);
	const String &GetTitle(int pane);

protected:
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	void OnToolTipNotify(TOOLTIPTEXT *);
	HBRUSH OnCtlColorPane(HSurface *, bool bActive);
	void OnContextMenu(POINT);

private:
	HToolTips *m_pToolTips;
	String m_sToolTipString;
	std::vector<String> m_rgOriginalText;
	bool m_rgActive[2];
	bool m_rgFocused[2];
	HEdit *m_Edit[2]; /**< Edit controls. */
};
