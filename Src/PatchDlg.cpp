/////////////////////////////////////////////////////////////////////////////
//    License (GPLv2+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
/////////////////////////////////////////////////////////////////////////////
/** 
 * @file  PatchDlg.cpp
 *
 * @brief Implementation of Patch creation dialog
 */
#include "StdAfx.h"
#include "Merge.h"
#include "SettingStore.h"
#include "LanguageSelect.h"
#include "PatchTool.h"
#include "PatchDlg.h"
#include "DIFF.H"
#include "coretools.h"
#include "paths.h"
#include "FileOrFolderSelect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CPatchDlg dialog

/** 
 * @brief Constructor, initializes members.
 */
CPatchDlg::CPatchDlg()
	: ODialog(IDD_GENERATE_PATCH)
{
}

template<ODialog::DDX_Operation op>
bool CPatchDlg::UpdateData()
{
	DDX_Check<op>(IDC_DIFF_IGNORECASE, m_ignoreCase);
	DDX_Check<op>(IDC_DIFF_IGNORE_BLANK_LINES, m_ignoreBlankLines);
	DDX_Check<op>(IDC_DIFF_APPLY_LINE_FILTERS, m_applyLineFilters);
	DDX_Check<op>(IDC_DIFF_WHITESPACE_COMPARE, m_whitespaceCompare, WHITESPACE_COMPARE_ALL);
	DDX_Check<op>(IDC_DIFF_WHITESPACE_IGNORE, m_whitespaceCompare, WHITESPACE_IGNORE_CHANGE);
	DDX_Check<op>(IDC_DIFF_WHITESPACE_IGNOREALL, m_whitespaceCompare, WHITESPACE_IGNORE_ALL);
	DDX_Check<op>(IDC_DIFF_APPENDFILE, m_appendFile);
	DDX_CBStringExact<op>(IDC_DIFF_FILE1, m_file1);
	DDX_CBStringExact<op>(IDC_DIFF_FILE2, m_file2);
	DDX_CBStringExact<op>(IDC_DIFF_FILERESULT, m_fileResult);
	DDX_Check<op>(IDC_DIFF_OPENTOEDITOR, m_openToEditor);
	DDX_Check<op>(IDC_DIFF_INCLCMDLINE, m_includeCmdLine);
	return true;
}

