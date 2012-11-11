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
		WORD idName; /**< Column name */
		WORD idDesc; /**< Description for column */
		int log; /**< Logical (shown) order number */
		int phy; /**< Physical (in memory) order number */
		int def_phy;
		column(WORD idName, WORD idDesc, int log, int phy, int def_phy)
			: idName(idName), idDesc(idDesc), log(log), phy(phy), def_phy(def_phy)
		{ } 
	};
	typedef stl::vector<column> ColumnArray;

// Construction
public:
	CDirColsDlg();   // standard constructor
	void AddColumn(WORD idName, WORD idDesc, int log, int phy, int def_phy)
	{
		column c(idName, idDesc, log, phy, def_phy);
		m_cols.push_back(c);
	}
	const ColumnArray &GetColumns() const { return m_cols; }

// Dialog Data
	HListView *m_listColumns;
	bool m_bReset;

// Implementation methods
protected:
	void LoadLists();
	void SelectItem(int index);
	void MoveItem(int index, int newIndex);

	static bool CompareColumnsByLogicalOrder(const column &, const column &);

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
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIRCOLSDLG_H__2FCB576C_C609_4623_8C55_F3870F22CA0B__INCLUDED_)
