/** 
 * @file  PropEditor.cpp
 *
 * @brief Implementation of PropEditor propertysheet
 */
#include "StdAfx.h"
#include "OptionsPanel.h"
#include "resource.h"
#include "LanguageSelect.h"
#include "PropEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/** @brief Maximum size for tabs in spaces. */
#define MAX_TABSIZE 64

/** 
 * @brief Constructor.
 */
PropEditor::PropEditor()
: OptionsPanel(IDD_PROPPAGE_EDITOR, sizeof *this)
{
}

/** 
 * @brief Function handling dialog data exchange between GUI and variables.
 */
template<ODialog::DDX_Operation op>
bool PropEditor::UpdateData()
{
	DDX_Check<op>(IDC_HILITE_CHECK, m_bHiliteSyntax);
	DDX_Check<op>(IDC_PROP_INSERT_TABS, m_nTabType, 0);
	DDX_Check<op>(IDC_PROP_INSERT_SPACES, m_nTabType, 1);
	DDX_Text<op>(IDC_TAB_EDIT, m_nTabSize);
	DDX_Check<op>(IDC_AUTOMRESCAN_CHECK, m_bAutomaticRescan);
	DDX_Check<op>(IDC_MIXED_EOL, m_bAllowMixedEol);
	DDX_Check<op>(IDC_VIEW_LINE_DIFFERENCES, m_bViewLineDifferences);
	DDX_Check<op>(IDC_EDITOR_CHARLEVEL, m_bBreakOnWords, 0);
	DDX_Check<op>(IDC_EDITOR_WORDLEVEL, m_bBreakOnWords, 1);
	DDX_CBIndex<op>(IDC_BREAK_TYPE, m_nBreakType);
	DDX_Text<op>(IDC_BREAK_CHARS, m_breakChars);
	return true;
}

LRESULT PropEditor::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		if (IsUserInputCommand(wParam))
			UpdateData<Get>();
		switch (wParam)
		{
		case MAKEWPARAM(IDC_VIEW_LINE_DIFFERENCES, BN_CLICKED):
		case MAKEWPARAM(IDC_EDITOR_CHARLEVEL, BN_CLICKED):
		case MAKEWPARAM(IDC_EDITOR_WORDLEVEL, BN_CLICKED):
			UpdateScreen();
			break;
		case MAKEWPARAM(IDC_TAB_EDIT, EN_CHANGE):
			m_nTabSize = ValidateNumber(reinterpret_cast<HEdit *>(lParam), 1, MAX_TABSIZE);
			break;
		}
		break;
	}
	return OptionsPanel::WindowProc(message, wParam, lParam);
}

/** 
 * @brief Reads options values from storage to UI.
 */
void PropEditor::ReadOptions()
{
	m_nTabSize = COptionsMgr::Get(OPT_TAB_SIZE);
	m_nTabType = COptionsMgr::Get(OPT_TAB_TYPE);
	m_bAutomaticRescan = COptionsMgr::Get(OPT_AUTOMATIC_RESCAN);
	m_bHiliteSyntax = COptionsMgr::Get(OPT_SYNTAX_HIGHLIGHT);
	m_bAllowMixedEol = COptionsMgr::Get(OPT_ALLOW_MIXED_EOL);
	m_bViewLineDifferences = COptionsMgr::Get(OPT_WORDDIFF_HIGHLIGHT);
	m_bBreakOnWords = COptionsMgr::Get(OPT_BREAK_ON_WORDS);
	m_nBreakType = COptionsMgr::Get(OPT_BREAK_TYPE);
	m_breakChars = COptionsMgr::Get(OPT_BREAK_SEPARATORS);
}

/** 
 * @brief Writes options values from UI to storage.
 */
void PropEditor::WriteOptions()
{
	COptionsMgr::SaveOption(OPT_TAB_SIZE, m_nTabSize);
	COptionsMgr::SaveOption(OPT_TAB_TYPE, m_nTabType);
	COptionsMgr::SaveOption(OPT_AUTOMATIC_RESCAN, m_bAutomaticRescan != FALSE);
	COptionsMgr::SaveOption(OPT_ALLOW_MIXED_EOL, m_bAllowMixedEol != FALSE);
	COptionsMgr::SaveOption(OPT_SYNTAX_HIGHLIGHT, m_bHiliteSyntax != FALSE);
	COptionsMgr::SaveOption(OPT_WORDDIFF_HIGHLIGHT, m_bViewLineDifferences != FALSE);
	COptionsMgr::SaveOption(OPT_BREAK_ON_WORDS, m_bBreakOnWords != FALSE);
	COptionsMgr::SaveOption(OPT_BREAK_TYPE, m_nBreakType);
	COptionsMgr::SaveOption(OPT_BREAK_SEPARATORS, m_breakChars);
}

/** 
 * @brief Called before propertysheet is drawn.
 */
BOOL PropEditor::OnInitDialog()
{
	if (HEdit *edit = static_cast<HEdit *>(GetDlgItem(IDC_TAB_EDIT)))
	{
		edit->SetLimitText(sizeof _CRT_STRINGIZE(MAX_TABSIZE) - 1);
	}

	if (HComboBox *combo = static_cast<HComboBox *>(GetDlgItem(IDC_BREAK_TYPE)))
	{
		combo->AddString(LanguageSelect.LoadString(IDS_BREAK_ON_WHITESPACE).c_str());
		combo->AddString(LanguageSelect.LoadString(IDS_BREAK_ON_PUNCTUATION).c_str());
	}
	return OptionsPanel::OnInitDialog();
}

/** 
 * @brief Update availability of line difference controls
 */
void PropEditor::UpdateScreen()
{
	UpdateData<Set>();
	// Can only choose char/word level if line differences are enabled
	GetDlgItem(IDC_EDITOR_CHARLEVEL)->EnableWindow(m_bViewLineDifferences);
	GetDlgItem(IDC_EDITOR_WORDLEVEL)->EnableWindow(m_bViewLineDifferences);
	// Can only choose break type if line differences are enabled & we're breaking on words
	GetDlgItem(IDC_BREAK_TYPE)->EnableWindow(m_bViewLineDifferences);
}
