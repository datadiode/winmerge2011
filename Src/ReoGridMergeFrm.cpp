/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997-2000  Thingamahoochie Software
//    Author: Dean Grimm
//    SPDX-License-Identifier: GPL-3.0-or-later
/////////////////////////////////////////////////////////////////////////////
/**
 * @file  ReoGridMergeFrm.cpp
 *
 * @brief Implementation file for CReoGridMergeFrame
 *
 */
#include "stdafx.h"
#include "Merge.h"
#include "MainFrm.h"
#include "DirFrame.h"
#include "LanguageSelect.h"
#include "SaveClosingDlg.h"
#include "ReoGridMergeFrm.h"
#include "paths.h"
#include "Common/coretools.h"
#include "Common/DllProxies.h"
#include "Common/AccChildren.h"
#include "Common/SettingStore.h"
#include <cmath>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static HEdit *GetFocusedEditControl()
{
	HWindow *pwndFocus = HWindow::GetFocus();
	if (pwndFocus && (pwndFocus->GetDlgCode() & DLGC_HASSETSEL))
		return static_cast<HEdit *>(pwndFocus);
	return NULL;
}

static void GetAccessibleValue(IAccessible *accessible, String &value)
{
	VARIANT var;
	var.vt = VT_I4;
	var.lVal = CHILDID_SELF;
	CMyComBSTR bstr;
	if (accessible)
		accessible->get_accValue(var, &bstr);
	value.assign(bstr, bstr.Length());
}

static long GetAccessibleState(IAccessible *accessible)
{
	VARIANT var;
	var.vt = VT_I4;
	var.lVal = CHILDID_SELF;
	CMyComBSTR bstr;
	if (accessible)
		accessible->get_accState(var, &var);
	return var.lVal;
}

static HWND GetAccessibleHWnd(IAccessible *accessible)
{
	HWND hwnd = NULL;
	if (accessible)
	{
		CMyComPtr<IOleWindow> spWindow;
		if (SUCCEEDED(accessible->QueryInterface(&spWindow)))
		{
			spWindow->GetWindow(&hwnd);
		}
	}
	return hwnd;
}

/////////////////////////////////////////////////////////////////////////////
// CReoGridMergeFrame

/////////////////////////////////////////////////////////////////////////////
// CReoGridMergeFrame construction/destruction

CReoGridMergeFrame::CReoGridMergeFrame(CMainFrame *pMDIFrame, CDirFrame *pDirDoc)
: CEditorFrame(pMDIFrame, pDirDoc, GetHandleSet<IDR_REOGRIDDOCTYPE>(), NULL)
{
	Subclass(pMDIFrame->CreateChildHandle());
	m_bAutoDelete = true;
}

CReoGridMergeFrame::~CReoGridMergeFrame()
{
	if (m_pDirDoc)
		m_pDirDoc->MergeDocClosing(this);
}

