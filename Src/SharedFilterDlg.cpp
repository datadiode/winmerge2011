/** 
 * @file  SharedFilterDlg.cpp
 *
 * @brief Dialog where user choose shared or private filter
 */
#include "StdAfx.h"
#include "Merge.h"
#include "LanguageSelect.h"
#include "SharedFilterDlg.h"
#include "OptionsMgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSharedFilterDlg dialog


/**
 * @brief A constructor.
 */
CSharedFilterDlg::CSharedFilterDlg()
	: ODialog(IDD_SHARED_FILTER)
{
}

/////////////////////////////////////////////////////////////////////////////
// CSharedFilterDlg message handlers

/**
 * @brief Dialog initialization.
 */
BOOL CSharedFilterDlg::OnInitDialog()
{
	ODialog::OnInitDialog();
	LanguageSelect.TranslateDialog(m_hWnd);

	if (COptionsMgr::Get(OPT_FILEFILTER_SHARED))
		CheckDlgButton(IDC_SHARED, BST_CHECKED);
	else
		CheckDlgButton(IDC_PRIVATE, BST_CHECKED);

	return TRUE;
}

LRESULT CSharedFilterDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			OnOK();
			// fall through
		case IDCANCEL:
			EndDialog(wParam);
			break;
		}
		break;
	}
	return ODialog::WindowProc(message, wParam, lParam);
}

/**
 * @brief Called when user closes the dialog with OK button.
 */
void CSharedFilterDlg::OnOK()
{
	BOOL bShared = IsDlgButtonChecked(IDC_SHARED);
	COptionsMgr::SaveOption(OPT_FILEFILTER_SHARED, bShared != BST_UNCHECKED);
	m_ChosenFolder = bShared ? m_SharedFolder : m_PrivateFolder;
}
