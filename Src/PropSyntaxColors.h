/** 
 * @file  PropSyntaxColors.h
 *
 * @brief Declaration file for PropSyntaxColors propertyheet
 *
 */
// ID line follows -- this is updated by SVN
// $Id$

class PropSyntaxColors : public OptionsPanel
{
// Construction & Destruction
public:
	PropSyntaxColors();

// Implement IOptionsPanel
	virtual void UpdateScreen();

protected:
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

	void LoadCustomColors();
	void SaveCustomColors();
	void BrowseColorAndSave(int id, int colorIndex);
	int GetCheckVal(UINT nColorIndex);
	void UpdateBoldStatus(int id, UINT colorIndex);
};
