/////////////////////////////////////////////////////////////////////////////
//	  WinMerge:  an interactive diff/merge utility
//	  Copyright (C) 1997-2000  Thingamahoochie Software
//	  Author: Dean Grimm
//
//	  This program is free software; you can redistribute it and/or modify
//	  it under the terms of the GNU General Public License as published by
//	  the Free Software Foundation; either version 2 of the License, or
//	  (at your option) any later version.
//
//	  This program is distributed in the hope that it will be useful,
//	  but WITHOUT ANY WARRANTY; without even the implied warranty of
//	  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
//	  GNU General Public License for more details.
//
//	  You should have received a copy of the GNU General Public License
//	  along with this program; if not, write to the Free Software
//	  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
/////////////////////////////////////////////////////////////////////////////
/** 
 * @file  ShellContextMenu.cpp
 *
 * @brief Main implementation file for CShellContextMenu
 */
// Revision ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "ShellContextMenu.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CShellContextMenu::CShellContextMenu(UINT cmdFirst, UINT cmdLast)
: m_cmdFirst(cmdFirst)
, m_cmdLast(cmdLast)
{
}

CShellContextMenu::~CShellContextMenu()
{
}

bool CShellContextMenu::HandleMenuMessage(UINT message, WPARAM wParam, LPARAM lParam, LRESULT& retval)
{
	HRESULT hr = E_NOINTERFACE;
	if (m_pShellContextMenu3)
		hr = m_pShellContextMenu3->HandleMenuMsg2(message, wParam, lParam, &retval);
	else if (m_pShellContextMenu2)
		hr = m_pShellContextMenu2->HandleMenuMsg(message, wParam, lParam);
	return SUCCEEDED(hr);
}

HMENU CShellContextMenu::QueryShellContextMenu(IContextMenu *pContextMenu)
{
	HMENU hShellContextMenu = CreatePopupMenu();
	pContextMenu->QueryContextMenu(hShellContextMenu, 0,
		m_cmdFirst, m_cmdLast, CMF_EXPLORE | CMF_CANRENAME);
	m_pPreferredMenu = pContextMenu;
	m_pPreferredMenu->QueryInterface(&m_pShellContextMenu2);
	m_pPreferredMenu->QueryInterface(&m_pShellContextMenu3);
	return hShellContextMenu;
}

bool CShellContextMenu::InvokeCommand(UINT nCmd, HWND hWnd)
{
	if (nCmd >= m_cmdFirst && nCmd <= m_cmdLast)
	{
		CMINVOKECOMMANDINFO ici = { sizeof(CMINVOKECOMMANDINFO) };
		ici.hwnd = hWnd;
		ici.lpVerb = MAKEINTRESOURCEA(nCmd - m_cmdFirst);
		ici.nShow = SW_SHOWNORMAL;
		HRESULT hr = m_pPreferredMenu->InvokeCommand(&ici);
		ASSERT(SUCCEEDED(hr));
		return SUCCEEDED(hr);
	}
	else 
	{
		return false;
	}
}
