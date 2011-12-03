/** 
 * @file  OptionsPanel.cpp
 *
 * @brief Implementation of OptionsPanel class.
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "OptionsMgr.h"
#include "OptionsPanel.h"
#include "LanguageSelect.h"

/**
 * @brief Constructor.
 */
OptionsPanel::OptionsPanel(UINT nIDTemplate)
: ODialog(nIDTemplate)
, m_nPageIndex(0)
{
}

BOOL OptionsPanel::OnInitDialog()
{
	ODialog::OnInitDialog();
	LanguageSelect.TranslateDialog(m_hWnd);
	UpdateScreen();
	return TRUE;
}

BOOL OptionsPanel::IsUserInputCommand(WPARAM wParam)
{
	UINT id = LOWORD(wParam);
	UINT code = HIWORD(wParam);
	UINT dlgcode = (UINT)SendDlgItemMessage(id, WM_GETDLGCODE);
	if (dlgcode & DLGC_HASSETSEL)
		return code == EN_CHANGE && SendDlgItemMessage(id, EM_GETMODIFY) != 0;
	if (dlgcode & DLGC_BUTTON)
		return code == BN_CLICKED;
	if (dlgcode & DLGC_WANTARROWS)
	{
		switch (code)
		{
		case CBN_SELCHANGE:
			// force the selected item's text into the edit control
			SendDlgItemMessage(id, CB_SETCURSEL, SendDlgItemMessage(id, CB_GETCURSEL));
			// fall through
		case CBN_EDITCHANGE:
			return TRUE;
		}
	}
	return FALSE;
}
