/**
 * @file  PropShell.h
 *
 * @brief Declaration of Shell options dialog class
 */

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
	BOOL m_bContextFlattened;
	BOOL m_bEnableDirShellContextMenu;
	BOOL m_bEnableMegeEditShellContextMenu;

// Implementation
protected:
	const BOOL m_bOwnsShellExtension;

	template<DDX_Operation>
			bool UpdateData();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

	void ReadContextRegValues();
	void SaveContextRegValues();
};
