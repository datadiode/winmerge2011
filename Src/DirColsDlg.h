/** 
 * @file  DirColsDlg.h
 *
 * @brief Declaration file for CDirColsDlg
 *
 * @date  Created: 2003-08-19
 */
// ID line follows -- this is updated by SVN
// $Id$


#if !defined(AFX_DIRCOLSDLG_H__2FCB576C_C609_4623_8C55_F3870F22CA0B__INCLUDED_)
#define AFX_DIRCOLSDLG_H__2FCB576C_C609_4623_8C55_F3870F22CA0B__INCLUDED_
#pragma once

/////////////////////////////////////////////////////////////////////////////
// CDirColsDlg dialog

/**
 * @brief A Dialog for choosing visible folder compare columns.
 * This class implements a dialog for choosing visible columns in folder
 * compare. Columns can be also re-ordered. There is one listview component
 * which lists all available columns. Every column name has a checkbox with
 * it. If the checkbox is checked, the column is visible.
 *
 * @note: Due to how columns handling code is implemented, hidden columns
 * must be always be last in the list with order number -1.
 * @todo: Allow hidden columns between visible columns.
 */
class CDirColsDlg : public ODialog
{
// Public types
public:
	/** @brief One column's information. */
	struct column
	{
		String name; /**< Column name */
		String desc; /**< Description for column */
		int log_col; /**< Logical (shown) order number */
		int phy_col; /**< Physical (in memory) order number */
		column(const String & colName, const String & dsc, int log, int phy)
			: name(colName), desc(dsc), log_col(log), phy_col(phy)
		{ } 
	};
	typedef stl::vector<column> ColumnArray;

// Construction
public:
	CDirColsDlg();   // standard constructor
	void AddColumn(const String &name, const String &desc, int log, int phy = -1)
		{ column c(name, desc, log, phy); m_cols.push_back(c); }
	void AddDefColumn(const String &name, int log, int phy = -1)
		{ column c(name, _T(""), log, phy); m_defCols.push_back(c); }
	const ColumnArray &GetColumns() const { return m_cols; }

// Dialog Data
	HListView *m_listColumns;
	BOOL m_bReset;

// Implementation methods
protected:
	void InitList();
	void LoadLists();
	void SelectItem(int index);
	void LoadDefLists();
	void SortArrayToLogicalOrder();
	void MoveItem(int index, int newIndex);
	void SanitizeOrder();

	static bool CompareColumnsByLogicalOrder( const column & el1, const column & el2 );

	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	LRESULT OnNotify(NMHDR *);
	void EnableUpDown();
	void OnUp();
	void OnDown();
	void OnOK();
	void OnDefaults();
	void OnLvnItemchangedColdlgList(NMLISTVIEW *);

	// Implementation data
private:
	ColumnArray m_cols; /**< Column list. */
	ColumnArray m_defCols; /**< Default columns. */
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIRCOLSDLG_H__2FCB576C_C609_4623_8C55_F3870F22CA0B__INCLUDED_)
