/** 
 * @file  OptionsPanel.h
 *
 * @brief Declaration file for OptionsPanel class.
 */
#include "OptionsMgr.h"

/**
 * @brief A base class for WinMerge options dialogs.
 */
class OptionsPanel
: public ZeroInit<OptionsPanel>
, public ODialog
{
public:
	int m_nPageIndex;
	using ODialog::m_idd;
	virtual void ReadOptions() { }
	virtual void WriteOptions() { }
	virtual void UpdateScreen() = 0;
protected:
	OptionsPanel(UINT nIDTemplate, size_t cb);
	virtual BOOL OnInitDialog();
	typedef enum { SET_DEFAULTS, WRITE_OPTIONS, READ_OPTIONS, INVALIDATE } OPERATION;
	void BrowseColor(int id, COLORREF &currentColor);
	void SerializeColor(OPERATION op, int id, COptionDef<COLORREF> &optionName, COLORREF &color);
	int ValidateNumber(HEdit *edit, int iMin, int iMax);
};
