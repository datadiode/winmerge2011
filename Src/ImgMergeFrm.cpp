/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997-2000  Thingamahoochie Software
//    Author: Dean Grimm
//    SPDX-License-Identifier: GPL-3.0-or-later
/////////////////////////////////////////////////////////////////////////////
/**
 * @file  ImgMergeFrm.cpp
 *
 * @brief Implementation file for CImgMergeFrame
 *
 */

#include "stdafx.h"
#include "Merge.h"
#include "MainFrm.h"
#include "DirFrame.h"
#include "LanguageSelect.h"
#include "ImgMergeFrm.h"
#include "paths.h"
#include "FileOrFolderSelect.h"
#include "SaveClosingDlg.h"
#include "Common/DllProxies.h"
#include "Common/SettingStore.h"
#include <cmath>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CImgMergeFrame

const LONG CImgMergeFrame::FloatScript[] =
{
	152, BY<1000>::X2R | BY<1000>::Y2B,
	0
};

/////////////////////////////////////////////////////////////////////////////
// CImgMergeFrame construction/destruction

CImgMergeFrame::CImgMergeFrame(CMainFrame *pMDIFrame, CDirFrame *pDirDoc)
: CEditorFrame(pMDIFrame, pDirDoc, GetHandleSet<IDR_IMGDOCTYPE>(), FloatScript, NULL)
#pragma warning(disable:warning_this_used_in_base_member_initializer_list)
, m_wndLocationBar(this, FloatScript)
#pragma warning(default:warning_this_used_in_base_member_initializer_list)
{
	Subclass(pMDIFrame->CreateChildHandle());
	CreateClient();
	RecalcLayout();
	m_bAutoDelete = true;
}

CImgMergeFrame::~CImgMergeFrame()
{
	if (m_pDirDoc)
		m_pDirDoc->MergeDocClosing(this);
}

void CImgMergeFrame::RecalcLayout()
{
	RECT rectClient;
	GetClientRect(&rectClient);
	if (m_wndLocationBar.m_hWnd)
	{
		RECT rect;
		m_wndLocationBar.GetWindowRect(&rect);
		const int cx = rect.right - rect.left;
		const int cy = rectClient.bottom - rectClient.top;
		switch (m_wndLocationBar.m_uHitTestCode)
		{
		case HTLEFT:
			rectClient.right -= cx - 4;
			m_wndLocationBar.SetWindowPos(NULL,
				rectClient.right, rectClient.top, cx, cy,
				SWP_SHOWWINDOW | SWP_NOZORDER);
			break;
		case HTRIGHT:
			rectClient.left += cx - 4;
			m_wndLocationBar.SetWindowPos(NULL,
				-4, rectClient.top, cx, cy,
				SWP_SHOWWINDOW | SWP_NOZORDER);
			break;
		default:
			m_wndLocationBar.ShowWindow(SW_HIDE);
			break;
		}
	}
	if (m_wndFilePathBar.m_hWnd)
	{
		m_wndFilePathBar.MoveWindow(
			rectClient.left, rectClient.top,
			rectClient.right - rectClient.left,
			rectClient.bottom - rectClient.top);
	}
	if (m_pImgMergeWindow->GetPaneCount() >= 2)
	{
		if (HDWP const hdwp = ::BeginDeferWindowPos(4))
		{
			RECT rc0, rc1, rc;
			::GetWindowRect(m_pImgMergeWindow->GetPaneHWND(0), &rc0);
			m_wndFilePathBar.ScreenToClient(&rc0);
			::GetWindowRect(m_pImgMergeWindow->GetPaneHWND(1), &rc1);
			m_wndFilePathBar.ScreenToClient(&rc1);
			if (HWindow *const pwndSibling = m_wndFilePathBar.GetDlgItem(IDC_STATIC_TITLE_LEFT))
			{
				pwndSibling->GetWindowRect(&rc);
				m_wndFilePathBar.ScreenToClient(&rc);
				::DeferWindowPos(hdwp, pwndSibling->m_hWnd, NULL,
					rc0.left, rc.top, rc0.right - rc0.left, rc.bottom - rc.top, SWP_NOZORDER);
			}
			if (HWindow *const pwndSibling = m_wndFilePathBar.GetDlgItem(IDC_STATIC_TITLE_RIGHT))
			{
				pwndSibling->GetWindowRect(&rc);
				m_wndFilePathBar.ScreenToClient(&rc);
				::DeferWindowPos(hdwp, pwndSibling->m_hWnd, NULL,
					rc1.left, rc.top, rc1.right - rc1.left, rc.bottom - rc.top, SWP_NOZORDER);
			}
			m_wndFilePathBar.GetClientRect(&rc);
			if (HWindow *const pwndSibling = m_wndFilePathBar.GetDlgItem(0x6000))
			{
				::DeferWindowPos(hdwp, pwndSibling->m_hWnd, NULL,
					rc0.left, rc.bottom - rc0.top, rc0.right - rc0.left, rc0.top, SWP_NOZORDER);
			}
			if (HWindow *const pwndSibling = m_wndFilePathBar.GetDlgItem(0x6001))
			{
				::DeferWindowPos(hdwp, pwndSibling->m_hWnd, NULL,
					rc1.left, rc.bottom - rc1.top, rc1.right - rc1.left, rc1.top, SWP_NOZORDER);
			}
			::EndDeferWindowPos(hdwp);
		}
	}
	UpdateHeaderPath(0);
	UpdateHeaderPath(1);
}

