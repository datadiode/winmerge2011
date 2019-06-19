/**
 * @file  PropHexEditor.h
 *
 * @brief Declaration file for PropHexEditor propertyheet
 */

/**
 * @brief Property page for hex editor options.
 */
class PropHexEditor : public OptionsPanel
{
// Construction
public:
	PropHexEditor();

// Implement IOptionsPanel
	virtual void ReadOptions() override;
	virtual void WriteOptions() override;
	virtual void UpdateScreen() override;

// Dialog Data
	UINT    m_nBytesPerLine;
	BOOL    m_bAutomaticBPL;

// Implementation
protected:
	template<DDX_Operation>
			bool UpdateData();
	virtual BOOL OnInitDialog() override;
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
};
