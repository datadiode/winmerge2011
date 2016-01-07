/////////////////////////////////////////////////////////////////////////////
//    License (GPLv2+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or (at
//    your option) any later version.
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
 * @file  FileFiltersDlg.cpp
 *
 * @brief Implementation of FileFilters -dialog
 */
#include "StdAfx.h"
#include "Merge.h"
#include "LanguageSelect.h"
#include "MainFrm.h"
#include "FileFiltersDlg.h"
#include "coretools.h"
#include "paths.h"
#include "SharedFilterDlg.h"
#include "TestFilterDlg.h"
#include "FileOrFolderSelect.h"

using std::vector;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/** @brief Template file used when creating new filefilter. */
static const TCHAR FILE_FILTER_TEMPLATE[] = _T("FileFilter.tmpl");

/** @brief Location for filters specific help to open. */
static const TCHAR FilterHelpLocation[] = _T("::/htmlhelp/Filters.html");

/////////////////////////////////////////////////////////////////////////////
// FileFiltersDlg dialog

/**
 * @brief Constructor.
 */
FileFiltersDlg::FileFiltersDlg()
: ODialog(IDD_FILEFILTERS)
, m_listFilters(NULL)
, m_Filters(globalFileFilter.GetFileFilters())
{
	m_strCaption = LanguageSelect.LoadDialogCaption(m_idd);
}

LRESULT FileFiltersDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case MAKEWPARAM(IDC_FILTERFILE_EDITBTN, BN_CLICKED):
			OnFiltersEditbtn();
			break;
		case MAKEWPARAM(IDC_FILTERFILE_TEST_BTN, BN_CLICKED):
			OnBnClickedFilterfileTestButton();
			break;
		case MAKEWPARAM(IDC_FILTERFILE_NEWBTN, BN_CLICKED):
			OnBnClickedFilterfileNewbutton();
			break;
		case MAKEWPARAM(IDC_FILTERFILE_DELETEBTN, BN_CLICKED):
			OnBnClickedFilterfileDelete();
			break;
		case MAKEWPARAM(IDC_FILTERFILE_INSTALL, BN_CLICKED):
			OnBnClickedFilterfileInstall();
			break;
		}
		break;
	case WM_HELP:
		theApp.m_pMainWnd->ShowHelp(FilterHelpLocation);
		return 0;
	case WM_NOTIFY:
		if (LRESULT lResult = OnNotify(reinterpret_cast<NMHDR *>(lParam)))
			return lResult;
		break;
	}
	return ODialog::WindowProc(message, wParam, lParam);
}

