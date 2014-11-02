/** 
 * @file  PropListColors.cpp
 *
 * @brief Implementation of PropListColors propertysheet
 */
#include "StdAfx.h"
#include "OptionsPanel.h"
#include "resource.h"
#include "SyntaxColors.h"
#include "PropListColors.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/** @brief Section name for settings in registry. */
static const TCHAR Section[] = _T("Custom Colors");

/** 
 * @brief Default constructor.
 */
PropListColors::PropListColors()
: OptionsPanel(IDD_PROPPAGE_COLORS_LIST, sizeof *this)
{
}

LRESULT PropListColors::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case MAKEWPARAM(IDC_DEFAULT_STANDARD_COLORS, BN_CLICKED):
			m_bCustomColors = IsDlgButtonChecked(IDC_DEFAULT_STANDARD_COLORS);
			OnDefaultsStandardColors();
			break;
		case MAKEWPARAM(IDC_LIST_LEFTONLY_BKGD_COLOR, BN_CLICKED):
			BrowseColor(IDC_LIST_LEFTONLY_BKGD_COLOR, m_clrLeftOnly);
			break;
		case MAKEWPARAM(IDC_LIST_RIGHTONLY_BKGD_COLOR, BN_CLICKED):
			BrowseColor(IDC_LIST_RIGHTONLY_BKGD_COLOR, m_clrRightOnly);
			break;
		case MAKEWPARAM(IDC_LIST_SUSPICIOUS_BKGD_COLOR, BN_CLICKED):
			BrowseColor(IDC_LIST_SUSPICIOUS_BKGD_COLOR, m_clrSuspicious);
			break;
		}
		break;
	case WM_DRAWITEM:
		return MessageReflect_ColorButton<WM_DRAWITEM>(wParam, lParam);
	}
	return OptionsPanel::WindowProc(message, wParam, lParam);
}

/** 
 * @brief Reads options values from storage to UI.
 * (Property sheet calls this before displaying all property pages)
 */
void PropListColors::ReadOptions()
{
	m_bCustomColors = COptionsMgr::Get(OPT_CLR_DEFAULT_LIST_COLORING) ? FALSE : TRUE;
	SerializeColors(READ_OPTIONS);
}

/** 
 * @brief Writes options values from UI to storage.
 * (Property sheet calls this after displaying all property pages)
 */
void PropListColors::WriteOptions()
{
	COptionsMgr::SaveOption(OPT_CLR_DEFAULT_LIST_COLORING, m_bCustomColors == FALSE);
	SerializeColors(WRITE_OPTIONS);
}

/**
 * @brief Load all colors, Save all colors, or set all colors to default
 * @param [in] op Operation to do, one of
 *  - SET_DEFAULTS : Sets colors to defaults
 *  - LOAD_COLORS : Loads colors from registry
 * (No save operation because BrowseColorAndSave saves immediately when user chooses)
 */
void PropListColors::UpdateScreen()
{
	CheckDlgButton(IDC_DEFAULT_STANDARD_COLORS, m_bCustomColors);
	EnableColorButtons(m_bCustomColors);
	SerializeColors(INVALIDATE);
}

/** 
 * @brief Set colors to track standard (theme) colors
 */
void PropListColors::OnDefaultsStandardColors()
{
	SerializeColors(SET_DEFAULTS);
	UpdateScreen();
}

/** 
 * @brief Enable / disable color controls on dialog.
 * @param [in] bEnable If TRUE color controls are enabled.
 */
void PropListColors::EnableColorButtons(BOOL bEnable)
{
	GetDlgItem(IDC_CUSTOM_COLORS_GROUP)->EnableWindow(bEnable);
	GetDlgItem(IDC_LEFTONLY_COLOR_LABEL)->EnableWindow(bEnable);
	GetDlgItem(IDC_RIGHTONLY_COLOR_LABEL)->EnableWindow(bEnable);
	GetDlgItem(IDC_SUSPICIOUS_COLOR_LABEL)->EnableWindow(bEnable);
	GetDlgItem(IDC_BACKGROUND_COLUMN_LABEL)->EnableWindow(bEnable);
	GetDlgItem(IDC_LIST_LEFTONLY_BKGD_COLOR)->EnableWindow(bEnable);
	GetDlgItem(IDC_LIST_RIGHTONLY_BKGD_COLOR)->EnableWindow(bEnable);
	GetDlgItem(IDC_LIST_SUSPICIOUS_BKGD_COLOR)->EnableWindow(bEnable);
}

void PropListColors::SerializeColors(OPERATION op)
{
	SerializeColor(op, IDC_LIST_LEFTONLY_BKGD_COLOR, OPT_LIST_LEFTONLY_BKGD_COLOR, m_clrLeftOnly);
	SerializeColor(op, IDC_LIST_RIGHTONLY_BKGD_COLOR, OPT_LIST_RIGHTONLY_BKGD_COLOR, m_clrRightOnly);
	SerializeColor(op, IDC_LIST_SUSPICIOUS_BKGD_COLOR, OPT_LIST_SUSPICIOUS_BKGD_COLOR, m_clrSuspicious);
}
