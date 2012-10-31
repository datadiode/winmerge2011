/** 
 * @file  DirCmpReportDlg.cpp
 *
 * @brief Implementation file for DirCmpReport dialog
 *
 */
// ID line follows -- this is updated by SVN
// $Id$
//

#include "StdAfx.h"
#include "Merge.h"
#include "LanguageSelect.h"
#include "Coretools.h"
#include "DirCmpReportDlg.h"
#include "DirReportTypes.h"
#include "paths.h"
#include "FileOrFolderSelect.h"
#include "SettingStore.h"

/**
 * @brief Constructor.
 */
DirCmpReportDlg::DirCmpReportDlg()
	: ODialog(IDD_DIRCMP_REPORT)
{
}

LRESULT DirCmpReportDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
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
		case MAKEWPARAM(IDC_REPORT_BROWSEFILE, BN_CLICKED):
			OnBtnClickReportBrowse();
			break;
		case MAKEWPARAM(IDC_REPORT_COPYCLIPBOARD, BN_DOUBLECLICKED):
			OnBtnDblclickCopyClipboard();
			break;
		case MAKEWPARAM(IDC_REPORT_FILE, CBN_DROPDOWN):
			reinterpret_cast<HSuperComboBox *>(lParam)->AdjustDroppedWidth();
			RegisterHotKey(m_hWnd, MAKEWPARAM(LOWORD(wParam), VK_DELETE), MOD_SHIFT, VK_DELETE);
			break;
		case MAKEWPARAM(IDC_REPORT_FILE, CBN_CLOSEUP):
			UnregisterHotKey(m_hWnd, MAKEWPARAM(LOWORD(wParam), VK_DELETE));
			break;
		}
		break;
	case WM_HOTKEY:
		if (HIWORD(wParam) == VK_DELETE)
		{
			HComboBox *const pCb = static_cast<HComboBox *>(GetDlgItem(LOWORD(wParam)));
			pCb->DeleteString(pCb->GetCurSel());
		}
		break;
	}
	return ODialog::WindowProc(message, wParam, lParam);
}

/**
 * @brief Definition for structure containing report types.
 * This struct is used to form a report types list. This list
 * can be then used to initialize the GUI for reports.
 */
struct ReportTypeInfo
{
	REPORT_TYPE reportType; /**< Report-type ID */
	int idDisplay; /**< Resource-string ID (shown in file-selection dialog) */
	int browseFilter; /**< File-extension filter (resource-string ID) */
};

/**
 * @brief List of report types.
 * This list is used to initialize the GUI.
 */
static const ReportTypeInfo f_types[] = {
	{ REPORT_TYPE_COMMALIST,
		IDS_REPORT_COMMALIST,
		IDS_TEXT_REPORT_FILES
	},
	{ REPORT_TYPE_TABLIST,
		IDS_REPORT_TABLIST,
		IDS_TEXT_REPORT_FILES
	},
	{ REPORT_TYPE_SIMPLEHTML,
		IDS_REPORT_SIMPLEHTML,
		IDS_HTML_REPORT_FILES
	},
	{ REPORT_TYPE_SIMPLEXML,
		IDS_REPORT_SIMPLEXML,
		IDS_XML_REPORT_FILES
	},
};

/**
 * @brief Dialog initializer function.
 */
BOOL DirCmpReportDlg::OnInitDialog()
{
	ODialog::OnInitDialog();
	LanguageSelect.TranslateDialog(m_hWnd);

	m_ctlStyle = static_cast<HComboBox *>(GetDlgItem(IDC_REPORT_STYLECOMBO));
	m_pCbReportFile = static_cast<HSuperComboBox *>(GetDlgItem(IDC_REPORT_FILE));
	
	CheckDlgButton(IDC_REPORT_COPYCLIPBOARD, m_bCopyToClipboard);

	m_pCbReportFile->LoadState(_T("ReportFiles"));

	m_nReportType = static_cast<REPORT_TYPE>(
		SettingStore.GetProfileInt(_T("Settings"), _T("DirCmpReportStyle"), 0));
	for (int i = 0; i < _countof(f_types); ++i)
	{
		const ReportTypeInfo &info = f_types[i];
		int ind = m_ctlStyle->InsertString(i, LanguageSelect.LoadString(info.idDisplay).c_str());
		m_ctlStyle->SetItemData(ind, info.reportType);
		if (info.reportType == m_nReportType)
		{
			m_ctlStyle->SetCurSel(ind);
		}
	}
	// Set selected path to variable so file selection dialog shows
	// correct filename and path.
	m_pCbReportFile->GetWindowText(m_sReportFile);

	return TRUE;
}

/**
 * @brief Browse for report file.
 */
void DirCmpReportDlg::OnBtnClickReportBrowse()
{
	m_pCbReportFile->GetWindowText(m_sReportFile);
	int filterid = f_types[m_ctlStyle->GetCurSel()].browseFilter;
	if (SelectFile(m_hWnd, m_sReportFile, IDS_SAVE_AS_TITLE, filterid, FALSE))
	{
		m_pCbReportFile->SetWindowText(m_sReportFile.c_str());
	}
}

/**
 * @brief Erase report file name.
 */
void DirCmpReportDlg::OnBtnDblclickCopyClipboard()
{
	m_pCbReportFile->SetWindowText(_T(""));
}

/**
 * @brief Close dialog.
 */
void DirCmpReportDlg::OnOK()
{
	m_pCbReportFile->GetWindowText(m_sReportFile);
	m_bCopyToClipboard = IsDlgButtonChecked(IDC_REPORT_COPYCLIPBOARD);
	int sel = m_ctlStyle->GetCurSel();
	m_nReportType = static_cast<REPORT_TYPE>(m_ctlStyle->GetItemData(sel));

	if (m_sReportFile.empty() && !m_bCopyToClipboard)
	{
		LanguageSelect.MsgBox(IDS_MUST_SPECIFY_OUTPUT, MB_ICONSTOP);
		m_pCbReportFile->SetFocus();
		return;
	}

	if (!m_sReportFile.empty())
	{
		if (paths_DoesPathExist(m_sReportFile.c_str()) == IS_EXISTING_FILE)
		{
			int overWrite = LanguageSelect.MsgBox(IDS_REPORT_FILEOVERWRITE,
					MB_YESNO | MB_ICONWARNING | MB_DONT_ASK_AGAIN);
			if (overWrite == IDNO)
				return;
		}
	}

	m_pCbReportFile->SaveState(_T("ReportFiles"));
	SettingStore.WriteProfileInt(_T("Settings"), _T("DirCmpReportStyle"), m_nReportType);
	EndDialog(IDOK);
}
