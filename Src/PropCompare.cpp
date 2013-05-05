/** 
 * @file  PropCompare.cpp
 *
 * @brief Implementation of PropCompare propertysheet
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "OptionsPanel.h"
#include "resource.h"
#include "PropCompare.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/** 
 * @brief Constructor.
 */
PropCompare::PropCompare()
: OptionsPanel(IDD_PROPPAGE_COMPARE, sizeof *this)
{
}

template<ODialog::DDX_Operation op>
bool PropCompare::UpdateData()
{
	DDX_Check<op>(IDC_IGNCASE_CHECK, m_bIgnoreCase);
	DDX_Check<op>(IDC_IGNBLANKS_CHECK, m_bIgnoreBlankLines);
	DDX_Check<op>(IDC_FILTERCOMMENTS_CHECK, m_bFilterCommentsLines);
	DDX_Check<op>(IDC_EOL_SENSITIVE, m_bIgnoreEol);
	DDX_Check<op>(IDC_WHITESPACE, m_nIgnoreWhite, 0);
	DDX_Check<op>(IDC_WHITE_CHANGE, m_nIgnoreWhite, 1);
	DDX_Check<op>(IDC_ALL_WHITE, m_nIgnoreWhite, 2);
	DDX_Check<op>(IDC_MOVED_BLOCKS, m_bMovedBlocks);
	DDX_Check<op>(IDC_MATCH_SIMILAR_LINES, m_bMatchSimilarLines);
	DDX_Text<op>(IDC_MATCH_SIMILAR_LINES_MAX, m_nMatchSimilarLinesMax);
	if (HEdit *pEdit = static_cast<HEdit *>(GetDlgItem(IDC_MATCH_SIMILAR_LINES_MAX)))
	{
		pEdit->EnableWindow(m_bMatchSimilarLines);
		pEdit->LimitText(3);
	}
	return true;
}

LRESULT PropCompare::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		if (IsUserInputCommand(wParam))
			UpdateData<Get>();
		switch (wParam)
		{
		case MAKEWPARAM(IDC_COMPARE_DEFAULTS, BN_CLICKED):
			OnDefaults();
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
void PropCompare::ReadOptions()
{
	m_nIgnoreWhite = COptionsMgr::Get(OPT_CMP_IGNORE_WHITESPACE);
	m_bIgnoreBlankLines = COptionsMgr::Get(OPT_CMP_IGNORE_BLANKLINES);
	m_bFilterCommentsLines = COptionsMgr::Get(OPT_CMP_FILTER_COMMENTLINES);
	m_bIgnoreCase = COptionsMgr::Get(OPT_CMP_IGNORE_CASE);
	m_bIgnoreEol = COptionsMgr::Get(OPT_CMP_IGNORE_EOL);
	m_bMovedBlocks = COptionsMgr::Get(OPT_CMP_MOVED_BLOCKS);
	m_bMatchSimilarLines = COptionsMgr::Get(OPT_CMP_MATCH_SIMILAR_LINES);
	m_nMatchSimilarLinesMax = COptionsMgr::Get(OPT_CMP_MATCH_SIMILAR_LINES_MAX);
}

/** 
 * @brief Writes options values from UI to storage.
 * Property sheet calls this after dialog is closed with OK button to
 * store values in member variables.
 */
void PropCompare::WriteOptions()
{
	COptionsMgr::SaveOption(OPT_CMP_IGNORE_WHITESPACE, m_nIgnoreWhite);
	COptionsMgr::SaveOption(OPT_CMP_IGNORE_BLANKLINES, m_bIgnoreBlankLines != FALSE);
	COptionsMgr::SaveOption(OPT_CMP_FILTER_COMMENTLINES, m_bFilterCommentsLines != FALSE);
	COptionsMgr::SaveOption(OPT_CMP_IGNORE_EOL, m_bIgnoreEol != FALSE);
	COptionsMgr::SaveOption(OPT_CMP_IGNORE_CASE, m_bIgnoreCase != FALSE);
	COptionsMgr::SaveOption(OPT_CMP_MOVED_BLOCKS, m_bMovedBlocks != FALSE);
	COptionsMgr::SaveOption(OPT_CMP_MATCH_SIMILAR_LINES, m_bMatchSimilarLines != FALSE);
	COptionsMgr::SaveOption(OPT_CMP_MATCH_SIMILAR_LINES_MAX, m_nMatchSimilarLinesMax);
}

/** 
 * @brief Called before propertysheet is drawn.
 */
void PropCompare::UpdateScreen()
{
	UpdateData<Set>();
}

/** 
 * @brief Sets options to defaults
 */
void PropCompare::OnDefaults()
{
	m_nIgnoreWhite = COptionsMgr::GetDefault(OPT_CMP_IGNORE_WHITESPACE);
	m_bIgnoreEol = COptionsMgr::GetDefault(OPT_CMP_IGNORE_EOL);
	m_bIgnoreBlankLines = COptionsMgr::GetDefault(OPT_CMP_IGNORE_BLANKLINES);
	m_bFilterCommentsLines = COptionsMgr::GetDefault(OPT_CMP_FILTER_COMMENTLINES);
	m_bIgnoreCase = COptionsMgr::GetDefault(OPT_CMP_IGNORE_CASE);
	m_bMovedBlocks = COptionsMgr::GetDefault(OPT_CMP_MOVED_BLOCKS);
	m_bMatchSimilarLines = COptionsMgr::GetDefault(OPT_CMP_MATCH_SIMILAR_LINES);
	UpdateData<Set>();
}
