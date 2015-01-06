/** 
 * @file  PropListColors.h
 *
 * @brief Declaration file for PropListColors propertyheet
 */

/** @brief Property page for colors options; used in options property sheet */
class PropListColors : public OptionsPanel
{

// Construction
public:

	PropListColors();

// Implement IOptionsPanel
	virtual void ReadOptions();
	virtual void WriteOptions();

// Dialog Data
private:
	BOOL m_bCustomColors;
	COLORREF	m_clrLeftOnly;
	COLORREF	m_clrRightOnly;
	COLORREF	m_clrSuspicious;
// Implementation
protected:
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

	void SerializeColors(OPERATION op);

	void UpdateScreen();
	void EnableColorButtons(BOOL bEnable);
	void OnDefaultsStandardColors();
};
