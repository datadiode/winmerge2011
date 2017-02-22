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
	~PropCodepage();

// Implement IOptionsPanel
	virtual void ReadOptions();
	virtual void WriteOptions();
	virtual void UpdateScreen();

// Dialog Data
	int		m_nCodepageSystem;
	int		m_nCustomCodepageValue;
	BOOL	m_bDetectCodepage;

private:
	static PropCodepage *m_pThis;
	HComboBox *m_pCbCustomCodepage;

// Implementation
protected:
	template<DDX_Operation>
			bool UpdateData();
	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

private:
	static BOOL CALLBACK EnumCodePagesProc(LPTSTR);
};
