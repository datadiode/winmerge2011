/** 
 * @file  DirViewColHandler.cpp
 *
 * @brief Methods of CDirView dealing with the listview (of file results)
 *
 * @date  Created: 2003-08-19
 */
#include "StdAfx.h"
#include "resource.h"
#include "LanguageSelect.h"
#include "SettingStore.h"
#include "Merge.h"
#include "DirView.h"
#include "DirFrame.h"
#include "coretools.h"
#include "DirColsDlg.h"
#include "OptionsMgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////


/**
 * @brief Get text for specified column.
 * This function retrieves the text for the specified colum. Text is
 * retrieved by using column-specific handler functions.
 * @param [in] pCtxt Compare context.
 * @param [in] col Column number.
 * @param [in] di Difference data.
 * @return Text for the specified column.
 */
String CDirView::ColGetTextToDisplay(int col, const DIFFITEM *di)
{
	CDiffContext const *const pCtxt = m_pFrame->GetDiffContext();
	// Custom properties have custom get functions
	const DirColInfo &colInfo = f_cols[col];
	return (*colInfo.getfnc)(pCtxt, reinterpret_cast<const char *>(di) + colInfo.offset);
}

/**
 * @brief Sort two items on specified column.
 * This function determines order of two items in specified column. Order
 * is determined by column-specific functions.
 * @param [in] pCtxt Compare context.
 * @param [in] col Column number to sort.
 * @param [in] ldi Left difference item data.
 * @param [in] rdi Right difference item data.
 * @return Order of items.
 */
int CDirView::ColSort(int col, const DIFFITEM *ldi, const DIFFITEM *rdi) const
{
	CDiffContext const *const pCtxt = m_pFrame->GetDiffContext();
	// Custom properties have custom sort functions
	const DirColInfo &colInfo = f_cols[col];
	const size_t offset = colInfo.offset;
	const void *arg1;
	const void *arg2;
	if (m_bTreeMode)
	{
		const DIFFITEM *lcur = ldi;
		const DIFFITEM *rcur = rdi;
		int lLevel = lcur->GetDepth();
		int rLevel = rcur->GetDepth();
		while (lLevel < rLevel)
		{
			--rLevel;
			rcur = rcur->parent;
		}
		while (lLevel > rLevel)
		{
			--lLevel;
			lcur = lcur->parent;
		}
		while (lcur->parent != rcur->parent)
		{
			lcur = lcur->parent;
			rcur = rcur->parent;
		}
		arg1 = reinterpret_cast<const char *>(lcur) + offset;
		arg2 = reinterpret_cast<const char *>(rcur) + offset;
	}
	else
	{
		arg1 = reinterpret_cast<const char *>(ldi) + offset;
		arg2 = reinterpret_cast<const char *>(rdi) + offset;
	}
	if (ColSortFncPtrType fnc = colInfo.sortfnc)
	{
		return (*fnc)(pCtxt, arg1, arg2);
	}
	if (ColGetFncPtrType fnc = colInfo.getfnc)
	{
		String p = (*fnc)(pCtxt, arg1);
		String q = (*fnc)(pCtxt, arg2);
		return lstrcmpi(p.c_str(), q.c_str());
	}
	return 0;
}

/// Update column names (as per selected UI language) / width / alignment
void CDirView::UpdateColumns(UINT lvcf)
{
	SetRedraw(FALSE);
	for (int i = 0; i < g_ncols; ++i)
	{
		const DirColInfo &colInfo = f_cols[i];
		int phys = ColLogToPhys(i);
		if (phys >= 0)
		{
			String s = LanguageSelect.LoadString(colInfo.idName);
			LV_COLUMN lvc;
			lvc.mask = lvcf;
			lvc.pszText = const_cast<LPTSTR>(s.c_str());
			lvc.fmt = colInfo.alignment;
			if (lvc.mask & LVCF_WIDTH)
			{
				string_format sWidthKey(_T("WDirHdr_%s_Width"), f_cols[i].regName);
				lvc.cx = max(10, SettingStore.GetProfileInt(_T("DirView"), sWidthKey.c_str(), DefColumnWidth));
			}
			SetColumn(phys, &lvc);
		}
	}
	const int sortCol = COptionsMgr::Get(OPT_DIRVIEW_SORT_COLUMN);
	if (sortCol >= 0 && sortCol < m_numcols)
	{
		bool bSortAscending = COptionsMgr::Get(OPT_DIRVIEW_SORT_ASCENDING);
		m_ctlSortHeader.SetSortImage(ColLogToPhys(sortCol), bSortAscending);
	}
	SetRedraw(TRUE);
	Invalidate();
}

CDirView::CompareState::CompareState(const CDirView *pView, int sortCol, bool bSortAscending)
: pView(pView)
, sortCol(sortCol)
, bSortAscending(bSortAscending)
{
}

