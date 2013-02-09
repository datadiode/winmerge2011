/** 
 * @file  DirColsDlg.cpp
 *
 * @brief Implementation file for CDirColsDlg
 *
 * @date  Created: 2003-08-19
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "merge.h"
#include "LanguageSelect.h"
#include "DirColsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDirColsDlg dialog

/**
 * @brief Default dialog constructor.
 * @param [in] pParent Dialog's parent component (window).
 */
CDirColsDlg::CDirColsDlg()
	: ODialog(IDD_DIRCOLS)
	, m_bReset(false)
	, m_listColumns(NULL)
{
}

LRESULT CDirColsDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
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
		case MAKEWPARAM(IDC_UP, BN_CLICKED):
			OnUp();
			break;
		case MAKEWPARAM(IDC_DOWN, BN_CLICKED):
			OnDown();
			break;
		case MAKEWPARAM(IDC_COLDLG_DEFAULTS, BN_CLICKED):
			OnDefaults();
			break;
		}
		break;
	case WM_NOTIFY:
		if (LRESULT lResult = OnNotify(reinterpret_cast<NMHDR *>(lParam)))
			return lResult;
		break;
	}
	return ODialog::WindowProc(message, wParam, lParam);
}

LRESULT CDirColsDlg::OnNotify(NMHDR *pNMHDR)
{
	switch (pNMHDR->idFrom)
	{
	case IDC_COLDLG_LIST:
		switch (pNMHDR->code)
		{
		case LVN_ITEMCHANGED:
			OnLvnItemchangedColdlgList(reinterpret_cast<NMLISTVIEW *>(pNMHDR));
			break;
		}
		break;
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CDirColsDlg message handlers

/**
 * @brief Dialog initialisation. Load column lists.
 */
BOOL CDirColsDlg::OnInitDialog() 
{
	ODialog::OnInitDialog();
	LanguageSelect.TranslateDialog(m_hWnd);
	m_listColumns = static_cast<HListView *>(GetDlgItem(IDC_COLDLG_LIST));
	// Show selection across entire row, plus enable infotips
	m_listColumns->SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
	m_listColumns->InsertColumn(0, _T(""));
	LoadLists();
	m_listColumns->SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
	// Sort m_cols so that it is in logical column order
	stl::sort(m_cols.begin(), m_cols.end(), &CompareColumnsByLogicalOrder);
	return TRUE;
}

/**
 * @brief Load listboxes on screen from column array.
 */
void CDirColsDlg::LoadLists()
{
	int i = 0;
	ColumnArray::iterator iter = m_cols.begin();
	while (iter != m_cols.end())
	{
		const column &c = *iter++;
		int x = m_listColumns->InsertItem(i++, LanguageSelect.LoadString(c.idName).c_str());
		m_listColumns->SetItemData(x, c.log);
		if ((m_bReset ? c.def_phy : c.phy) >= 0)
			m_listColumns->SetCheck(x, TRUE);
	}
	// Set first item to selected state
	SelectItem(0);
}

/**
 * @brief Select item in list.
 * @param [in] index Index of item to select.
 */
void CDirColsDlg::SelectItem(int index)
{
	m_listColumns->SetItemState(index, LVIS_SELECTED, LVIS_SELECTED);
}

/**
 * @brief Compare column order of two columns.
 * @param [in] el1 First column to compare.
 * @param [in] el2 Second column to compare.
 * @return Column order.
 */
bool CDirColsDlg::CompareColumnsByLogicalOrder( const column & el1, const column & el2 )
{
	return el1.log < el2.log;
}

/**
 * @brief Move column name in the list.
 * @param [in] index Index of column to move.
 * @param [in] newIndex New index for the column to move.
 */
void CDirColsDlg::MoveItem(int index, int newIndex)
{
	// Get current column data
	String text = m_listColumns->GetItemText(index, 0);
	BOOL checked = m_listColumns->GetCheck(index);
	UINT state = m_listColumns->GetItemState(index, LVIS_SELECTED);
	DWORD_PTR data = m_listColumns->GetItemData(index);

	// Delete column
	m_listColumns->DeleteItem(index);
	
	// Insert column in new index
	m_listColumns->InsertItem(newIndex, text.c_str());
	m_listColumns->SetItemState(newIndex, state, LVIS_SELECTED);
	m_listColumns->SetItemData(newIndex, data);
	if (checked)
		m_listColumns->SetCheck(newIndex);
}

/**
 * @brief Move selected items up in list.
 */
void CDirColsDlg::OnUp()
{
	int lower = m_listColumns->GetNextItem(-1, LVNI_SELECTED);
	int upper = lower + m_listColumns->GetSelectedCount();
	MoveItem(lower - 1, upper - 1);
	m_listColumns->EnsureVisible(lower - 1, FALSE);
	EnableUpDown();
}

/**
 * @brief Move selected items down (in show list)
 */
void CDirColsDlg::OnDown() 
{
	int lower = m_listColumns->GetNextItem(-1, LVNI_SELECTED);
	int upper = lower + m_listColumns->GetSelectedCount();
	MoveItem(upper, lower);
	m_listColumns->EnsureVisible(lower + 1, FALSE);
	EnableUpDown();
}

/**
 * @brief User clicked ok, so we update m_cols and close.
 */
void CDirColsDlg::OnOK() 
{
	int phy = 0;
	for (int i = 0; i < m_listColumns->GetItemCount(); i++)
	{
		BOOL checked = m_listColumns->GetCheck(i);
		stl_size_t data = static_cast<stl_size_t>(m_listColumns->GetItemData(i));
		m_cols[data].phy = checked ? phy++ : -1;
	}
	EndDialog(IDOK);
}

/**
 * @brief Empty lists and load default columns and order.
 */
void CDirColsDlg::OnDefaults()
{
	m_listColumns->DeleteAllItems();
	m_bReset = true;
	LoadLists();
}

void CDirColsDlg::EnableUpDown()
{
	int lower = m_listColumns->GetNextItem(-1, LVNI_SELECTED);
	int upper = lower + m_listColumns->GetSelectedCount();
	int count = m_listColumns->GetItemCount();
	bool contiguous = m_listColumns->GetNextItem(upper - 1, LVNI_SELECTED) == -1;
	GetDlgItem(IDC_UP)->EnableWindow(contiguous && lower > 0);
	GetDlgItem(IDC_DOWN)->EnableWindow(contiguous && upper < count);
}

/**
 * @brief Update description when selected item changes.
 */
void CDirColsDlg::OnLvnItemchangedColdlgList(NMLISTVIEW *pNM)
{
	if ((pNM->uChanged & LVIF_STATE) && (pNM->uNewState & LVIS_SELECTED) != (pNM->uOldState & LVIS_SELECTED))
	{
		int ind = m_listColumns->GetNextItem(-1, LVNI_SELECTED);
		if (ind != -1)
		{
			stl_size_t data = static_cast<stl_size_t>(m_listColumns->GetItemData(ind));
			SetDlgItemText(IDC_COLDLG_DESC, LanguageSelect.LoadString(m_cols[data].idDesc).c_str());
			EnableUpDown();
		}
	}
}
