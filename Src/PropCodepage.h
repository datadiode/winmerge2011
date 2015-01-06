/**
 * @file  PropCodepage.h
 *
 * @brief Declaration of PropCodepage class
 */

class PropCodepage : public OptionsPanel
{
// Construction
public:
	PropCodepage();

// Implement IOptionsPanel
	virtual void ReadOptions();
	virtual void WriteOptions();
	virtual void UpdateScreen();

// Dialog Data
	int		m_nCodepageSystem;
	int		m_nCustomCodepageValue;
	BOOL	m_bDetectCodepage;

// Implementation
protected:
	template<DDX_Operation>
			bool UpdateData();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
};
