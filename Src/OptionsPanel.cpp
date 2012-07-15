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
