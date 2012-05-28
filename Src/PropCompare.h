/** 
 * @file  PropCompare.h
 *
 * @brief Declaration of PropCompare propertysheet
 */
// ID line follows -- this is updated by SVN
// $Id$

/**
 * @brief Property page to set compare options for WinMerge.
 *
 * Whitespace compare:
 *  - Compare all whitespaces, recommended for merging!
 *  - Ignore changes in whitespaces (amount of spaces etc)
 *  - Ignore all whitespace characters
 */
class PropCompare : public OptionsPanel
{
// Construction
public:
	PropCompare();

// Implement IOptionsPanel
	virtual void ReadOptions();
	virtual void WriteOptions();
	virtual void UpdateScreen();

// Dialog Data
	BOOL    m_bIgnoreEol;
	BOOL    m_bIgnoreCase;
	BOOL    m_bIgnoreBlankLines;
	int     m_nIgnoreWhite;
	BOOL    m_bMovedBlocks;
	BOOL    m_bMatchSimilarLines;
	int     m_nMatchSimilarLinesMax;
	BOOL    m_bFilterCommentsLines;

// Implementation
protected:
	template<DDX_Operation>
			bool UpdateData();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	void OnDefaults();
};
