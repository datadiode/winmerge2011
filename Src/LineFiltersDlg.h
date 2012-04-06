/** 
 * @file  LineFiltersDlg.h
 *
 * @brief Declaration file for Line Filter dialog
 *
 */
// ID line follows -- this is updated by SVN
// $Id$

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

	void InitList();
	void DoInPlaceEditingEvents();
	int AddRow(LPCTSTR filter = NULL, BOOL enabled = FALSE);
	void EditSelectedFilter();

private:
	HListView *m_LvFilter; /**< List control having filter strings */
	HEdit *m_EdFilter;
	HEdit *m_pEdTestCase;
	HListView *m_pLvTestCase;
};
