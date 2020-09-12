/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997-2000  Thingamahoochie Software
//    Author: Dean Grimm
//    SPDX-License-Identifier: GPL-3.0-or-later
/////////////////////////////////////////////////////////////////////////////
/**
 * @file  SQLiteMergeFrm.cpp
 *
 * @brief Implementation file for CSQLiteMergeFrame
 *
 */
#include "stdafx.h"
#include "Merge.h"
#include "MainFrm.h"
#include "DirFrame.h"
#include "SQLiteMergeFrm.h"
#include "paths.h"
#include "Common/coretools.h"
#include "Common/DllProxies.h"
#include "Common/AccChildren.h"
#include "Common/SettingStore.h"
#include <cmath>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CSQLiteMergeFrame

/////////////////////////////////////////////////////////////////////////////
// CSQLiteMergeFrame construction/destruction

CSQLiteMergeFrame::CSQLiteMergeFrame(CMainFrame *pMDIFrame, CDirFrame *pDirDoc)
: CEditorFrame(pMDIFrame, pDirDoc, GetHandleSet<IDR_SQLITEDOCTYPE>(), NULL)
{
	Subclass(pMDIFrame->CreateChildHandle());
	m_bAutoDelete = true;
}

CSQLiteMergeFrame::~CSQLiteMergeFrame()
{
	if (m_pDirDoc)
		m_pDirDoc->MergeDocClosing(this);
}

