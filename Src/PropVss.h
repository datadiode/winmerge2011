/**
 * @file  PropVss.h
 *
 * @brief Declaration of VSS properties dialog.
 */
// ID line follows -- this is updated by SVN
// $Id$

/** @brief Options property page covering Visual SourceSafe integration */
class PropVss : public OptionsPanel
{

// Construction & Destruction
public:
	PropVss();

// Implement IOptionsPanel
	virtual void ReadOptions();
	virtual void WriteOptions();
	virtual void UpdateScreen();

	String	m_strPath;
	int		m_nVerSys;

protected:
	HTreeView *m_pTvClearCaseTypeMgrSetup;
	bool m_bClearCaseTypeMgrSetupModified;
	template<DDX_Operation>
			bool UpdateData();

	DWORD GetClearCaseVerbs();
	void SetClearCaseVerbs(DWORD);
			
	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	LRESULT OnNotify(UNotify *);

	void OnBrowseButton();
};
