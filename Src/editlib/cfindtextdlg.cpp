////////////////////////////////////////////////////////////////////////////
//  File:       cfindtextdlg.cpp
//  Version:    1.0.0.0
//  Created:    29-Dec-1998
//
//  Author:     Stcherbatchenko Andrei
//  E-mail:     windfall@gmx.de
//
//  Implementation of the CFindTextDlg dialog, a part of Crystal Edit -
//  syntax coloring text editor.
//
//  You are free to use or modify this code to the following restrictions:
//  - Acknowledge me somewhere in your about box, simple "Parts of code by.."
//  will be enough. If you can't (or don't want to), contact me personally.
//  - LEAVE THIS HEADER INTACT
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//  19-Jul-99
//      Ferdinand Prantl:
//  +   FEATURE: regular expressions, go to line and things ...
//  +   FEATURE: some other things I've forgotten ...
//
//  ... it's being edited very rapidly so sorry for non-commented
//        and maybe "ugly" code ...
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "LanguageSelect.h"
#include "cfindtextdlg.h"
#include "ccrystaltextview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFindTextDlg dialog

CFindTextDlg::CFindTextDlg(CCrystalTextView *pBuddy)
	: ODialog(IDD_EDIT_FIND)
	, m_pBuddy(pBuddy)
	, m_nDirection(1)
	, m_bConfirmed(false)
{
}

template<ODialog::DDX_Operation op>
bool CFindTextDlg::UpdateData()
{
	DDX_Check<op>(IDC_EDIT_DIRECTION_UP, m_nDirection, 0);
	DDX_Check<op>(IDC_EDIT_DIRECTION_DOWN, m_nDirection, 1);
	DDX_Check<op>(IDC_EDIT_MATCH_CASE, m_bMatchCase);
	DDX_CBStringExact<op>(IDC_EDIT_FINDTEXT, m_sText);
	DDX_Check<op>(IDC_EDIT_WHOLE_WORD, m_bWholeWord);
	DDX_Check<op>(IDC_EDIT_REGEXP, m_bRegExp);
	DDX_Check<op>(IDC_FINDDLG_DONTWRAP, m_bNoWrap);
	return true;
}

void CFindTextDlg::UpdateRegExp()
{
	if (m_bRegExp)
	{
		GetDlgItem(IDC_EDIT_WHOLE_WORD)->EnableWindow(FALSE);
		m_bWholeWord = FALSE;
	}
	else
	{
		GetDlgItem(IDC_EDIT_WHOLE_WORD)->EnableWindow(TRUE);
	}
}

LRESULT CFindTextDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			OnOK();
			break;
		case IDCANCEL:
			OnCancel();
			break;
		case MAKEWPARAM(IDC_EDIT_REGEXP, BN_CLICKED):
			OnRegExp();
			break;
		case MAKEWPARAM(IDC_EDIT_FINDTEXT, CBN_SELCHANGE):
			OnChangeSelected();
			// fall through
		case MAKEWPARAM(IDC_EDIT_FINDTEXT, CBN_EDITCHANGE):
			OnChangeEditText();
			break;
		case MAKEWPARAM(IDC_EDIT_FINDTEXT, CBN_DROPDOWN):
			reinterpret_cast<HSuperComboBox *>(lParam)->AdjustDroppedWidth();
			RegisterHotKey(m_hWnd, MAKEWPARAM(LOWORD(wParam), VK_DELETE), MOD_SHIFT, VK_DELETE);
			break;
		case MAKEWPARAM(IDC_EDIT_FINDTEXT, CBN_CLOSEUP):
			UnregisterHotKey(m_hWnd, MAKEWPARAM(LOWORD(wParam), VK_DELETE));
			break;
		}
		break;
	case WM_HOTKEY:
		if (HIWORD(wParam) == VK_DELETE)
		{
			HComboBox *const pCb = static_cast<HComboBox *>(GetDlgItem(LOWORD(wParam)));
			pCb->DeleteString(pCb->GetCurSel());
			OnChangeEditText();
		}
		break;
	case WM_SHOWWINDOW:
		if (wParam != FALSE)
			CenterWindow(m_pWnd, m_pBuddy->m_pWnd);
		break;
	}
	return ODialog::WindowProc(message, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////
// CFindTextDlg message handlers

void CFindTextDlg::OnOK()
{
	UpdateData<Get>();
	m_bConfirmed = true;
	ASSERT (m_pBuddy != NULL);
	BOOL bCursorToLeft = FALSE;
	DWORD dwSearchFlags = 0;
	if (m_bMatchCase)
		dwSearchFlags |= FIND_MATCH_CASE;
	if (m_bWholeWord)
		dwSearchFlags |= FIND_WHOLE_WORD;
	if (m_bRegExp)
		dwSearchFlags |= FIND_REGEXP;
	if (m_nDirection == 0)
	{
		dwSearchFlags |= FIND_DIRECTION_UP;
		// When finding upwards put cursor to begin of selection
		bCursorToLeft = TRUE;
	}
	m_pCbFindText->SaveState(_T("Files\\FindInFile"));
	POINT ptTextPos;
	if (!m_pBuddy->FindText(m_sText.c_str(), m_ptCurrentPos, dwSearchFlags, !m_bNoWrap, ptTextPos))
	{
		LanguageSelect.Format(
			IDS_EDIT_TEXT_NOT_FOUND, m_sText.c_str()
		).MsgBox(MB_ICONINFORMATION);
		m_ptCurrentPos.x = 0;
		m_ptCurrentPos.y = 0;
		return;
	}
	m_pBuddy->HighlightText(ptTextPos, m_pBuddy->m_nLastFindWhatLen, bCursorToLeft);
	EndDialog(IDOK);
}

void CFindTextDlg::OnChangeSelected()
{
	int sel = m_pCbFindText->GetCurSel();
	if (sel != CB_ERR)
	{
		String sText;
		m_pCbFindText->GetLBText(sel, sText);
		m_pCbFindText->SetWindowText(sText.c_str());
	}
}

void CFindTextDlg::OnChangeEditText()
{
	UpdateData<Get>();
	UpdateControls();
}

BOOL CFindTextDlg::OnInitDialog()
{
	ODialog::OnInitDialog();
	LanguageSelect.TranslateDialog(m_hWnd);
	m_pCbFindText = static_cast<HSuperComboBox *>(GetDlgItem(IDC_EDIT_FINDTEXT));
	m_pCbFindText->LoadState(_T("Files\\FindInFile"));
	UpdateData<Set>();
	UpdateControls();
	return TRUE;
}

void CFindTextDlg::OnCancel()
{
	UpdateData<Get>();
	EndDialog(IDCANCEL);
}

void CFindTextDlg::OnRegExp()
{
	UpdateData<Get>();
	UpdateRegExp();
	UpdateData<Set>();
}


//
// Update controls, enabling/disabling according to what's appropriate
//
void CFindTextDlg::UpdateControls()
{
	GetDlgItem(IDOK)->EnableWindow(!m_sText.empty());
	UpdateRegExp();
}
