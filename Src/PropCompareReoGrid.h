/**
 * @file  PropCompareImage.h
 *
 * @brief Declaration of PropCompareImage propertysheet
 */

/**
 * @brief Property page to set image compare options for WinMerge.
 */
class PropCompareReoGrid : public OptionsPanel
{
// Construction
public:
	PropCompareReoGrid();

// Implement IOptionsPanel
	virtual void ReadOptions() override;
	virtual void WriteOptions() override;
	virtual void UpdateScreen() override;

// Dialog Data
	String m_sFilePatterns;

// Implementation
protected:
	template<DDX_Operation>
			bool UpdateData();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	void OnDefaults();
};
