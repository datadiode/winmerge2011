/** 
 * @file  PropSyntaxColors.cpp
 *
 * @brief Implementation of PropSyntaxColors propertysheet
 */
#include "StdAfx.h"
#include "OptionsPanel.h"
#include "resource.h"
#include "SyntaxColors.h"
#include "PropSyntaxColors.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

PropSyntaxColors::PropSyntaxColors()
: OptionsPanel(IDD_PROPPAGE_COLORS_SYNTAX, sizeof *this)
{
}

LRESULT PropSyntaxColors::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case MAKEWPARAM(IDC_SCOLORS_BDEFAULTS, BN_CLICKED):
			SyntaxColors.SetDefault(COLORINDEX_KEYWORD);
			SyntaxColors.SetDefault(COLORINDEX_FUNCNAME);
			SyntaxColors.SetDefault(COLORINDEX_COMMENT);
			SyntaxColors.SetDefault(COLORINDEX_NUMBER);
			SyntaxColors.SetDefault(COLORINDEX_OPERATOR);
			SyntaxColors.SetDefault(COLORINDEX_STRING);
			SyntaxColors.SetDefault(COLORINDEX_PREPROCESSOR);
			SyntaxColors.SetDefault(COLORINDEX_USER1);
			SyntaxColors.SetDefault(COLORINDEX_USER2);
			UpdateScreen();
			break;
		case MAKEWPARAM(IDC_SCOLOR_KEYWORDS, BN_CLICKED):
			BrowseColorAndSave(IDC_SCOLOR_KEYWORDS, COLORINDEX_KEYWORD);
			break;
		case MAKEWPARAM(IDC_SCOLOR_FUNCTIONS, BN_CLICKED):
			BrowseColorAndSave(IDC_SCOLOR_FUNCTIONS, COLORINDEX_FUNCNAME);
			break;
		case MAKEWPARAM(IDC_SCOLOR_COMMENTS, BN_CLICKED):
			BrowseColorAndSave(IDC_SCOLOR_COMMENTS, COLORINDEX_COMMENT);
			break;
		case MAKEWPARAM(IDC_SCOLOR_NUMBERS, BN_CLICKED):
			BrowseColorAndSave(IDC_SCOLOR_NUMBERS, COLORINDEX_NUMBER);
			break;
		case MAKEWPARAM(IDC_SCOLOR_OPERATORS, BN_CLICKED):
			BrowseColorAndSave(IDC_SCOLOR_OPERATORS, COLORINDEX_OPERATOR);
			break;
		case MAKEWPARAM(IDC_SCOLOR_STRINGS, BN_CLICKED):
			BrowseColorAndSave(IDC_SCOLOR_STRINGS, COLORINDEX_STRING);
			break;
		case MAKEWPARAM(IDC_SCOLOR_PREPROCESSOR, BN_CLICKED):
			BrowseColorAndSave(IDC_SCOLOR_PREPROCESSOR, COLORINDEX_PREPROCESSOR);
			break;
		case MAKEWPARAM(IDC_SCOLOR_USER1, BN_CLICKED):
			BrowseColorAndSave(IDC_SCOLOR_USER1, COLORINDEX_USER1);
			break;
		case MAKEWPARAM(IDC_SCOLOR_USER2, BN_CLICKED):
			BrowseColorAndSave(IDC_SCOLOR_USER2, COLORINDEX_USER2);
			break;
		case MAKEWPARAM(IDC_SCOLOR_KEYWORDS_BOLD, BN_CLICKED):
			UpdateBoldStatus(IDC_SCOLOR_KEYWORDS_BOLD, COLORINDEX_KEYWORD);
			break;
		case MAKEWPARAM(IDC_SCOLOR_FUNCTIONS_BOLD, BN_CLICKED):
			UpdateBoldStatus(IDC_SCOLOR_FUNCTIONS_BOLD, COLORINDEX_FUNCNAME);
			break;
		case MAKEWPARAM(IDC_SCOLOR_COMMENTS_BOLD, BN_CLICKED):
			UpdateBoldStatus(IDC_SCOLOR_COMMENTS_BOLD, COLORINDEX_COMMENT);
			break;
		case MAKEWPARAM(IDC_SCOLOR_NUMBERS_BOLD, BN_CLICKED):
			UpdateBoldStatus(IDC_SCOLOR_NUMBERS_BOLD, COLORINDEX_NUMBER);
			break;
		case MAKEWPARAM(IDC_SCOLOR_OPERATORS_BOLD, BN_CLICKED):
			UpdateBoldStatus(IDC_SCOLOR_OPERATORS_BOLD, COLORINDEX_OPERATOR);
			break;
		case MAKEWPARAM(IDC_SCOLOR_STRINGS_BOLD, BN_CLICKED):
			UpdateBoldStatus(IDC_SCOLOR_STRINGS_BOLD, COLORINDEX_STRING);
			break;
		case MAKEWPARAM(IDC_SCOLOR_PREPROCESSOR_BOLD, BN_CLICKED):
			UpdateBoldStatus(IDC_SCOLOR_PREPROCESSOR_BOLD, COLORINDEX_PREPROCESSOR);
			break;
		case MAKEWPARAM(IDC_SCOLOR_USER1_BOLD, BN_CLICKED):
			UpdateBoldStatus(IDC_SCOLOR_USER1_BOLD, COLORINDEX_USER1);
			break;
		case MAKEWPARAM(IDC_SCOLOR_USER2_BOLD, BN_CLICKED):
			UpdateBoldStatus(IDC_SCOLOR_USER2_BOLD, COLORINDEX_USER2);
			break;
		}
		break;
	case WM_DRAWITEM:
		return MessageReflect_ColorButton<WM_DRAWITEM>(wParam, lParam);
	}
	return OptionsPanel::WindowProc(message, wParam, lParam);
}

