/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997-2000  Thingamahoochie Software
//    Author: Dean Grimm
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
 * @file  HexMergeFrm.cpp
 *
 * @brief Implementation file for CHexMergeFrame
 *
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "stdafx.h"
#include "Merge.h"
#include "MainFrm.h"
#include "DirFrame.h"
#include "LanguageSelect.h"
#include "HexMergeFrm.h"
#include "HexMergeView.h"
#include "CompareStats.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHexMergeFrame

/////////////////////////////////////////////////////////////////////////////
// CHexMergeFrame construction/destruction

const LONG CHexMergeFrame::FloatScript[] =
{
	IDC_STATIC_TITLE_LEFT, BY<500>::X2R,
	IDC_STATIC_TITLE_RIGHT, BY<500>::X2L | BY<1000>::X2R,
	0x1000, BY<500>::X2R | BY<1000>::Y2B,
	0x1001, BY<500>::X2L | BY<1000>::X2R | BY<1000>::Y2B,
	0x6000, BY<1000>::Y2T | BY<1000>::Y2B | BY<500>::X2R,
	0x6001, BY<500>::X2L | BY<1000>::X2R | BY<1000>::Y2T | BY<1000>::Y2B,
	0
};

const LONG CHexMergeFrame::SplitScript[] =
{
	0x6001, IDC_STATIC_TITLE_LEFT, ~20,
	0
};

CHexMergeFrame::CHexMergeFrame(CMainFrame *pMDIFrame, CDirFrame *pDirDoc)
: CEditorFrame(pMDIFrame, pDirDoc, GetHandleSet<IDR_HEXDOCTYPE>(), FloatScript, SplitScript)
{
	ASSERT(m_pView[0] == NULL);
	ASSERT(m_pView[1] == NULL);
	SubclassWindow(pMDIFrame->CreateChildHandle());
	CreateClient();
	RecalcLayout();
	m_bAutoDelete = true;
}

CHexMergeFrame::~CHexMergeFrame()
{
	if (m_pDirDoc)
		m_pDirDoc->MergeDocClosing(this);
}

/**
 * @brief Customize a heksedit control's settings
 */
static void Customize(IHexEditorWindow::Settings *settings)
{
	settings->bSaveIni = FALSE;
	settings->bAutoOffsetLen = FALSE;
	settings->iMinOffsetLen = 8;
	settings->iMaxOffsetLen = 8;
	settings->iAutomaticBPL = FALSE;
	settings->iBytesPerLine = 16;
	settings->iFontSize = 8;
}

/**
 * @brief Customize a heksedit control's colors
 */
static void Customize(IHexEditorWindow::Colors *colors)
{
	colors->iSelBkColorValue = RGB(224, 224, 224);
	colors->iDiffBkColorValue = COptionsMgr::Get(OPT_CLR_DIFF);
	colors->iSelDiffBkColorValue = COptionsMgr::Get(OPT_CLR_SELECTED_DIFF);
	colors->iDiffTextColorValue = COptionsMgr::Get(OPT_CLR_DIFF_TEXT);
	colors->iSelDiffTextColorValue = COptionsMgr::Get(OPT_CLR_SELECTED_DIFF_TEXT);
}

/**
 * @brief Customize a heksedit control's settings and colors
 */
static void Customize(IHexEditorWindow *pif)
{
	Customize(pif->get_settings());
	Customize(pif->get_colors());
	LANGID wLangID = (LANGID)GetThreadLocale();
	pif->load_lang(wLangID);
}

/**
 * @brief Create a status bar to be associated with a heksedit control
 */
