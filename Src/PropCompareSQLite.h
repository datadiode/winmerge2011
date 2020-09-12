/**
 * @file  PropCompareSQLite.h
 *
 * @brief Declaration of PropCompareSQLite propertysheet
 */

/**
 * @brief Property page to set image compare options for WinMerge.
 */
class PropCompareSQLite : public OptionsPanel
{
// Construction
public:
	PropCompareSQLite();

// Implement IOptionsPanel
	virtual void ReadOptions() override;
	virtual void WriteOptions() override;
	virtual void UpdateScreen() override;

// Dialog Data
	String m_sFilePatterns;
	int m_nCompareFlags;
	BOOL m_bPromptForOptions;
	BOOL m_bCompareBlobFields;
	BOOL m_bUseFilePatterns;

// Implementation
protected:
	template<DDX_Operation>
			bool UpdateData();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	void OnDefaults();
};
