/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997  Dean P. Grimm
//    SPDX-License-Identifier: GPL-3.0-or-later
/////////////////////////////////////////////////////////////////////////////
/**
 * @file  ReoGridMergeFrm.h
 *
 * @brief interface of the CReoGridMergeFrame class
 *
 */
#pragma once

/**
 * @brief Frame class for file compare, handles panes, statusbar etc.
 */
class CReoGridMergeFrame
	: ZeroInit<CReoGridMergeFrame>
	, public CEditorFrame
	, public IUIAutomationEventHandler
{
public:
	CReoGridMergeFrame(CMainFrame *, CDirFrame * = NULL);

// Operations
	virtual void OpenDocs(FileLocation &, FileLocation &, bool bROLeft, bool bRORight) override;
	virtual bool SaveModified() override;
	virtual void ReplaceSelection(int pane, String const &text) override;
	void UpdateResources();

	virtual void RecalcLayout() override;
	virtual BOOL PreTranslateMessage(MSG *) override;

	// IUnknown
	STDMETHOD(QueryInterface)(REFIID iid, void **ppv)
	{
		static const QITAB rgqit[] =
		{
			QITABENT(CReoGridMergeFrame, IUIAutomationEventHandler),
			{ 0 }
		};
		return QISearch(this, rgqit, iid, ppv);
	}
	STDMETHOD_(ULONG, AddRef)()
	{
		return 1;
	}
	STDMETHOD_(ULONG, Release)()
	{
		return 1;
	}
	// IUIAutomationEventHandler
	virtual HRESULT STDMETHODCALLTYPE HandleAutomationEvent(IUIAutomationElement *sender, EVENTID eventId);

// Implementation
private:
	virtual ~CReoGridMergeFrame();
	void SetTitle();
	void SetLastCompareResult(bool);
	void UpdateLastCompareResult();
	void UpdateDiffItem(CDirFrame *);
	void UpdateCmdUI();
	virtual FRAMETYPE GetFrameType() const { return FRAME_REOGRID; }
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	template<UINT>
	LRESULT OnWndMsg(WPARAM, LPARAM);

	void OnToolsCompareSelection();

	BYTE m_cmdStateSaveLeft;
	BYTE m_cmdStateSaveRight;

	CMyComPtr<IUIAutomationElement> m_spToolStrip;
	CMyComPtr<IAccessible> m_spAccExit;
	CMyComPtr<IAccessible> m_spAccSave;
	CMyComPtr<IAccessible> m_spAccSaveLeft;
	CMyComPtr<IAccessible> m_spAccSaveLeftAs;
	CMyComPtr<IAccessible> m_spAccSaveRight;
	CMyComPtr<IAccessible> m_spAccSaveRightAs;
	CMyComPtr<IAccessible> m_spAccFormatCells;
	CMyComPtr<IAccessible> m_spAccRecalculate;
	CMyComPtr<IAccessible> m_spAccLanguage;
	CMyComPtr<IAccessible> m_spAccCut;
	CMyComPtr<IAccessible> m_spAccCopy;
	CMyComPtr<IAccessible> m_spAccPaste;
	CMyComPtr<IAccessible> m_spAccUndo;
	CMyComPtr<IAccessible> m_spAccRedo;
	CMyComPtr<IAccessible> m_spAccPrevDiff;
	CMyComPtr<IAccessible> m_spAccNextDiff;
	CMyComPtr<IAccessible> m_spAccFirstDiff;
	CMyComPtr<IAccessible> m_spAccLastDiff;
	CMyComPtr<IAccessible> m_spAccCopyLeftToRight;
	CMyComPtr<IAccessible> m_spAccCopyRightToLeft;
	CMyComPtr<IAccessible> m_spAccZoom;
	CMyComPtr<IAccessible> m_spAccLeftHeader;
	CMyComPtr<IAccessible> m_spAccRightHeader;
	CMyComPtr<IAccessible> m_spAccLeftGrid;
	CMyComPtr<IAccessible> m_spAccRightGrid;
	CMyComPtr<IAccessible> m_spAccLeftGridClient;
	CMyComPtr<IAccessible> m_spAccRightGridClient;
};
