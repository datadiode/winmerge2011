/**
 * @file  PropTextColors.h
 *
 * @brief Declaration file for PropTextColors propertyheet
 */

/** @brief Property page for colors options; used in options property sheet */
class PropTextColors : public OptionsPanel
{

// Construction
public:

	PropTextColors();

// Implement IOptionsPanel
	virtual void ReadOptions() override;
	virtual void WriteOptions() override;
	virtual void UpdateScreen() override;

// Dialog Data
private:
	BOOL m_bCustomColors;
// Implementation
protected:
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	void BrowseColorAndSave(int id, int colorIndex);
	void EnableColorButtons(BOOL bEnable);
	void OnDefaultsStandardColors();
};
