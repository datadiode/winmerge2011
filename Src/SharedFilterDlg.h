/** 
 * @file  SharedFilterDlg.h
 *
 * @brief Declaration file for CSharedFilterDlg.
 *
 */
#pragma once

/////////////////////////////////////////////////////////////////////////////
// CSharedFilterDlg dialog

/**
 * @brief A dialog for selecting shared/private filter creation.
 * This dialog allows user to select if the new filter is a shared filter
 * (placed into WinMerge executable's subfolder) or private filter
 * (placed into profile folder).
 */
class CSharedFilterDlg : public ODialog
{
public:
	static String PromptForNewFilter(LPCTSTR SharedFolder, LPCTSTR PrivateFolder);

// Construction
public:
	CSharedFilterDlg();   // standard constructor
// Dialog Data
	String m_SharedFolder;  /**< Folder for shared filters. */
	String m_PrivateFolder; /**< Folder for private filters. */
	String m_ChosenFolder;
// Implementation
protected:
	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	void OnOK();
};
