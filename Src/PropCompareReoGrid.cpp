/**
 * @file  PropCompareReoGrid.cpp
 *
 * @brief Implementation of PropCompareReoGrid propertysheet
 */
#include "stdafx.h"
#include "OptionsPanel.h"
#include "resource.h"
#include "LanguageSelect.h"
#include "PropCompareReoGrid.h"
#include "WildcardDropList.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/**
 * @brief Constructor.
 * @param [in] optionsMgr Pointer to COptionsMgr.
 */
PropCompareReoGrid::PropCompareReoGrid() 
: OptionsPanel(IDD_PROPPAGE_COMPARE_REOGRID, sizeof *this)
{
}

template<ODialog::DDX_Operation op>
bool PropCompareReoGrid::UpdateData()
{
	DDX_Text<op>(IDC_FILEPATS, m_sFilePatterns);
	return true;
}

LRESULT PropCompareReoGrid::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		if (IsUserInputCommand(wParam))
			UpdateData<Get>();
		switch (wParam)
		{
		case MAKEWPARAM(IDC_DEFAULTS, BN_CLICKED):
			OnDefaults();
			break;
		case MAKEWPARAM(IDC_FILEPATS, CBN_DROPDOWN):
			WildcardDropList::OnDropDown(reinterpret_cast<HWND>(lParam), 6,
				(COptionsMgr::GetDefault(OPT_CMP_REOGRID_FILEPATTERNS) + _T(";*.csv;*.tsv")).c_str());
			break;
		case MAKEWPARAM(IDC_FILEPATS, CBN_CLOSEUP):
			WildcardDropList::OnCloseUp(reinterpret_cast<HWND>(lParam));
			UpdateData<Get>();
			break;
		}
		break;
	}
	return OptionsPanel::WindowProc(message, wParam, lParam);
}

/** 
 * @brief Reads options values from storage to UI.
 * Property sheet calls this before displaying GUI to load values
 * into members.
 */
void PropCompareReoGrid::ReadOptions()
{
	m_sFilePatterns = COptionsMgr::Get(OPT_CMP_REOGRID_FILEPATTERNS);
}

/** 
 * @brief Writes options values from UI to storage.
 * Property sheet calls this after dialog is closed with OK button to
 * store values in member variables.
 */
void PropCompareReoGrid::WriteOptions()
{
	WildcardRemoveDuplicatePatterns(m_sFilePatterns);
	COptionsMgr::SaveOption(OPT_CMP_REOGRID_FILEPATTERNS, m_sFilePatterns);
}

/** 
 * @brief Sets options to defaults
 */
void PropCompareReoGrid::OnDefaults()
{
	m_sFilePatterns = COptionsMgr::GetDefault(OPT_CMP_REOGRID_FILEPATTERNS);
	UpdateScreen();
}

void PropCompareReoGrid::UpdateScreen()
{
	UpdateData<Set>();
}
