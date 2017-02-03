/** 
 * @file PreferencesDlg.cpp
 *
 * @brief Implementation file for CPreferencesDlg
 *
 * @note This code originates from AbstractSpoon / TodoList
 * (http://www.abstractspoon.com/) but is modified to use in
 * WinMerge.
 */
#include "StdAfx.h"
#include "resource.h"
#include "SyntaxColors.h"
#include "PreferencesDlg.h"
#include "Merge.h"
#include "MainFrm.h"
#include "SettingStore.h"
#include "LanguageSelect.h"
#include "FileOrFolderSelect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/**
 * @brief Location for file compare specific help to open.
 */
static const TCHAR OptionsHelpLocation[] = _T("::/htmlhelp/Configuration.html");

/////////////////////////////////////////////////////////////////////////////
// CPreferencesDlg dialog

String CPreferencesDlg::m_pathMRU;

CPreferencesDlg::CPreferencesDlg()
: ODialog(IDD_PREFERENCES)
, m_nPageCount(0)
, m_nPageIndex(0)
, m_tcPages(NULL)
{
}

CPreferencesDlg::~CPreferencesDlg()
{
}

LRESULT CPreferencesDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
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
		case MAKEWPARAM(IDC_TREEOPT_HELP, BN_CLICKED):
			OnHelpButton();
			break;
		case MAKEWPARAM(IDC_TREEOPT_IMPORT, BN_CLICKED):
			OnImportButton();
			break;
		case MAKEWPARAM(IDC_TREEOPT_EXPORT, BN_CLICKED):
			OnExportButton();
			break;
		}
		break;
	case WM_HELP:
		OnHelpButton();
		return 0;
	case WM_NOTIFY:
		if (LRESULT lResult = OnNotify(reinterpret_cast<NMHDR *>(lParam)))
			return lResult;
		break;
	case WM_DESTROY:
		OnDestroy();
		break;
	}
	return ODialog::WindowProc(message, wParam, lParam);
}

