/** 
 * @file  PropArchive.h
 *
 * @brief Declaration file for PropArchive propertyheet
 */

/** 
 * @brief Class for Archive Support options page.
 */
class PropArchive : public OptionsPanel
{
public:
	PropArchive();

// Implement IOptionsPanel
	virtual void ReadOptions();
	virtual void WriteOptions();
	virtual void UpdateScreen();

// Dialog Data

protected:
	template<DDX_Operation>
			bool UpdateData();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

public:
	BOOL m_bEnableSupport;
	BOOL m_bProbeType;
};
