/**
 * @file  PropLexers.cpp
 *
 * @brief Implementation of PropLexers propertysheet
 */
#include "stdafx.h"
#include "OptionsPanel.h"
#include "resource.h"
#include "LanguageSelect.h"
#include "PropLexers.h"
#include "WildcardDropList.h"
#include "Environment.h"
#include "paths.h"
#include "editlib/TextBlock.h"
#include <strsafe.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/**
 * @brief Constructor.
 */
PropLexers::PropLexers()
: OptionsPanel(IDD_PROPPAGE_LEXERS, sizeof *this)
{
}

/**
 * @brief Called before propertysheet is drawn.
 */
BOOL PropLexers::OnInitDialog()
{
	m_pLv = static_cast<HListView *>(GetDlgItem(IDC_LV_LEXERS));
	m_pLv->SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
	m_pLv->InsertColumn(0, LanguageSelect.LoadString(IDS_LEXERS_LEXERTITLE).c_str());
	m_pLv->InsertColumn(1, LanguageSelect.LoadString(IDS_LEXERS_FILETYPESTITLE).c_str());
	for (int i = 0; i < TextDefinition::m_SourceDefsCount; ++i)
	{
		m_pLv->InsertItem(i, TextDefinition::m_StaticSourceDefs[i].name);
		m_pLv->SetItemText(i, 1, TextDefinition::m_SourceDefs[i].exts);
		WORD loWord = LOWORD(TextDefinition::m_SourceDefs[i].flags);
		if (loWord == 0)
			loWord = LOWORD(TextDefinition::m_StaticSourceDefs[i].flags);
		WORD hiWord = 1 << (TextDefinition::m_SourceDefs[i].flags >> 28);
		if (hiWord == 1)
			hiWord = 1 << (TextDefinition::m_StaticSourceDefs[i].flags >> 28);
		m_pLv->SetItemData(i, MAKELPARAM(loWord, hiWord));
	}
	m_pLv->SetColumnWidth(0, LVSCW_AUTOSIZE);
	m_pLv->SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
	return OptionsPanel::OnInitDialog();
}

/**
 * @brief Called on doubleclick within a cell, or Alt+Enter.
 */
void PropLexers::OnLvnItemactivate(NMHDR *pNMHDR)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	HMenu *pMenu = NULL;
	if (TextDefinition::m_StaticSourceDefs[pNMLV->iItem].flags & TextDefinition::SRCOPT_HTML_LEXIS)
	{
		pMenu = HMenu::CreateMenu();
		pMenu->AppendMenu(MF_OWNERDRAW, IDS_LEXER_RECOGNIZE_HTML4_TAGS,
			reinterpret_cast<LPCWSTR>(TextDefinition::SRCOPT_HTML4_LEXIS));
		pMenu->AppendMenu(MF_OWNERDRAW, IDS_LEXER_RECOGNIZE_HTML5_TAGS,
			reinterpret_cast<LPCWSTR>(TextDefinition::SRCOPT_HTML5_LEXIS));
		switch (TextDefinition::m_StaticSourceDefs[pNMLV->iItem].flags & COOKIE_PARSER_GLOBAL)
		{
		case SRCOPT_COOKIE(COOKIE_PARSER):
			// static or unobvious kind of dynamic HTML (TODO: Offer other options here?)
			pMenu->AppendMenu(MF_OWNERDRAW, IDS_LEXER_RECOGNIZE_MWSL_TAG,
				reinterpret_cast<LPCWSTR>(0x10000 << (SRCOPT_COOKIE(COOKIE_PARSER_MWSL) >> 28)));
			break;
		}
	}
	WildcardDropList::OnItemActivate(pNMLV->hdr.hwndFrom, pNMLV->iItem,
		1, 4, TextDefinition::m_StaticSourceDefs[pNMLV->iItem].exts, true, pMenu->m_hMenu);
}

LRESULT PropLexers::OnNotify(NMHDR *pNMHDR)
{
	switch (pNMHDR->idFrom)
	{
	case IDC_LV_LEXERS:
		switch (pNMHDR->code)
		{
		case LVN_ITEMACTIVATE:
			OnLvnItemactivate(pNMHDR);
			break;
		}
		break;
	}
	return 0;
}

LRESULT PropLexers::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case MAKEWPARAM(IDC_LEXERS_DEFAULTS, BN_CLICKED):
			OnDefaults();
			break;
		}
		break;
	case WM_NOTIFY:
		if (LRESULT lResult = OnNotify(reinterpret_cast<NMHDR *>(lParam)))
			return lResult;
		break;
	}
	return OptionsPanel::WindowProc(message, wParam, lParam);
}

/**
 * @brief Reads options values from storage to UI.
 * Property sheet calls this before displaying GUI to load values
 * into members.
 */
void PropLexers::ReadOptions()
{
}

/**
 * @brief Writes options values from UI to storage.
 * Property sheet calls this after dialog is closed with OK button to
 * store values in member variables.
 */
void PropLexers::WriteOptions()
{
	if (m_hWnd == NULL)
		return;
	TCHAR buffer[0x8000];
	LPTSTR bufptr = buffer;
	size_t remaining = _countof(buffer);
	StringCchPrintfEx(bufptr, remaining, &bufptr, &remaining, 0, _T("version=ignore"));
	++bufptr;
	--remaining;
	for (int i = 0; i < TextDefinition::m_SourceDefsCount; ++i)
	{
		TCHAR text[4096];
		m_pLv->GetItemText(i, 1, text, _countof(text));
		if (FAILED(StringCchPrintfEx(bufptr, remaining, &bufptr, &remaining, 0, _T("%s=%s"), TextDefinition::m_StaticSourceDefs[i].name, text)))
			break;
		UINT_PTR const uItemData = m_pLv->GetItemData(i);
		if (uItemData & TextDefinition::SRCOPT_HTML4_LEXIS)
			if (FAILED(StringCchPrintfEx(bufptr, remaining, &bufptr, &remaining, 0, _T(":HTML4"))))
				break;
		if (uItemData & TextDefinition::SRCOPT_HTML5_LEXIS)
			if (FAILED(StringCchPrintfEx(bufptr, remaining, &bufptr, &remaining, 0, _T(":HTML5"))))
				break;
		if (uItemData & (0x10000 << (SRCOPT_COOKIE(COOKIE_PARSER_MWSL) >> 28)))
			if (FAILED(StringCchPrintfEx(bufptr, remaining, &bufptr, &remaining, 0, _T(":MWSL"))))
				break;
		++bufptr;
		--remaining;
	}
	if (remaining)
	{
		*++bufptr = _T('\0');
		TextDefinition::FreeParserAssociations();
		TextDefinition::ScanParserAssociations(buffer);
		TextDefinition::DumpParserAssociations(buffer);
		String supplementFolder = COptionsMgr::Get(OPT_SUPPLEMENT_FOLDER);
		supplementFolder = env_ExpandVariables(supplementFolder.c_str());
		String ini = paths_ConcatPath(supplementFolder, _T("Supplement.ini"));
		WritePrivateProfileSection(_T("Parsers"), buffer, ini.c_str());
	}
}

/**
 * @brief Sets options to defaults
 */
void PropLexers::OnDefaults()
{
	for (int i = 0; i < TextDefinition::m_SourceDefsCount; ++i)
	{
		m_pLv->SetItemText(i, 1, TextDefinition::m_StaticSourceDefs[i].exts);
	}
}

void PropLexers::UpdateScreen()
{
}
