/** 
 * @file  TestFilterDlg.h
 *
 * @brief Declaration file for CTestFilterDlg class
 *
 */
#pragma once

struct FileFilter;

/**
 * @brief Dialog allowing user to test out file filter strings.
 * The user can type a text and test if selected file filter matches to it.
 */
class CTestFilterDlg : public ODialog
{
// Construction
public:
	CTestFilterDlg(FileFilter *pFileFilter);

// Implementation data
private:
	FileFilter *const m_pFileFilter; /**< Selected file filter. */

// Implementation methods
private:
	bool CheckText(String text);
	void AppendResult(String result);

	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

	void OnTestBtn();
};
