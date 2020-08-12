/**
 * @file  PropCompareImage.h
 *
 * @brief Declaration of PropCompareImage propertysheet
 */

/**
 * @brief Property page to set image compare options for WinMerge.
 */
class PropLexers : public OptionsPanel
{
// Construction
public:
	PropLexers();

// Implement IOptionsPanel
	virtual void ReadOptions() override;
	virtual void WriteOptions() override;
	virtual void UpdateScreen() override;

// Implementation
protected:
	template<DDX_Operation>
			bool UpdateData();
	virtual BOOL OnInitDialog() override;
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	LRESULT OnNotify(NMHDR *);
	void OnDefaults();
	void OnLvnKeydown(NMHDR *);
	void OnLvnItemactivate(NMHDR *);

	HListView *m_pLv;
};