void CReoGridMergeFrame::RecalcLayout()
{
	if (HWindow *const pwndGuest = GetTopWindow())
	{
		RECT rc;
		GetClientRect(&rc);
		pwndGuest->MoveWindow(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
	}
}

HRESULT CReoGridMergeFrame::HandleAutomationEvent(IUIAutomationElement *, EVENTID id)
{
	TRACE("HandleAutomationEvent(%d)\n", id);
	UpdateCmdUI();
	return S_OK;
}

void CReoGridMergeFrame::OpenDocs(
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

	DWORD const result = m_pMDIFrame->CLRExec(L"ReoGridCompare", string_format(
		L"/ParentWindow %p \"%s\" \"%s\"", m_hWnd,
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
						m_spAccExit = menuitemChildren.Find(L"Exit", ROLE_SYSTEM_MENUITEM);
						m_spAccSave = menuitemChildren.Find(L"Save", ROLE_SYSTEM_MENUITEM);
						if (AccChildren menuitemChildren2 = menuitemChildren.Find(L"SaveLeft", ROLE_SYSTEM_MENUITEM))
						{
							m_spAccSaveLeft = menuitemChildren2.Find(L"SaveLeft", ROLE_SYSTEM_MENUITEM);
							m_spAccSaveLeftAs = menuitemChildren2.Find(L"SaveLeftAs", ROLE_SYSTEM_MENUITEM);
						}
						if (AccChildren menuitemChildren2 = menuitemChildren.Find(L"SaveRight", ROLE_SYSTEM_MENUITEM))
						{
							m_spAccSaveRight = menuitemChildren2.Find(L"SaveRight", ROLE_SYSTEM_MENUITEM);
							m_spAccSaveRightAs = menuitemChildren2.Find(L"SaveRightAs", ROLE_SYSTEM_MENUITEM);
						}
					}
					if (AccChildren menuitemChildren = menubarChildren.Find(L"Cells", ROLE_SYSTEM_MENUITEM))
					{
						m_spAccFormatCells = menuitemChildren.Find(L"Format", ROLE_SYSTEM_MENUITEM);
					}
					if (AccChildren menuitemChildren = menubarChildren.Find(L"Formula", ROLE_SYSTEM_MENUITEM))
					{
						m_spAccRecalculate = menuitemChildren.Find(L"Recalculate", ROLE_SYSTEM_MENUITEM);
					}
					if (AccChildren menuitemChildren = menubarChildren.Find(L"Tools", ROLE_SYSTEM_MENUITEM))
					{
						m_spAccLanguage = menuitemChildren.Find(L"Language", ROLE_SYSTEM_MENUITEM);
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
					m_spAccCut = toolbarChildren.Find(L"Cut", ROLE_SYSTEM_PUSHBUTTON);
					m_spAccCopy = toolbarChildren.Find(L"Copy", ROLE_SYSTEM_PUSHBUTTON);
					m_spAccPaste = toolbarChildren.Find(L"Paste", ROLE_SYSTEM_PUSHBUTTON);
					m_spAccUndo = toolbarChildren.Find(L"Undo", ROLE_SYSTEM_PUSHBUTTON);
					m_spAccRedo = toolbarChildren.Find(L"Redo", ROLE_SYSTEM_PUSHBUTTON);
					m_spAccPrevDiff = toolbarChildren.Find(L"PrevDiff", ROLE_SYSTEM_PUSHBUTTON);
					m_spAccNextDiff = toolbarChildren.Find(L"NextDiff", ROLE_SYSTEM_PUSHBUTTON);
					m_spAccFirstDiff = toolbarChildren.Find(L"FirstDiff", ROLE_SYSTEM_PUSHBUTTON);
					m_spAccLastDiff = toolbarChildren.Find(L"LastDiff", ROLE_SYSTEM_PUSHBUTTON);
					m_spAccCopyLeftToRight = toolbarChildren.Find(L"CopyLeftToRight", ROLE_SYSTEM_PUSHBUTTON);
					m_spAccCopyRightToLeft = toolbarChildren.Find(L"CopyRightToLeft", ROLE_SYSTEM_PUSHBUTTON);
				}
			}
			if (AccChildren windowChildren = formChildren.Find(L"FontStrip", ROLE_SYSTEM_WINDOW))
			{
				if (AccChildren toolbarChildren = windowChildren.Find(L"FontStrip", ROLE_SYSTEM_TOOLBAR))
				{
					m_spAccZoom = toolbarChildren.Find(L"Zoom", ROLE_SYSTEM_COMBOBOX);
				}
			}
			if (AccChildren windowChildren = formChildren.Find(L"SplitContainer", ROLE_SYSTEM_WINDOW))
			{
				if (AccChildren clientChildren = windowChildren.Find(L"SplitContainer", ROLE_SYSTEM_CLIENT))
				{
					if (AccChildren windowChildren = clientChildren.Find(L"LeftPanel", ROLE_SYSTEM_WINDOW))
					{
						if (AccChildren clientChildren = windowChildren.Find(L"LeftPanel", ROLE_SYSTEM_CLIENT))
						{
							if (AccChildren windowChildren = clientChildren.Find(L"Header", ROLE_SYSTEM_WINDOW))
							{
								m_spAccLeftHeader = windowChildren.Find(NULL, ROLE_SYSTEM_TEXT);
							}
							m_spAccLeftGrid = clientChildren.Find(L"Grid", ROLE_SYSTEM_WINDOW);
						}
					}
					if (AccChildren windowChildren = clientChildren.Find(L"RightPanel", ROLE_SYSTEM_WINDOW))
					{
						if (AccChildren clientChildren = windowChildren.Find(L"RightPanel", ROLE_SYSTEM_CLIENT))
						{
							if (AccChildren windowChildren = clientChildren.Find(L"Header", ROLE_SYSTEM_WINDOW))
							{
								m_spAccRightHeader = windowChildren.Find(NULL, ROLE_SYSTEM_TEXT);
							}
							m_spAccRightGrid = clientChildren.Find(L"Grid", ROLE_SYSTEM_WINDOW);
						}
					}
				}
			}
		}
	}

	m_pMDIFrame->AddAutomationEventHandler(m_spToolStrip, this);

	// Initial UpdateCmdUI() triggers SetTitle() due to out of sync m_cmdStateSave* flags.
	ActivateFrame();
	PostMessage(WM_COMMAND, ID_DIR_RESCAN);
}

