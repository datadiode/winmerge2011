/**
 * @file  PropFileTransforms.cpp
 *
 * @brief Implementation of PropFileTransforms propertysheet
 */
#include "stdafx.h"
#include "Merge.h"
#include "OptionsPanel.h"
#include "resource.h"
#include "LanguageSelect.h"
#include "PropFileTransforms.h"
#include "Environment.h"
#include "paths.h"
#include "common/coretools.h"
#include <strsafe.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/**
 * @brief Constructor.
 */
PropFileTransforms::PropFileTransforms() 
: OptionsPanel(IDD_PROPPAGE_FILETRANSFORMS, sizeof *this)
{
}

void PropFileTransforms::Scan(LPTSTR p)
{
	p = ScanProfileSectVer(p);
	if (p == NULL)
		return;
	LVITEM item;
	item.iItem = 0;
	while (TCHAR *q = _tcschr(p, _T('=')))
	{
		const size_t r = _tcslen(q);
		*q++ = _T('\0');
		item.state = INDEXTOSTATEIMAGEMASK(1);
		item.pszText = EatPrefixTrim(p, _T("#~"));
		if (item.pszText == NULL)
		{
			item.state = INDEXTOSTATEIMAGEMASK(2);
			item.pszText = p;
		}
		item.iSubItem = 0;
		item.mask = LVFIF_TEXT | LVFIF_STATE;
		item.stateMask = LVIS_STATEIMAGEMASK;
		m_pLv->InsertItem(&item);
		m_pLv->SetItemState(item.iItem, &item);
		item.iSubItem = 1;
		item.mask = LVFIF_TEXT;
		item.pszText = q;
		m_pLv->SetItem(&item);
		++item.iItem;
		p = q + r;
	}
}

/**
 * @brief Called before propertysheet is drawn.
 */
BOOL PropFileTransforms::OnInitDialog()
{
	m_pLv = static_cast<HListView *>(GetDlgItem(IDC_LV_LEXERS));
	m_pLv->SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
	m_pLv->InsertColumn(0, LanguageSelect.LoadString(IDS_FILETRANSFORMS_TYPETITLE).c_str());
	m_pLv->InsertColumn(1, LanguageSelect.LoadString(IDS_FILETRANSFORMS_METHTITLE).c_str());
	String supplementFolder = COptionsMgr::Get(OPT_SUPPLEMENT_FOLDER);
	supplementFolder = env_ExpandVariables(supplementFolder.c_str());
	String ini = paths_ConcatPath(supplementFolder, _T("Supplement.ini"));
	TCHAR buffer[0x8000];
	if (GetPrivateProfileSection(_T("FileTransforms"), buffer, _countof(buffer), ini.c_str()))
	{
		Scan(buffer);
	}
	m_pLv->SetColumnWidth(0, LVSCW_AUTOSIZE);
	m_pLv->SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
	return OptionsPanel::OnInitDialog();
}

/**
 * @brief Called when item state changes.
 */
void PropFileTransforms::OnLvnItemchanged(NMHDR *pNMHDR)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if (pNMLV->uChanged & LVIF_STATE)
	{
		if ((pNMLV->uOldState & LVIS_STATEIMAGEMASK) != 0 &&
			(pNMLV->uNewState & LVIS_STATEIMAGEMASK) > (pNMLV->uOldState & LVIS_STATEIMAGEMASK))
		{
			TCHAR key[32];
			m_pLv->GetItemText(pNMLV->iItem, 0, key, _countof(key));
			LVFINDINFO info;
			info.flags = LVFI_STRING;
			info.psz = key;
			int i = -1;
			while ((i = m_pLv->FindItem(&info, i)) != -1)
			{
				if (i != pNMLV->iItem)
				{
					m_pLv->SetCheck(i, FALSE);
				}
			}
		}
	}
}

LRESULT PropFileTransforms::OnNotify(NMHDR *pNMHDR)
{
	switch (pNMHDR->idFrom)
	{
	case IDC_LV_FILETRANSFORMS:
		switch (pNMHDR->code)
		{
		case LVN_ITEMCHANGED:
			OnLvnItemchanged(pNMHDR);
			break;
		}
		break;
	}
	return 0;
}

LRESULT PropFileTransforms::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case MAKEWPARAM(IDC_FILETRANSFORMS_DEFAULTS, BN_CLICKED):
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
void PropFileTransforms::ReadOptions()
{
}

/** 
 * @brief Writes options values from UI to storage.
 * Property sheet calls this after dialog is closed with OK button to
 * store values in member variables.
 */
void PropFileTransforms::WriteOptions()
{
	if (m_hWnd == NULL)
		return;
	TCHAR buffer[0x8000];
	LPTSTR bufptr = DumpProfileSectVer(buffer);
	size_t remaining = buffer + _countof(buffer) - bufptr;
	int const n = m_pLv->GetItemCount();
	for (int i = 0; i < n; ++i)
	{
		TCHAR key[32];
		m_pLv->GetItemText(i, 0, key, _countof(key));
		TCHAR value[4096];
		m_pLv->GetItemText(i, 1, value, _countof(value));
		LPCTSTR const prefix = &_T("\0#~ ")[!m_pLv->GetCheck(i)];
		if (FAILED(StringCchPrintfEx(bufptr, remaining, &bufptr, &remaining, 0, _T("%s%s = %s"), prefix, key, value)))
			break;
		++bufptr;
		--remaining;
	}
	if (remaining)
	{
		*bufptr = _T('\0');
		String supplementFolder = COptionsMgr::Get(OPT_SUPPLEMENT_FOLDER);
		supplementFolder = env_ExpandVariables(supplementFolder.c_str());
		String ini = paths_ConcatPath(supplementFolder, _T("Supplement.ini"));
		WritePrivateProfileSection(_T("FileTransforms"), buffer, ini.c_str());
	}
}

/**
 * @brief Sets options to defaults
 */
void PropFileTransforms::OnDefaults()
{
	TCHAR buffer[0x8000];
	CMergeApp::DumpFileTransformDefaults(buffer);
	m_pLv->DeleteAllItems();
	Scan(buffer);
}

void PropFileTransforms::UpdateScreen()
{
}