LRESULT CPatchDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		if (IsUserInputCommand(wParam))
			UpdateData<Get>();
		switch (wParam)
		{
		case IDOK:
			OnOK();
			break;
		case IDCANCEL:
			EndDialog(IDCANCEL);
			break;
		case MAKEWPARAM(IDC_DIFF_BROWSE_FILE1, BN_CLICKED):
			OnDiffBrowseFile1();
			break;
		case MAKEWPARAM(IDC_DIFF_BROWSE_FILE2, BN_CLICKED):
			OnDiffBrowseFile2();
			break;
		case MAKEWPARAM(IDC_DIFF_BROWSE_RESULT, BN_CLICKED):
			OnDiffBrowseResult();
			break;
		case MAKEWPARAM(IDC_DIFF_DEFAULTS, BN_CLICKED):
			OnDefaultSettings();
			break;
		case MAKEWPARAM(IDC_DIFF_SWAPFILES, BN_CLICKED):
			OnDiffSwapFiles();
			break;
		case MAKEWPARAM(IDC_DIFF_FILE1, CBN_SELCHANGE):
			OnSelchangeFile1Combo();
			break;
		case MAKEWPARAM(IDC_DIFF_FILE2, CBN_SELCHANGE):
			OnSelchangeFile2Combo();
			break;
		case MAKEWPARAM(IDC_DIFF_FILERESULT, CBN_SELCHANGE):
			OnSelchangeResultCombo();
			break;
		case MAKEWPARAM(IDC_DIFF_STYLE, CBN_SELCHANGE):
			OnSelchangeDiffStyle();
			break;
		case MAKEWPARAM(IDC_DIFF_FILE1, CBN_DROPDOWN):
		case MAKEWPARAM(IDC_DIFF_FILE2, CBN_DROPDOWN):
		case MAKEWPARAM(IDC_DIFF_CONTEXT, CBN_DROPDOWN):
		case MAKEWPARAM(IDC_DIFF_FILERESULT, CBN_DROPDOWN):
			reinterpret_cast<HSuperComboBox *>(lParam)->AdjustDroppedWidth();
			RegisterHotKey(m_hWnd, MAKEWPARAM(LOWORD(wParam), VK_DELETE), MOD_SHIFT, VK_DELETE);
			break;
		case MAKEWPARAM(IDC_DIFF_FILE1, CBN_CLOSEUP):
		case MAKEWPARAM(IDC_DIFF_FILE2, CBN_CLOSEUP):
		case MAKEWPARAM(IDC_DIFF_CONTEXT, CBN_CLOSEUP):
		case MAKEWPARAM(IDC_DIFF_FILERESULT, CBN_CLOSEUP):
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

/////////////////////////////////////////////////////////////////////////////
// CPatchDlg message handlers

/** 
 * @brief Called when dialog is closed with OK.
 * Check options and filenames given and close the dialog.
 */
void CPatchDlg::OnOK()
{
	// There are two different cases: single files or
	// multiple files.  Multiple files are selected from DirView.
	// Only if single files selected, filenames are checked here.
	// Filenames read from Dirview must be valid ones.
	size_t selectCount = m_fileList.size();
	if (selectCount == 1)
	{
		PATH_EXISTENCE file1Exist = paths_DoesPathExist(m_file1.c_str());
		PATH_EXISTENCE file2Exist = paths_DoesPathExist(m_file2.c_str());
		if ((file1Exist != IS_EXISTING_FILE) || (file2Exist != IS_EXISTING_FILE))
		{
			if (file1Exist != IS_EXISTING_FILE)
				LanguageSelect.MsgBox(IDS_DIFF_ITEM1NOTFOUND, MB_ICONSTOP);
			if (file2Exist != IS_EXISTING_FILE)
				LanguageSelect.MsgBox(IDS_DIFF_ITEM2NOTFOUND, MB_ICONSTOP);
			return;
		}
	}

	// Check that a result file was specified
	if (m_fileResult.empty())
	{
		LanguageSelect.MsgBox(IDS_MUST_SPECIFY_OUTPUT, MB_ICONSTOP);
		m_pCbResult->SetFocus();
		return;
	}
 
	// Check that result (patch) file is absolute path
	if (PathIsRelative(m_fileResult.c_str()))
	{
		LanguageSelect.MsgBox(IDS_PATH_NOT_ABSOLUTE, m_fileResult.c_str(), MB_ICONSTOP);
		m_pCbResult->SetFocus();
		return;
	}
	
	// Result file already exists and append not selected
	if (m_pCbResult->SendDlgItemMessage(1001, EM_GETMODIFY) && !m_appendFile &&
		paths_DoesPathExist(m_fileResult.c_str()) == IS_EXISTING_FILE)
	{
		if (IDYES != LanguageSelect.MsgBox(IDS_DIFF_FILEOVERWRITE,
			MB_YESNO | MB_ICONWARNING | MB_DONT_ASK_AGAIN))
		{
			return;
		}
	}
	// else it's OK to write new file

	m_outputStyle = (enum output_style) m_pCbStyle->GetCurSel();

	m_contextLines = GetDlgItemInt(IDC_DIFF_CONTEXT);

	SaveSettings();

	// Save combobox history
	m_pCbResult->SaveState(_T("Files\\DiffFileResult"));
	m_pCbContext->SaveState(_T("PatchCreator\\DiffContext"));
	// Don't save filenames if multiple file selected (as editbox reads
	// [X files selected])
	if (selectCount <= 1)
	{
		m_pCbFile1->SaveState(_T("Files\\DiffFile1"));
		m_pCbFile2->SaveState(_T("Files\\DiffFile2"));
	}
	EndDialog(selectCount > 0 ? IDOK : IDCANCEL);
}

/** 
 * @brief Initialise dialog data.
 *
 * There are two cases for filename editboxes:
 * - if one file was added to list then we show that filename
 * - if multiple files were added we show text [X files selected]
 */
BOOL CPatchDlg::OnInitDialog()
{
	ODialog::OnInitDialog();
	LanguageSelect.TranslateDialog(m_hWnd);

	m_pCbStyle = static_cast<HComboBox *>(GetDlgItem(IDC_DIFF_STYLE));
	m_pCbContext = static_cast<HSuperComboBox *>(GetDlgItem(IDC_DIFF_CONTEXT));
	m_pCbFile1 = static_cast<HSuperComboBox *>(GetDlgItem(IDC_DIFF_FILE1));
	m_pCbFile2 = static_cast<HSuperComboBox *>(GetDlgItem(IDC_DIFF_FILE2));
	m_pCbResult = static_cast<HSuperComboBox *>(GetDlgItem(IDC_DIFF_FILERESULT));

	// Load combobox history
	m_pCbFile1->LoadState(_T("Files\\DiffFile1"));
	m_pCbFile2->LoadState(_T("Files\\DiffFile2"));
	m_pCbContext->LoadState(_T("PatchCreator\\DiffContext"));
	m_pCbResult->LoadState(_T("Files\\DiffFileResult"));

	size_t count = m_fileList.size();

	// If one file added, show filenames on dialog
	if (count == 1)
	{
        const PATCHFILES& files = m_fileList.front();
		m_file1 = files.lfile;
		m_file2 = files.rfile;
	}
	else if (count > 1)	// Multiple files added, show number of files
	{
		String msg = LanguageSelect.FormatStrings(
			IDS_DIFF_SELECTEDFILES, NumToStr(count).c_str());
		m_file1 = msg;
		m_file2 = msg;
	}
	UpdateData<Set>();

	// Add patch styles to combobox
	m_pCbStyle->AddString(LanguageSelect.LoadString(IDS_DIFF_NORMAL).c_str());
	m_pCbStyle->AddString(LanguageSelect.LoadString(IDS_DIFF_CONTEXT).c_str());
	m_pCbStyle->AddString(LanguageSelect.LoadString(IDS_DIFF_UNIFIED).c_str());

	m_outputStyle = OUTPUT_NORMAL;
	m_pCbStyle->SetCurSel(0);
	if (HWindow *pWnd = m_pCbContext->GetDlgItem(1001))
	{
		pWnd->SetStyle(pWnd->GetStyle() | ES_NUMBER);
	}

	// Add default context line counts to combobox if its empty
	if (m_pCbContext->GetCount() == 0)
	{
		m_pCbContext->AddString(_T("0"));
		m_pCbContext->AddString(_T("1"));
		m_pCbContext->AddString(_T("3"));
		m_pCbContext->AddString(_T("5"));
		m_pCbContext->AddString(_T("7"));
	}
	
	LoadSettings();

	return TRUE;
}

/**
 * @brief Select the left file.
 */
void CPatchDlg::OnDiffBrowseFile1()
{
	String s = m_file1;
	if (SelectFile(m_hWnd, s, IDS_OPEN_TITLE, NULL, TRUE))
	{
		ChangeFile(s.c_str(), TRUE);
		m_pCbFile1->SetWindowText(s.c_str());
	}
}

/**
 * @brief Select the right file.
 */
void CPatchDlg::OnDiffBrowseFile2()
{
	String s = m_file2;
	if (SelectFile(m_hWnd, s, IDS_OPEN_TITLE, NULL, TRUE))
	{
		ChangeFile(s.c_str(), FALSE);
		m_pCbFile2->SetWindowText(s.c_str());
	}
}

/**
 * @brief Changes original file to patch.
 * This function sets new file for left/right file to create patch from.
 * @param [in] sFile New file for patch creation.
 * @param [in] bLeft If true left file is changed, otherwise right file.
 */
void CPatchDlg::ChangeFile(LPCTSTR sFile, BOOL bLeft)
{
	PATCHFILES pf;
	int count = GetItemCount();

	if (count == 1)
	{
		pf = GetItemAt(0);
	}
	else if (count > 1)
	{
		if (bLeft)
			m_file1.clear();
		else
			m_file2.clear();
	}
	ClearItems();

	// Change file
	if (bLeft)
	{
		pf.lfile = sFile;
		m_file1 = sFile;
	}
	else
	{
		pf.rfile = sFile;
		m_file2 = sFile;
	}
	AddItem(pf);
}

/**
 * @brief Select the patch file.
 */
void CPatchDlg::OnDiffBrowseResult()
{
	if (SelectFile(m_hWnd, m_fileResult, IDS_SAVE_AS_TITLE, NULL, FALSE))
	{
		m_pCbResult->SetWindowText(m_fileResult.c_str());
	}
}

/**
 * @brief Called when File1 combo selection is changed.
 */
void CPatchDlg::OnSelchangeFile1Combo()
{
	int sel = m_pCbFile1->GetCurSel();
	if (sel != CB_ERR)
	{
		String file;
		m_pCbFile1->GetLBText(sel, file);
		m_pCbFile1->SetWindowText(file.c_str());
		ChangeFile(file.c_str(), TRUE);
		m_file1 = file.c_str();
	}
}

/**
 * @brief Called when File2 combo selection is changed.
 */
void CPatchDlg::OnSelchangeFile2Combo()
{
	int sel = m_pCbFile2->GetCurSel();
	if (sel != CB_ERR)
	{
		String file;
		m_pCbFile2->GetLBText(sel, file);
		m_pCbFile2->SetWindowText(file.c_str());
		ChangeFile(file.c_str(), FALSE);
		m_file2 = file.c_str();
	}
}

/**
 * @brief Called when Result combo selection is changed.
 */
void CPatchDlg::OnSelchangeResultCombo()
{
	int sel = m_pCbResult->GetCurSel();
	if (sel != CB_ERR)
	{
		String sText;
		m_pCbResult->GetLBText(sel, sText);
		m_pCbResult->SetWindowText(sText.c_str());
	}
}

/** 
 * @brief Called when diff style dropdown selection is changed.
 * Called when diff style dropdown selection is changed.
 * If the new selection is context patch or unified patch format then
 * enable context lines selection control. Otherwise context lines selection
 * is disabled.
 */
void CPatchDlg::OnSelchangeDiffStyle()
{
	int selection = m_pCbStyle->GetCurSel();
	// Only context and unified formats allow context lines
	if ((selection == OUTPUT_CONTEXT) ||
			(selection == OUTPUT_UNIFIED))
	{
		m_pCbContext->EnableWindow(TRUE);
		// 3 lines is default context for Difftools too
		m_pCbContext->SetCurSel(2);
	}
	else
	{
		m_contextLines = 0;
		m_pCbContext->SetCurSel(0);
		m_pCbContext->EnableWindow(FALSE);
	}
}

/** 
 * @brief Swap filenames on file1 and file2.
 */
void CPatchDlg::OnDiffSwapFiles()
{
	m_pCbFile1->GetWindowText(m_file2);
	m_pCbFile2->GetWindowText(m_file1);
	m_pCbFile1->SetWindowText(m_file1.c_str());
	m_pCbFile2->SetWindowText(m_file2.c_str());
	std::vector<PATCHFILES>::iterator iter = m_fileList.begin();
	while (iter != m_fileList.end())
	{
		iter->swap_sides();
		++iter;
	}
}

/** 
 * @brief Add patch item to internal list.
 * @param [in] pf Patch item to add.
 */
void CPatchDlg::AddItem(const PATCHFILES& pf)
{
	m_fileList.push_back(pf);
}

/** 
 * @brief Returns amount of patch items in the internal list.
 * @return Count of patch items in the list.
 */
int CPatchDlg::GetItemCount()
{
	return m_fileList.size();
}

/** 
 * @brief Return item in the internal list at given position
 * @param [in] position Zero-based index of item to get
 * @return PATCHFILES from given position.
 */
const PATCHFILES& CPatchDlg::GetItemAt(int position)
{
	return m_fileList.at(position);
}

/** 
 * @brief Empties internal item list.
 */
void CPatchDlg::ClearItems()
{
	m_fileList.clear();
}

/** 
 * @brief Updates patch dialog settings from member variables.
 */
void CPatchDlg::UpdateSettings()
{
	UpdateData<Set>();
	m_pCbStyle->SetCurSel(m_outputStyle);
	m_pCbContext->SelectString(-1, NumToStr(m_contextLines).c_str());
	m_pCbContext->EnableWindow(
		m_outputStyle == OUTPUT_CONTEXT || m_outputStyle == OUTPUT_UNIFIED);
}

/** 
 * @brief Loads patch dialog settings from registry.
 */
void CPatchDlg::LoadSettings()
{
	if (CRegKeyEx key = SettingStore.GetSectionKey(_T("PatchCreator")))
	{
		int patchStyle = key.ReadDword(_T("PatchStyle"), 0);
		if (patchStyle < OUTPUT_NORMAL || patchStyle > OUTPUT_UNIFIED)
			patchStyle = OUTPUT_NORMAL;
		m_outputStyle = (enum output_style) patchStyle;
		
		m_contextLines = key.ReadDword(_T("ContextLines"), 0);
		if (m_contextLines < 0 || m_contextLines > 50)
			m_contextLines = 0;

		m_ignoreCase = key.ReadDword(_T("IgnoreCase"), FALSE);
		m_ignoreBlankLines = key.ReadDword(_T("IgnoreBlankLines"), FALSE);
		m_applyLineFilters = key.ReadDword(_T("ApplyLineFilters"), FALSE);
		
		m_whitespaceCompare = key.ReadDword(_T("Whitespace"), WHITESPACE_COMPARE_ALL);
		if (m_whitespaceCompare < WHITESPACE_COMPARE_ALL ||
			m_whitespaceCompare > WHITESPACE_IGNORE_ALL)
		{
			m_whitespaceCompare = WHITESPACE_COMPARE_ALL;
		}
		
		m_openToEditor = key.ReadDword(_T("OpenToEditor"), FALSE);
		m_includeCmdLine = key.ReadDword(_T("IncludeCmdLine"), FALSE);
	}
	UpdateSettings();
}

/** 
 * @brief Saves patch dialog settings to registry.
 */
void CPatchDlg::SaveSettings()
{
	if (CRegKeyEx key = SettingStore.GetSectionKey(_T("PatchCreator")))
	{
		key.WriteDword(_T("PatchStyle"), m_outputStyle);
		key.WriteDword(_T("ContextLines"), m_contextLines);
		key.WriteDword(_T("IgnoreCase"), m_ignoreCase);
		key.WriteDword(_T("IgnoreBlankLines"), m_ignoreBlankLines);
		key.WriteDword(_T("ApplyLineFilters"), m_applyLineFilters);
		key.WriteDword(_T("Whitespace"), m_whitespaceCompare);
		key.WriteDword(_T("OpenToEditor"), m_openToEditor);
		key.WriteDword(_T("IncludeCmdLine"), m_includeCmdLine);
	}
}

/** 
 * @brief Resets patch dialog settings to defaults.
 */
void CPatchDlg::OnDefaultSettings()
{
	m_outputStyle = OUTPUT_UNIFIED;
	m_contextLines = 5;
	m_ignoreCase = FALSE;
	m_ignoreBlankLines = FALSE;
	m_applyLineFilters = FALSE;
	m_whitespaceCompare = WHITESPACE_COMPARE_ALL;
	m_openToEditor = FALSE;
	m_includeCmdLine = FALSE;

	UpdateSettings();
}
