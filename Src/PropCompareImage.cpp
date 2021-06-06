/**
 * @file  PropCompareImage.cpp
 *
 * @brief Implementation of PropCompareImage propertysheet
 */
#include "stdafx.h"
#include "OptionsPanel.h"
#include "resource.h"
#include "LanguageSelect.h"
#include "PropCompareImage.h"
#include "WildcardDropList.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/**
 * @brief Constructor.
 * @param [in] optionsMgr Pointer to COptionsMgr.
 */
PropCompareImage::PropCompareImage()
: OptionsPanel(IDD_PROPPAGE_COMPARE_IMAGE, sizeof *this)
{
}

template<ODialog::DDX_Operation op>
bool PropCompareImage::UpdateData()
{
	DDX_Text<op>(IDC_FILEPATS, m_sFilePatterns);
	return true;
}

LRESULT PropCompareImage::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
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
				COptionsMgr::GetDefault(OPT_CMP_IMG_FILEPATTERNS).c_str());
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
void PropCompareImage::ReadOptions()
{
	m_sFilePatterns = COptionsMgr::Get(OPT_CMP_IMG_FILEPATTERNS);
}

/**
 * @brief Writes options values from UI to storage.
 * Property sheet calls this after dialog is closed with OK button to
 * store values in member variables.
 */
void PropCompareImage::WriteOptions()
{
	WildcardRemoveDuplicatePatterns(m_sFilePatterns);
	COptionsMgr::SaveOption(OPT_CMP_IMG_FILEPATTERNS, m_sFilePatterns);
}

/**
 * @brief Sets options to defaults
 */
void PropCompareImage::OnDefaults()
{
	m_sFilePatterns = COptionsMgr::GetDefault(OPT_CMP_IMG_FILEPATTERNS);
	UpdateScreen();
}

void PropCompareImage::UpdateScreen()
{
	UpdateData<Set>();
}
