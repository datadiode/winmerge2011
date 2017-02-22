/**
 * @file  LoadSaveCodepageDlg.cpp
 *
 * @brief Implementation of the dialog used to select codepages
 */
#include "stdafx.h"
#include "Merge.h"
#include "LanguageSelect.h"
#include "MainFrm.h"
#include "resource.h"
#include "LoadSaveCodepageDlg.h"
#include "Common/SuperComboBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLoadSaveCodepageDlg dialog

CLoadSaveCodepageDlg *CLoadSaveCodepageDlg::m_pThis = NULL;

CLoadSaveCodepageDlg::CLoadSaveCodepageDlg()
: OResizableDialog(IDD_LOAD_SAVE_CODEPAGE)
, m_bAffectsLeft(TRUE)
, m_bAffectsRight(TRUE)
, m_bLoadSaveSameCodepage(TRUE)
, m_nLoadCodepage(-1)
, m_nSaveCodepage(-1)
, m_bEnableSaveCodepage(false)
, m_pCbLoadCodepage(NULL)
, m_pCbSaveCodepage(NULL)
{
	static const LONG FloatScript[] =
	{
		IDC_AFFECTS_GROUP,			BY<1000>::X2R,
		IDC_LEFT_FILES_LABEL,		BY<1000>::X2R,
		IDC_RIGHT_FILES_LABEL,		BY<1000>::X2R,
		IDC_CODEPAGE_GROUP,			BY<1000>::X2R,
		IDC_LOAD_CODEPAGE_TEXTBOX,	BY<1000>::X2R,
		IDC_SAVE_CODEPAGE_TEXTBOX,	BY<1000>::X2R,
		IDOK,						BY<1000>::X2L | BY<1000>::X2R,
		IDCANCEL,					BY<1000>::X2L | BY<1000>::X2R,
		0
	};
	CFloatState::FloatScript = FloatScript;
	m_pThis = this;
}

CLoadSaveCodepageDlg::~CLoadSaveCodepageDlg()
{
	m_pThis = NULL;
}

template<ODialog::DDX_Operation op>
bool CLoadSaveCodepageDlg::UpdateData()
{
	DDX_Check<op>(IDC_AFFECTS_LEFT_BTN, m_bAffectsLeft);
	DDX_Check<op>(IDC_AFFECTS_RIGHT_BTN, m_bAffectsRight);
	DDX_Check<op>(IDC_LOAD_SAVE_SAME_CODEPAGE, m_bLoadSaveSameCodepage);
	DDX_Text<op>(IDC_LOAD_CODEPAGE_TEXTBOX, m_nLoadCodepage);
	DDX_Text<op>(IDC_SAVE_CODEPAGE_TEXTBOX, m_nSaveCodepage);
	return true;
}

