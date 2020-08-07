/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997  Dean P. Grimm
//    SPDX-License-Identifier: GPL-3.0-or-later
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
 * Does not at all do what its name implies.
 */
class CSplashWnd : public OWindow
{
// Construction
public:
	CSplashWnd(HWindow *);
private:
	~CSplashWnd();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
};
