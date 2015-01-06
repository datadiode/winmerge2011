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
	virtual void ReadOptions();
	virtual void WriteOptions();
	virtual void UpdateScreen();

// Dialog Data
	UINT    m_nBytesPerLine;
	BOOL    m_bAutomaticBPL;

// Implementation
protected:
	template<DDX_Operation>
			bool UpdateData();
	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
};
