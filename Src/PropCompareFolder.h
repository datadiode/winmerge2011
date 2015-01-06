/** 
 * @file  PropCompareFolder.h
 *
 * @brief Declaration of PropCompareFolder propertysheet
 */

/**
 * @brief Property page to set folder compare options for WinMerge.
 *
 * Compare methods:
 *  - compare by contents
 *  - compare by modified date
 *  - compare by file size
 *  - compare by date and size
 *  - compare by quick contents
 */
class PropCompareFolder : public OptionsPanel
{
// Construction
public:
	PropCompareFolder();

// Implement IOptionsPanel
	virtual void ReadOptions();
	virtual void WriteOptions();
	virtual void UpdateScreen();

// Dialog Data
	int     m_compareMethod;
	BOOL    m_bStopAfterFirst;
	BOOL    m_bIgnoreSmallTimeDiff;
	BOOL    m_bSelfCompare;
	BOOL    m_bWalkUniques;
	BOOL    m_bCacheResults;
	UINT    m_nQuickCompareLimit;
	int     m_nCompareThreads;
// Implementation
protected:
	template<DDX_Operation>
			bool UpdateData();
	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	void OnDefaults();
	void OnCbnSelchangeComparemethodcombo();
};
