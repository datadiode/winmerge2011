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
OptionsPanel::OptionsPanel(UINT nIDTemplate, size_t cb)
: ZeroInit(cb)
, ODialog(nIDTemplate)
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

UINT OptionsPanel::ValidateNumber(HEdit *edit, UINT uMin, UINT uMax)
{
	const int nIDDlgItem = edit->GetDlgCtrlID();
	const UINT u = GetDlgItemInt(nIDDlgItem, NULL, FALSE);
	const UINT v = u < uMin ? uMin : u > uMax ? uMax : u;
	if (v != u)
	{
		if (edit->GetModify())
			MessageBeep(0);
		int sel = edit->GetSel();
		SetDlgItemInt(nIDDlgItem, v, FALSE);
		int len = edit->GetWindowTextLength();
		if (sel != 0)
			sel = len;
		edit->SetSel(sel, len);
	}
	return v;
}
