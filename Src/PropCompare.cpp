/** 
 * @file  PropCompare.cpp
 *
 * @brief Implementation of PropCompare propertysheet
 */
#include "StdAfx.h"
#include "OptionsPanel.h"
#include "resource.h"
#include "PropCompare.h"
#include "CompareOptions.h"

#ifdef _DEBUG
#define new DEBUG_NEW
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
	DDX_Check<op>(IDC_DIFF_IGNORECASE, m_bIgnoreCase);
	DDX_Check<op>(IDC_DIFF_IGNORE_BLANK_LINES, m_bIgnoreBlankLines);
	DDX_Check<op>(IDC_FILTERCOMMENTS_CHECK, m_bFilterCommentsLines);
	DDX_Check<op>(IDC_DIFF_IGNOREEOL, m_bIgnoreEol);
	DDX_Check<op>(IDC_DIFF_WS_IGNORE_TAB_EXPANSION, m_bIgnoreTabExpansion);
	DDX_Check<op>(IDC_DIFF_WS_IGNORE_TRAILING_SPACE, m_bIgnoreTrailingSpace);
	DDX_Check<op>(IDC_DIFF_WHITESPACE_COMPARE, m_nIgnoreWhite, 0);
	DDX_Check<op>(IDC_DIFF_WHITESPACE_IGNORE, m_nIgnoreWhite, 1);
	DDX_Check<op>(IDC_DIFF_WHITESPACE_IGNOREALL, m_nIgnoreWhite, 2);
	DDX_Check<op>(IDC_DIFF_MINIMAL, m_bMinimal);
	DDX_Check<op>(IDC_DIFF_SPEED_LARGE_FILES, m_bSpeedLargeFiles);
	DDX_Check<op>(IDC_DIFF_APPLY_HISTORIC_COST_LIMIT, m_bApplyHistoricCostLimit);
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
		case MAKEWPARAM(IDC_DIFF_WS_IGNORE_TAB_EXPANSION, BN_CLICKED):
		case MAKEWPARAM(IDC_DIFF_WS_IGNORE_TRAILING_SPACE, BN_CLICKED):
			UpdateScreen();
			break;
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
	// NB: OPT_CMP_IGNORE_WHITESPACE combines radio options with check options!
	m_nIgnoreWhite = COptionsMgr::Get(OPT_CMP_IGNORE_WHITESPACE);
	m_bIgnoreTabExpansion = (m_nIgnoreWhite & WHITESPACE_IGNORE_TAB_EXPANSION) != 0;
	m_bIgnoreTrailingSpace = (m_nIgnoreWhite & WHITESPACE_IGNORE_TRAILING_SPACE) != 0;
	m_nIgnoreWhite &= WHITESPACE_RADIO_OPTIONS_MASK;
	m_bMinimal = COptionsMgr::Get(OPT_CMP_MINIMAL);
	m_bSpeedLargeFiles = COptionsMgr::Get(OPT_CMP_SPEED_LARGE_FILES);
	m_bApplyHistoricCostLimit = COptionsMgr::Get(OPT_CMP_APPLY_HISTORIC_COST_LIMIT);
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
	// NB: OPT_CMP_IGNORE_WHITESPACE combines radio options with check options!
	COptionsMgr::SaveOption(OPT_CMP_IGNORE_WHITESPACE, m_nIgnoreWhite |
		(m_bIgnoreTabExpansion ? WHITESPACE_IGNORE_TAB_EXPANSION : 0) |
		(m_bIgnoreTrailingSpace ? WHITESPACE_IGNORE_TRAILING_SPACE : 0));
	COptionsMgr::SaveOption(OPT_CMP_MINIMAL, m_bMinimal != FALSE);
	COptionsMgr::SaveOption(OPT_CMP_SPEED_LARGE_FILES, m_bSpeedLargeFiles != FALSE);
	COptionsMgr::SaveOption(OPT_CMP_APPLY_HISTORIC_COST_LIMIT, m_bApplyHistoricCostLimit != FALSE);
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
	// When using any of the whitespace check options, the radio options are N/A
	BOOL bEnable = !(m_bIgnoreTabExpansion || m_bIgnoreTrailingSpace);
	GetDlgItem(IDC_DIFF_WHITESPACE_COMPARE)->EnableWindow(bEnable);
	GetDlgItem(IDC_DIFF_WHITESPACE_IGNORE)->EnableWindow(bEnable);
	GetDlgItem(IDC_DIFF_WHITESPACE_IGNOREALL)->EnableWindow(bEnable);
}

/** 
 * @brief Sets options to defaults
 */
void PropCompare::OnDefaults()
{
	m_nIgnoreWhite = COptionsMgr::GetDefault(OPT_CMP_IGNORE_WHITESPACE);
	m_bIgnoreTabExpansion = (m_nIgnoreWhite & WHITESPACE_IGNORE_TAB_EXPANSION) != 0;
	m_bIgnoreTrailingSpace = (m_nIgnoreWhite & WHITESPACE_IGNORE_TRAILING_SPACE) != 0;
	m_nIgnoreWhite &= WHITESPACE_RADIO_OPTIONS_MASK;
	m_bMinimal = COptionsMgr::GetDefault(OPT_CMP_MINIMAL);
	m_bSpeedLargeFiles = COptionsMgr::GetDefault(OPT_CMP_SPEED_LARGE_FILES);
	m_bApplyHistoricCostLimit = COptionsMgr::GetDefault(OPT_CMP_APPLY_HISTORIC_COST_LIMIT);
	m_bIgnoreEol = COptionsMgr::GetDefault(OPT_CMP_IGNORE_EOL);
	m_bIgnoreBlankLines = COptionsMgr::GetDefault(OPT_CMP_IGNORE_BLANKLINES);
	m_bFilterCommentsLines = COptionsMgr::GetDefault(OPT_CMP_FILTER_COMMENTLINES);
	m_bIgnoreCase = COptionsMgr::GetDefault(OPT_CMP_IGNORE_CASE);
	m_bMovedBlocks = COptionsMgr::GetDefault(OPT_CMP_MOVED_BLOCKS);
	m_bMatchSimilarLines = COptionsMgr::GetDefault(OPT_CMP_MATCH_SIMILAR_LINES);
	UpdateData<Set>();
}