CHexMergeView *CHexMergeFrame::CreatePane(int iPane)
{
	RECT rect;
	HEdit *pEdit = m_wndFilePathBar.GetControlRect(iPane, &rect);
	const int xFilePathEdit = rect.left;
	const int cyFilePathEdit = rect.bottom;

	LONG additionalStyles = 0;
	if (iPane == 1)
	{
		additionalStyles = WS_VSCROLL | SBS_SIZEGRIP;
		rect.right += GetSystemMetrics(SM_CXVSCROLL);
	}

	CHexMergeView *pView = new CHexMergeView(this, iPane);
	pView->SubclassWindow(HWindow::CreateEx(WS_EX_CLIENTEDGE, CHexMergeView::RegisterClass(), NULL,
		WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_TABSTOP | additionalStyles & WS_VSCROLL,
		rect.left, rect.bottom, rect.right - rect.left, 0,
		m_wndFilePathBar.m_pWnd, 0x1000 + iPane));

	HWindow *pParent = wine_version ? m_wndFilePathBar.m_pWnd : pView->m_pWnd;
	const int cxStatusBar = rect.right - rect.left;
	HStatusBar *pBar = HStatusBar::Create(
		WS_CHILD | WS_VISIBLE | CCS_NOPARENTALIGN | additionalStyles & SBS_SIZEGRIP,
		rect.left, rect.bottom, cxStatusBar, rect.bottom,
		pParent, 0x6000 + iPane);
	pBar->GetWindowRect(&rect);
	pParent->ScreenToClient(&rect);
	if (wine_version)
	{
		pBar->SetParent(pView->m_pWnd);
		pView->SendMessage(WM_PARENTNOTIFY, MAKEWPARAM(WM_CREATE, 0x6000 + iPane), (LPARAM)pBar);
	}
	pBar->SetParent(m_wndFilePathBar.m_pWnd);
	const int cyStatusBar = rect.bottom - rect.top;
	pBar->SetStyle(pBar->GetStyle() | CCS_NORESIZE);

	pBar->SetWindowPos(reinterpret_cast<HWindow *>(HWND_BOTTOM),
		xFilePathEdit, rect.top, cxStatusBar, cyStatusBar,
		SWP_NOACTIVATE);

	pView->SetWindowPos(NULL,
		xFilePathEdit, cyStatusBar,
		cxStatusBar, cyFilePathEdit - cyStatusBar,
		SWP_NOZORDER | SWP_NOACTIVATE);

	pEdit->SetWindowPos(NULL, 0, 0,
		cxStatusBar, cyStatusBar,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

	pView->m_pStatusBar = pBar;
	
	return pView;
}

/**
 * @brief Create the splitter, the filename bar, the status bar, and the two views
 */
void CHexMergeFrame::CreateClient()
{
	// Merge frame has a header bar at top
	m_wndFilePathBar.Create(m_hWnd);

	CHexMergeView *pLeft = CreatePane(0);
	CHexMergeView *pRight = CreatePane(1);

	// get the IHexEditorWindow interfaces
	IHexEditorWindow *pifLeft = pLeft->GetInterface();
	IHexEditorWindow *pifRight = pRight->GetInterface();

	if (pifLeft == NULL || pifLeft->get_interface_version() < HEKSEDIT_INTERFACE_VERSION)
	{
		return;
	}

	// tell the heksedit controls about each other
	pifLeft->set_sibling(pifRight);
	pifRight->set_sibling(pifLeft);

	// adjust a few settings and colors
	Customize(pifLeft);
	Customize(pifRight);

	// tell merge doc about these views
	SetMergeViews(pLeft, pRight);
}

/**
 * @brief Reflect comparison result in window's icon.
 * @param nResult [in] Last comparison result which the application returns.
 */
void CHexMergeFrame::SetLastCompareResult(int nResult)
{
	HICON hCurrent = GetIcon();
	HICON hReplace = LanguageSelect.LoadIcon(
		nResult == 0 ? IDI_EQUALBINARY : IDI_BINARYDIFF);
		
	if (hCurrent != hReplace)
	{
		SetIcon(hReplace, nResult == 0 ?
			CompareStats::DIFFIMG_BINSAME: CompareStats::DIFFIMG_BINDIFF);
		BOOL bMaximized;
		m_pMDIFrame->GetActiveDocFrame(&bMaximized);
		// When MDI maximized the window icon is drawn on the menu bar, so we
		// need to notify it that our icon has changed.
		if (bMaximized)
		{
			m_pMDIFrame->DrawMenuBar();
		}
	}
}

/**
 * @brief Update any resources necessary after a GUI language change
 */
void CHexMergeFrame::UpdateResources()
{
}

void CHexMergeFrame::UpdateCmdUI()
{
	if (m_pMDIFrame->GetActiveDocFrame() != this)
		return;
	BYTE enableSaveLeft = m_pView[0]->GetModified() ? MF_ENABLED : MF_GRAYED;
	BYTE enableSaveRight = m_pView[1]->GetModified() ? MF_ENABLED : MF_GRAYED;
	m_pMDIFrame->UpdateCmdUI<ID_FILE_SAVE_LEFT>(enableSaveLeft);
	m_pMDIFrame->UpdateCmdUI<ID_FILE_SAVE_RIGHT>(enableSaveRight);
	m_pMDIFrame->UpdateCmdUI<ID_FILE_SAVE>(enableSaveLeft & enableSaveRight);
}

template<>
LRESULT CHexMergeFrame::OnWndMsg<WM_COMMAND>(WPARAM wParam, LPARAM lParam)
{
	CHexMergeView *pActiveView = GetActiveView();
	switch (const UINT id = lParam ? static_cast<UINT>(wParam) : LOWORD(wParam))
	{
	case ID_FIRSTDIFF:
		pActiveView->OnFirstdiff();
		break;
	case ID_LASTDIFF:
		pActiveView->OnLastdiff();
		break;
	case ID_NEXTDIFF:
		pActiveView->OnNextdiff();
		break;
	case ID_PREVDIFF:
		pActiveView->OnPrevdiff();
		break;
	case ID_EDIT_FIND:
		pActiveView->OnEditFind();
		break;
	case ID_EDIT_REPLACE:
		pActiveView->OnEditReplace();
		break;
	case ID_EDIT_REPEAT:
		pActiveView->OnEditRepeat();
		break;
	case ID_EDIT_CUT:
		pActiveView->OnEditCut();
		break;
	case ID_EDIT_COPY:
		pActiveView->OnEditCopy();
		break;
	case ID_EDIT_PASTE:
		pActiveView->OnEditPaste();
		break;
	case ID_EDIT_CLEAR:
		pActiveView->OnEditClear();
		break;
	case ID_EDIT_SELECT_ALL:
		pActiveView->OnEditSelectAll();
		break;
	case ID_L2R:
		OnL2r();
		break;
	case ID_R2L:
		OnR2l();
		break;
	case ID_ALL_LEFT:
		OnAllLeft();
		break;
	case ID_ALL_RIGHT:
		OnAllRight();
		break;
	case ID_VIEW_ZOOMIN:
		OnViewZoom(1);
		break;
	case ID_VIEW_ZOOMOUT:
		OnViewZoom(-1);
		break;
	case ID_VIEW_ZOOMNORMAL:
		OnViewZoom(0);
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
		if (HWindow *pWndNext = GetNextDlgTabItem(HWindow::GetFocus()))
		{
			pWndNext->SetFocus();
		}
		break;
	case IDCANCEL:
		PostMessage(WM_CLOSE);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void CHexMergeFrame::RecalcBytesPerLine()
{
	RECT rect;
	CHexMergeView *pNarrowestView = NULL;
	int weight = INT_MAX;
	int i;
	for (i = 0; i < _countof(m_pView); ++i)
	{
		CHexMergeView *pView = m_pView[i];
		if (pView == NULL)
			return;
		pView->GetClientRect(&rect);
		if (weight > rect.right)
		{
			pNarrowestView = pView;
			weight = rect.right;
		}
	}
	for (i = 0; i < _countof(m_pView); ++i)
	{
		CHexMergeView *pView = m_pView[i];
		if (pView == pNarrowestView)
		{
			IHexEditorWindow *pif = pView->GetInterface();
			IHexEditorWindow::Settings *settings = pif->get_settings();
			settings->iAutomaticBPL = COptionsMgr::Get(OPT_AUTOMATIC_BPL);
			settings->iBytesPerLine = COptionsMgr::Get(OPT_BYTES_PER_LINE);
			pif->resize_window();
			settings->iAutomaticBPL = FALSE;
			weight = settings->iBytesPerLine;
		}
	}
	for (i = 0; i < _countof(m_pView); ++i)
	{
		CHexMergeView *pView = m_pView[i];
		if (pView != pNarrowestView)
		{
			IHexEditorWindow *pif = pView->GetInterface();
			IHexEditorWindow::Settings *settings = pif->get_settings();
			settings->iBytesPerLine = weight;
			pif->resize_window();
		}
	}
}

LRESULT CHexMergeFrame::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
			m_pMDIFrame->UpdateCmdUI<ID_NEXTDIFF>(MF_ENABLED);
			m_pMDIFrame->UpdateCmdUI<ID_PREVDIFF>(MF_ENABLED);
			m_pMDIFrame->UpdateCmdUI<ID_EDIT_CUT>(MF_ENABLED);
			m_pMDIFrame->UpdateCmdUI<ID_EDIT_COPY>(MF_ENABLED);
			m_pMDIFrame->UpdateCmdUI<ID_EDIT_PASTE>(MF_ENABLED);
			m_pMDIFrame->UpdateCmdUI<ID_L2R>(MF_ENABLED);
			m_pMDIFrame->UpdateCmdUI<ID_R2L>(MF_ENABLED);
			m_pMDIFrame->UpdateCmdUI<ID_ALL_LEFT>(MF_ENABLED);
			m_pMDIFrame->UpdateCmdUI<ID_ALL_RIGHT>(MF_ENABLED);
		}
		break;
	case WM_CLOSE:
		if (!SaveModified())
			return 0;
		break;
	}
	return CDocFrame::WindowProc(uMsg, wParam, lParam);
}

BOOL CHexMergeFrame::PreTranslateMessage(MSG *pMsg)
{
	// First check our own accelerators, then pass down to heksedit.
	// This is essential for, e.g., ID_VIEW_ZOOMIN/ID_VIEW_ZOOMOUT.
	if (m_pMDIFrame->m_hAccelTable &&
		::TranslateAccelerator(m_pMDIFrame->m_hWnd, m_pMDIFrame->m_hAccelTable, pMsg))
	{
		return TRUE;
	}
	if (CHexMergeView *pView = GetActiveView())
	{
		HACCEL hAccel = m_pHandleSet->m_hAccelShared;
		if (hAccel && ::TranslateAccelerator(m_hWnd, hAccel, pMsg))
		{
			return TRUE;
		}
		if (pView->PreTranslateMessage(pMsg))
		{
			return TRUE;
		}
	}
	return FALSE;
}
