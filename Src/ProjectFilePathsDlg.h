/**
 * @file  ProjectFilePathsDlg.h
 *
 * @brief Declaration file for ProjectFilePathsDlg dialog
 */
// RCS ID line follows -- this is updated by CVS
// $Id$

#ifndef _PROJECTFILEPATHSDLG_H_
#define _PROJECTFILEPATHSDLG_H_

/**
 * @brief Dialog allowing user to load, edit and save project files.
 */
class ProjectFilePathsDlg : public ODialog
{
public:
	ProjectFilePathsDlg();   // standard constructor

	String m_strCaption;
	String m_sLeftFile;
	String m_sRightFile;
	String m_sFilter;
	BOOL m_bIncludeSubfolders;
	BOOL m_bLeftPathReadOnly;
	BOOL m_bRightPathReadOnly;

protected:
	template<DDX_Operation>
			bool UpdateData();
	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	String AskProjectFileName(BOOL bOpen);
// Implementation data
private:
	void OnBnClickedProjLfileBrowse();
	void OnBnClickedProjRfileBrowse();
	void OnBnClickedProjFilterSelect();
	void OnBnClickedProjOpen();
	void OnBnClickedProjSave();
};

#endif // _PROJECTFILEPATHSDLG_H_
