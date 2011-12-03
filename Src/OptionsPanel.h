/** 
 * @file  OptionsPanel.h
 *
 * @brief Declaration file for OptionsPanel class.
 *
 */
// ID line follows -- this is updated by SVN
// $Id$

class COptionsMgr;

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
	BOOL IsUserInputCommand(WPARAM);
};
