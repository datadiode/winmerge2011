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
 * @file  ChildFrm.h
 *
 * @brief interface of the CHexMergeFrame class
 *
 */
#define HEKSEDIT_INTERFACE_VERSION 1
#include "heksedit.h"

class CHexMergeView;

/** 
 * @brief Frame class for file compare, handles panes, statusbar etc.
 */
class CHexMergeFrame
	: ZeroInit<CHexMergeFrame>
	, public CEditorFrame
	, private CFloatFlags
{
public:
	CHexMergeFrame(CMainFrame *, CDirFrame * = NULL);
	void RecalcBytesPerLine();
	void UpdateResources();
	BOOL PreTranslateMessage(MSG *);
	void UpdateCmdUI();
	HRESULT OpenDocs(const FileLocation &, const FileLocation &, BOOL bROLeft, BOOL bRORight);
	bool SaveModified();
	void SetTitle();

private:
	virtual ~CHexMergeFrame();
	virtual FRAMETYPE GetFrameType() const { return FRAME_BINARY; }
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	template<UINT>
	LRESULT OnWndMsg(WPARAM, LPARAM);
	void CreateClient();
	CHexMergeView *CreatePane(int);
	void SetLastCompareResult(int nResult);
	int UpdateDiffItem(CDirFrame *);
	bool PromptAndSaveIfNeeded(bool bAllowCancel);
	CHexMergeView *GetActiveView() const;
	void UpdateHeaderPath(int pane);
	static void CopySel(CHexMergeView *pViewSrc, CHexMergeView *pViewDst);
	static void CopyAll(CHexMergeView *pViewSrc, CHexMergeView *pViewDst);
	HRESULT LoadOneFile(int index, const FileLocation &fileinfo, BOOL readOnly);
	void OnRefresh();
	void OnFileSave();
	void OnFileSaveLeft();
	void OnFileSaveRight();
	void OnFileSaveAsLeft();
	void OnFileSaveAsRight();
	void OnL2r();
	void OnR2l();
	void OnAllRight();
	void OnAllLeft();
	void OnViewZoom(int direction);

// Implementation data
	static const LONG FloatScript[];
	static const LONG SplitScript[];
	CHexMergeView *m_pView[MERGE_VIEW_COUNT]; /**< Pointer to left/right view */
};