void CSQLiteMergeFrame::RecalcLayout()
{
	if (HWindow *const pwndGuest = GetTopWindow())
	{
		RECT rc;
		GetClientRect(&rc);
		pwndGuest->MoveWindow(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
	}
}

HRESULT CSQLiteMergeFrame::HandleAutomationEvent(IUIAutomationElement *, EVENTID)
{
	UpdateCmdUI();
	return S_OK;
}

void CSQLiteMergeFrame::OpenDocs(
	FileLocation &filelocLeft,
	FileLocation &filelocRight,
	bool bROLeft, bool bRORight)
{
	m_strPath[0] = filelocLeft.filepath;
	ASSERT(m_nBufferType[0] == BUFFER_NORMAL); // should have been initialized to BUFFER_NORMAL in constructor
	if (!filelocLeft.description.empty())
	{
		m_strDesc[0] = filelocLeft.description;
		m_nBufferType[0] = BUFFER_NORMAL_NAMED;
	}
	m_strPath[1] = filelocRight.filepath;
	ASSERT(m_nBufferType[1] == BUFFER_NORMAL); // should have been initialized to BUFFER_NORMAL in constructor
	if (!filelocRight.description.empty())
	{
		m_strDesc[1] = filelocRight.description;
		m_nBufferType[1] = BUFFER_NORMAL_NAMED;
	}

	SetTitle();

	int const flags = COptionsMgr::Get(OPT_CMP_SQLITE_COMPAREFLAGS);
	DWORD const result = m_pMDIFrame->CLRExecSQLiteCompare(
		L"SQLiteTurbo.Program", L"Embed", string_format(
		L"/ParentWindow %p%hs%hs%hs \"%s\" \"%s\"", m_hWnd,
		&"\0 /CompareSchemaOnly"[(flags & SQLITE_CMP_RADIO_OPTIONS_MASK) == SQLITE_CMP_SCHEMA_ONLY],
		&"\0 /CompareBlobFields"[(flags & SQLITE_CMP_COMPARE_BLOB_FIELDS) != 0],
		&"\0 /ShowCompareDialog"[(flags & SQLITE_CMP_PROMPT_FOR_OPTIONS) != 0],
		paths_UndoMagic(wcsdupa(filelocLeft.filepath.c_str())),
		paths_UndoMagic(wcsdupa(filelocRight.filepath.c_str()))).c_str());

	if (result == IDCANCEL)
		OException::ThrowSilent();

	if (HWindow *const pwndGuest = GetTopWindow())
	{
		CMyComPtr<IAccessible> spAccessible;
		OException::Check(AccessibleObjectFromWindow(pwndGuest->m_hWnd, OBJID_CLIENT, IID_PPV_ARGS(&spAccessible)));
		if (AccChildren formChildren = spAccessible)
		{
			if (AccChildren windowChildren = formChildren.Find(L"MenuStrip", ROLE_SYSTEM_WINDOW))
			{
				if (AccChildren menubarChildren = windowChildren.Find(L"MenuStrip", ROLE_SYSTEM_MENUBAR))
				{
					if (AccChildren menuitemChildren = menubarChildren.Find(L"File", ROLE_SYSTEM_MENUITEM))
					{
						m_spGenerateChangeScriptLeftToRight = menuitemChildren.Find(L"GenerateChangeScriptLeftToRight", ROLE_SYSTEM_MENUITEM);
						m_spGenerateChangeScriptRightToLeft = menuitemChildren.Find(L"GenerateChangeScriptRightToLeft", ROLE_SYSTEM_MENUITEM);
						m_spAccExit = menuitemChildren.Find(L"Exit", ROLE_SYSTEM_MENUITEM);
					}
					if (AccChildren menuitemChildren = menubarChildren.Find(L"View", ROLE_SYSTEM_MENUITEM))
					{
						m_spAccRefresh = menuitemChildren.Find(L"Refresh", ROLE_SYSTEM_MENUITEM);
					}
				}
			}
			if (AccChildren windowChildren = formChildren.Find(L"ToolStrip", ROLE_SYSTEM_WINDOW))
			{
				IAccessible *const toolbar = windowChildren.Find(L"ToolStrip", ROLE_SYSTEM_TOOLBAR);
				if (IUIAutomation *const automation = m_pMDIFrame->UIAutomation())
					automation->ElementFromIAccessible(toolbar, 0, &m_spToolStrip);
				if (AccChildren toolbarChildren = toolbar)
				{
					m_spAccPrevDiff = toolbarChildren.Find(L"PrevDiff", ROLE_SYSTEM_PUSHBUTTON);
					m_spAccNextDiff = toolbarChildren.Find(L"NextDiff", ROLE_SYSTEM_PUSHBUTTON);
					m_spAccCopyLeftToRight = toolbarChildren.Find(L"CopyLeftToRight", ROLE_SYSTEM_PUSHBUTTON);
					m_spAccCopyRightToLeft = toolbarChildren.Find(L"CopyRightToLeft", ROLE_SYSTEM_PUSHBUTTON);
					m_spAccEditSelectedItem = toolbarChildren.Find(L"EditSelectedItem", ROLE_SYSTEM_PUSHBUTTON);
					m_spAccExportDataDiffs = toolbarChildren.Find(L"ExportDataDiffs", ROLE_SYSTEM_PUSHBUTTON);
				}
			}
		}
	}

	m_pMDIFrame->AddAutomationEventHandler(m_spToolStrip, this);

	ActivateFrame();
	PostMessage(WM_COMMAND, ID_REFRESH);
}

/**
 * @brief Update document filenames to title
 */
void CSQLiteMergeFrame::SetTitle()
{
	string_format sTitle(_T("%s - %s"),
		!m_strDesc[0].empty() ? m_strDesc[0].c_str() : PathFindFileName(m_strPath[0].c_str()),
		!m_strDesc[1].empty() ? m_strDesc[1].c_str() : PathFindFileName(m_strPath[1].c_str()));
	SetWindowText(sTitle.c_str());
}

/**
 * @brief Asks and then saves modified files.
 * This function is not applicable to SQLite Compare, and hence does nothing.
 */
bool CSQLiteMergeFrame::SaveModified()
{
	return true;
}

/**
 * @brief Update any resources necessary after a GUI language change
 */
void CSQLiteMergeFrame::UpdateResources()
{
}

void CSQLiteMergeFrame::UpdateCmdUI()
{
	if (m_pMDIFrame->GetActiveDocFrame() != this)
		return;
	m_pMDIFrame->UpdateCmdUI<ID_NEXTDIFF>(CMainFrame::GetCommandState(m_spAccNextDiff) | 0x10);
	m_pMDIFrame->UpdateCmdUI<ID_PREVDIFF>(CMainFrame::GetCommandState(m_spAccPrevDiff) | 0x10);
	m_pMDIFrame->UpdateCmdUI<ID_CURDIFF>(CMainFrame::GetCommandState(m_spAccEditSelectedItem));
	m_pMDIFrame->UpdateCmdUI<ID_TOOLS_EXPORT_DATA_DIFFS>(CMainFrame::GetCommandState(m_spAccExportDataDiffs));
}

/**
 * @brief Check for keyboard commands
 */
BOOL CSQLiteMergeFrame::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
		return FALSE; // Shortcut IsDialogMessage() to retain OPT_CLOSE_WITH_ESC functionality
	return ::IsDialogMessage(m_hWnd, pMsg);
}

