/**
 * @file  ProjectFilePathsDlg.h
 *
 * @brief Declaration file for ProjectFilePathsDlg dialog
 */
// RCS ID line follows -- this is updated by CVS
// $Id$

#ifndef _PROJECTFILEPATHSDLG_H_
#define _PROJECTFILEPATHSDLG_H_

#include "ProjectFile.h"

/**
 * @brief Dialog allowing user to load, edit and save project files.
 */
class ProjectFilePathsDlg
	: ZeroInit<ProjectFilePathsDlg>
	, public ODialog
	, public ProjectFile
{
public:
	ProjectFilePathsDlg();   // standard constructor

	String m_strCaption;

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
