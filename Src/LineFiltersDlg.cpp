/**
 *  @file LineFiltersDlg.cpp
 *
 *  @brief Implementation of Line Filter dialog
 */ 
// ID line follows -- this is updated by SVN
// $Id$

#include "stdafx.h"
#include "merge.h"
#include "LanguageSelect.h"
#include "LineFiltersList.h"
#include "MainFrm.h"
#include "LineFiltersDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/** @brief Location for file compare specific help to open. */
static const TCHAR FilterHelpLocation[] = _T("::/htmlhelp/Filters.html");

/////////////////////////////////////////////////////////////////////////////
// LineFiltersDlg property page

/**
 * @brief Constructor.
 */
LineFiltersDlg::LineFiltersDlg(LineFiltersList *pList)
: ODialog(IDD_PROPPAGE_FILTER)
, m_pList(pList)
, m_filtersList(NULL)
{
	//{{AFX_DATA_INIT(LineFiltersDlg)
	m_bIgnoreRegExp = FALSE;
	//}}AFX_DATA_INIT
	m_strCaption = LanguageSelect.LoadDialogCaption(m_idd).c_str();
}

LRESULT LineFiltersDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case MAKEWPARAM(IDC_LFILTER_ADDBTN, BN_CLICKED):
			OnBnClickedLfilterAddBtn();
			break;
		case MAKEWPARAM(IDC_LFILTER_EDITBTN, BN_CLICKED):
			OnBnClickedLfilterEditbtn();
			break;
		case MAKEWPARAM(IDC_LFILTER_REMOVEBTN, BN_CLICKED):
			OnBnClickedLfilterRemovebtn();
			break;
		}
		break;
	case WM_HELP:
		GetMainFrame()->ShowHelp(FilterHelpLocation);
		return 0;
	case WM_NOTIFY:
		if (LRESULT lResult = OnNotify(reinterpret_cast<NMHDR *>(lParam)))
			return lResult;
		break;
	}
	return ODialog::WindowProc(message, wParam, lParam);
}

LRESULT LineFiltersDlg::OnNotify(NMHDR *pNMHDR)
{
	switch (pNMHDR->idFrom)
	{
	case IDC_LFILTER_LIST:
		switch (pNMHDR->code)
		{
		case LVN_ITEMACTIVATE:
			EditSelectedFilter();
			break;
		case LVN_KEYDOWN:
			if (LOWORD(reinterpret_cast<LPNMKEY>(pNMHDR)->nVKey) == VK_F2)
				EditSelectedFilter();
			break;
		case LVN_ENDLABELEDIT:
			return 1;
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

/**
 * @brief Initialize the dialog.
 */
BOOL LineFiltersDlg::OnInitDialog()
{
	ODialog::OnInitDialog();
	LanguageSelect.TranslateDialog(m_hWnd);
	m_filtersList = static_cast<HListView *>(GetDlgItem(IDC_LFILTER_LIST));
	InitList();
	CheckDlgButton(IDC_IGNOREREGEXP, m_bIgnoreRegExp);
	return TRUE;
}

/**
 * @brief Initialize the filter list in the dialog.
 * This function adds current line filters to the filter list.
 */
void LineFiltersDlg::InitList()
{
	// Show selection across entire row.
	// Also enable infotips.
	m_filtersList->SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

	String title = LanguageSelect.LoadString(IDS_FILTERLINE_REGEXP);
	m_filtersList->InsertColumn(1, title.c_str(), LVCFMT_LEFT, 500);

	int count = m_pList->GetCount();
	for (int i = 0; i < count; i++)
	{
		const LineFilterItem &item = m_pList->GetAt(i);
		AddRow(item.filterStr.c_str(), item.enabled);
	}
	m_filtersList->SetItemState(0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
}

/**
 * @brief Add new row to the list control.
 * @param [in] Filter string to add.
 * @param [in] enabled Is filter enabled?
 * @return Index of added row.
 */
int LineFiltersDlg::AddRow(LPCTSTR filter /*= NULL*/, BOOL enabled /*=FALSE*/)
{
	int items = m_filtersList->GetItemCount();
	int ind = m_filtersList->InsertItem(items, filter);
	m_filtersList->SetCheck(ind, enabled);
	return ind;
}

/**
 * @brief Edit currently selected filter.
 */
void LineFiltersDlg::EditSelectedFilter()
{
	m_filtersList->SetFocus();
	int sel = m_filtersList->GetNextItem(-1, LVNI_SELECTED);
	if (sel > -1)
	{
		m_filtersList->EditLabel(sel);
	}
}

/**
 * @brief Called when Add-button is clicked.
 */
void LineFiltersDlg::OnBnClickedLfilterAddBtn()
{
	int ind = AddRow(_T(""));
	if (ind >= -1)
	{
		m_filtersList->SetItemState(ind, LVIS_SELECTED, LVIS_SELECTED);
		m_filtersList->EnsureVisible(ind, FALSE);
		EditSelectedFilter();
	}
}

/**
 * @brief Called when Edit button is clicked.
 */
void LineFiltersDlg::OnBnClickedLfilterEditbtn()
{
	EditSelectedFilter();
}

/**
 * @brief Save filters to list when exiting the dialog.
 */
void LineFiltersDlg::OnApply()
{
	m_pList->Empty();
	for (int i = 0; i < m_filtersList->GetItemCount(); i++)
	{
		String text = m_filtersList->GetItemText(i, 0);
		BOOL enabled = m_filtersList->GetCheck(i);
		m_pList->AddFilter(text.c_str(), enabled != FALSE);
	}
	m_bIgnoreRegExp = IsDlgButtonChecked(IDC_IGNOREREGEXP);
}

/**
 * @brief Called when Remove button is clicked.
 */
void LineFiltersDlg::OnBnClickedLfilterRemovebtn()
{
	int sel = m_filtersList->GetNextItem(-1, LVNI_SELECTED);
	if (sel != -1)
	{
		m_filtersList->DeleteItem(sel);
	}

	int newSel = min(m_filtersList->GetItemCount() - 1, sel);
	if (newSel >= -1)
	{
		m_filtersList->SetItemState(newSel, LVIS_SELECTED, LVIS_SELECTED);
		BOOL bPartialOk = FALSE;
		m_filtersList->EnsureVisible(newSel, bPartialOk);
	}
}
