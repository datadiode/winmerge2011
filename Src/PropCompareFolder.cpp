/** 
 * @file  PropCompareFolder.cpp
 *
 * @brief Implementation of PropCompareFolder propertysheet
 */
#include "StdAfx.h"
#include "OptionsPanel.h"
#include "resource.h"
#include "LanguageSelect.h"
#include "PropCompareFolder.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const int Mega = 1024 * 1024;

/** 
 * @brief Constructor.
 */
PropCompareFolder::PropCompareFolder()
: OptionsPanel(IDD_PROPPAGE_COMPARE_FOLDER, sizeof *this)
{
}

template<ODialog::DDX_Operation op>
bool PropCompareFolder::UpdateData()
{
	DDX_CBIndex<op>(IDC_COMPAREMETHODCOMBO, m_compareMethod);
	DDX_Check<op>(IDC_COMPARE_STOPFIRST, m_bStopAfterFirst);
	DDX_Check<op>(IDC_IGNORE_SMALLTIMEDIFF, m_bIgnoreSmallTimeDiff);
	DDX_Check<op>(IDC_COMPARE_SELFCOMPARE, m_bSelfCompare);
	DDX_Check<op>(IDC_COMPARE_WALKUNIQUES, m_bWalkUniques);
	DDX_Check<op>(IDC_COMPARE_CACHE_RESULTS, m_bCacheResults);
	DDX_Text<op>(IDC_COMPARE_QUICKC_LIMIT, m_nQuickCompareLimit);
	DDX_Text<op>(IDC_COMPARE_THREAD_COUNT, m_nCompareThreads);
	return true;
}

