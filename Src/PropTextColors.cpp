/** 
 * @file  PropTextColors.cpp
 *
 * @brief Implementation of PropTextColors propertysheet
 */
#include "StdAfx.h"
#include "OptionsPanel.h"
#include "resource.h"
#include "SyntaxColors.h"
#include "PropTextColors.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/** 
 * @brief Default constructor.
 */
PropTextColors::PropTextColors()
: OptionsPanel(IDD_PROPPAGE_COLORS_TEXT, sizeof *this)
{
}

LRESULT PropTextColors::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
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
		case MAKEWPARAM(IDC_WHITESPACE_BKGD_COLOR, BN_CLICKED):
			BrowseColorAndSave(IDC_WHITESPACE_BKGD_COLOR, COLORINDEX_WHITESPACE);
			break;
		case MAKEWPARAM(IDC_REGULAR_BKGD_COLOR, BN_CLICKED):
			BrowseColorAndSave(IDC_REGULAR_BKGD_COLOR, COLORINDEX_BKGND);
			break;
		case MAKEWPARAM(IDC_REGULAR_TEXT_COLOR, BN_CLICKED):
			BrowseColorAndSave(IDC_REGULAR_TEXT_COLOR, COLORINDEX_NORMALTEXT);
			break;
		case MAKEWPARAM(IDC_SELECTION_BKGD_COLOR, BN_CLICKED):
			BrowseColorAndSave(IDC_SELECTION_BKGD_COLOR, COLORINDEX_SELBKGND);
			break;
		case MAKEWPARAM(IDC_SELECTION_TEXT_COLOR, BN_CLICKED):
			BrowseColorAndSave(IDC_SELECTION_TEXT_COLOR, COLORINDEX_SELTEXT);
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
void PropTextColors::ReadOptions()
{
	m_bCustomColors = COptionsMgr::Get(OPT_CLR_DEFAULT_TEXT_COLORING) ? FALSE : TRUE;
}

/** 
 * @brief Writes options values from UI to storage.
 * (Property sheet calls this after displaying all property pages)
 */
void PropTextColors::WriteOptions()
{
	COptionsMgr::SaveOption(OPT_CLR_DEFAULT_TEXT_COLORING, m_bCustomColors == FALSE);
}

/** 
 * @brief Let user browse common color dialog, and select a color
 * @param [in] colorButton Button for which to change color.
 * @param [in] colorIndex Index to color table.
 */
void PropTextColors::BrowseColorAndSave(int id, int colorIndex)
{
	if (SyntaxColors.ChooseColor(m_hWnd, id))
		SyntaxColors.SetColor(colorIndex, GetDlgItemInt(id));
}

/**
 * @brief Load all colors, Save all colors, or set all colors to default
 * @param [in] op Operation to do, one of
 *  - SET_DEFAULTS : Sets colors to defaults
 *  - LOAD_COLORS : Loads colors from registry
 * (No save operation because BrowseColorAndSave saves immediately when user chooses)
 */
void PropTextColors::UpdateScreen()
{
	CheckDlgButton(IDC_DEFAULT_STANDARD_COLORS, m_bCustomColors);
	EnableColorButtons(m_bCustomColors);
	SetDlgItemInt(IDC_WHITESPACE_BKGD_COLOR, SyntaxColors.GetColor(COLORINDEX_WHITESPACE));
	SetDlgItemInt(IDC_REGULAR_BKGD_COLOR, SyntaxColors.GetColor(COLORINDEX_BKGND));
	SetDlgItemInt(IDC_REGULAR_TEXT_COLOR, SyntaxColors.GetColor(COLORINDEX_NORMALTEXT));
	SetDlgItemInt(IDC_SELECTION_BKGD_COLOR, SyntaxColors.GetColor(COLORINDEX_SELBKGND));
	SetDlgItemInt(IDC_SELECTION_TEXT_COLOR, SyntaxColors.GetColor(COLORINDEX_SELTEXT));
}

/** 
 * @brief Set colors to track standard (theme) colors
 */
void PropTextColors::OnDefaultsStandardColors()
{
	if (!m_bCustomColors)
	{
		// Reset all text colors to default every time user checks defaults button
		SyntaxColors.SetDefault(COLORINDEX_WHITESPACE);
		SyntaxColors.SetDefault(COLORINDEX_BKGND);
		SyntaxColors.SetDefault(COLORINDEX_NORMALTEXT);
		SyntaxColors.SetDefault(COLORINDEX_SELBKGND);
		SyntaxColors.SetDefault(COLORINDEX_SELTEXT);
	}
	UpdateScreen();
}

/** 
 * @brief Enable / disable color controls on dialog.
 * @param [in] bEnable If TRUE color controls are enabled.
 */
void PropTextColors::EnableColorButtons(BOOL bEnable)
{
	GetDlgItem(IDC_CUSTOM_COLORS_GROUP)->EnableWindow(bEnable);
	GetDlgItem(IDC_WHITESPACE_COLOR_LABEL)->EnableWindow(bEnable);
	GetDlgItem(IDC_TEXT_COLOR_LABEL)->EnableWindow(bEnable);
	GetDlgItem(IDC_SELECTION_COLOR_LABEL)->EnableWindow(bEnable);
	GetDlgItem(IDC_BACKGROUND_COLUMN_LABEL)->EnableWindow(bEnable);
	GetDlgItem(IDC_TEXT_COLUMN_LABEL)->EnableWindow(bEnable);
	GetDlgItem(IDC_WHITESPACE_BKGD_COLOR)->EnableWindow(bEnable);
	GetDlgItem(IDC_REGULAR_BKGD_COLOR)->EnableWindow(bEnable);
	GetDlgItem(IDC_REGULAR_TEXT_COLOR)->EnableWindow(bEnable);
	GetDlgItem(IDC_SELECTION_BKGD_COLOR)->EnableWindow(bEnable);
	GetDlgItem(IDC_SELECTION_TEXT_COLOR)->EnableWindow(bEnable);
}
