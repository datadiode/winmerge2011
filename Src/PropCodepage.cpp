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
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "OptionsPanel.h"
#include "resource.h"
#include "PropCodepage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

PropCodepage::PropCodepage()
: OptionsPanel(IDD_PROPPAGE_CODEPAGE, sizeof *this)
{
}

template<ODialog::DDX_Operation op>
bool PropCodepage::UpdateData()
{
	DDX_Check<op>(IDC_CP_SYSTEM, m_nCodepageSystem, 0);
	DDX_Check<op>(IDC_CP_UI, m_nCodepageSystem, 1);
	DDX_Check<op>(IDC_CP_CUSTOM, m_nCodepageSystem, 2);
	DDX_Text<op>(IDC_CUSTOM_CP_NUMBER, m_nCustomCodepageValue);
	DDX_Check<op>(IDC_DETECT_CODEPAGE, m_bDetectCodepage);
	return true;
}

LRESULT PropCodepage::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		if (IsUserInputCommand(wParam))
			UpdateData<Get>();
		switch (wParam)
		{
		case MAKEWPARAM(IDC_CP_SYSTEM, BN_CLICKED):
		case MAKEWPARAM(IDC_CP_CUSTOM, BN_CLICKED):
		case MAKEWPARAM(IDC_CP_UI, BN_CLICKED):
			UpdateScreen();
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

void PropCodepage::UpdateScreen()
{
	UpdateData<Set>();
	// Enable/disable "Custom codepage" edit field
	GetDlgItem(IDC_CUSTOM_CP_NUMBER)->EnableWindow(IsDlgButtonChecked(IDC_CP_CUSTOM));	
}
