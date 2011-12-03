/** 
 * @file  SharedFilterDlg.h
 *
 * @brief Declaration file for CSharedFilterDlg.
 *
 */
// RCS ID line follows -- this is updated by CVS
// $Id$

#if !defined(AFX_SHAREDFILTERDLG_H__94FD9E42_5C27_49DE_B2FB_77A0B0B03A87__INCLUDED_)
#define AFX_SHAREDFILTERDLG_H__94FD9E42_5C27_49DE_B2FB_77A0B0B03A87__INCLUDED_


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

// Implementation
protected:

// Dialog Data

	String m_SharedFolder;  /**< Folder for shared filters. */
	String m_PrivateFolder; /**< Folder for private filters. */


	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	void OnOK();

// Implementation data
private:
	String m_ChosenFolder;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SHAREDFILTERDLG_H__94FD9E42_5C27_49DE_B2FB_77A0B0B03A87__INCLUDED_)