LRESULT PropCompareFolder::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		if (IsUserInputCommand(wParam))
			UpdateData<Get>();
		switch (wParam)
		{
		case MAKEWPARAM(IDC_COMPAREFOLDER_DEFAULTS, BN_CLICKED):
			OnDefaults();
			break;
		case MAKEWPARAM(IDC_COMPAREMETHODCOMBO, CBN_SELCHANGE):
			UpdateScreen();
			break;
		case MAKEWPARAM(IDC_COMPARE_QUICKC_LIMIT, EN_CHANGE):
			m_nQuickCompareLimit = ValidateNumber(reinterpret_cast<HEdit *>(lParam), 0, 2000);
			break;
		case MAKEWPARAM(IDC_COMPARE_THREAD_COUNT, EN_CHANGE):
			m_nCompareThreads = ValidateNumber(reinterpret_cast<HEdit *>(lParam), -31, 31);
			break;
		case MAKEWPARAM(IDC_COMPARE_THREAD_COUNT, EN_SETFOCUS):
			RegisterHotKey(m_hWnd, MAKEWPARAM(LOWORD(wParam), VK_OEM_MINUS), 0, VK_OEM_MINUS);
			break;
		case MAKEWPARAM(IDC_COMPARE_THREAD_COUNT, EN_KILLFOCUS):
			UnregisterHotKey(m_hWnd, MAKEWPARAM(LOWORD(wParam), VK_OEM_MINUS));
			break;
		}
		break;
	case WM_HOTKEY:
		if (HIWORD(wParam) == VK_OEM_MINUS)
		{
			HEdit *pEdit = static_cast<HEdit *>(GetDlgItem(LOWORD(wParam)));
			pEdit->ReplaceSel(_T(""));
			int i = GetDlgItemInt(LOWORD(wParam));
			if (i > 0)
				SetDlgItemInt(LOWORD(wParam), -i);
			else if (i == 0)
				pEdit->SetWindowText(_T("-"));
			pEdit->SetSel(1, 1);
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
void PropCompareFolder::ReadOptions()
{
	m_compareMethod = COptionsMgr::Get(OPT_CMP_METHOD);
	m_bStopAfterFirst = COptionsMgr::Get(OPT_CMP_STOP_AFTER_FIRST);
	m_bIgnoreSmallTimeDiff = COptionsMgr::Get(OPT_IGNORE_SMALL_FILETIME);
	m_bSelfCompare = COptionsMgr::Get(OPT_CMP_SELF_COMPARE);
	m_bWalkUniques = COptionsMgr::Get(OPT_CMP_WALK_UNIQUES);
	m_bCacheResults = COptionsMgr::Get(OPT_CMP_CACHE_RESULTS);
	m_nQuickCompareLimit = COptionsMgr::Get(OPT_CMP_QUICK_LIMIT) / Mega;
	m_nCompareThreads = COptionsMgr::Get(OPT_CMP_COMPARE_THREADS);
}

/** 
 * @brief Writes options values from UI to storage.
 * Property sheet calls this after dialog is closed with OK button to
 * store values in member variables.
 */
void PropCompareFolder::WriteOptions()
{
	COptionsMgr::SaveOption(OPT_CMP_METHOD, m_compareMethod);
	COptionsMgr::SaveOption(OPT_CMP_STOP_AFTER_FIRST, m_bStopAfterFirst != FALSE);
	COptionsMgr::SaveOption(OPT_IGNORE_SMALL_FILETIME, m_bIgnoreSmallTimeDiff != FALSE);
	COptionsMgr::SaveOption(OPT_CMP_SELF_COMPARE, m_bSelfCompare != FALSE);
	COptionsMgr::SaveOption(OPT_CMP_WALK_UNIQUES, m_bWalkUniques != FALSE);
	COptionsMgr::SaveOption(OPT_CMP_CACHE_RESULTS, m_bCacheResults != FALSE);
	COptionsMgr::SaveOption(OPT_CMP_QUICK_LIMIT, m_nQuickCompareLimit * Mega);
	COptionsMgr::SaveOption(OPT_CMP_COMPARE_THREADS, m_nCompareThreads);
}

/** 
 * @brief Called before propertysheet is drawn.
 */
BOOL PropCompareFolder::OnInitDialog()
{
	if (HComboBox *combo = static_cast<HComboBox *>(GetDlgItem(IDC_COMPAREMETHODCOMBO)))
	{
		String item = LanguageSelect.LoadString(ID_COMPMETHOD_FULL_CONTENTS);
		combo->AddString(item.c_str());
		item = LanguageSelect.LoadString(ID_COMPMETHOD_QUICK_CONTENTS);
		combo->AddString(item.c_str());
		item = LanguageSelect.LoadString(ID_COMPMETHOD_MODDATE);
		combo->AddString(item.c_str());
		item = LanguageSelect.LoadString(ID_COMPMETHOD_DATESIZE);
		combo->AddString(item.c_str());
		item = LanguageSelect.LoadString(ID_COMPMETHOD_SIZE);
		combo->AddString(item.c_str());
	}
	return OptionsPanel::OnInitDialog();
}

/** 
 * @brief Sets options to defaults
 */
void PropCompareFolder::OnDefaults()
{
	m_compareMethod = COptionsMgr::GetDefault(OPT_CMP_METHOD);
	m_bStopAfterFirst = COptionsMgr::GetDefault(OPT_CMP_STOP_AFTER_FIRST);
	m_bSelfCompare = COptionsMgr::GetDefault(OPT_CMP_SELF_COMPARE);
	m_bWalkUniques = COptionsMgr::GetDefault(OPT_CMP_WALK_UNIQUES);
	m_bCacheResults = COptionsMgr::GetDefault(OPT_CMP_CACHE_RESULTS);
	m_nQuickCompareLimit = COptionsMgr::GetDefault(OPT_CMP_QUICK_LIMIT) / Mega;
	m_nCompareThreads = COptionsMgr::GetDefault(OPT_CMP_COMPARE_THREADS);
	UpdateScreen();
}

/** 
 * @brief Called when compare method dropdown selection is changed.
 * Enables / disables "Stop compare after first difference" checkbox.
 * That checkbox is valid only for quick contents compare method.
 */
void PropCompareFolder::UpdateScreen()
{
	UpdateData<Set>();
	HComboBox *combo = static_cast<HComboBox *>(GetDlgItem(IDC_COMPAREMETHODCOMBO));
	GetDlgItem(IDC_COMPARE_STOPFIRST)->EnableWindow(combo->GetCurSel() == 1);
}