LRESULT CLoadSaveCodepageDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
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
			EndDialog(IDCANCEL);
			break;
		case MAKEWPARAM(IDC_AFFECTS_LEFT_BTN, BN_CLICKED):
			OnAffectsLeftBtnClicked();
			break;
		case MAKEWPARAM(IDC_AFFECTS_RIGHT_BTN, BN_CLICKED):
			OnAffectsRightBtnClicked();
			break;
		case MAKEWPARAM(IDC_LOAD_SAVE_SAME_CODEPAGE, BN_CLICKED):
			UpdateSaveGroup();
			break;
		case MAKEWPARAM(IDC_LOAD_CODEPAGE_TEXTBOX, CBN_DROPDOWN):
		case MAKEWPARAM(IDC_SAVE_CODEPAGE_TEXTBOX, CBN_DROPDOWN):
			reinterpret_cast<HSuperComboBox *>(lParam)->AdjustDroppedWidth();
			break;
		}
		break;
	}
	return OResizableDialog::WindowProc(message, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////
// CLoadSaveCodepageDlg message handlers

/**
 * @brief Handler for WM_INITDIALOG; conventional location to initialize controls
 * At this point dialog and control windows exist
 */
BOOL CLoadSaveCodepageDlg::OnInitDialog() 
{
	OResizableDialog::OnInitDialog();
	LanguageSelect.TranslateDialog(m_hWnd);

	SetDlgItemText(IDC_LEFT_FILES_LABEL, m_sAffectsLeftString.c_str());
	SetDlgItemText(IDC_RIGHT_FILES_LABEL, m_sAffectsRightString.c_str());

	UpdateData<Set>();
	UpdateSaveGroup();

	m_pCbLoadCodepage = static_cast<HComboBox *>(GetDlgItem(IDC_LOAD_CODEPAGE_TEXTBOX));
	m_pCbSaveCodepage = static_cast<HComboBox *>(GetDlgItem(IDC_SAVE_CODEPAGE_TEXTBOX));

	EnumSystemCodePages(EnumCodePagesProc, CP_INSTALLED);

	return TRUE;
}

/**
 * @brief If user unchecks left, then check right (to ensure never have nothing checked)
 */
void CLoadSaveCodepageDlg::OnAffectsLeftBtnClicked()
{
	if (IsDlgButtonChecked(IDC_AFFECTS_LEFT_BTN) != BST_CHECKED)
		CheckDlgButton(IDC_AFFECTS_RIGHT_BTN, BST_CHECKED);
}

/**
 * @brief If user unchecks right, then check left (to ensure never have nothing checked)
 */
void CLoadSaveCodepageDlg::OnAffectsRightBtnClicked()
{
	if (IsDlgButtonChecked(IDC_AFFECTS_RIGHT_BTN) != BST_CHECKED)
		CheckDlgButton(IDC_AFFECTS_LEFT_BTN, BST_CHECKED);
}

/**
 * @brief Disable save group if save codepage slaved to load codepage
 */
void CLoadSaveCodepageDlg::UpdateSaveGroup()
{
	UpdateData<Get>();
	GetDlgItem(IDC_LOAD_SAVE_SAME_CODEPAGE)->EnableWindow(m_bEnableSaveCodepage);
	bool EnableSave = m_bEnableSaveCodepage && !m_bLoadSaveSameCodepage;
	GetDlgItem(IDC_SAVE_CODEPAGE_TEXTBOX)->EnableWindow(EnableSave);
}

/**
 * @brief User pressed Ok, ensure data members set correctly
 */
void CLoadSaveCodepageDlg::OnOK()
{
	UpdateData<Get>();
	if (m_bLoadSaveSameCodepage)
		m_nSaveCodepage = m_nLoadCodepage;
	EndDialog(IDOK);
}

/**
 * @brief Callback for use with EnumSystemCodePages()
 */
BOOL CLoadSaveCodepageDlg::EnumCodePagesProc(LPTSTR lpCodePageString)
{
	if (UINT const codepage = _tcstol(lpCodePageString, &lpCodePageString, 10))
	{
		CPINFOEX info;
		if (GetCPInfoEx(codepage, 0, &info))
		{
			int lower = 0;
			int upper = m_pThis->m_pCbLoadCodepage->GetCount();
			while (lower < upper)
			{
				int const match = (upper + lower) >> 1;
				UINT const cmp = static_cast<UINT>(m_pThis->m_pCbLoadCodepage->GetItemData(match));
				if (cmp >= info.CodePage)
					upper = match;
				if (cmp <= info.CodePage)
					lower = match + 1;
			}
			// Cosmetic: Remove excess spaces between numeric identifier and what follows it
			if (_tcstol(info.CodePageName, &lpCodePageString, 10) && *lpCodePageString == _T(' '))
				StrTrim(lpCodePageString + 1, _T(" "));
			int index = m_pThis->m_pCbLoadCodepage->InsertString(lower, info.CodePageName);
			m_pThis->m_pCbLoadCodepage->SetItemData(index, info.CodePage);
			if (codepage == m_pThis->m_nLoadCodepage)
				m_pThis->m_pCbLoadCodepage->SetCurSel(index);
			index = m_pThis->m_pCbSaveCodepage->InsertString(lower, info.CodePageName);
			m_pThis->m_pCbSaveCodepage->SetItemData(index, info.CodePage);
			if (codepage == m_pThis->m_nSaveCodepage)
				m_pThis->m_pCbSaveCodepage->SetCurSel(index);
		}
	}
	return TRUE; // continue enumeration
}
