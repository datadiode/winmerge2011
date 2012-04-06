/** 
 * @file  TestFilterDlg.h
 *
 * @brief Declaration file for CTestFilterDlg class
 *
 */
// ID line follows -- this is updated by SVN
// $Id$

#pragma once

struct FileFilter;
class FileFilterMgr;

/**
 * @brief Dialog allowing user to test out file filter strings.
 * The user can type a text and test if selected file filter matches to it.
 */
class CTestFilterDlg : public ODialog
{
// Construction
public:
	CTestFilterDlg(FileFilter *pFileFilter, FileFilterMgr *pFilterMgr);

// Implementation data
private:
	FileFilter *const m_pFileFilter; /**< Selected file filter. */
	FileFilterMgr *const m_pFileFilterMgr; /**< File filter manager. */

// Implementation methods
private:
	bool CheckText(String text);
	void AppendResult(String result);

	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

	void OnTestBtn();
};
