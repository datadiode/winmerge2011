/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997-2000  Thingamahoochie Software
//    Author: Dean Grimm
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
/////////////////////////////////////////////////////////////////////////////
/**
 * @file  PropCodepage.h
 *
 * @brief Implementation file for PropCodepage propertyheet
 *
 */
#include "StdAfx.h"
#include "OptionsPanel.h"
#include "resource.h"
#include "PropCodepage.h"
#include "Common/SuperComboBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

PropCodepage *PropCodepage::m_pThis = NULL;

PropCodepage::PropCodepage()
: OptionsPanel(IDD_PROPPAGE_CODEPAGE, sizeof *this)
{
	m_pThis = this;
}

PropCodepage::~PropCodepage()
{
	m_pThis = NULL;
}

template<ODialog::DDX_Operation op>
bool PropCodepage::UpdateData()
{
	DDX_Check<op>(IDC_CP_SYSTEM, m_nCodepageSystem, 0);
	DDX_Check<op>(IDC_CP_UI, m_nCodepageSystem, 1);
	DDX_Check<op>(IDC_CP_CUSTOM, m_nCodepageSystem, 2);
	// IDC_CUSTOM_CP_NUMBER not listed here as it needs special handling
	DDX_Check<op>(IDC_DETECT_CODEPAGE, m_bDetectCodepage);
	return true;
}

LRESULT PropCodepage::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		if (IsUserInputCommand(wParam))
		{
			// Read data from controls
			UpdateData<Get>();
			m_nCustomCodepageValue = GetDlgItemInt(IDC_CUSTOM_CP_NUMBER);
		}
		switch (wParam)
		{
		case MAKEWPARAM(IDC_CP_SYSTEM, BN_CLICKED):
		case MAKEWPARAM(IDC_CP_CUSTOM, BN_CLICKED):
		case MAKEWPARAM(IDC_CP_UI, BN_CLICKED):
			UpdateScreen();
			break;
		case MAKEWPARAM(IDC_CUSTOM_CP_NUMBER, CBN_DROPDOWN):
			reinterpret_cast<HSuperComboBox *>(lParam)->AdjustDroppedWidth();
			break;
		}
		break;
	}
	return OptionsPanel::WindowProc(message, wParam, lParam);
}

/**
 * @brief Reads options values from storage to UI.
 */
void PropCodepage::ReadOptions()
{
	m_nCodepageSystem = COptionsMgr::Get(OPT_CP_DEFAULT_MODE);
	m_nCustomCodepageValue = COptionsMgr::Get(OPT_CP_DEFAULT_CUSTOM);
	m_bDetectCodepage = COptionsMgr::Get(OPT_CP_DETECT);
}

/**
 * @brief Writes options values from UI to storage.
 */
void PropCodepage::WriteOptions()
{
	COptionsMgr::SaveOption(OPT_CP_DEFAULT_MODE, m_nCodepageSystem);
	COptionsMgr::SaveOption(OPT_CP_DEFAULT_CUSTOM, m_nCustomCodepageValue);
	COptionsMgr::SaveOption(OPT_CP_DETECT, m_bDetectCodepage != FALSE);
}

/**
 * @brief Called before propertysheet is drawn.
 */
BOOL PropCodepage::OnInitDialog()
{
	m_pCbCustomCodepage = static_cast<HComboBox *>(GetDlgItem(IDC_CUSTOM_CP_NUMBER));
	EnumSystemCodePages(EnumCodePagesProc, CP_INSTALLED);
	return OptionsPanel::OnInitDialog();
}

void PropCodepage::UpdateScreen()
{
	// Write data to controls
	UpdateData<Set>();
	m_pCbCustomCodepage->SelectString(-1, NumToStr(m_nCustomCodepageValue, 10).c_str());
	if (GetDlgItemInt(IDC_CUSTOM_CP_NUMBER) != m_nCustomCodepageValue)
		SetDlgItemInt(IDC_CUSTOM_CP_NUMBER, m_nCustomCodepageValue);
	// Update availability of controls
	m_pCbCustomCodepage->EnableWindow(IsDlgButtonChecked(IDC_CP_CUSTOM));
}

/**
 * @brief Callback for use with EnumSystemCodePages()
 */
BOOL PropCodepage::EnumCodePagesProc(LPTSTR lpCodePageString)
{
	if (UINT const codepage = _tcstol(lpCodePageString, &lpCodePageString, 10))
	{
		CPINFOEX info;
		if (GetCPInfoEx(codepage, 0, &info))
		{
			int lower = 0;
			int upper = m_pThis->m_pCbCustomCodepage->GetCount();
			while (lower < upper)
			{
				int const match = (upper + lower) >> 1;
				UINT const cmp = static_cast<UINT>(m_pThis->m_pCbCustomCodepage->GetItemData(match));
				if (cmp >= info.CodePage)
					upper = match;
				if (cmp <= info.CodePage)
					lower = match + 1;
			}
			// Cosmetic: Remove excess spaces between numeric identifier and what follows it
			if (_tcstol(info.CodePageName, &lpCodePageString, 10) && *lpCodePageString == _T(' '))
				StrTrim(lpCodePageString + 1, _T(" "));
			int index = m_pThis->m_pCbCustomCodepage->InsertString(lower, info.CodePageName);
			m_pThis->m_pCbCustomCodepage->SetItemData(index, info.CodePage);
		}
	}
	return TRUE; // continue enumeration
}
