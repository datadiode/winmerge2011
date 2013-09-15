/** 
 * @file  PropEditor.h
 *
 * @brief Declaration file for PropEditor propertyheet
 *
 */

/**
 * @brief Property page for editor options.
 *
 * Editor options affect to editor behavior. For example syntax highlighting
 * and tabs.
 */
class PropEditor : public OptionsPanel
{
// Construction
public:
	PropEditor();

// Implement IOptionsPanel
	virtual void ReadOptions();
	virtual void WriteOptions();
	virtual void UpdateScreen();

// Dialog Data
	BOOL    m_bHiliteSyntax;
	int	    m_nTabType;
	UINT    m_nTabSize;
	BOOL    m_bAutomaticRescan;
	BOOL    m_bAllowMixedEol;
	BOOL    m_bSeparateCombiningChars;
	BOOL    m_bViewLineDifferences;
	BOOL    m_bBreakOnWords;
	int     m_nBreakType;
	String m_breakChars;

// Implementation
protected:
	template<DDX_Operation>
			bool UpdateData();
	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
};