/**
 * @brief Update document filenames to title
 */
void CReoGridMergeFrame::SetTitle()
{
	string_format sTitle(_T("%s%s - %s%s"),
		&_T("\0* ")[!(m_cmdStateSaveLeft & MF_GRAYED)],
		!m_strDesc[0].empty() ? m_strDesc[0].c_str() : PathFindFileName(m_strPath[0].c_str()),
		&_T("\0* ")[!(m_cmdStateSaveRight & MF_GRAYED)],
		!m_strDesc[1].empty() ? m_strDesc[1].c_str() : PathFindFileName(m_strPath[1].c_str()));
	SetWindowText(sTitle.c_str());
}

/**
 * @brief Reflect comparison result in window's icon.
 * @param f [in] Last comparison result which the application returns.
 */
void CReoGridMergeFrame::SetLastCompareResult(bool flag)
{
	LoadIcon(flag ? CompareStats::DIFFIMG_BINDIFF : CompareStats::DIFFIMG_BINSAME);
}

void CReoGridMergeFrame::UpdateLastCompareResult()
{
	SetLastCompareResult(!(CMainFrame::GetCommandState(m_spAccNextDiff) & MF_GRAYED));
}

/**
 * @brief Update associated diff item
 */
void CReoGridMergeFrame::UpdateDiffItem(CDirFrame *pDirDoc)
{
	// If directory compare has results
	if (pDirDoc)
	{
		const String &pathLeft = m_strPath[0];
		const String &pathRight = m_strPath[1];
		if (DIFFITEM *di = pDirDoc->FindItemFromPaths(pathLeft.c_str(), pathRight.c_str()))
		{
			pDirDoc->GetDiffContext()->UpdateDiffItem(di);
		}
	}
	UpdateLastCompareResult();
}

/**
 * @brief Asks and then saves modified files.
 * This function is not applicable to SQLite Compare, and hence does nothing.
 */
bool CReoGridMergeFrame::SaveModified()
{
	BOOL bLModified = !(CMainFrame::GetCommandState(m_spAccSaveLeft) & MF_GRAYED);
	BOOL bRModified = !(CMainFrame::GetCommandState(m_spAccSaveRight) & MF_GRAYED);

	if (!bLModified && !bRModified) // Both files unmodified
		return true;

	SaveClosingDlg dlg;
	dlg.m_bAskForLeft = bLModified;
	dlg.m_bAskForRight = bRModified;
	dlg.m_sLeftFile = m_strPath[0].empty() ? m_strDesc[0] : m_strPath[0];
	dlg.m_sRightFile = m_strPath[1].empty() ? m_strDesc[1] : m_strPath[1];

	bool result = true;
	bool bLSaveSuccess = false;
	bool bRSaveSuccess = false;

	if (LanguageSelect.DoModal(dlg) == IDOK)
	{
		if (bLModified)
		{
			if (dlg.m_leftSave == SaveClosingDlg::SAVECLOSING_SAVE)
			{
				CMainFrame::DoDefaultAction(m_spAccSaveLeft);
				if (CMainFrame::GetCommandState(m_spAccSaveLeft) & MF_GRAYED)
					bLSaveSuccess = true;
				else
					result = false;
			}
		}
		if (bRModified)
		{
			if (dlg.m_rightSave == SaveClosingDlg::SAVECLOSING_SAVE)
			{
				CMainFrame::DoDefaultAction(m_spAccSaveRight);
				if (CMainFrame::GetCommandState(m_spAccSaveRight) & MF_GRAYED)
					bRSaveSuccess = true;
				else
					result = false;
			}
		}
	}
	else
	{
		result = false;
	}

	// If file were modified and saving was successfull,
	// update status on dir view
	if (bLSaveSuccess || bRSaveSuccess)
	{
		UpdateDiffItem(m_pDirDoc);
	}

	return result;
}

/**
 * @brief Update any resources necessary after a GUI language change
 */
void CReoGridMergeFrame::UpdateResources()
{
	CMainFrame::DoDefaultAction(m_spAccLanguage);
}

