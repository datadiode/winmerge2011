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
#include "codepage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLoadSaveCodepageDlg dialog

CLoadSaveCodepageDlg::CLoadSaveCodepageDlg()
: OResizableDialog(IDD_LOAD_SAVE_CODEPAGE)
, m_bAffectsLeft(TRUE)
, m_bAffectsRight(TRUE)
, m_bLoadSaveSameCodepage(TRUE)
, m_nLoadCodepage(-1)
, m_nSaveCodepage(-1)
, m_bEnableSaveCodepage(false)
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

	HComboBox *pCbLoadCodepage = static_cast<HComboBox *>(GetDlgItem(IDC_LOAD_CODEPAGE_TEXTBOX));
	HComboBox *pCbSaveCodepage = static_cast<HComboBox *>(GetDlgItem(IDC_SAVE_CODEPAGE_TEXTBOX));

	const stl::map<int, int> &f_codepage_status = codepage_status();
	stl::map<int, int>::const_iterator it = f_codepage_status.begin();
	while (it != f_codepage_status.end())
	{
		CPINFOEX info;
		int codepage = it->first;
		if (GetCPInfoEx(codepage, 0, &info))
		{
			TCHAR desc[300];
			wsprintf(desc, _T("%05d - %s"), codepage, info.CodePageName);
			int index = pCbLoadCodepage->AddString(desc);
			if (codepage == m_nLoadCodepage)
				pCbLoadCodepage->SetCurSel(index);
			index = pCbSaveCodepage->AddString(desc);
			if (codepage == m_nSaveCodepage)
				pCbSaveCodepage->SetCurSel(index);
		}
		++it;
	}

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
