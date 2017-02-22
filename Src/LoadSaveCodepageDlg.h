/** 
 * @file  LoadSaveCodepageDlg.h
 *
 * @brief Declaration of the dialog used to select codepages
 */
#pragma once

/////////////////////////////////////////////////////////////////////////////
// CLoadSaveCodepageDlg dialog

class CLoadSaveCodepageDlg : public OResizableDialog
{
public:
// Construction
	CLoadSaveCodepageDlg();
	~CLoadSaveCodepageDlg();

// Dialog Data
	BOOL    m_bAffectsLeft;
	BOOL    m_bAffectsRight;
	BOOL    m_bLoadSaveSameCodepage;

	String m_sAffectsLeftString;
	String m_sAffectsRightString;
	int m_nLoadCodepage;
	int m_nSaveCodepage;
	bool m_bEnableSaveCodepage;

private:
	static CLoadSaveCodepageDlg *m_pThis;
	HComboBox *m_pCbLoadCodepage;
	HComboBox *m_pCbSaveCodepage;

// Implementation
protected:
	template<DDX_Operation>
			bool UpdateData();
	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

	void OnAffectsLeftBtnClicked();
	void OnAffectsRightBtnClicked();
	void OnOK();

private:
	void UpdateSaveGroup();
	static BOOL CALLBACK EnumCodePagesProc(LPTSTR);
};
