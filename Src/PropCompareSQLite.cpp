/**
 * @file  PropCompareSQLite.cpp
 *
 * @brief Implementation of PropCompareSQLite propertysheet
 */
#include "stdafx.h"
#include "OptionsPanel.h"
#include "resource.h"
#include "LanguageSelect.h"
#include "PropCompareSQLite.h"
#include "WildcardDropList.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/**
 * @brief Constructor.
 * @param [in] optionsMgr Pointer to COptionsMgr.
 */
PropCompareSQLite::PropCompareSQLite()
: OptionsPanel(IDD_PROPPAGE_COMPARE_SQLITE, sizeof *this)
{
}

template<ODialog::DDX_Operation op>
bool PropCompareSQLite::UpdateData()
{
	DDX_Check<op>(IDC_COMPARESQLITE_DETECT_BY_SIGNATURE, m_bUseFilePatterns, FALSE);
	DDX_Check<op>(IDC_COMPARESQLITE_DETECT_BY_FILENAME, m_bUseFilePatterns, TRUE);
	DDX_Text<op>(IDC_COMPARESQLITE_PATTERNS, m_sFilePatterns);
	DDX_Check<op>(IDC_COMPARESQLITE_SCHEMA_ONLY, m_nCompareFlags, SQLITE_CMP_SCHEMA_ONLY);
	DDX_Check<op>(IDC_COMPARESQLITE_SCHEMA_AND_DATA, m_nCompareFlags, SQLITE_CMP_SCHEMA_AND_DATA);
	DDX_Check<op>(IDC_COMPARESQLITE_PROMPT_FOR_OPTIONS, m_bPromptForOptions);
	DDX_Check<op>(IDC_COMPARESQLITE_BLOB_FIELDS, m_bCompareBlobFields);
	return true;
}

LRESULT PropCompareSQLite::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		if (IsUserInputCommand(wParam))
			UpdateData<Get>();
		switch (wParam)
		{
		case MAKEWPARAM(IDC_COMPARESQLITE_DEFAULTS, BN_CLICKED):
			OnDefaults();
			break;
		case MAKEWPARAM(IDC_COMPARESQLITE_SCHEMA_ONLY, BN_CLICKED):
		case MAKEWPARAM(IDC_COMPARESQLITE_SCHEMA_AND_DATA, BN_CLICKED):
		case MAKEWPARAM(IDC_COMPARESQLITE_DETECT_BY_FILENAME, BN_CLICKED):
		case MAKEWPARAM(IDC_COMPARESQLITE_DETECT_BY_SIGNATURE, BN_CLICKED):
			UpdateScreen();
			break;
		case MAKEWPARAM(IDC_COMPARESQLITE_PATTERNS, CBN_DROPDOWN):
			WildcardDropList::OnDropDown(reinterpret_cast<HWND>(lParam), 6,
				COptionsMgr::GetDefault(OPT_CMP_SQLITE_FILEPATTERNS).c_str());
			break;
		case MAKEWPARAM(IDC_COMPARESQLITE_PATTERNS, CBN_CLOSEUP):
			WildcardDropList::OnCloseUp(reinterpret_cast<HWND>(lParam));
			UpdateData<Get>();
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
void PropCompareSQLite::ReadOptions()
{
	m_sFilePatterns = COptionsMgr::Get(OPT_CMP_SQLITE_FILEPATTERNS);
	m_nCompareFlags = COptionsMgr::Get(OPT_CMP_SQLITE_COMPAREFLAGS);
	// Decompose the flags
	m_bUseFilePatterns = (m_nCompareFlags & SQLITE_CMP_USE_FILE_PATTERNS) != 0;
	m_bPromptForOptions = (m_nCompareFlags & SQLITE_CMP_PROMPT_FOR_OPTIONS) != 0;
	m_bCompareBlobFields = (m_nCompareFlags & SQLITE_CMP_COMPARE_BLOB_FIELDS) != 0;
	m_nCompareFlags &= SQLITE_CMP_RADIO_OPTIONS_MASK;
}

/**
 * @brief Writes options values from UI to storage.
 * Property sheet calls this after dialog is closed with OK button to
 * store values in member variables.
 */
void PropCompareSQLite::WriteOptions()
{
	WildcardRemoveDuplicatePatterns(m_sFilePatterns);
	COptionsMgr::SaveOption(OPT_CMP_SQLITE_FILEPATTERNS, m_sFilePatterns);
	COptionsMgr::SaveOption(OPT_CMP_SQLITE_COMPAREFLAGS, m_nCompareFlags |
		(m_bUseFilePatterns ? SQLITE_CMP_USE_FILE_PATTERNS : 0) |
		(m_bPromptForOptions ? SQLITE_CMP_PROMPT_FOR_OPTIONS : 0) |
		(m_bCompareBlobFields ? SQLITE_CMP_COMPARE_BLOB_FIELDS : 0));
}

/**
 * @brief Sets options to defaults
 */
void PropCompareSQLite::OnDefaults()
{
	m_sFilePatterns = COptionsMgr::GetDefault(OPT_CMP_SQLITE_FILEPATTERNS);
	m_nCompareFlags = COptionsMgr::GetDefault(OPT_CMP_SQLITE_COMPAREFLAGS);
	UpdateScreen();
}

void PropCompareSQLite::UpdateScreen()
{
	UpdateData<Set>();
	GetDlgItem(IDC_COMPARESQLITE_PATTERNS)->EnableWindow(m_bUseFilePatterns);
	BOOL bEnable = m_nCompareFlags == SQLITE_CMP_SCHEMA_AND_DATA;
	GetDlgItem(IDC_COMPARESQLITE_BLOB_FIELDS)->EnableWindow(bEnable);
}