LRESULT FileFiltersDlg::OnNotify(NMHDR *pNMHDR)
{
	switch (pNMHDR->idFrom)
	{
	case IDC_FILTERFILE_LIST:
		switch (pNMHDR->code)
		{
		case LVN_ITEMCHANGED:
			OnLvnItemchangedFilterfileList(pNMHDR);
			break;
		case NM_DBLCLK:
			OnFiltersEditbtn();
			break;
		}
		break;
	default:
		switch (pNMHDR->code)
		{
	    case PSN_APPLY:
			OnApply();
			break;
		}
		break;
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CFiltersDlg message handlers

/**
 * @brief Initialise listcontrol containing filters.
 */
void FileFiltersDlg::InitList()
{
	// Show selection across entire row.
	// Also enable infotips.
	m_listFilters->SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

	String title = LanguageSelect.LoadString(IDS_FILTERFILE_NAMETITLE);
	m_listFilters->InsertColumn(0, title.c_str(), LVCFMT_LEFT, 150);
	title = LanguageSelect.LoadString(IDS_FILTERFILE_DESCTITLE);
	m_listFilters->InsertColumn(1, title.c_str(), LVCFMT_LEFT, 350);
	title = LanguageSelect.LoadString(IDS_FILTERFILE_PATHTITLE);
	m_listFilters->InsertColumn(2, title.c_str(), LVCFMT_LEFT, 350);

	title = LanguageSelect.LoadString(IDS_USERCHOICE_NONE);
	m_listFilters->InsertItem(1, title.c_str());
	m_listFilters->SetItemText(0, 1, title.c_str());
	m_listFilters->SetItemText(0, 2, title.c_str());

	const int count = m_Filters.size();

	for (int i = 0; i < count; i++)
	{
		AddToGrid(i);
	}
}

/**
 * @brief Select filter by index in the listview.
 * @param [in] index Index of filter to select.
 */
void FileFiltersDlg::SelectFilterByIndex(int index)
{
	m_listFilters->SetItemState(index, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	m_listFilters->EnsureVisible(index, FALSE);
}

/**
 * @brief Called before dialog is shown.
 * @return Always TRUE.
 */
BOOL FileFiltersDlg::OnInitDialog()
{
	ODialog::OnInitDialog();
	LanguageSelect.TranslateDialog(m_hWnd);
	m_listFilters = static_cast<HListView *>(GetDlgItem(IDC_FILTERFILE_LIST));
	InitList();
	if (m_sFileFilterPath.empty())
		SelectFilterByIndex(0);

	return TRUE;
}

/**
 * @brief Add filter from filter-list index to dialog.
 * @param [in] filterIndex Index of filter to add.
 */
void FileFiltersDlg::AddToGrid(int filterIndex)
{
	const FileFilter *filter = m_Filters.at(filterIndex);
	const int item = filterIndex + 1;
	m_listFilters->InsertItem(item, filter->name.c_str());
	m_listFilters->SetItemText(item, 1, filter->description.c_str());
	m_listFilters->SetItemText(item, 2, filter->fullpath.c_str());
	if (m_sFileFilterPath == filter->fullpath)
		SelectFilterByIndex(item);
}

/**
 * @brief Called when dialog is closed with "OK" button.
 */
void FileFiltersDlg::OnApply()
{
	int sel = m_listFilters->GetNextItem(-1, LVNI_SELECTED);
	m_sFileFilterPath = m_listFilters->GetItemText(sel, 2);
}

/**
 * @brief Open selected filter for editing.
 *
 * This opens selected file filter file for user to edit. Other WinMerge UI is
 * not (anymore) blocked during editing. We let user continue working with
 * WinMerge while editing filter(s). Before opening this dialog and before
 * doing directory compare we re-load changed filter files from disk. So we
 * always compare with latest saved filters.
 * @sa CMainFrame::OnToolsFilters()
 * @sa CDirFrame::Rescan()
 */
void FileFiltersDlg::OnFiltersEditbtn()
{
	int sel = m_listFilters->GetNextItem(-1, LVNI_SELECTED);
	// Can't edit first "None"
	if (sel > 0)
	{
		String path = m_listFilters->GetItemText(sel, 2);
		EditFileFilter(path.c_str());
	}
}

/**
 * @brief Edit file filter in external editor.
 * @param [in] path Full path to file filter to edit.
 */
void FileFiltersDlg::EditFileFilter(LPCTSTR path)
{
	CMainFrame::OpenFileToExternalEditor(path);
}

/**
 * @brief Is item in list the <None> item?
 * @param [in] item Item to test.
 * @return true if item is <None> item.
 */
bool FileFiltersDlg::IsFilterItemNone(int item) const
{
	String txtNone = LanguageSelect.LoadString(IDS_USERCHOICE_NONE);
	String txt = m_listFilters->GetItemText(item, 0);
	return lstrcmpi(txt.c_str(), txtNone.c_str()) == 0;
}

/**
 * @brief Called when item state is changed.
 *
 * Disable Edit-button when "None" filter is selected.
 * @param [in] pNMHDR Listview item data.
 * @param [out] pResult Result of the action is returned in here.
 */
void FileFiltersDlg::OnLvnItemchangedFilterfileList(NMHDR *pNMHDR)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// If item got selected
	if (pNMLV->uNewState & LVIS_SELECTED)
	{
		BOOL enable = !IsFilterItemNone(pNMLV->iItem);
		GetDlgItem(IDC_FILTERFILE_TEST_BTN)->EnableWindow(enable);
		GetDlgItem(IDC_FILTERFILE_EDITBTN)->EnableWindow(enable);
		GetDlgItem(IDC_FILTERFILE_DELETEBTN)->EnableWindow(enable);
	}
}

/**
 * @brief Called when user presses "Test" button.
 *
 * Asks filename for new filter from user (using standard
 * file picker dialog) and copies template file to that
 * name. Opens new filterfile for editing.
 * @todo (At least) Warn if user puts filter to outside
 * filter directories?
 */
void FileFiltersDlg::OnBnClickedFilterfileTestButton()
{
	int sel = m_listFilters->GetNextItem(-1, LVNI_SELECTED);
	if (sel == -1)
		return;
	if (IsFilterItemNone(sel))
		return;
	
	m_sFileFilterPath = m_listFilters->GetItemText(sel, 2);

	if (FileFilter *filter = globalFileFilter.GetFilterByPath(m_sFileFilterPath.c_str()))
	{
		// Ensure filter is up-to-date (user probably just edited it)
		filter->Load();
		CTestFilterDlg dlg(filter);
		LanguageSelect.DoModal(dlg);
	}
}

/**
 * @brief Called when user presses "New..." button.
 *
 * Asks filename for new filter from user (using standard
 * file picker dialog) and copies template file to that
 * name. Opens new filterfile for editing.
 * @todo (At least) Warn if user puts filter to outside
 * filter directories?
 * @todo Can global filter path be empty (I think not - Kimmo).
 */
void FileFiltersDlg::OnBnClickedFilterfileNewbutton()
{
	String path = globalFileFilter.GetGlobalFilterPath();
	String userPath = globalFileFilter.GetUserFilterPath();

	// Format path to template file
	String templatePath = paths_ConcatPath(path, FILE_FILTER_TEMPLATE);
	if (paths_DoesPathExist(templatePath.c_str()) != IS_EXISTING_FILE)
	{
		LanguageSelect.FormatStrings(
			IDS_FILEFILTER_TMPL_MISSING,
			FILE_FILTER_TEMPLATE, templatePath.c_str()
		).MsgBox(MB_ICONERROR);
		return;
	}

	if (!userPath.empty())
	{
		CSharedFilterDlg dlg;
		dlg.m_SharedFolder = path;
		dlg.m_PrivateFolder = userPath;
		if (LanguageSelect.DoModal(dlg) != IDOK)
			return;
		path = dlg.m_ChosenFolder;
	}

	if (!paths_EndsWithSlash(path.c_str()))
		path.push_back(_T('\\'));
	
	if (SelectFile(m_hWnd, path, IDS_FILEFILTER_SAVENEW,
		IDS_FILEFILTER_FILEMASK, FALSE, FileFilterExt + 2))
	{
		// Open-dialog asks about overwriting, so we can overwrite filter file
		// user has already allowed it.
		if (!CopyFile(templatePath.c_str(), path.c_str(), FALSE))
		{
			LanguageSelect.MsgBox(IDS_FILEFILTER_TMPL_COPY, templatePath.c_str(), MB_ICONERROR);
			return;
		}
		EditFileFilter(path.c_str());
		// Remove all from filterslist and re-add so we can update UI
		globalFileFilter.LoadAllFileFilters();
		UpdateFiltersList();
	}
}

/**
 * @brief Delete selected filter.
 */
void FileFiltersDlg::OnBnClickedFilterfileDelete()
{
	int sel = m_listFilters->GetNextItem(-1, LVNI_SELECTED);
	// Can't delete first "None"
	if (sel > 0)
	{
		String path = m_listFilters->GetItemText(sel, 2);
		int response = LanguageSelect.FormatStrings(
			IDS_CONFIRM_DELETE_SINGLE, path.c_str()
		).MsgBox(MB_ICONWARNING | MB_YESNO);
		if (response == IDYES)
		{
			if (DeleteFile(path.c_str()))
			{
				// Remove all from filterslist and re-add so we can update UI
				globalFileFilter.LoadAllFileFilters();
				UpdateFiltersList();
			}
			else
			{
				LanguageSelect.MsgBox(IDS_FILEFILTER_DELETE_FAIL, path.c_str(), MB_ICONSTOP);
			}
		}
	}
}

/**
 * @brief Update filters to list.
 */
void FileFiltersDlg::UpdateFiltersList()
{
	int count = m_Filters.size();

	m_listFilters->DeleteAllItems();

	String title = LanguageSelect.LoadString(IDS_USERCHOICE_NONE);
	m_listFilters->InsertItem(0, title.c_str());
	m_listFilters->SetItemText(0, 1, title.c_str());
	m_listFilters->SetItemText(0, 2, title.c_str());

	for (int i = 0; i < count; i++)
	{
		AddToGrid(i);
	}
}

/**
 * @brief Install new filter.
 * This function is called when user selects "Install" button from GUI.
 * Function allows easy installation of new filters for user. For example
 * when user has downloaded filter file from net. First we ask user to
 * select filter to install. Then we copy selected filter to private
 * filters folder.
 */
void FileFiltersDlg::OnBnClickedFilterfileInstall()
{
	String path;
	String userPath = globalFileFilter.GetUserFilterPath();

	if (SelectFile(m_hWnd, path, IDS_FILEFILTER_INSTALL, IDS_FILEFILTER_FILEMASK, TRUE))
	{
		LPCTSTR filename = PathFindFileName(path.c_str());
		userPath = paths_ConcatPath(userPath, filename);
		if (!CopyFile(path.c_str(), userPath.c_str(), TRUE))
		{
			// If file already exists, ask from user
			// If user wants to, overwrite existing filter
			if (paths_DoesPathExist(userPath.c_str()) == IS_EXISTING_FILE)
			{
				int res = LanguageSelect.MsgBox(IDS_FILEFILTER_OVERWRITE, MB_YESNO |
					MB_ICONWARNING);
				if (res == IDYES)
				{
					if (!CopyFile(path.c_str(), userPath.c_str(), FALSE))
					{
						LanguageSelect.MsgBox(IDS_FILEFILTER_INSTALLFAIL, MB_ICONSTOP);
					}
				}
			}
			else
			{
				LanguageSelect.MsgBox(IDS_FILEFILTER_INSTALLFAIL, MB_ICONSTOP);
			}
		}
		// Remove all from filterslist and re-add so we can update UI
		globalFileFilter.LoadAllFileFilters();
		UpdateFiltersList();
	}
}