void CImgMergeFrame::OpenDocs(
	FileLocation &filelocLeft,
	FileLocation &filelocRight,
	bool bROLeft, bool bRORight)
{
	//m_pView[index]->SetReadOnly(readOnly);
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

	m_pImgMergeWindow->OpenImages(m_strPath[0].c_str(), m_strPath[1].c_str());

	UpdateDiffItem(NULL);

	ActivateFrame();

	if (COptionsMgr::Get(OPT_SCROLL_TO_FIRST))
		m_pImgMergeWindow->FirstDiff();

	for (int pane = 0; pane < m_pImgMergeWindow->GetPaneCount(); ++pane)
	{
		HWND hwndPane = m_pImgMergeWindow->GetPaneHWND(pane);
		LONG style = ::GetWindowLong(hwndPane, GWL_STYLE);
		::SetWindowLong(hwndPane, GWL_STYLE, style | WS_TABSTOP);
	}

	::SetFocus(m_pImgMergeWindow->GetPaneHWND(0));

	UpdateCmdUI();
}

bool CImgMergeFrame::IsModified() const
{
	for (int pane = 0; pane < m_pImgMergeWindow->GetPaneCount(); ++pane)
		if (m_pImgMergeWindow->IsModified(pane))
			return true;
	return false;
}

BOOL CImgMergeFrame::IsFileChangedOnDisk(int pane) const
{
	BOOL bChanged = FALSE;
	FileInfo fileInfo;
	if (fileInfo.Update(m_strPath[pane].c_str()))
	{
		UINT64 lower = min(fileInfo.mtime.castTo<UINT64>(), m_fileInfo[pane].mtime.castTo<UINT64>());
		UINT64 upper = max(fileInfo.mtime.castTo<UINT64>(), m_fileInfo[pane].mtime.castTo<UINT64>());
		bool bIgnoreSmallDiff = COptionsMgr::Get(OPT_IGNORE_SMALL_FILETIME);
		UINT64 tolerance = bIgnoreSmallDiff ? SmallTimeDiff * FileTime::TicksPerSecond : 0;
		bChanged = upper - lower > tolerance || m_fileInfo[pane].size.int64 != fileInfo.size.int64;
	}
	return bChanged;
}

void CImgMergeFrame::OnChildPaneEvent(const IImgMergeWindow::Event& evt)
{
	CImgMergeFrame *const pFrame = static_cast<CImgMergeFrame *>(evt.userdata);
	switch (evt.eventType)
	{
	case IImgMergeWindow::KEYDOWN:
		switch (evt.keycode)
		{
		case VK_PRIOR:
		case VK_NEXT:
			::SendMessage(pFrame->m_pImgMergeWindow->GetPaneHWND(evt.pane), WM_VSCROLL, evt.keycode == VK_PRIOR ? SB_PAGEUP : SB_PAGEDOWN, 0);
			break;
		case VK_LEFT:
		case VK_RIGHT:
		case VK_UP:
		case VK_DOWN:
			if (GetKeyState(VK_SHIFT) < 0)
			{
				int nActivePane = pFrame->m_pImgMergeWindow->GetActivePane();
				int m = GetKeyState(VK_CONTROL) < 0 ? 8 : 1;
				int dx = (-(evt.keycode == VK_LEFT) + (evt.keycode == VK_RIGHT)) * m;
				int dy = (-(evt.keycode == VK_UP) + (evt.keycode == VK_DOWN)) * m;
				pFrame->m_pImgMergeWindow->AddImageOffset(nActivePane, dx, dy);
			}
			break;
		}
		break;
	case IImgMergeWindow::MOUSEMOVE:
	case IImgMergeWindow::LBUTTONUP:
		pFrame->UpdateCmdUI();
		break;
	}
}

/**
 * @brief Create the splitter, the filename bar, the status bar, and the two views
 */
