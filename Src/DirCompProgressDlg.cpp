/** 
 * @file  DirCompProgressDlg.cpp
 *
 * @brief Implementation file for Directory compare state dialog
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "Merge.h"
#include "LanguageSelect.h"
#include "DirCompProgressDlg.h"
#include "MainFrm.h"
#include "DirFrame.h"
#include "CompareStats.h"
#include "CompareStatisticsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/** @brief ID for timer updating UI. */
static const UINT IDT_UPDATE = 1;

/** @brief Interval (in milliseconds) for UI updates. */
static const UINT UPDATE_INTERVAL = 400;

/**
 * @brief Constructor.
 * @param [in] pParent Parent window for progress dialog.
 */
DirCompProgressDlg::DirCompProgressDlg(CDirFrame *pDirDoc)
: ODialog(IDD_DIRCOMP_PROGRESS)
, m_pDirDoc(pDirDoc)
, m_pStatsDlg(new CompareStatisticsDlg(pDirDoc->GetCompareStats()))
{
	LanguageSelect.Create(*this, theApp.m_pMainWnd->GetLastActivePopup()->m_hWnd);
}

DirCompProgressDlg::~DirCompProgressDlg()
{
	KillTimer(IDT_UPDATE);
	DestroyWindow();
	delete m_pStatsDlg;
}

LRESULT DirCompProgressDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case IDCANCEL:
			m_pDirDoc->AbortCurrentScan();
			break;
		}
		break;
	case WM_TIMER:
		OnTimer(wParam);
		break;
	case WM_SETCURSOR:
		return GetParent()->SendMessage(message, wParam, lParam);
	}
	return ODialog::WindowProc(message, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////

/**
 * @brief Initialize the dialog.
 * Center the dialog to main window.
 * @return TRUE (see the comment inside function).
 */
BOOL DirCompProgressDlg::OnInitDialog() 
{
	ODialog::OnInitDialog();
	SetFocus();
	LanguageSelect.TranslateDialog(m_hWnd);
	LanguageSelect.Create(*m_pStatsDlg, m_hWnd);
	m_pStatsDlg->SendDlgItemMessage(IDOK, WM_CLOSE);
	m_pStatsDlg->SetStyle(m_pStatsDlg->GetStyle() & ~(WS_POPUP | WS_CAPTION | WS_SYSMENU) | WS_CHILD);
	m_pStatsDlg->SetExStyle(m_pStatsDlg->GetExStyle() & ~WS_EX_DLGMODALFRAME);
	RECT rectOuter, rectInner, rectGroup;
	GetClientRect(&rectOuter);
	HWND hwndGroupBox = ::FindWindowEx(m_pStatsDlg->m_hWnd, NULL, WC_BUTTON, NULL);
	::GetClientRect(hwndGroupBox, &rectGroup);
	::MapWindowPoints(hwndGroupBox, m_pStatsDlg->m_hWnd, (LPPOINT)&rectGroup, 1);
	m_pStatsDlg->GetClientRect(&rectInner);
	m_pStatsDlg->SetParent(m_pWnd);
	m_pStatsDlg->SetWindowPos(NULL,
		(rectOuter.right - rectGroup.left - rectGroup.right) / 2,
		rectOuter.bottom - rectInner.bottom, 0, 0, SWP_NOSIZE | SWP_NOZORDER |
		SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_FRAMECHANGED);

	// Reset all UI fields to zero.
	SendDlgItemMessage(IDC_PROGRESSCOMPARE, PBM_SETPOS, 0);
	SetDlgItemInt(IDC_ITEMSCOMPARED, 0);
	SetDlgItemInt(IDC_ITEMSTOTAL, 0);

	SetTimer(IDT_UPDATE, UPDATE_INTERVAL, NULL);
	return TRUE;
}

/**
 * @brief Timer message received.
 * Handle timer messages. When timer fires, update the dialog.
 * @param [in] nIDEvent ID of the timer that fired.
 */
void DirCompProgressDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == IDT_UPDATE)
	{
		const CompareStats *const pCompareStats = m_pDirDoc->GetCompareStats();
		const int totalItems = pCompareStats->GetTotalItems();
		const int comparedItems = pCompareStats->GetComparedItems();
		SetDlgItemInt(IDC_ITEMSTOTAL, totalItems);
		SetDlgItemInt(IDC_ITEMSCOMPARED, comparedItems);
		SendDlgItemMessage(IDC_PROGRESSCOMPARE, PBM_SETRANGE32, 0, totalItems);
		SendDlgItemMessage(IDC_PROGRESSCOMPARE, PBM_SETPOS, comparedItems);
		m_pStatsDlg->Update();
	}
}
