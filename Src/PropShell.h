/**
 * @file  PropShell.h
 *
 * @brief Declaration of Shell options dialog class
 */
// ID line follows -- this is updated by SVN
// $Id$

/**
 * @brief Class for Shell options -propertypage.
 */
class PropShell : public OptionsPanel
{
// Construction
public:
	PropShell();

// Implement IOptionsPanel
	virtual void ReadOptions();
	virtual void WriteOptions();
	virtual void UpdateScreen();

	BOOL m_bContextAdded;
	BOOL m_bContextAdvanced;
	BOOL m_bContextSubfolders;
	BOOL m_bEnableShellContextMenu;

// Implementation
protected:
	template<DDX_Operation>
			bool UpdateData();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

	void GetContextRegValues();

	void SaveMergePath();
};
