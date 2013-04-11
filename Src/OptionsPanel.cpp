/** 
 * @file  OptionsPanel.cpp
 *
 * @brief Implementation of OptionsPanel class.
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "OptionsPanel.h"
#include "LanguageSelect.h"
#include "SyntaxColors.h"

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

/** 
 * @brief Let user browse common color dialog, and select a color
 */
void OptionsPanel::BrowseColor(int id, COLORREF &currentColor)
{
	if (SyntaxColors.ChooseColor(m_hWnd, id))
		currentColor = GetDlgItemInt(id);
}

void OptionsPanel::SerializeColor(OPERATION op, int id, COptionDef<COLORREF> &optionName, COLORREF &color)
{
	switch (op)
	{
	case SET_DEFAULTS:
		color = COptionsMgr::GetDefault(optionName);
		break;

	case WRITE_OPTIONS:
		COptionsMgr::SaveOption(optionName, color);
		break;

	case READ_OPTIONS:
		color = COptionsMgr::Get(optionName);
		break;

	case INVALIDATE:
		SetDlgItemInt(id, color);
		break;
	}
}
