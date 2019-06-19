/**
 * @file  PropVss.h
 *
 * @brief Declaration of VSS properties dialog.
 */

/** @brief Options property page covering Visual SourceSafe integration */
class PropVss : public OptionsPanel
{

// Construction & Destruction
public:
	PropVss();

// Implement IOptionsPanel
	virtual void ReadOptions() override;
	virtual void WriteOptions() override;
	virtual void UpdateScreen() override;

	String	m_strPath;
	int		m_nVerSys;

protected:
	HTreeView *m_pTvClearCaseTypeMgrSetup;
	bool m_bClearCaseTypeMgrSetupModified;
	template<DDX_Operation>
			bool UpdateData();

	COptionDef<String> *GetPathOption();
	DWORD GetClearCaseVerbs();
	void SetClearCaseVerbs(DWORD);

	virtual BOOL OnInitDialog() override;
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	LRESULT OnNotify(UNotify *);

	void OnBrowseButton();
};