/** 
 * @brief Let user browse common color dialog, and select a color & save to registry
 */
void PropSyntaxColors::BrowseColorAndSave(int id, int colorIndex)
{
	if (SyntaxColors.ChooseColor(m_hWnd, id))
		SyntaxColors.SetColor(colorIndex, GetDlgItemInt(id));
}

void PropSyntaxColors::UpdateScreen()
{
	SetDlgItemInt(IDC_SCOLOR_KEYWORDS, SyntaxColors.GetColor(COLORINDEX_KEYWORD));
	SetDlgItemInt(IDC_SCOLOR_FUNCTIONS, SyntaxColors.GetColor(COLORINDEX_FUNCNAME));
	SetDlgItemInt(IDC_SCOLOR_COMMENTS, SyntaxColors.GetColor(COLORINDEX_COMMENT));
	SetDlgItemInt(IDC_SCOLOR_NUMBERS, SyntaxColors.GetColor(COLORINDEX_NUMBER));
	SetDlgItemInt(IDC_SCOLOR_OPERATORS, SyntaxColors.GetColor(COLORINDEX_OPERATOR));
	SetDlgItemInt(IDC_SCOLOR_STRINGS, SyntaxColors.GetColor(COLORINDEX_STRING));
	SetDlgItemInt(IDC_SCOLOR_PREPROCESSOR, SyntaxColors.GetColor(COLORINDEX_PREPROCESSOR));
	SetDlgItemInt(IDC_SCOLOR_USER1, SyntaxColors.GetColor(COLORINDEX_USER1));
	SetDlgItemInt(IDC_SCOLOR_USER2, SyntaxColors.GetColor(COLORINDEX_USER2));
	CheckDlgButton(IDC_SCOLOR_KEYWORDS_BOLD, SyntaxColors.GetBold(COLORINDEX_KEYWORD));
	CheckDlgButton(IDC_SCOLOR_FUNCTIONS_BOLD, SyntaxColors.GetBold(COLORINDEX_FUNCNAME));
	CheckDlgButton(IDC_SCOLOR_COMMENTS_BOLD, SyntaxColors.GetBold(COLORINDEX_COMMENT));
	CheckDlgButton(IDC_SCOLOR_NUMBERS_BOLD, SyntaxColors.GetBold(COLORINDEX_NUMBER));
	CheckDlgButton(IDC_SCOLOR_OPERATORS_BOLD, SyntaxColors.GetBold(COLORINDEX_OPERATOR));
	CheckDlgButton(IDC_SCOLOR_STRINGS_BOLD, SyntaxColors.GetBold(COLORINDEX_STRING));
	CheckDlgButton(IDC_SCOLOR_PREPROCESSOR_BOLD, SyntaxColors.GetBold(COLORINDEX_PREPROCESSOR));
	CheckDlgButton(IDC_SCOLOR_USER1_BOLD, SyntaxColors.GetBold(COLORINDEX_USER1));
	CheckDlgButton(IDC_SCOLOR_USER2_BOLD, SyntaxColors.GetBold(COLORINDEX_USER2));
}

int PropSyntaxColors::GetCheckVal(UINT nColorIndex)
{
	return SyntaxColors.GetBold(nColorIndex) ? BST_CHECKED : BST_UNCHECKED;
}

void PropSyntaxColors::UpdateBoldStatus(int id, UINT colorIndex)
{
	SyntaxColors.SetBold(colorIndex, IsDlgButtonChecked(id) == BST_CHECKED);
}
