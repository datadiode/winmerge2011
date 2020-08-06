/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997  Dean P. Grimm
//    SPDX-License-Identifier: GPL-3.0-or-later
/////////////////////////////////////////////////////////////////////////////
/**
 * @file  ImgMergeFrm.h
 *
 * @brief interface of the CImgMergeFrame class
 *
 */
#pragma once

#include "WinIMergeLib.h"

/** 
 * @brief Frame class for file compare, handles panes, statusbar etc.
 */
class CImgMergeFrame
	: ZeroInit<CImgMergeFrame>
	, public CEditorFrame
	, private CFloatFlags
{
public:
	CImgMergeFrame(CMainFrame *, CDirFrame * = NULL);

// Operations
public:
	HRESULT OpenDocs(const FileLocation &, const FileLocation &, BOOL bROLeft, BOOL bRORight);
	bool SaveModified();
	void UpdateResources();
	bool IsModified() const;
	BOOL IsFileChangedOnDisk(int pane) const;

	virtual void ActivateFrame() override;
	virtual void SavePosition() override;
	virtual void RecalcLayout() override;
	virtual BOOL PreTranslateMessage(MSG *) override;
	void UpdateCmdUI();

// Implementation
private:
	virtual ~CImgMergeFrame();
	void LoadOptions();
	void SaveOptions();
	int UpdateDiffItem(CDirFrame *);
	void SetLastCompareResult(int nResult);
	void UpdateHeaderPath(int pane);
	void SetTitle();
    void UpdateLastCompareResult();
	void OnFileSave();
	void OnFileSaveLeft();
	void OnFileSaveRight();
	void OnFileSaveAsLeft();
	void OnFileSaveAsRight();
	void OnFirstdiff();
	void OnLastdiff();
	void OnNextdiff();
	void OnPrevdiff();
	void OnX2Y(int srcPane, int dstPane);
	void OnL2r();
	void OnR2l();
	void OnAllLeft();
	void OnAllRight();
	void OnEditUndo();
	void OnEditRedo();
	void OnViewZoomIn();
	void OnViewZoomOut();
	void OnViewZoomNormal();
    void OnWindowChangePane();
    int TrySaveAs(int pane, String &);
	bool DoSave(int pane);
	bool DoSaveAs(int pane);
	static void OnChildPaneEvent(const IImgMergeWindow::Event& evt);
    static void CALLBACK HandleWinEvent(HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD);
	IImgMergeWindow *m_pImgMergeWindow;
	IImgToolWindow *m_pImgToolWindow;

	virtual FRAMETYPE GetFrameType() const { return FRAME_IMGFILE; }
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	template<UINT>
	LRESULT OnWndMsg(WPARAM, LPARAM);
	BOOL CreateClient();
// Implementation data
	CSubFrame m_wndLocationBar;
    FileInfo m_fileInfo[2];
    HStatusBar *m_pStatusBar[2];
	static const LONG FloatScript[];
};
