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
	LineFiltersDlg(LineFiltersList *);

// Dialog Data
	BOOL	m_bIgnoreRegExp;
	String	m_strCaption;

// Implementation
protected:

	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	LRESULT OnNotify(NMHDR *);
	void OnApply();
	void OnBnClickedLfilterAddBtn();
	void OnBnClickedLfilterEditbtn();
	void OnBnClickedLfilterRemovebtn();

	void InitList();
	int AddRow(LPCTSTR filter = NULL, BOOL enabled = FALSE);
	void EditSelectedFilter();

private:
	HListView *m_filtersList; /**< List control having filter strings */

	LineFiltersList *const m_pList; /**< Helper list for getting/setting filters. */
};
