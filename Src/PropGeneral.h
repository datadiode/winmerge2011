/**
 * @file  PropGeneral.h
 *
 * @brief Declaration of PropGeneral class
 */

/**
 * @brief Class for General options -propertypage.
 */
class PropGeneral : public OptionsPanel
{
// Construction
public:
	PropGeneral();
	~PropGeneral();

// Implement IOptionsPanel
	virtual void ReadOptions();
	virtual void WriteOptions();
	virtual void UpdateScreen();

// Dialog Data
	BOOL  m_bScroll;
	BOOL  m_bDisableSplash;
	BOOL  m_bSingleInstance;
	BOOL  m_bVerifyPaths;
	BOOL  m_bCloseWindowWithEsc;
	BOOL  m_bAskMultiWindowClose;
	BOOL	m_bMultipleFileCmp;
	BOOL	m_bMultipleDirCmp;
	int		m_nAutoCompleteSource;
	BOOL	m_bPreserveFiletime;
	BOOL	m_bShowSelectFolderOnStartup;

// Implementation
protected:
	template<DDX_Operation>
			bool UpdateData();
	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	void OnResetAllMessageBoxes();
};
