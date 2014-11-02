/** 
 * @file  TestFilterDlg.cpp
 *
 * @brief Dialog for testing file filters
 */
#include "StdAfx.h"
#include "LanguageSelect.h"
#include "resource.h"
#include "TestFilterDlg.h"
#include "FileFilter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/**
 * @brief Constructor.
 * @param [in] pParent Parent window.
 * @param [in] pFileFilter File filter to test.
 * @param [in] pFilterMgr File filter manager.
 */
CTestFilterDlg::CTestFilterDlg(FileFilter *pFileFilter)
: ODialog(IDD_TEST_FILTER)
, m_pFileFilter(pFileFilter)
{
}

LRESULT CTestFilterDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case IDCANCEL:
			EndDialog(IDCANCEL);
			break;
		case MAKEWPARAM(IDC_TEST_BTN, BN_CLICKED):
			OnTestBtn();
			break;
		}
		break;
	}
	return ODialog::WindowProc(message, wParam, lParam);
}


/////////////////////////////////////////////////////////////////////////////
// CTestFilterDlg message handlers

/**
 * @brief Initialize the dialog.
 * @return FALSE always.
 */
BOOL CTestFilterDlg::OnInitDialog()
{
	ODialog::OnInitDialog();
	LanguageSelect.TranslateDialog(m_hWnd);

	GetDlgItem(IDC_TEST_TEXT)->SetFocus();

	SetDlgItemText(IDC_HEADER_FILTER_NAME, m_pFileFilter->name.c_str());
	
	return FALSE;
}

/** @brief User pressed Text button. */
void CTestFilterDlg::OnTestBtn() 
{
	String text;
	GetDlgItemText(IDC_TEST_TEXT, text);

	bool passed = CheckText(text);
	text += _T(": ");
	text += passed ? _T("passed") : _T("failed");

	AppendResult(text);
}

/**
 * @brief Test text against filter.
 * @param [in] text Text to test.
 * @return TRUE if text passes the filter, FALSE otherwise.
 */
bool CTestFilterDlg::CheckText(String text)
{
	// Convert any forward slashes to canonical Windows-style backslashes
	string_replace(text, _T('/'), _T('\\'));
	if (IsDlgButtonChecked(IDC_IS_DIRECTORY))
		return m_pFileFilter->TestDirNameAgainstFilter(_T(""), text.c_str());
	else
		return m_pFileFilter->TestFileNameAgainstFilter(_T(""), text.c_str());
}

/**
 * @brief Add new result to end of result edit box.
 * @param [in] result Result text to add.
 */
void CTestFilterDlg::AppendResult(String result)
{
	HEdit *edit = static_cast<HEdit *>(GetDlgItem(IDC_RESULTS));
	if (edit->GetWindowTextLength() > 0)
		result.insert(0, _T("\r\n"));
	int len = edit->GetWindowTextLength();
	edit->SetSel(len, len);
	edit->ReplaceSel(result.c_str());
}