template<>
LRESULT CSQLiteMergeFrame::OnWndMsg<WM_COMMAND>(WPARAM wParam, LPARAM lParam)
{
	switch (const UINT id = lParam ? static_cast<UINT>(wParam) : LOWORD(wParam))
	{
	case ID_NEXTDIFF:
		CMainFrame::DoDefaultAction(m_spAccNextDiff);
		break;
	case ID_PREVDIFF:
		CMainFrame::DoDefaultAction(m_spAccPrevDiff);
		break;
	case ID_CURDIFF:
		CMainFrame::DoDefaultAction(m_spAccEditSelectedItem);
		break;
	case ID_L2R:
		CMainFrame::DoDefaultAction(m_spAccCopyLeftToRight);
		break;
	case ID_R2L:
		CMainFrame::DoDefaultAction(m_spAccCopyRightToLeft);
		break;
	case ID_REFRESH:
		CMainFrame::DoDefaultAction(m_spAccRefresh);
		if (COptionsMgr::Get(OPT_SCROLL_TO_FIRST))
			CMainFrame::DoDefaultAction(m_spAccNextDiff);
		break;
	case ID_TOOLS_EXPORT_DATA_DIFFS:
		CMainFrame::DoDefaultAction(m_spAccExportDataDiffs);
		break;
	case ID_TOOLS_GENERATE_CHANGE_SCRIPT_L2R:
		CMainFrame::DoDefaultAction(m_spGenerateChangeScriptLeftToRight);
		break;
	case ID_TOOLS_GENERATE_CHANGE_SCRIPT_R2L:
		CMainFrame::DoDefaultAction(m_spGenerateChangeScriptRightToLeft);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

LRESULT CSQLiteMergeFrame::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		if (LRESULT lResult = OnWndMsg<WM_COMMAND>(wParam, lParam))
			return lResult;
		break;
	case WM_NCACTIVATE:
		if (wParam)
		{
			m_pMDIFrame->InitCmdUI();
			m_pMDIFrame->UpdateCmdUI<ID_L2R>(MF_ENABLED | 0x10); // 0x10 keeps ID_L2RNEXT disabled
			m_pMDIFrame->UpdateCmdUI<ID_R2L>(MF_ENABLED | 0x10); // 0x10 keeps ID_R2LNEXT disabled
			m_pMDIFrame->UpdateCmdUI<ID_REFRESH>(MF_ENABLED);
			UpdateCmdUI();
		}
		break;
	case WM_CLOSE:
		m_pMDIFrame->RemoveAutomationEventHandler(m_spToolStrip, this);
		CMainFrame::DoDefaultAction(m_spAccExit);
		// Recover from main window occasionally being left inactive
		m_pMDIFrame->SetActiveWindow();
		break;
	}
	return CDocFrame::WindowProc(uMsg, wParam, lParam);
}
