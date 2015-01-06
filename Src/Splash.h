/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997  Dean P. Grimm
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
 * @file  Src/Splash.h
 *
 * @brief Declaration file for CSplashWnd
 *
 */
#pragma once

/**
 * @brief Splash screen class.
 * Loads image from resource and shows it as a splash screen
 * at program startup.
 */
class CSplashWnd : public OWindow
{
// Construction
public:
	CSplashWnd(HWindow *);
private:
	~CSplashWnd();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	LRESULT OnCustomdraw(NMCUSTOMDRAW *);

	CMyComPtr<IPicture> m_spIPicture;
};
