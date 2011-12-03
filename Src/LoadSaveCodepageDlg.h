/** 
 * @file  LoadSaveCodepageDlg.h
 *
 * @brief Declaration of the dialog used to select codepages
 */
// ID line follows -- this is updated by SVN
// $Id$


#if !defined(AFX_LOADSAVECODEPAGEDLG_H__B9A16700_6F1A_4DF1_8EB3_0A1D772DCE91__INCLUDED_)
#define AFX_LOADSAVECODEPAGEDLG_H__B9A16700_6F1A_4DF1_8EB3_0A1D772DCE91__INCLUDED_
#pragma once

/////////////////////////////////////////////////////////////////////////////
// CLoadSaveCodepageDlg dialog

class CLoadSaveCodepageDlg : public OResizableDialog
{
public:
// Construction
	CLoadSaveCodepageDlg();   // standard constructor

// Implementation methods
private:
	void UpdateSaveGroup();

// Implementation data
public:

// Dialog Data
	BOOL    m_bAffectsLeft;
	BOOL    m_bAffectsRight;
	BOOL    m_bLoadSaveSameCodepage;

	String m_sAffectsLeftString;
	String m_sAffectsRightString;
	int m_nLoadCodepage;
	int m_nSaveCodepage;
	bool m_bEnableSaveCodepage;

// Implementation
protected:
	template<DDX_Operation>
			bool UpdateData();
	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

	void OnAffectsLeftBtnClicked();
	void OnAffectsRightBtnClicked();
	void OnOK();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOADSAVECODEPAGEDLG_H__B9A16700_6F1A_4DF1_8EB3_0A1D772DCE91__INCLUDED_)