/// Compare two specified rows during a sort operation (windows callback)
int CALLBACK CDirView::CompareState::CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CompareState *pThis = reinterpret_cast<CompareState*>(lParamSort);
	// Sort special items always first in dir view
	if (lParam1 == 0)
		return -1;
	if (lParam2 == 0)
		return 1;

	const DIFFITEM *ldi = reinterpret_cast<DIFFITEM *>(lParam1);
	const DIFFITEM *rdi = reinterpret_cast<DIFFITEM *>(lParam2);
	// compare 'left' and 'right' parameters as appropriate
	int retVal = pThis->pView->ColSort(pThis->sortCol, ldi, rdi);
	// return compare result, considering sort direction
	return pThis->bSortAscending ? retVal : -retVal;
}

/// Add new item to list view
int CDirView::AddNewItem(int i, const DIFFITEM *di, int iImage, int iIndent)
{
	LV_ITEM lvItem;
	lvItem.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE | LVIF_INDENT;
	lvItem.iItem = i;
	lvItem.iIndent = iIndent;
	lvItem.iSubItem = 0;
	lvItem.pszText = LPSTR_TEXTCALLBACK;
	lvItem.lParam = reinterpret_cast<LPARAM>(di);
	lvItem.iImage = iImage;
	return InsertItem(&lvItem);
}

/**
 * @brief Update listview display of details for specified row
 * @note Customising shownd data should be done here
 */
void CDirView::UpdateDiffItemStatus(UINT nIdx)
{
	RedrawItems(nIdx, nIdx);
}

static std::vector<String> rgDispinfoText(2); // used in function below

/**
 * @brief Allocate a text buffer to assign to NMLVDISPINFO::item::pszText
 * Quoting from SDK Docs:
 *	If the LVITEM structure is receiving item text, the pszText and cchTextMax
 *	members specify the address and size of a buffer. You can either copy text to
 *	the buffer or assign the address of a string to the pszText member. In the
 *	latter case, you must not change or delete the string until the corresponding
 *	item text is deleted or two additional LVN_GETDISPINFO messages have been sent.
 */
static LPTSTR NTAPI AllocDispinfoText(String &s)
{
	static int i = 0;
	rgDispinfoText[i].swap(s);
	LPCTSTR pszText = rgDispinfoText[i].c_str();
	i ^= 1;
	return (LPTSTR)pszText;
}

/**
 * @brief Respond to LVN_GETDISPINFO message
 */
void CDirView::ReflectGetdispinfo(NMLVDISPINFO *pParam)
{
	int const nIdx = pParam->item.iItem;
	int const i = ColPhysToLog(pParam->item.iSubItem);
	WORD const idName = f_cols[i].idName;
	DIFFITEM *di = GetDiffItem(nIdx);
	if (di == NULL)
	{
		if (idName == IDS_COLHDR_FILENAME)
		{
			pParam->item.pszText = _T("..");
		}
		return;
	}
	if (pParam->item.mask & LVIF_TEXT)
	{
		String s = ColGetTextToDisplay(i, di);
		// Add '*' to newer time field
		if ((idName == IDS_COLHDR_LTIMEM && di->left.mtime > di->right.mtime) ||
			(idName == IDS_COLHDR_RTIMEM && di->left.mtime < di->right.mtime))
		{
			s.insert(0, m_szAsterisk);
		}
		pParam->item.pszText = AllocDispinfoText(s);
	}
	if (pParam->item.mask & LVIF_IMAGE)
	{
		pParam->item.iImage = CompareStats::GetColImage(di);
	}
}

/// store current column orders into registry
void CDirView::SaveColumnOrders()
{
	ASSERT(m_colorder.size() == m_numcols);
	ASSERT(m_invcolorder.size() == m_numcols);
    for (int i = 0; i < m_numcols; ++i)
	{
		string_format RegName(_T("WDirHdr_%s_Order"), f_cols[i].regName);
		int ord = m_colorder[i];
		SettingStore.WriteProfileInt(_T("DirView"), RegName.c_str(), ord);
	}
}

/**
 * @brief Load column orders from registry
 */
void CDirView::LoadColumnOrders()
{
	ASSERT(m_numcols == -1);
	m_numcols = g_ncols;
	ClearColumnOrders();
	m_dispcols = 0;

	// Load column orders
	// Break out if one is missing
	// Break out & mark failure (m_dispcols == -1) if one is invalid
	int i;
	for (i = 0; i < m_numcols; ++i)
	{
		string_format RegName(_T("WDirHdr_%s_Order"), f_cols[i].regName);
		int ord = SettingStore.GetProfileInt(_T("DirView"), RegName.c_str(), -2);
		if (ord < -1 || ord >= m_numcols)
			break;
		m_colorder[i] = ord;
		if (ord >= 0)
		{
			++m_dispcols;
			if (m_invcolorder[ord] != -1)
			{
				m_dispcols = -1;
				break;
			}
			m_invcolorder[ord] = i;
		}
	}
	// Check that a contiguous range was set
	for (i = 0; i < m_dispcols; ++i)
	{
		if (m_invcolorder[i] < 0)
		{
			m_dispcols = -1;
			break;
		}
	}
	// Must have at least one column
	if (m_dispcols < 1)
	{
		ResetColumnOrdering();
	}
	ValidateColumnOrdering();
}

