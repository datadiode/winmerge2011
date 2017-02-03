/** 
 * @file  PreferencesDlg.h
 *
 * @brief Declaration of CPreferencesDlg class
 *
 * @note This code originates from AbstractSpoon / TodoList
 * (http://www.abstractspoon.com/) but is modified to use in
 * WinMerge.
 */
#include "OptionsPanel.h"
#include "PropGeneral.h"
#include "PropCompare.h"
#include "PropEditor.h"
#include "PropHexEditor.h"
#include "PropVss.h"
#include "PropRegistry.h"
#include "PropColors.h"
#include "PropTextColors.h"
#include "PropListColors.h"
#include "PropSyntaxColors.h"
#include "PropCodepage.h"
#include "PropArchive.h"
#include "PropBackups.h"
#include "PropShell.h"
#include "PropCompareFolder.h"

class COptionsMgr;

/////////////////////////////////////////////////////////////////////////////
// CPreferencesDlg dialog

class CPreferencesDlg : public ODialog
{
// Construction
public:
	CPreferencesDlg();
	virtual ~CPreferencesDlg();

protected:
// Dialog Data
	HTreeView *m_tcPages;

	int m_nPageCount;	
	int m_nPageIndex;
	PropGeneral m_pageGeneral;
	PropCompare m_pageCompare;
	PropEditor m_pageEditor;
	PropHexEditor m_pageHexEditor;
	PropVss m_pageVss;	
	PropRegistry m_pageSystem;
	PropCodepage m_pageCodepage;
	PropMergeColors m_pageMergeColors;
	PropTextColors m_pageTextColors;
	PropListColors m_pageListColors;
	PropSyntaxColors m_pageSyntaxColors;
	PropArchive m_pageArchive;
	PropBackups m_pageBackups;
	PropShell m_pageShell;
	PropCompareFolder m_pageCompareFolder;

	static String m_pathMRU;

// Implementation
protected:
	void OnOK();
	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	LRESULT OnNotify(NMHDR *);
	void OnDestroy();
	void OnHelpButton();
	void OnImportButton();
	void OnExportButton();
	void OnSelchangingPages(NMTREEVIEW *);
	void OnSelchangedPages(NMTREEVIEW *);
	HTREEITEM AddBranch(UINT, HTREEITEM = TVI_ROOT);
	HTREEITEM AddPage(OptionsPanel *, HTREEITEM = TVI_ROOT);
	HTREEITEM GetSelector(HTREEITEM, HTREEITEM);
	String GetItemPath(HTREEITEM);
	void ReadOptions();
	void SaveOptions();
	void ReadOptions(OptionsPanel *);
};
