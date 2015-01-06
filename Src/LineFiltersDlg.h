/** 
 * @file  LineFiltersDlg.h
 *
 * @brief Declaration file for Line Filter dialog
 *
 */
#pragma once

class LineFiltersList;

/**
 * @brief A dialog for editing and selecting used line filters.
 * This dialog allows user to add, edit and remove line filters. Currently
 * active filters are selected by enabling their checkbox.
 */
class LineFiltersDlg : public ODialog
{
// Construction
public:
	LineFiltersDlg();

// Dialog Data
	BOOL	m_bIgnoreRegExp;
	BOOL	m_bLineFiltersDirty;
	String	m_strCaption;
	String	m_strTestCase;

// Implementation
protected:

	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	LRESULT OnNotify(UNotify *);
	void OnCustomdraw(HSurface *);
	void OnApply();
	void OnBnClickedLfilterAddBtn();
	void OnBnClickedLfilterEditbtn();
	void OnBnClickedLfilterRemovebtn();
	void OnClick(UNotify *);
	void OnSelchanged();
	void OnCheckStateChange(HTREEITEM);

	void InitList();
	void PopulateSection(HTREEITEM);
	void DoInPlaceEditingEvents();
	HTREEITEM AddRow(LPCTSTR filter = NULL, int usage = 0, int cChildren = 0);
	HTREEITEM GetEditableItem();
	void EditSelectedFilter();

private:
	String m_inifile;
	HTreeView *m_TvFilter; /**< List control having filter strings */
	HEdit *m_EdFilter;
	HEdit *m_pEdTestCase;
	HListView *m_pLvTestCase;
};
