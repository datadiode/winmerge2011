/** 
 * @file  OptionsPanel.h
 *
 * @brief Declaration file for OptionsPanel class.
 *
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "OptionsMgr.h"

/**
 * @brief A base class for WinMerge options dialogs.
 */
class OptionsPanel : public ODialog //, public IOptionsPanel
{
public:
	OptionsPanel(UINT nIDTemplate);
	int m_nPageIndex;
	using ODialog::m_idd;
	virtual void ReadOptions() { }
	virtual void WriteOptions() { }
	virtual void UpdateScreen() = 0;
protected:
	virtual BOOL OnInitDialog();
	typedef enum { SET_DEFAULTS, WRITE_OPTIONS, READ_OPTIONS, INVALIDATE } OPERATION;
	void BrowseColor(int id, COLORREF &currentColor);
	void SerializeColor(OPERATION op, int id, COptionDef<COLORREF> &optionName, COLORREF &color);
};