/**
 * @brief Sanity check column ordering
 */
void CDirView::ValidateColumnOrdering()
{
#if _DEBUG
	ASSERT(m_invcolorder[0]>=0);
	ASSERT(m_numcols == g_ncols);
	// Check that any logical->physical mapping is reversible
	for (int i=0; i<m_numcols; ++i)
	{
		int phy = m_colorder[i];
		if (phy >= 0)
		{
			int log = m_invcolorder[phy];
			ASSERT(i == log);
		}
	}
	// Bail out if header doesn't exist yet
	int hdrcnt = GetHeaderCtrl()->GetItemCount();
	if (hdrcnt)
	{
		ASSERT(hdrcnt == m_dispcols);
	}
#endif
}

/**
 * @brief Set column ordering to default initial order
 */
void CDirView::ResetColumnOrdering()
{
	ClearColumnOrders();
	m_dispcols = 0;
	for (int i = 0; i < m_numcols; ++i)
	{
		int phy = f_cols[i].physicalIndex;
		m_colorder[i] = phy;
		if (phy >= 0)
		{
			m_invcolorder[phy] = i;
			++m_dispcols;
		}
	}
	ValidateColumnOrdering();
}

/**
 * @brief Reset all current column ordering information
 */
void CDirView::ClearColumnOrders()
{
	m_colorder.resize(m_numcols);
	m_invcolorder.resize(m_numcols);
	for (int i = 0; i < m_numcols; ++i)
	{
		m_colorder[i] = -1;
		m_invcolorder[i] = -1;
	}
}

/**
 * @brief Return display name of column
 */
String CDirView::GetColDisplayName(int col) const
{
	return LanguageSelect.LoadString(f_cols[col].idName);
}

/**
 * @brief Return description of column
 */
String CDirView::GetColDescription(int col) const
{
	return LanguageSelect.LoadString(f_cols[col].idDesc);
}

/**
 * @brief Remove any windows reordering of columns (params are physical columns)
 */
void CDirView::MoveColumn(int psrc, int pdest)
{
	if (psrc == pdest || pdest == -1)
		return;
	// actually moved column
	m_colorder[m_invcolorder[psrc]] = pdest;
	// shift all other affected columns
	int dir = psrc > pdest ? +1 : -1;
	int i;
	for (i = pdest; i != psrc; i += dir)
	{
		m_colorder[m_invcolorder[i]] = i + dir;
	}
	// fix inverse mapping
	for (i = 0; i < m_numcols; ++i)
	{
		int ord = m_colorder[i];
		if (ord >= 0)
			m_invcolorder[ord] = i;
	}
	ValidateColumnOrdering();
	UpdateColumns(LVCF_TEXT | LVCF_FMT | LVCF_WIDTH);
	ValidateColumnOrdering();
}

/**
 * @brief User examines & edits which columns are displayed in dirview, and in which order
 */
void CDirView::OnEditColumns()
{
	CDirColsDlg dlg;
	// List all the currently displayed columns
	int log, phy;
	for (phy = 0; phy < m_dispcols; ++phy)
	{
		log = ColPhysToLog(phy);
		const DirColInfo &col = f_cols[log];
		dlg.AddColumn(col.idName, col.idDesc, log, phy, col.physicalIndex);
	}
	// Now add all the columns not currently displayed
	for (log = 0; log < m_numcols; ++log)
	{
		if (ColLogToPhys(log) == -1)
		{
			const DirColInfo &col = f_cols[log];
			dlg.AddColumn(col.idName, col.idDesc, log, -1, col.physicalIndex);
		}
	}

	if (LanguageSelect.DoModal(dlg) != IDOK)
		return;

	if (dlg.m_bReset)
		ResetColumnWidths();
	else
		SaveColumnWidths(); // save current widths to registry

	// Reset our data to reflect the new data from the dialog
	const CDirColsDlg::ColumnArray &cols = dlg.GetColumns();
	ClearColumnOrders();
	m_dispcols = 0;
	const int sortColumn = COptionsMgr::Get(OPT_DIRVIEW_SORT_COLUMN);
	for (CDirColsDlg::ColumnArray::const_iterator iter = cols.begin(); iter != cols.end(); ++iter)
	{
		log = iter->log;
		phy = iter->phy;
		m_colorder[log] = phy;
		if (phy >= 0)
		{
			++m_dispcols;
			m_invcolorder[phy] = log;
		}

		// If sorted column was hidden, reset sorting
		if (log == sortColumn && phy < 0)
		{
			COptionsMgr::Reset(OPT_DIRVIEW_SORT_COLUMN);
			COptionsMgr::Reset(OPT_DIRVIEW_SORT_ASCENDING);
		}
	}
	if (m_dispcols < 1)
	{
		// Ignore them if they didn't leave a column showing
		ResetColumnOrdering();
	}
	else
	{
		ReloadColumns();
	}
	ValidateColumnOrdering();
}
