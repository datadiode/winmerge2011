/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997  Dean P. Grimm
//    SPDX-License-Identifier: GPL-3.0-or-later
/////////////////////////////////////////////////////////////////////////////
/**
 * @file  ImgMergeFrm.h
 *
 * @brief interface of the CSQLiteMergeFrame class
 *
 */
#pragma once

/**
 * @brief Frame class for file compare, handles panes, statusbar etc.
 */
class CSQLiteMergeFrame
	: ZeroInit<CSQLiteMergeFrame>
	, public CEditorFrame
    , public IUIAutomationEventHandler
{
public:
	CSQLiteMergeFrame(CMainFrame *, CDirFrame * = NULL);

// Operations
	void OpenDocs(FileLocation &, FileLocation &, bool bROLeft, bool bRORight);
	bool SaveModified();
	void UpdateResources();

	virtual void RecalcLayout() override;
	virtual BOOL PreTranslateMessage(MSG *) override;

	// IUnknown
	STDMETHOD(QueryInterface)(REFIID iid, void **ppv)
	{
		static const QITAB rgqit[] =
		{
			QITABENT(CSQLiteMergeFrame, IUIAutomationEventHandler),
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
	virtual ~CSQLiteMergeFrame();
	void SetTitle();
	void UpdateCmdUI();
	virtual FRAMETYPE GetFrameType() const { return FRAME_SQLITEDB; }
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	template<UINT>
	LRESULT OnWndMsg(WPARAM, LPARAM);

    CMyComPtr<IUIAutomationElement> m_spToolStrip;
    CMyComPtr<IAccessible> m_spGenerateChangeScriptLeftToRight;
    CMyComPtr<IAccessible> m_spGenerateChangeScriptRightToLeft;
    CMyComPtr<IAccessible> m_spAccExit;
    CMyComPtr<IAccessible> m_spAccRefresh;
    CMyComPtr<IAccessible> m_spAccPrevDiff;
    CMyComPtr<IAccessible> m_spAccNextDiff;
    CMyComPtr<IAccessible> m_spAccCopyLeftToRight;
    CMyComPtr<IAccessible> m_spAccCopyRightToLeft;
    CMyComPtr<IAccessible> m_spAccEditSelectedItem;
    CMyComPtr<IAccessible> m_spAccExportDataDiffs;
};
