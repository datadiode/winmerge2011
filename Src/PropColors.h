/** 
 * @file  PropColors.h
 *
 * @brief Declaration file for PropMergeColors propertyheet
 */

/** @brief Property page for colors options; used in options property sheet */
class PropMergeColors : public OptionsPanel
{
// Construction
public:
	PropMergeColors();

// Implement IOptionsPanel
	virtual void ReadOptions();
	virtual void WriteOptions();
	virtual void UpdateScreen();
	
// Dialog Data
private:
	COLORREF	m_clrDiff;
	COLORREF	m_clrSelDiff;
	COLORREF	m_clrDiffDeleted;
	COLORREF	m_clrSelDiffDeleted;
	COLORREF	m_clrDiffText;
	COLORREF	m_clrSelDiffText;
	COLORREF	m_clrTrivial;
	COLORREF	m_clrTrivialDeleted;
	COLORREF	m_clrTrivialText;
	COLORREF	m_clrMoved;
	COLORREF	m_clrMovedDeleted;
	COLORREF	m_clrMovedText;
	COLORREF	m_clrSelMoved;
	COLORREF	m_clrSelMovedDeleted;
	COLORREF	m_clrSelMovedText;
	COLORREF	m_clrWordDiff;
	COLORREF	m_clrWordDiffDeleted;
	COLORREF	m_clrWordDiffText;
	COLORREF	m_clrSelWordDiff;
	COLORREF	m_clrSelWordDiffDeleted;
	COLORREF	m_clrSelWordDiffText;

	BOOL m_bCrossHatchDeletedLines;

// Implementation
protected:


	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

	void SerializeColors(OPERATION op);
};
