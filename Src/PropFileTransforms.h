/**
 * @file  PropFileTransforms.h
 *
 * @brief Declaration of PropFileTransforms propertysheet
 */

/**
 * @brief Property page to set image compare options for WinMerge.
 */
class PropFileTransforms : public OptionsPanel
{
// Construction
public:
	PropFileTransforms();

// Implement IOptionsPanel
	virtual void ReadOptions() override;
	virtual void WriteOptions() override;
	virtual void UpdateScreen() override;

	static void DumpDefaults(LPTSTR);
// Implementation
protected:
	void Scan(LPTSTR);
	virtual BOOL OnInitDialog() override;
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	LRESULT OnNotify(NMHDR *);
	void OnDefaults();
	void OnLvnItemchanged(NMHDR *);

	HListView *m_pLv;
};
