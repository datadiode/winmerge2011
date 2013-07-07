/** 
 * @file  PropColors.cpp
 *
 * @brief Implementation of PropMergeColors propertysheet
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "OptionsPanel.h"
#include "resource.h"
#include "SyntaxColors.h"
#include "PropColors.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/** 
 * @brief Default constructor.
 */
PropMergeColors::PropMergeColors()
: OptionsPanel(IDD_PROPPAGE_COLORS_WINMERGE, sizeof *this)
{
}

LRESULT PropMergeColors::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case MAKEWPARAM(IDC_COLORDEFAULTS_BTN, BN_CLICKED):
			SerializeColors(SET_DEFAULTS);
			SerializeColors(INVALIDATE);
			break;
		case MAKEWPARAM(IDC_DIFFERENCE_COLOR, BN_CLICKED):
			BrowseColor(IDC_DIFFERENCE_COLOR, m_clrDiff);
			break;
		case MAKEWPARAM(IDC_SEL_DIFFERENCE_COLOR, BN_CLICKED):
			BrowseColor(IDC_SEL_DIFFERENCE_COLOR, m_clrSelDiff);
			break;
		case MAKEWPARAM(IDC_DIFFERENCE_DELETED_COLOR, BN_CLICKED):
			BrowseColor(IDC_DIFFERENCE_DELETED_COLOR, m_clrDiffDeleted);
			break;
		case MAKEWPARAM(IDC_SEL_DIFFERENCE_DELETED_COLOR, BN_CLICKED):
			BrowseColor(IDC_SEL_DIFFERENCE_DELETED_COLOR, m_clrSelDiffDeleted);
			break;
		case MAKEWPARAM(IDC_DIFFERENCE_TEXT_COLOR, BN_CLICKED):
			BrowseColor(IDC_DIFFERENCE_TEXT_COLOR, m_clrDiffText);
			break;
		case MAKEWPARAM(IDC_SEL_DIFFERENCE_TEXT_COLOR, BN_CLICKED):
			BrowseColor(IDC_SEL_DIFFERENCE_TEXT_COLOR, m_clrSelDiffText);
			break;
		case MAKEWPARAM(IDC_TRIVIAL_DIFF_COLOR, BN_CLICKED):
			BrowseColor(IDC_TRIVIAL_DIFF_COLOR, m_clrTrivial);
			break;
		case MAKEWPARAM(IDC_TRIVIAL_DIFF_DELETED_COLOR, BN_CLICKED):
			BrowseColor(IDC_TRIVIAL_DIFF_DELETED_COLOR, m_clrTrivialDeleted);
			break;
		case MAKEWPARAM(IDC_TRIVIAL_DIFF_TEXT_COLOR, BN_CLICKED):
			BrowseColor(IDC_TRIVIAL_DIFF_TEXT_COLOR, m_clrTrivialText);
			break;
		case MAKEWPARAM(IDC_MOVEDBLOCK_COLOR, BN_CLICKED):
			BrowseColor(IDC_MOVEDBLOCK_COLOR, m_clrMoved);
			break;
		case MAKEWPARAM(IDC_MOVEDBLOCK_DELETED_COLOR, BN_CLICKED):
			BrowseColor(IDC_MOVEDBLOCK_DELETED_COLOR, m_clrMovedDeleted);
			break;
		case MAKEWPARAM(IDC_MOVEDBLOCK_TEXT_COLOR, BN_CLICKED):
			BrowseColor(IDC_MOVEDBLOCK_TEXT_COLOR, m_clrMovedText);
			break;
		case MAKEWPARAM(IDC_SEL_MOVEDBLOCK_COLOR, BN_CLICKED):
			BrowseColor(IDC_SEL_MOVEDBLOCK_COLOR, m_clrSelMoved);
			break;
		case MAKEWPARAM(IDC_SEL_MOVEDBLOCK_DELETED_COLOR, BN_CLICKED):
			BrowseColor(IDC_SEL_MOVEDBLOCK_DELETED_COLOR, m_clrSelMovedDeleted);
			break;
		case MAKEWPARAM(IDC_SEL_MOVEDBLOCK_TEXT_COLOR, BN_CLICKED):
			BrowseColor(IDC_SEL_MOVEDBLOCK_TEXT_COLOR, m_clrSelMovedText);
			break;
		case MAKEWPARAM(IDC_WORDDIFF_COLOR, BN_CLICKED):
			BrowseColor(IDC_WORDDIFF_COLOR, m_clrWordDiff);
			break;
		case MAKEWPARAM(IDC_WORDDIFF_DELETED_COLOR, BN_CLICKED):
			BrowseColor(IDC_WORDDIFF_DELETED_COLOR, m_clrWordDiffDeleted);
			break;
		case MAKEWPARAM(IDC_SEL_WORDDIFF_COLOR, BN_CLICKED):
			BrowseColor(IDC_SEL_WORDDIFF_COLOR, m_clrSelWordDiff);
			break;
		case MAKEWPARAM(IDC_SEL_WORDDIFF_DELETED_COLOR, BN_CLICKED):
			BrowseColor(IDC_SEL_WORDDIFF_DELETED_COLOR, m_clrSelWordDiffDeleted);
			break;
		case MAKEWPARAM(IDC_WORDDIFF_TEXT_COLOR, BN_CLICKED):
			BrowseColor(IDC_WORDDIFF_TEXT_COLOR, m_clrWordDiffText);
			break;
		case MAKEWPARAM(IDC_SEL_WORDDIFF_TEXT_COLOR, BN_CLICKED):
			BrowseColor(IDC_SEL_WORDDIFF_TEXT_COLOR, m_clrSelWordDiffText);
			break;
		case MAKEWPARAM(IDC_CROSS_HATCH_DELETED_LINES, BN_CLICKED):
			m_bCrossHatchDeletedLines = IsDlgButtonChecked(IDC_CROSS_HATCH_DELETED_LINES);
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
void PropMergeColors::ReadOptions()
{
	SerializeColors(READ_OPTIONS);
	m_bCrossHatchDeletedLines = COptionsMgr::Get(OPT_CROSS_HATCH_DELETED_LINES);
}

