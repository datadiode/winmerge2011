/** 
 * @file  DirCompProgressDlg.cpp
 *
 * @brief Implementation file for Directory compare state dialog
 */
#include "StdAfx.h"
#include "Merge.h"
#include "LanguageSelect.h"
#include "DirCompProgressDlg.h"
#include "MainFrm.h"
#include "DirFrame.h"
#include "CompareStatisticsDlg.h"
#include "paths.h"

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
, m_rotPauseContinue(0)
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
			m_pDirDoc->GetCompareStats()->Continue();
			break;
		case IDC_PAUSE_CONTINUE:
			switch (m_rotPauseContinue)
			{
			case 0:
				m_pDirDoc->GetCompareStats()->Pause();
				break;
			case 1:
				m_pDirDoc->GetCompareStats()->Continue();
				break;
			}
			std::rotate(m_strPauseContinue.begin(),
				m_strPauseContinue.begin() + m_strPauseContinue.find('\t') + 1,
				m_strPauseContinue.end());
			SetDlgItemText(IDC_PAUSE_CONTINUE,
				m_strPauseContinue.substr(0, m_strPauseContinue.find('\t')).c_str());
			m_rotPauseContinue = (m_rotPauseContinue + 1) % 2;
			break;
		}
		break;
	case WM_TIMER:
		OnTimer(wParam);
		break;
	case WM_SETCURSOR:
		return GetParent()->SendMessage(message, wParam, lParam);
	case WM_CTLCOLORBTN:
		reinterpret_cast<HSurface *>(wParam)->SetBkColor(RGB(255,0,0));
		break;
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

	GetDlgItemText(IDC_PAUSE_CONTINUE, m_strPauseContinue);
	SetDlgItemText(IDC_PAUSE_CONTINUE, m_strPauseContinue.substr(0, m_strPauseContinue.find('\t')).c_str());

	SetTimer(IDT_UPDATE, UPDATE_INTERVAL);
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
		CompareStats *const pCompareStats = m_pDirDoc->GetCompareStats();
		if (const int totalItems = pCompareStats->GetTotalItems())
		{
			SetDlgItemInt(IDC_ITEMSTOTAL, totalItems);
			SendDlgItemMessage(IDC_PROGRESSCOMPARE, PBM_SETRANGE32, 0, totalItems);
			const int comparedItems = pCompareStats->GetComparedItems();
			CDiffContext *ctxt = m_pDirDoc->GetDiffContext();
			SetDlgItemInt(IDC_ITEMSCOMPARED, comparedItems);
			SendDlgItemMessage(IDC_PROGRESSCOMPARE, PBM_SETPOS, comparedItems);
			if (const DIFFITEM *di = pCompareStats->GetCurDiffItem())
			{
				if (HEdit *pEdit = static_cast<HEdit *>(GetDlgItem(IDC_LEFT_EDIT)))
				{
					String path = ctxt->GetLeftFilepathAndName(di);
					pEdit->SetWindowText(paths_CompactPath(pEdit, path));
				}
				if (HEdit *pEdit = static_cast<HEdit *>(GetDlgItem(IDC_RIGHT_EDIT)))
				{
					String path = ctxt->GetRightFilepathAndName(di);
					pEdit->SetWindowText(paths_CompactPath(pEdit, path));
				}
			}
			m_pStatsDlg->Update();
		}
	}
}