void CReoGridMergeFrame::UpdateCmdUI()
{
	if (m_pMDIFrame->GetActiveDocFrame() != this)
		return;
	m_pMDIFrame->UpdateCmdUI<ID_FILE_SAVE>(CMainFrame::GetCommandState(m_spAccSave));
	// For ID_EDIT_UNDO, leave keyboard accelerators enabled even if menu item is grayed
	m_pMDIFrame->UpdateCmdUI<ID_EDIT_UNDO>(CMainFrame::GetCommandState(m_spAccUndo) | MF_DISABLED);
	m_pMDIFrame->UpdateCmdUI<ID_EDIT_REDO>(CMainFrame::GetCommandState(m_spAccRedo));
	m_pMDIFrame->UpdateCmdUI<ID_NEXTDIFF>(CMainFrame::GetCommandState(m_spAccNextDiff));
	m_pMDIFrame->UpdateCmdUI<ID_PREVDIFF>(CMainFrame::GetCommandState(m_spAccPrevDiff));
	m_pMDIFrame->UpdateCmdUI<ID_L2R>(CMainFrame::GetCommandState(m_spAccCopyLeftToRight));
	m_pMDIFrame->UpdateCmdUI<ID_R2L>(CMainFrame::GetCommandState(m_spAccCopyRightToLeft));
	BYTE const cmdStateSaveLeft = CMainFrame::GetCommandState(m_spAccSaveLeft);
	BYTE const cmdStateSaveRight = CMainFrame::GetCommandState(m_spAccSaveRight);
	m_pMDIFrame->UpdateCmdUI<ID_FILE_SAVE_LEFT>(cmdStateSaveLeft);
	m_pMDIFrame->UpdateCmdUI<ID_FILE_SAVE_RIGHT>(cmdStateSaveRight);
	if (m_cmdStateSaveLeft != cmdStateSaveLeft || m_cmdStateSaveRight != cmdStateSaveRight)
	{
		m_cmdStateSaveLeft = cmdStateSaveLeft;
		m_cmdStateSaveRight = cmdStateSaveRight;
		SetTitle();
	}
	UpdateLastCompareResult();
}

/**
 * @brief Check for keyboard commands
 */
BOOL CReoGridMergeFrame::PreTranslateMessage(MSG* pMsg)
{
	return ::IsDialogMessage(m_hWnd, pMsg);
}