void PropMergeColors::UpdateScreen()
{
	SerializeColors(INVALIDATE);
	CheckDlgButton(IDC_CROSS_HATCH_DELETED_LINES, m_bCrossHatchDeletedLines);
}

/** 
 * @brief Writes options values from UI to storage.
 * (Property sheet calls this after displaying all property pages)
 */
void PropMergeColors::WriteOptions()
{
	SerializeColors(WRITE_OPTIONS);
	COptionsMgr::SaveOption(OPT_CROSS_HATCH_DELETED_LINES, m_bCrossHatchDeletedLines != FALSE);
}

void PropMergeColors::SerializeColors(OPERATION op)
{
	SerializeColor(op, IDC_DIFFERENCE_COLOR, OPT_CLR_DIFF, m_clrDiff);
	SerializeColor(op, IDC_DIFFERENCE_DELETED_COLOR, OPT_CLR_DIFF_DELETED, m_clrDiffDeleted);
	SerializeColor(op, IDC_SEL_DIFFERENCE_COLOR, OPT_CLR_SELECTED_DIFF, m_clrSelDiff);

	SerializeColor(op, IDC_DIFFERENCE_TEXT_COLOR, OPT_CLR_DIFF_TEXT, m_clrDiffText);
	SerializeColor(op, IDC_SEL_DIFFERENCE_DELETED_COLOR, OPT_CLR_SELECTED_DIFF_DELETED, m_clrSelDiffDeleted);
	SerializeColor(op, IDC_SEL_DIFFERENCE_TEXT_COLOR, OPT_CLR_SELECTED_DIFF_TEXT, m_clrSelDiffText);

	SerializeColor(op, IDC_TRIVIAL_DIFF_COLOR, OPT_CLR_TRIVIAL_DIFF, m_clrTrivial);
	SerializeColor(op, IDC_TRIVIAL_DIFF_DELETED_COLOR, OPT_CLR_TRIVIAL_DIFF_DELETED, m_clrTrivialDeleted);
	SerializeColor(op, IDC_TRIVIAL_DIFF_TEXT_COLOR, OPT_CLR_TRIVIAL_DIFF_TEXT, m_clrTrivialText);
	
	SerializeColor(op, IDC_MOVEDBLOCK_COLOR, OPT_CLR_MOVEDBLOCK, m_clrMoved);
	SerializeColor(op, IDC_MOVEDBLOCK_DELETED_COLOR, OPT_CLR_MOVEDBLOCK_DELETED, m_clrMovedDeleted);
	SerializeColor(op, IDC_MOVEDBLOCK_TEXT_COLOR, OPT_CLR_MOVEDBLOCK_TEXT, m_clrMovedText);
	
	SerializeColor(op, IDC_SEL_MOVEDBLOCK_COLOR, OPT_CLR_SELECTED_MOVEDBLOCK, m_clrSelMoved);
	SerializeColor(op, IDC_SEL_MOVEDBLOCK_DELETED_COLOR, OPT_CLR_SELECTED_MOVEDBLOCK_DELETED, m_clrSelMovedDeleted);
	SerializeColor(op, IDC_SEL_MOVEDBLOCK_TEXT_COLOR, OPT_CLR_SELECTED_MOVEDBLOCK_TEXT, m_clrSelMovedText);
	
	SerializeColor(op, IDC_WORDDIFF_COLOR, OPT_CLR_WORDDIFF, m_clrWordDiff);
	SerializeColor(op, IDC_WORDDIFF_DELETED_COLOR, OPT_CLR_WORDDIFF_DELETED, m_clrWordDiffDeleted);
	SerializeColor(op, IDC_WORDDIFF_TEXT_COLOR, OPT_CLR_WORDDIFF_TEXT, m_clrWordDiffText);

	SerializeColor(op, IDC_SEL_WORDDIFF_COLOR, OPT_CLR_SELECTED_WORDDIFF, m_clrSelWordDiff);
	SerializeColor(op, IDC_SEL_WORDDIFF_DELETED_COLOR, OPT_CLR_SELECTED_WORDDIFF_DELETED, m_clrSelWordDiffDeleted);
	SerializeColor(op, IDC_SEL_WORDDIFF_TEXT_COLOR, OPT_CLR_SELECTED_WORDDIFF_TEXT, m_clrSelWordDiffText);
}