BOOL CImgMergeFrame::CreateClient()
{
	// Merge frame has a header bar at top
	m_wndFilePathBar.Create(m_hWnd, 0);
	m_pImgMergeWindow = WinIMergeLib->WinIMerge_CreateWindow(WinIMergeLib->H, m_wndFilePathBar.m_hWnd, 152);
	m_wndLocationBar.Subclass(HWindow::CreateEx(
		WS_EX_CONTROLPARENT, WinMergeWindowClass, NULL,
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		0, 0, 0, 0, m_pWnd, ID_VIEW_IMAGETOOLBOX));
	m_pImgToolWindow = WinIMergeLib->WinIMerge_CreateToolWindow(WinIMergeLib->H, m_wndLocationBar.m_hWnd, m_pImgMergeWindow);
	if (HWND hImgToolWindow = m_pImgToolWindow ? m_pImgToolWindow->GetHWND() : NULL)
	{
		::SetWindowLongPtr(hImgToolWindow, GWLP_ID, 152);
		LONG style = ::GetWindowLong(hImgToolWindow, GWL_EXSTYLE);
		::SetWindowLong(hImgToolWindow, GWL_EXSTYLE, style | WS_EX_CONTROLPARENT);
		::SetWindowPos(hImgToolWindow, NULL, 4, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
		RECT rc;
		::GetWindowRect(hImgToolWindow, &rc);
		m_wndLocationBar.SetWindowPos(NULL, 0, 0, rc.right - rc.left + 8, rc.bottom - rc.top,
			SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
	}
	RECT rect0, rect1;
	HEdit *pEdit0 = m_wndFilePathBar.GetControlRect(0, &rect0);
	HEdit *pEdit1 = m_wndFilePathBar.GetControlRect(1, &rect1);
	const int cyFilePathEdit = rect1.bottom;

	HStatusBar *pBar = HStatusBar::Create(
		WS_CHILD | WS_VISIBLE | CCS_NOPARENTALIGN | SBS_SIZEGRIP, 0, 0, 0, 0,
		m_wndFilePathBar.m_pWnd, 0x6000);
	pBar->SetStyle(pBar->GetStyle() | CCS_NORESIZE);

	pBar = HStatusBar::Create(
		WS_CHILD | WS_VISIBLE | CCS_NOPARENTALIGN | SBS_SIZEGRIP, 0, 0, 0, 0,
		m_wndFilePathBar.m_pWnd, 0x6001);
	pBar->SetStyle(pBar->GetStyle() | CCS_NORESIZE);

	RECT rect;
	pBar->GetWindowRect(&rect);
	m_wndFilePathBar.ScreenToClient(&rect);
	const int cyStatusBar = rect.bottom - rect.top;

	::SetWindowPos(m_pImgMergeWindow->GetHWND(), NULL,
		rect0.left, cyStatusBar,
		rect1.right - rect0.left, cyFilePathEdit - cyStatusBar,
		SWP_NOZORDER | SWP_NOACTIVATE);

	pEdit0->SetWindowPos(NULL, 0, 0,
		rect0.right - rect0.left, cyStatusBar,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	pEdit1->SetWindowPos(NULL, 0, 0,
		rect1.right - rect1.left, cyStatusBar,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

	m_pImgMergeWindow->SetDiffColor(COptionsMgr::Get(OPT_CLR_DIFF));
	m_pImgMergeWindow->SetDiffDeletedColor(COptionsMgr::Get(OPT_CLR_DIFF_DELETED));
	m_pImgMergeWindow->SetSelDiffColor(COptionsMgr::Get(OPT_CLR_SELECTED_DIFF));
	m_pImgMergeWindow->SetSelDiffDeletedColor(COptionsMgr::Get(OPT_CLR_SELECTED_DIFF_DELETED));
	m_pImgMergeWindow->AddEventListener(OnChildPaneEvent, this);
	LoadOptions();
	
	if (int value = COptionsMgr::Get(OPT_SHOW_IMAGETOOLBOX))
		m_wndLocationBar.m_uHitTestCode = value < 0 ? HTLEFT : HTRIGHT;

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CImgMergeFrame message handlers

void CImgMergeFrame::LoadOptions()
{
	m_pImgMergeWindow->SetShowDifferences(COptionsMgr::Get(OPT_CMP_IMG_SHOWDIFFERENCES));
	m_pImgMergeWindow->SetOverlayMode(static_cast<IImgMergeWindow::OVERLAY_MODE>(COptionsMgr::Get(OPT_CMP_IMG_OVERLAYMOVE)));
	m_pImgMergeWindow->SetOverlayAlpha(COptionsMgr::Get(OPT_CMP_IMG_OVERLAYALPHA) / 100.0);
	m_pImgMergeWindow->SetDraggingMode(static_cast<IImgMergeWindow::DRAGGING_MODE>(COptionsMgr::Get(OPT_CMP_IMG_DRAGGING_MODE)));
	m_pImgMergeWindow->SetZoom(COptionsMgr::Get(OPT_CMP_IMG_ZOOM) / 1000.0);
	m_pImgMergeWindow->SetUseBackColor(COptionsMgr::Get(OPT_CMP_IMG_USEBACKCOLOR));
	COLORREF clrBackColor = COptionsMgr::Get(OPT_CMP_IMG_BACKCOLOR);
	RGBQUAD backColor = {GetBValue(clrBackColor), GetGValue(clrBackColor), GetRValue(clrBackColor)};
	m_pImgMergeWindow->SetBackColor(backColor);
	m_pImgMergeWindow->SetDiffBlockSize(COptionsMgr::Get(OPT_CMP_IMG_DIFFBLOCKSIZE));
	m_pImgMergeWindow->SetDiffColorAlpha(COptionsMgr::Get(OPT_CMP_IMG_DIFFCOLORALPHA) / 100.0);
	m_pImgMergeWindow->SetColorDistanceThreshold(COptionsMgr::Get(OPT_CMP_IMG_THRESHOLD) / 1000.0);
	m_pImgMergeWindow->SetInsertionDeletionDetectionMode(static_cast<IImgMergeWindow::INSERTION_DELETION_DETECTION_MODE>(COptionsMgr::Get(OPT_CMP_IMG_INSERTIONDELETIONDETECTION_MODE)));
}

void CImgMergeFrame::SaveOptions()
{
	COptionsMgr::SaveOption(OPT_CMP_IMG_SHOWDIFFERENCES, m_pImgMergeWindow->GetShowDifferences());
	COptionsMgr::SaveOption(OPT_CMP_IMG_OVERLAYMOVE, static_cast<int>(m_pImgMergeWindow->GetOverlayMode()));
	COptionsMgr::SaveOption(OPT_CMP_IMG_OVERLAYALPHA, static_cast<int>(m_pImgMergeWindow->GetOverlayAlpha() * 100));
	COptionsMgr::SaveOption(OPT_CMP_IMG_DRAGGING_MODE, static_cast<int>(m_pImgMergeWindow->GetDraggingMode()));
	COptionsMgr::SaveOption(OPT_CMP_IMG_ZOOM, static_cast<int>(m_pImgMergeWindow->GetZoom() * 1000));
	COptionsMgr::SaveOption(OPT_CMP_IMG_USEBACKCOLOR, m_pImgMergeWindow->GetUseBackColor());
	RGBQUAD backColor = m_pImgMergeWindow->GetBackColor();
	COptionsMgr::SaveOption(OPT_CMP_IMG_BACKCOLOR, RGB(backColor.rgbRed, backColor.rgbGreen, backColor.rgbBlue));
	COptionsMgr::SaveOption(OPT_CMP_IMG_DIFFBLOCKSIZE, m_pImgMergeWindow->GetDiffBlockSize());
	COptionsMgr::SaveOption(OPT_CMP_IMG_DIFFCOLORALPHA, static_cast<int>(m_pImgMergeWindow->GetDiffColorAlpha() * 100.0));
	COptionsMgr::SaveOption(OPT_CMP_IMG_THRESHOLD, static_cast<int>(m_pImgMergeWindow->GetColorDistanceThreshold() * 1000));
	COptionsMgr::SaveOption(OPT_CMP_IMG_INSERTIONDELETIONDETECTION_MODE, static_cast<int>(m_pImgMergeWindow->GetInsertionDeletionDetectionMode()));
}

/**
 * @brief Saves both files
 */
void CImgMergeFrame::OnFileSave() 
{
	for (int pane = 0; pane < m_pImgMergeWindow->GetPaneCount(); ++pane)
		DoSave(pane);
}

/**
 * @brief Saves left-side file
 */
void CImgMergeFrame::OnFileSaveLeft() 
{
	DoSave(0);
}

/**
 * @brief Saves right-side file
 */
void CImgMergeFrame::OnFileSaveRight()
{
	DoSave(1);
}

/**
 * @brief Saves left-side file with name asked
 */
void CImgMergeFrame::OnFileSaveAsLeft()
{
	DoSaveAs(0);
}

/**
 * @brief Saves right-side file with name asked
 */
void CImgMergeFrame::OnFileSaveAsRight()
{
	DoSaveAs(1);
}

void CImgMergeFrame::OnWindowChangePane()
{
	int pane = m_pImgMergeWindow->GetActivePane() ? 0 : 1;
	::SetFocus(m_pImgMergeWindow->GetPaneHWND(pane));
}

/**
 * @brief Write path and filename to headerbar
 * @note SetText() does not repaint unchanged text
 */
void CImgMergeFrame::UpdateHeaderPath(int pane)
{
	String sText;

	if (m_nBufferType[pane] == BUFFER_UNNAMED ||
		m_nBufferType[pane] == BUFFER_NORMAL_NAMED)
	{
		sText = m_strDesc[pane];
	}
	else
	{
		sText = m_strPath[pane];
		if (m_pDirDoc)
		{
			if (pane == 0)
				m_pDirDoc->ApplyLeftDisplayRoot(sText);
			else
				m_pDirDoc->ApplyRightDisplayRoot(sText);
		}
	}
	m_wndFilePathBar.SetText(pane, sText.c_str(), m_pImgMergeWindow->IsModified(pane));
	SetTitle();
}

/**
 * @brief Update document filenames to title
 */
void CImgMergeFrame::SetTitle()
{
	string_format sTitle(_T("%s%s - %s%s"),
		&_T("\0* ")[m_pImgMergeWindow->IsModified(0)],
		!m_strDesc[0].empty() ? m_strDesc[0].c_str() : PathFindFileName(m_strPath[0].c_str()),
		&_T("\0* ")[m_pImgMergeWindow->IsModified(1)],
		!m_strDesc[1].empty() ? m_strDesc[1].c_str() : PathFindFileName(m_strPath[1].c_str()));
	SetWindowText(sTitle.c_str());
}

void CImgMergeFrame::UpdateLastCompareResult()
{
	SetLastCompareResult(m_pImgMergeWindow->GetDiffCount() > 0 ? 1 : 0);
}

/**
 * @brief Reflect comparison result in window's icon.
 * @param nResult [in] Last comparison result which the application returns.
 */
void CImgMergeFrame::SetLastCompareResult(int nResult)
{
	LoadIcon(nResult == 0 ?
		CompareStats::DIFFIMG_BINSAME: CompareStats::DIFFIMG_BINDIFF);
}

/**
 * @brief Update associated diff item
 */
int CImgMergeFrame::UpdateDiffItem(CDirFrame *pDirDoc)
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
	int result = m_pImgMergeWindow->GetDiffCount() > 0 ? 1 : 0;
	SetLastCompareResult(result);
	return result;
}

int CImgMergeFrame::TrySaveAs(int pane, String &strPath)
{
	if (m_pImgMergeWindow->SaveImageAs(pane, strPath.c_str()))
	{
		m_strPath[pane] = strPath;
		UpdateDiffItem(m_pDirDoc);
		m_fileInfo[pane].Update(m_strPath[pane].c_str());
		UpdateHeaderPath(pane);
		return 0;
	}
	String sError = GetSysError(GetLastError());
	return LanguageSelect.FormatStrings(IDS_FILESAVE_FAILED,
		paths_UndoMagic(wcsdupa(strPath.c_str())), sError.c_str()
	).MsgBox(MB_OKCANCEL | MB_HIGHLIGHT_ARGUMENTS | MB_ICONWARNING);
}

bool CImgMergeFrame::DoSave(int pane)
{
	if (m_pImgMergeWindow->IsModified(pane))
	{
		// Ask user what to do about FILE_ATTRIBUTE_READONLY
		String strPath = m_pImgMergeWindow->GetFileName(pane);
		if (DWORD attr = paths_IsReadonlyFile(strPath.c_str()))
			if (theApp.m_pMainWnd->HandleReadonlySave(attr, strPath) == IDCANCEL)
				return false;
		// Take a chance to create a backup
		if (!theApp.m_pMainWnd->CreateBackup(false, strPath.c_str()))
			return false;
		if (int choice = TrySaveAs(pane, strPath))
			return choice == IDOK && DoSaveAs(pane);
	}
	return true;
}

bool CImgMergeFrame::DoSaveAs(int pane)
{
	String strPath = m_strPath[pane];
	HWND const parent = m_pImgMergeWindow->GetPaneHWND(pane);
	UINT const title = pane == 0 ? IDS_SAVE_LEFT_AS : IDS_SAVE_RIGHT_AS;
	int choice = IDOK;
	while (choice == IDOK && SelectFile(parent, strPath, title, 0, FALSE))
		choice = TrySaveAs(pane, strPath);
	return choice == 0;
}

/**
 * @brief Asks and then saves modified files.
 *
 * This function saves modified files. Dialog is shown for user to select
 * modified file(s) one wants to save or discard changed. Cancelling of
 * save operation is allowed unless denied by parameter. After successfully
 * save operation file statuses are updated to directory compare.
 * @return true if user selected "OK" so next operation can be
 * executed. If false user choosed "Cancel".
 * @note If filename is empty, we assume scratchpads are saved,
 * so instead of filename, description is shown.
 * @todo If we have filename and description for file, what should
 * we do after saving to different filename? Empty description?
 */
bool CImgMergeFrame::SaveModified()
{
	const bool bLModified = m_pImgMergeWindow->IsModified(0);
	const bool bRModified = m_pImgMergeWindow->IsModified(1);

	if (!bLModified && !bRModified)
		 return true;

	SaveClosingDlg dlg;
	dlg.m_bAskForLeft = bLModified;
	dlg.m_bAskForRight = bRModified;
	dlg.m_sLeftFile = m_strPath[0].empty() ? m_strDesc[0] : m_strPath[0];
	dlg.m_sRightFile = m_strPath[1].empty() ? m_strDesc[1] : m_strPath[1];

	bool result = true;

	if (LanguageSelect.DoModal(dlg) == IDOK)
	{
		if (bLModified && dlg.m_leftSave == SaveClosingDlg::SAVECLOSING_SAVE)
		{
			if (!DoSave(0))
				result = false;
		}

		if (bRModified && dlg.m_rightSave == SaveClosingDlg::SAVECLOSING_SAVE)
		{
			if (!DoSave(1))
				result = false;
		}
	}
	else
	{	
		result = false;
	}
	return result;
}

void CImgMergeFrame::ActivateFrame()
{
	TCHAR entry[8];
	GetAtomName(static_cast<ATOM>(m_pHandleSet->m_id), entry, _countof(entry));
	DWORD dim = SettingStore.GetProfileInt(_T("ScreenLayout"), entry, 0);
	int cx = SHORT LOWORD(dim);
	int cy = SHORT HIWORD(dim);
	m_wndLocationBar.SetWindowPos(NULL,
		0, 0, cx, cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	RecalcLayout();
	CMDIChildWnd::ActivateFrame();
}

/**
 * @brief Save coordinates of the frame, splitters, and bars
 *
 * @note Do not save the maximized/restored state here. We are interested
 * in the state of the active frame, and maybe this frame is not active
 */
void CImgMergeFrame::SavePosition()
{
	RECT rectLocationBar;
	m_wndLocationBar.GetWindowRect(&rectLocationBar);
	DWORD dim = MAKELONG(rectLocationBar.right - rectLocationBar.left, 0);
	TCHAR entry[8];
	GetAtomName(static_cast<ATOM>(m_pHandleSet->m_id), entry, _countof(entry));
	SettingStore.WriteProfileInt(_T("ScreenLayout"), entry, dim);
}

/**
 * @brief Update any resources necessary after a GUI language change
 */
void CImgMergeFrame::UpdateResources()
{
}

/**
 * @brief Check for keyboard commands
 */
BOOL CImgMergeFrame::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
		return FALSE; // Shortcut IsDialogMessage() to retain OPT_CLOSE_WITH_ESC functionality
	return ::IsDialogMessage(m_hWnd, pMsg);
}

/**
 * @brief Synchronize control and status bar placements with splitter position,
 * update mod indicators, synchronize scrollbars
 */
void CImgMergeFrame::UpdateCmdUI()
{
	POINT pt = { -1, -1 };
	POINT ptCursor;
	GetCursorPos(&ptCursor);
	for (int pane = 0; pane < m_pImgMergeWindow->GetPaneCount(); ++pane)
	{
		RECT rc;
		::GetWindowRect(m_pImgMergeWindow->GetPaneHWND(pane), &rc);
		if (PtInRect(&rc, ptCursor))
			pt = m_pImgMergeWindow->GetCursorPos(pane);
	}
		
	double colorDistance01 = m_pImgMergeWindow->GetColorDistance(0, 1, pt.x, pt.y);

	int const nActivePane = m_pImgMergeWindow->GetActivePane();

	for (int pane = 0; pane < m_pImgMergeWindow->GetPaneCount(); ++pane)
	{
		RGBQUAD color = m_pImgMergeWindow->GetPixelColor(pane, pt.x, pt.y);
		// Update mod indicators
		String ind = m_wndFilePathBar.GetTitle(pane);
		if (m_pImgMergeWindow->IsModified(pane) ? ind[0] != _T('*') : ind[0] == _T('*'))
			UpdateHeaderPath(pane);
		if (nActivePane != -1)
			m_wndFilePathBar.SetActive(pane, pane == nActivePane);
		POINT ptReal;
		String text;
		if (m_pImgMergeWindow->ConvertToRealPos(pane, pt, ptReal))
		{
			text += string_format(_T("Pt:(%d,%d) RGBA:(%d,%d,%d,%d) "), ptReal.x, ptReal.y,
				color.rgbRed, color.rgbGreen, color.rgbBlue, color.rgbReserved);
			text += string_format(_T("Dist:%g "), colorDistance01);
		}

		text += string_format(_T("Page:%d/%d Zoom:%d%% %dx%dpx %dbpp"), 
				m_pImgMergeWindow->GetCurrentPage(pane) + 1,
				m_pImgMergeWindow->GetPageCount(pane),
				static_cast<int>(m_pImgMergeWindow->GetZoom() * 100),
				m_pImgMergeWindow->GetImageWidth(pane),
				m_pImgMergeWindow->GetImageHeight(pane),
				m_pImgMergeWindow->GetImageBitsPerPixel(pane)
				);

		HStatusBar *pBar = static_cast<HStatusBar *>(m_wndFilePathBar.GetDlgItem(0x6000 + pane));
		pBar->SetPartText(0, text.c_str());
	}
	m_pMDIFrame->UpdateCmdUI<ID_FILE_SAVE>(IsModified() ? MF_ENABLED : MF_GRAYED);
	m_pMDIFrame->UpdateCmdUI<ID_EDIT_UNDO>(m_pImgMergeWindow->IsUndoable() ? MF_ENABLED : MF_GRAYED);
	m_pMDIFrame->UpdateCmdUI<ID_EDIT_REDO>(m_pImgMergeWindow->IsRedoable() ? MF_ENABLED : MF_GRAYED);
	m_pMDIFrame->UpdateCmdUI<ID_NEXTDIFF>(m_pImgMergeWindow->GetNextDiffIndex() != -1 ? MF_ENABLED : MF_GRAYED);
	m_pMDIFrame->UpdateCmdUI<ID_PREVDIFF>(m_pImgMergeWindow->GetPrevDiffIndex() != -1 ? MF_ENABLED : MF_GRAYED);
	int const diffIndex = m_pImgMergeWindow->GetCurrentDiffIndex();
	m_pMDIFrame->UpdateCmdUI<ID_R2L>(diffIndex != -1 && !m_pImgMergeWindow->GetReadOnly(0) ? MF_ENABLED : MF_GRAYED);
	m_pMDIFrame->UpdateCmdUI<ID_L2R>(diffIndex != -1 && !m_pImgMergeWindow->GetReadOnly(1) ? MF_ENABLED : MF_GRAYED);
	int const diffCount = m_pImgMergeWindow->GetDiffCount();
	m_pMDIFrame->UpdateCmdUI<ID_ALL_LEFT>(diffCount != 0 && !m_pImgMergeWindow->GetReadOnly(0) ? MF_ENABLED : MF_GRAYED);
	m_pMDIFrame->UpdateCmdUI<ID_ALL_RIGHT>(diffCount != 0 && !m_pImgMergeWindow->GetReadOnly(1) ? MF_ENABLED : MF_GRAYED);
}

/**
 * @brief Undo last action
 */
void CImgMergeFrame::OnEditUndo()
{
	m_pImgMergeWindow->Undo();
	UpdateLastCompareResult();
}

/**
 * @brief Redo last action
 */
void CImgMergeFrame::OnEditRedo()
{
	m_pImgMergeWindow->Redo();
	UpdateLastCompareResult();
}

/**
 * @brief Called when user selects View/Zoom In from menu.
 */
void CImgMergeFrame::OnViewZoomIn()
{
	m_pImgMergeWindow->SetZoom(m_pImgMergeWindow->GetZoom() + 0.1);
}

/**
 * @brief Called when user selects View/Zoom Out from menu.
 */
void CImgMergeFrame::OnViewZoomOut()
{
	m_pImgMergeWindow->SetZoom(m_pImgMergeWindow->GetZoom() - 0.1);
}

/**
 * @brief Called when user selects View/Zoom Normal from menu.
 */
void CImgMergeFrame::OnViewZoomNormal()
{
	m_pImgMergeWindow->SetZoom(1.0);
}

/**
 * @brief Go to first diff
 *
 * Called when user selects "First Difference"
 * @sa CImgMergeFrame::SelectDiff()
 */
void CImgMergeFrame::OnFirstdiff()
{
	m_pImgMergeWindow->FirstDiff();
}

/**
 * @brief Go to last diff
 */
void CImgMergeFrame::OnLastdiff()
{
	m_pImgMergeWindow->LastDiff();
}

/**
 * @brief Go to next diff and select it.
 */
void CImgMergeFrame::OnNextdiff()
{
	if (m_pImgMergeWindow->GetCurrentDiffIndex() != m_pImgMergeWindow->GetDiffCount() - 1)
		m_pImgMergeWindow->NextDiff();
}

/**
 * @brief Go to previous diff and select it.
 */
void CImgMergeFrame::OnPrevdiff()
{
	if (m_pImgMergeWindow->GetCurrentDiffIndex() > 0)
		m_pImgMergeWindow->PrevDiff();
}

void CImgMergeFrame::OnX2Y(int srcPane, int dstPane)
{
	m_pImgMergeWindow->CopyDiff(m_pImgMergeWindow->GetCurrentDiffIndex(), srcPane, dstPane);
	UpdateLastCompareResult();
}

/**
 * @brief Copy diff from left pane to right pane
 */
void CImgMergeFrame::OnL2r()
{
	OnX2Y(0, 1);
}

/**
 * @brief Copy diff from right pane to left pane
 */
void CImgMergeFrame::OnR2l()
{
	OnX2Y(1, 0);
}

/**
 * @brief Copy all diffs from right pane to left pane
 */
void CImgMergeFrame::OnAllLeft()
{
	WaitStatusCursor waitstatus;
	m_pImgMergeWindow->CopyDiffAll(1, 0);
	UpdateLastCompareResult();
}

/**
 * @brief Copy all diffs from left pane to right pane
 */
void CImgMergeFrame::OnAllRight()
{
	WaitStatusCursor waitstatus;
	m_pImgMergeWindow->CopyDiffAll(0, 1);
	UpdateLastCompareResult();
}

template<>
LRESULT CImgMergeFrame::OnWndMsg<WM_COMMAND>(WPARAM wParam, LPARAM lParam)
{
	switch (const UINT id = lParam ? static_cast<UINT>(wParam) : LOWORD(wParam))
	{
	case ID_FIRSTDIFF:
		OnFirstdiff();
		UpdateCmdUI();
		break;
	case ID_LASTDIFF:
		OnLastdiff();
		UpdateCmdUI();
		break;
	case ID_NEXTDIFF:
		OnNextdiff();
		UpdateCmdUI();
		break;
	case ID_PREVDIFF:
		OnPrevdiff();
		UpdateCmdUI();
		break;
	case ID_EDIT_UNDO:
		OnEditUndo();
		UpdateCmdUI();
		break;
	case ID_EDIT_REDO:
		OnEditRedo();
		UpdateCmdUI();
		break;
	case ID_L2R:
		OnL2r();
		UpdateCmdUI();
		break;
	case ID_R2L:
		OnR2l();
		UpdateCmdUI();
		break;
	case ID_ALL_LEFT:
		OnAllLeft();
		UpdateCmdUI();
		break;
	case ID_ALL_RIGHT:
		OnAllRight();
		UpdateCmdUI();
		break;
	case ID_VIEW_IMAGETOOLBOX:
		if (int value = OnViewSubFrame(OPT_SHOW_IMAGETOOLBOX))
			m_wndLocationBar.m_uHitTestCode = value < 0 ? HTLEFT : HTRIGHT;
		else
			m_wndLocationBar.m_uHitTestCode = HTNOWHERE;
		RecalcLayout();
		break;
	case ID_VIEW_ZOOMIN:
		OnViewZoomIn();
		break;
	case ID_VIEW_ZOOMOUT:
		OnViewZoomOut();
		break;
	case ID_VIEW_ZOOMNORMAL:
		OnViewZoomNormal();
		break;
	case ID_FILE_SAVE:
		OnFileSave();
		UpdateCmdUI();
		break;
	case ID_FILE_SAVE_LEFT:
		OnFileSaveLeft();
		UpdateCmdUI();
		break;
	case ID_FILE_SAVE_RIGHT:
		OnFileSaveRight();
		UpdateCmdUI();
		break;
	case ID_FILE_SAVEAS_LEFT:
		OnFileSaveAsLeft();
		UpdateCmdUI();
		break;
	case ID_FILE_SAVEAS_RIGHT:
		OnFileSaveAsRight();
		UpdateCmdUI();
		break;
	case ID_NEXT_PANE:
	case ID_WINDOW_CHANGE_PANE:
		OnWindowChangePane();
		UpdateCmdUI();
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void CImgMergeFrame::HandleWinEvent(HWINEVENTHOOK hook, DWORD, HWND hwnd, LONG, LONG, DWORD, DWORD)
{
	HWindow *const pwnd = reinterpret_cast<HWindow *>(hwnd);
	HWindow *const pwndFrame = pwnd->GetParent()->GetParent();
	static_cast<CImgMergeFrame *>(OWindow::FromHandle(pwndFrame))->RecalcLayout();
	UnhookWinEvent(hook);
}

LRESULT CImgMergeFrame::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		if (LRESULT lResult = OnWndMsg<WM_COMMAND>(wParam, lParam))
			return lResult;
		break;
	case WM_MOUSEACTIVATE:
		switch (HIWORD(lParam))
		{
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
			POINT pt;
			GetCursorPos(&pt);
			HWND hwndHit = ::WindowFromPoint(pt);
			if (hwndHit == m_pImgMergeWindow->GetHWND())
			{
				SetWinEventHook(EVENT_SYSTEM_CAPTUREEND, EVENT_SYSTEM_CAPTUREEND, NULL,
					HandleWinEvent, GetCurrentProcessId(), GetCurrentThreadId(), WINEVENT_OUTOFCONTEXT);
			}
			else if (::IsChild(m_pImgMergeWindow->GetHWND(), hwndHit))
			{
				int pane = m_pImgMergeWindow->GetPaneCount();
				while (pane)
				{
					HWND hwndPane = m_pImgMergeWindow->GetPaneHWND(--pane);
					bool active = false;
					if (hwndPane == hwndHit)
					{
						m_pImgMergeWindow->SetActivePane(pane);
						active = true;
					}
					m_wndFilePathBar.SetActive(pane, active);
				}
			}
			break;
		}
		break;
	case WM_NCACTIVATE:
		if (wParam)
		{
			m_pMDIFrame->InitCmdUI();
			UpdateCmdUI();
		}
		break;
	case WM_CLOSE:
		if (!SaveModified())
			return 0;
		SaveOptions();
		break;
	case WM_DESTROY:
		if (m_pImgMergeWindow)
		{
			VERIFY(WinIMergeLib->WinIMerge_DestroyWindow(m_pImgMergeWindow));
			m_pImgMergeWindow = NULL;
		}
		if (m_pImgToolWindow)
		{
			VERIFY(WinIMergeLib->WinIMerge_DestroyToolWindow(m_pImgToolWindow));
			m_pImgToolWindow = NULL;
		}
		break;
	}
	return CDocFrame::WindowProc(uMsg, wParam, lParam);
}