template<>
LRESULT CReoGridMergeFrame::OnWndMsg<WM_COMMAND>(WPARAM wParam, LPARAM lParam)
{
	switch (const UINT id = lParam ? static_cast<UINT>(wParam) : LOWORD(wParam))
	{
	case ID_FILE_SAVE:
		CMainFrame::DoDefaultAction(m_spAccSave);
		UpdateDiffItem(m_pDirDoc);
		break;
	case ID_FILE_SAVE_LEFT:
		CMainFrame::DoDefaultAction(m_spAccSaveLeft);
		UpdateDiffItem(m_pDirDoc);
		break;
	case ID_FILE_SAVE_RIGHT:
		CMainFrame::DoDefaultAction(m_spAccSaveRight);
		UpdateDiffItem(m_pDirDoc);
		break;
	case ID_FILE_SAVEAS_LEFT:
		CMainFrame::DoDefaultAction(m_spAccSaveLeftAs);
		GetAccessibleValue(m_spAccLeftHeader, m_strPath[0]);
		SetTitle();
		UpdateDiffItem(m_pDirDoc);
		break;
	case ID_FILE_SAVEAS_RIGHT:
		CMainFrame::DoDefaultAction(m_spAccSaveRightAs);
		GetAccessibleValue(m_spAccRightHeader, m_strPath[1]);
		SetTitle();
		UpdateDiffItem(m_pDirDoc);
		break;
	case ID_NEXTDIFF:
		CMainFrame::DoDefaultAction(m_spAccNextDiff);
		break;
	case ID_PREVDIFF:
		CMainFrame::DoDefaultAction(m_spAccPrevDiff);
		break;
	case ID_FIRSTDIFF:
		CMainFrame::DoDefaultAction(m_spAccFirstDiff);
		break;
	case ID_LASTDIFF:
		CMainFrame::DoDefaultAction(m_spAccLastDiff);
		break;
	case ID_L2R:
		CMainFrame::DoDefaultAction(m_spAccCopyLeftToRight);
		break;
	case ID_L2RNEXT:
		CMainFrame::DoDefaultAction(m_spAccCopyLeftToRight);
		CMainFrame::DoDefaultAction(m_spAccNextDiff);
		break;
	case ID_R2L:
		CMainFrame::DoDefaultAction(m_spAccCopyRightToLeft);
		break;
	case ID_R2LNEXT:
		CMainFrame::DoDefaultAction(m_spAccCopyRightToLeft);
		CMainFrame::DoDefaultAction(m_spAccNextDiff);
		break;
	case ID_EDIT_CUT:
		if (HEdit *pwndEdit = GetFocusedEditControl())
			pwndEdit->Cut();
		else
			CMainFrame::DoDefaultAction(m_spAccCut);
		break;
	case ID_EDIT_COPY:
		if (HEdit *pwndEdit = GetFocusedEditControl())
			pwndEdit->Copy();
		else
			CMainFrame::DoDefaultAction(m_spAccCopy);
		break;
	case ID_EDIT_PASTE:
		if (HEdit *pwndEdit = GetFocusedEditControl())
			pwndEdit->Paste();
		else
			CMainFrame::DoDefaultAction(m_spAccPaste);
		break;
	case ID_EDIT_UNDO:
		if (HEdit *pwndEdit = GetFocusedEditControl())
			pwndEdit->Undo();
		else
			CMainFrame::DoDefaultAction(m_spAccUndo);
		break;
	case ID_EDIT_REDO:
		CMainFrame::DoDefaultAction(m_spAccRedo);
		break;
	case ID_VIEW_ZOOMIN:
	case ID_VIEW_ZOOMNORMAL:
	case ID_VIEW_ZOOMOUT:
		if (HComboBox *const pwndCombo = reinterpret_cast<HComboBox *>(GetAccessibleHWnd(m_spAccZoom)))
		{
			C_ASSERT(ID_VIEW_ZOOMNORMAL - ID_VIEW_ZOOMIN == 1);
			C_ASSERT(ID_VIEW_ZOOMNORMAL - ID_VIEW_ZOOMOUT == -1);
			TCHAR text[6] = _T("100%");
			int index = ID_VIEW_ZOOMNORMAL - id;
			if (index)
				pwndCombo->GetWindowText(text, _countof(text));
			index += pwndCombo->FindStringExact(-1, text);
			if (index >= 0 && index < pwndCombo->GetCount() && index != pwndCombo->GetCurSel())
			{
				pwndCombo->SetCurSel(index);
				// Do some magic to trigger an event in the hosted application
				COMBOBOXINFO info;
				info.cbSize = sizeof info;
				if (::GetComboBoxInfo(pwndCombo->m_hWnd, &info))
				{
					HEdit *pwndItem = reinterpret_cast<HEdit *>(info.hwndItem);
					pwndItem->SetSel(0, 0);
					pwndItem->ReplaceSel(_T(""));
					if (::GetFocus() == info.hwndItem)
						pwndItem->SetSel(0, -1);
				}
			}
		}
		break;
	case ID_NEXT_PANE:
	case ID_WINDOW_CHANGE_PANE:
		if (HWND const hwnd = GetAccessibleHWnd(
			GetAccessibleState(m_spAccLeftHeader) & (STATE_SYSTEM_FOCUSED | STATE_SYSTEM_READONLY)
			? m_spAccRightGrid : m_spAccLeftGrid))
		{
			::SetFocus(hwnd);
		}
		break;
	case ID_REFRESH:
		CMainFrame::DoDefaultAction(m_spAccRecalculate);
		break;
	case ID_CURDIFF:
		CMainFrame::DoDefaultAction(m_spAccFormatCells);
		break;
	case ID_DIR_RESCAN:
		// Take some initial actions after files are loaded
		UpdateCmdUI();
		if (COptionsMgr::Get(OPT_SCROLL_TO_FIRST))
			CMainFrame::DoDefaultAction(m_spAccFirstDiff);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

LRESULT CReoGridMergeFrame::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
			m_pMDIFrame->UpdateCmdUI<ID_EDIT_CUT>(MF_ENABLED);
			m_pMDIFrame->UpdateCmdUI<ID_EDIT_COPY>(MF_ENABLED);
			m_pMDIFrame->UpdateCmdUI<ID_EDIT_PASTE>(MF_ENABLED);
			m_pMDIFrame->UpdateCmdUI<ID_REFRESH>(MF_ENABLED);
			UpdateCmdUI();
		}
		break;
	case WM_CLOSE:
		if (!SaveModified())
			return 0;
		m_pMDIFrame->RemoveAutomationEventHandler(m_spToolStrip, this);
		CMainFrame::DoDefaultAction(m_spAccExit);
		// Recover from main window occasionally being left inactive
		m_pMDIFrame->SetActiveWindow();
		break;
	}
	return CDocFrame::WindowProc(uMsg, wParam, lParam);
}