LRESULT CPreferencesDlg::OnNotify(NMHDR *pNMHDR)
{
	switch (pNMHDR->idFrom)
	{
	case IDC_TREEOPT_PAGES:
		switch (pNMHDR->code)
		{
		case TVN_SELCHANGING:
			OnSelchangingPages(reinterpret_cast<NMTREEVIEW *>(pNMHDR));
			break;
		case TVN_SELCHANGED:
			OnSelchangedPages(reinterpret_cast<NMTREEVIEW *>(pNMHDR));
			break;
		}
		break;
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CPreferencesDlg message handlers

BOOL CPreferencesDlg::OnInitDialog() 
{
	ODialog::OnInitDialog();
	LanguageSelect.TranslateDialog(m_hWnd);

	m_tcPages = static_cast<HTreeView *>(GetDlgItem(IDC_TREEOPT_PAGES));
	m_tcPages->SetIndent(0);

	m_nPageCount = 0;
	m_nPageIndex = SettingStore.GetProfileInt(_T("Settings"), _T("OptStartPage"), 0);
	// '>' is used as path separator.
	// For example "General" creates top-level "General" page
	// and "General>Colors" creates "Colors" sub-page for "General"
	AddPage(&m_pageGeneral);
	if (HTREEITEM htiBranch = AddBranch(IDS_OPTIONSPG_COMPARE))
	{
		AddPage(&m_pageCompare, htiBranch);
		AddPage(&m_pageCompareFolder, htiBranch);
	}
	if (HTREEITEM htiBranch = AddBranch(IDS_OPTIONSPG_EDITOR))
	{
		AddPage(&m_pageEditor, htiBranch);
		AddPage(&m_pageHexEditor, htiBranch);
	}
	if (HTREEITEM htiBranch = AddBranch(IDS_OPTIONSPG_COLORS))
	{
		AddPage(&m_pageMergeColors, htiBranch);
		AddPage(&m_pageSyntaxColors, htiBranch);
		AddPage(&m_pageTextColors, htiBranch);
		AddPage(&m_pageListColors, htiBranch);
	}
	AddPage(&m_pageArchive);
	AddPage(&m_pageSystem);
	AddPage(&m_pageBackups);
	AddPage(&m_pageVss);
	AddPage(&m_pageCodepage);
	AddPage(&m_pageShell);

	ReadOptions();
	
	SetFocus();

	return TRUE;
}

void CPreferencesDlg::OnOK()
{
	EndDialog(IDOK);
	SaveOptions();
}

void CPreferencesDlg::OnDestroy()
{
	SettingStore.WriteProfileInt(_T("Settings"), _T("OptStartPage"), m_nPageIndex);
}

void CPreferencesDlg::OnHelpButton() 
{
	theApp.m_pMainWnd->ShowHelp(OptionsHelpLocation);
}

HTREEITEM CPreferencesDlg::AddBranch(UINT nTopHeading, HTREEITEM htiParent)
{
	String sItem = LanguageSelect.LoadString(nTopHeading);
	TVINSERTSTRUCT tvis;
	tvis.hParent = htiParent;
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.mask = TVIF_TEXT | TVIF_STATE;
	tvis.item.state = tvis.item.stateMask = TVIS_EXPANDED;
	tvis.item.pszText = const_cast<LPTSTR>(sItem.c_str());
	return m_tcPages->InsertItem(&tvis);
}

HTREEITEM CPreferencesDlg::AddPage(OptionsPanel* pPage, HTREEITEM htiParent)
{
	String sPath = LanguageSelect.LoadDialogCaption(pPage->m_idd);
	TVINSERTSTRUCT tvis;
	tvis.hParent = htiParent;
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
	tvis.item.pszText = const_cast<LPTSTR>(sPath.c_str());
	tvis.item.lParam = reinterpret_cast<LPARAM>(pPage);
	HTREEITEM hti = m_tcPages->InsertItem(&tvis);
	pPage->m_nPageIndex = m_nPageCount++;
	if (pPage->m_nPageIndex == m_nPageIndex)
		m_tcPages->SelectItem(hti);
	return hti;
}

void CPreferencesDlg::OnSelchangingPages(NMTREEVIEW *pNM)
{
	if (HTREEITEM htiSel = GetSelector(pNM->itemOld.hItem, pNM->itemNew.hItem))
	{
		OptionsPanel *pPage =
			reinterpret_cast<OptionsPanel *>(m_tcPages->GetItemData(htiSel));
		ASSERT(pPage);
		pPage->ShowWindow(SW_HIDE);
	}	
}

void CPreferencesDlg::OnSelchangedPages(NMTREEVIEW *pNM)
{
	if (HTREEITEM htiSel = GetSelector(pNM->itemNew.hItem, pNM->itemOld.hItem))
	{
		OptionsPanel *pPage =
			reinterpret_cast<OptionsPanel *>(m_tcPages->GetItemData(htiSel));
		ASSERT(pPage);
		// create the page if not yet done
		if (!pPage->m_hWnd && !LanguageSelect.Create(*pPage, m_hWnd))
			return;
		pPage->ShowWindow(SW_SHOW);
		m_nPageIndex = pPage->m_nPageIndex;
		// update caption
		String sPath = GetItemPath(htiSel);
		SetWindowText(LanguageSelect.FormatStrings(IDS_OPTIONS_TITLE, sPath.c_str()));
		m_tcPages->SetFocus();
	}
}

HTREEITEM CPreferencesDlg::GetSelector(HTREEITEM htiSel, HTREEITEM htiUnSel)
{
	if (htiSel != NULL)
	{
		if (HTREEITEM htiChild = m_tcPages->GetChildItem(htiSel))
		{
			htiSel = htiChild != htiUnSel ? htiChild : NULL;
		}
		else if (htiUnSel != NULL && m_tcPages->GetChildItem(htiUnSel) == htiSel)
		{
			htiSel = NULL;
		}
	}
	return htiSel;
}

String CPreferencesDlg::GetItemPath(HTREEITEM hti)
{
	String sPath = m_tcPages->GetItemText(hti);
	while (hti = m_tcPages->GetParentItem(hti))
		sPath = m_tcPages->GetItemText(hti) + _T(" > ") + sPath;
	return sPath;
}

/**
 * @brief Read options from storage to UI controls.
 * @param [in] bUpdate If TRUE UpdateData() is called
 */
void CPreferencesDlg::ReadOptions()
{
	ReadOptions(&m_pageGeneral);
	ReadOptions(&m_pageMergeColors);
	ReadOptions(&m_pageTextColors);
	ReadOptions(&m_pageListColors);
	ReadOptions(&m_pageSyntaxColors);
	ReadOptions(&m_pageSystem);
	ReadOptions(&m_pageCompare);
	ReadOptions(&m_pageCompareFolder);
	ReadOptions(&m_pageEditor);
	ReadOptions(&m_pageHexEditor);
	ReadOptions(&m_pageCodepage);
	ReadOptions(&m_pageVss);
	ReadOptions(&m_pageArchive);
	ReadOptions(&m_pageBackups);
	ReadOptions(&m_pageShell);
}

/**
 * @brief Write options from UI to storage.
 */
void CPreferencesDlg::SaveOptions()
{
	// TODO: Could GetAppRegistryKey() in advance
	m_pageGeneral.WriteOptions();
	m_pageSystem.WriteOptions();
	m_pageCompare.WriteOptions();
	m_pageCompareFolder.WriteOptions();
	m_pageEditor.WriteOptions();
	m_pageHexEditor.WriteOptions();
	m_pageMergeColors.WriteOptions();
	m_pageTextColors.WriteOptions();
	m_pageListColors.WriteOptions();
	m_pageSyntaxColors.WriteOptions();
	SyntaxColors.SaveToRegistry();
	m_pageCodepage.WriteOptions();
	m_pageVss.WriteOptions();	
	m_pageArchive.WriteOptions();
	m_pageBackups.WriteOptions();
	m_pageShell.WriteOptions();
	// Persist the fonts
	OPT_FONT_FILECMP_LOGFONT.SaveOption();
	OPT_FONT_DIRCMP_LOGFONT.SaveOption();
}

/**
 * @brief Imports options from file.
 */
void CPreferencesDlg::OnImportButton()
{
	if (SelectFile(m_hWnd, m_pathMRU, IDS_OPT_IMPORT_CAPTION, IDS_INIFILES, TRUE))
	{
		if (IOptionDef::ImportOptions(m_pathMRU.c_str()) == ERROR_SUCCESS &&
			SyntaxColors.ImportOptions(m_pathMRU.c_str()) == ERROR_SUCCESS)
		{
			ReadOptions();
			LanguageSelect.MsgBox(IDS_OPT_IMPORT_DONE, MB_ICONINFORMATION);
		}
		else
		{
			LanguageSelect.MsgBox(IDS_OPT_IMPORT_ERR, MB_ICONWARNING);
		}
	}
}

/**
 * @brief Exports options to file.
 */
void CPreferencesDlg::OnExportButton()
{
	if (SelectFile(m_hWnd, m_pathMRU, IDS_OPT_EXPORT_CAPTION, IDS_INIFILES, FALSE, _T("ini")))
	{
		SaveOptions();
		if (IOptionDef::ExportOptions(m_pathMRU.c_str()) == ERROR_SUCCESS &&
			SyntaxColors.ExportOptions(m_pathMRU.c_str()) == ERROR_SUCCESS)
		{
			LanguageSelect.MsgBox(IDS_OPT_EXPORT_DONE, MB_ICONINFORMATION);
		}
		else
		{
			LanguageSelect.MsgBox(IDS_OPT_EXPORT_ERR, MB_ICONWARNING);
		}
	}
}

/**
 * @brief Do a safe UpdateData call for propertypage.
 * This function does safe UpdateData call for given propertypage. As it is,
 * all propertypages may not have been yet initialized properly, so we must
 * have some care when calling updateData for them.
 * @param [in] pPage Propertypage to update.
 * @param bSaveAndValidate UpdateData direction parameter.
 */
void CPreferencesDlg::ReadOptions(OptionsPanel *pPage)
{
	pPage->ReadOptions();
	if (pPage->m_hWnd)
		pPage->UpdateScreen();
}
