/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997-2000  Thingamahoochie Software
//    Author: Dean Grimm
//    SPDX-License-Identifier: GPL-3.0-or-later
/////////////////////////////////////////////////////////////////////////////
/**
 * @file Src/Splash.cpp
 *
 * @brief Implementation of splashscreen class
 *
 */
#include "stdafx.h"
#include "Merge.h"
#include "MainFrm.h"
#include "Splash.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
//   Splash Screen class

/**
 * @brief Constructor.
 */
CSplashWnd::CSplashWnd(HWindow *pWndMain)
{
	CLIENTCREATESTRUCT ccs = { NULL, 0xFF00 };

	Subclass(HWindow::CreateEx(0, _T("mdiclient"), NULL,
		WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | MDIS_ALLCHILDSTYLES,
		0, 0, 0, 0, pWndMain, 0x1000, NULL, &ccs));

	m_bAutoDelete = true;
}

/**
 * @brief Destructor.
 */
CSplashWnd::~CSplashWnd()
{
}

LRESULT CSplashWnd::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult;
	BOOL fMaximized;
	switch (uMsg)
	{
	case WM_MDIACTIVATE:
		// Don't surprise user by closing a wrong DocFrame.
		theApp.m_pMainWnd->m_invocationMode = MergeCmdLineInfo::InvocationModeNone;
		fMaximized = FALSE;
		if (OWindow::WindowProc(WM_MDIGETACTIVE, 0, (LPARAM)&fMaximized) == 0)
			break;
		if (!fMaximized)
			break;
		OWindow::WindowProc(WM_SETREDRAW, FALSE, 0);
		lResult = OWindow::WindowProc(uMsg, wParam, lParam);
		OWindow::WindowProc(WM_SETREDRAW, TRUE, 0);
		RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
		return lResult;
	case WM_MDINEXT:
		lParam = !lParam;
		break;
	case WM_MDICREATE:
		lResult = OWindow::WindowProc(uMsg, wParam, lParam);
		if (HTabCtrl *pTc = theApp.m_pMainWnd->GetTabBar())
		{
			int index = pTc->GetItemCount();
			TCITEM item;
			item.mask = TCIF_PARAM;
			item.lParam = lResult;
			pTc->InsertItem(index, &item);
			theApp.m_pMainWnd->RecalcLayout();
		}
		return lResult;
	case WM_MDIDESTROY:
		// Avoid InitCmdUI() if operating on a premature MDI child window.
		if (reinterpret_cast<HWindow *>(wParam)->IsWindowVisible())
			theApp.m_pMainWnd->InitCmdUI();
		lResult = OWindow::WindowProc(uMsg, wParam, lParam);
		if (HTabCtrl *pTc = theApp.m_pMainWnd->GetTabBar())
		{
			int index = pTc->GetItemCount();
			while (index)
			{
				TCITEM item;
				item.mask = TCIF_PARAM;
				pTc->GetItem(--index, &item);
				if (item.lParam == wParam)
					pTc->DeleteItem(index);
			}
			theApp.m_pMainWnd->RecalcLayout();
		}
		return lResult;
	}
	lResult = OWindow::WindowProc(uMsg, wParam, lParam);
	return lResult;
}
