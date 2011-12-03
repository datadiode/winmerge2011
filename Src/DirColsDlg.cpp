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
	, m_bReset(FALSE)
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
 * @brief Initialize listcontrol in dialog.
 */
void CDirColsDlg::InitList()
{
	// Show selection across entire row.
	// Also enable infotips.
	m_listColumns->SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
	m_listColumns->InsertColumn(0, _T(""), LVCFMT_LEFT, 150);
}

/**
 * @brief Dialog initialisation. Load column lists.
 */
BOOL CDirColsDlg::OnInitDialog() 
{
	ODialog::OnInitDialog();
	LanguageSelect.TranslateDialog(m_hWnd);
	m_listColumns = static_cast<HListView *>(GetDlgItem(IDC_COLDLG_LIST));
	InitList();
	LoadLists();
	return TRUE;
}

/**
 * @brief Load listboxes on screen from column array.
 */
void CDirColsDlg::LoadLists()
{
	for (ColumnArray::iterator iter = m_cols.begin(); iter != m_cols.end(); ++iter)
	{
		const column & c = *iter;
		int x = m_listColumns->InsertItem(m_listColumns->GetItemCount(),
			c.name.c_str());
		m_listColumns->SetItemData(x, c.log_col);
		if (c.phy_col >= 0)
			m_listColumns->SetCheck(x, TRUE);
	}
	SortArrayToLogicalOrder();
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
 * @brief Load listboxes on screen from defaults column array
 */
void CDirColsDlg::LoadDefLists()
{
	for (ColumnArray::iterator iter = m_defCols.begin(); iter != m_defCols.end(); ++iter)
	{
		const column & c = *iter;
		int x = m_listColumns->InsertItem(m_listColumns->GetItemCount(),
			c.name.c_str());
		m_listColumns->SetItemData(x, c.log_col);
		if (c.phy_col >= 0)
			m_listColumns->SetCheck(x, TRUE);
	}
	SortArrayToLogicalOrder();
}
/**
 * @brief Sort m_cols so that it is in logical column order.
 */
void CDirColsDlg::SortArrayToLogicalOrder()
{
	stl::sort(m_cols.begin(), m_cols.end(), &CompareColumnsByLogicalOrder);
}

/**
 * @brief Compare column order of two columns.
 * @param [in] el1 First column to compare.
 * @param [in] el2 Second column to compare.
 * @return Column order.
 */
bool CDirColsDlg::CompareColumnsByLogicalOrder( const column & el1, const column & el2 )
{
   return el1.log_col < el2.log_col;
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
 * @brief Move hidden columns as last items in the list.
 */
void CDirColsDlg::SanitizeOrder()
{
	// Find last visible column.
	int i = m_listColumns->GetItemCount() - 1;
	for ( ; i >= 0; i--)
	{
		if (m_listColumns->GetCheck(i))
			break;
	}

	// Move all hidden columns below last visible column.
	for (int j = i; j >= 0; j--)
	{
		if (!m_listColumns->GetCheck(j))
		{
			MoveItem(j, i);
			i--;
		}
	}
}

/**
 * @brief User clicked ok, so we update m_cols and close.
 */
void CDirColsDlg::OnOK() 
{
	SanitizeOrder();
	for (int i = 0; i < m_listColumns->GetItemCount(); i++)
	{
		BOOL checked = m_listColumns->GetCheck(i);
		DWORD_PTR data = m_listColumns->GetItemData(i);
		m_cols[data].phy_col = checked ? i : -1;
	}
	EndDialog(IDOK);
}

/**
 * @brief Empty lists and load default columns and order.
 */
void CDirColsDlg::OnDefaults()
{
	m_listColumns->DeleteAllItems();
	m_bReset = TRUE;
	LoadDefLists();
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
			DWORD_PTR data = m_listColumns->GetItemData(ind);
			SetDlgItemText(IDC_COLDLG_DESC, m_cols[data].desc.c_str());
			EnableUpDown();
		}
	}
}
