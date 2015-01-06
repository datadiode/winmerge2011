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
 * @file Src/Splash.cpp
 *
 * @brief Implementation of splashscreen class
 *
 */
#include "stdafx.h"
#include "Merge.h"
#include "MainFrm.h"
#include "LanguageSelect.h"
#include "Splash.h"
#include "Common/version.h"

#pragma comment(lib, "wininet.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/** @brief Image dimensions. */
static const SIZE ImageDimensions = { 550, 350 };

/** 
 * @brief Area for version text in splash image.
 * @note Translations may have language name after version number.
 */
static const RECT VersionTextArea = { 346, 257, 547, 272 };
/** @brief Font size for version text. */
static const UINT VersionTextSize = 9;
/** @brief Drawing style for version text. */
static const UINT VersionTextStyle = DT_CENTER | DT_VCENTER;

/** @brief Area for developers list. */
static const RECT DevelopersArea = { 16, 87, 315, 272 };
/** @brief Font size for developers list text. */
static const UINT DevelopersTextSize = 10;
/** @brief Drawing style for developers list text. */
static const UINT DevelopersTextStyle = DT_NOPREFIX | DT_TOP;

/** @brief Area for copyright text. */
static const RECT CopyrightArea = { 10, 284, 539, 339 };
/** @brief Font size for copyright text. */
static const UINT CopyrightTextSize = 8;
/** @brief Drawing style for copyright text. */
static const UINT CopyrightTextStyle = DT_NOPREFIX | DT_TOP | DT_WORDBREAK;

/** @brief Font used in splash screen texts. */
static const TCHAR FontName[] = _T("Tahoma");

/** @brief ID for the timer closing splash screen. */
static const UINT_PTR SplashTimerID = 1;

/////////////////////////////////////////////////////////////////////////////
//   Splash Screen class

/** 
 * @brief Constructor.
 */
CSplashWnd::CSplashWnd(HWindow *pWndMain)
{
	CLIENTCREATESTRUCT ccs = { NULL, 0xFF00 };

	SubclassWindow(HWindow::CreateEx(WS_EX_CLIENTEDGE, _T("mdiclient"), NULL,
		WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | MDIS_ALLCHILDSTYLES,
		0, 0, 0, 0, pWndMain, 0x1000, NULL, &ccs));

	//BringWindowToTop(m_hWnd);
	m_bAutoDelete = true;
	if (COptionsMgr::Get(OPT_DISABLE_SPLASH))
		return;
	SetTimer(SplashTimerID, 5000);
	HListView::Create(WS_CHILD | WS_VISIBLE,
		0, 0, ImageDimensions.cx, ImageDimensions.cy,
		m_pWnd, SplashTimerID, NULL, WS_EX_NOPARENTNOTIFY);

	if (HRSRC hRsrc = FindResource(NULL, MAKEINTRESOURCE(IDR_SPLASH), _T("IMAGE")))
	{
		if (HGLOBAL hGlob = LoadResource(NULL, hRsrc))
		{
			if (BYTE *pbData = static_cast<BYTE *>(LockResource(hGlob)))
			{
				DWORD cbData = SizeofResource(NULL, hRsrc);
				if (IStream *pstm = SHCreateMemStream(pbData, cbData))
				{
					OleLoadPicture(pstm, cbData, FALSE, IID_IPicture, (void**)&m_spIPicture);
					pstm->Release();
				}
			}
		}
	}	
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
	case WM_TIMER:
		C_ASSERT(SplashTimerID == WM_CREATE);
	case WM_PARENTNOTIFY:
		switch (LOWORD(wParam))
		{
		case WM_CREATE:
			KillTimer(SplashTimerID);
			SendDlgItemMessage(SplashTimerID, WM_CLOSE);
			m_spIPicture.Release();
			break;
		}
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
	case WM_NOTIFY:
		switch (reinterpret_cast<NMHDR *>(lParam)->code)
		{
		case NM_CUSTOMDRAW:
			return OnCustomdraw(reinterpret_cast<NMCUSTOMDRAW *>(lParam));
		}
		break;
	case WM_SIZE:
		if (HWND hwndSplash = ::GetDlgItem(m_hWnd, SplashTimerID))
		{
			::SetWindowPos(hwndSplash, NULL,
				(SHORT LOWORD(lParam) - ImageDimensions.cx) / 2,
				(SHORT HIWORD(lParam) - ImageDimensions.cy) / 2,
				0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
		}
		break;
	}
	lResult = OWindow::WindowProc(uMsg, wParam, lParam);
	return lResult;
}

/** 
 * @brief Paint splashscreen.
 * Draws image to window size (which is already set to image size).
 * Then adds texts over image.
 */
LRESULT CSplashWnd::OnCustomdraw(NMCUSTOMDRAW *pnm)
{
	if (pnm->dwDrawStage == CDDS_PREPAINT)
		return CDRF_NOTIFYPOSTPAINT;
	if (pnm->dwDrawStage != CDDS_POSTPAINT)
		return CDRF_DODEFAULT;

	HSurface *pdc = reinterpret_cast<HSurface *>(pnm->hdc);

	if (m_spIPicture)
	{
		long hmWidth = 0;
		long hmHeight = 0;
		m_spIPicture->get_Width(&hmWidth);
		m_spIPicture->get_Height(&hmHeight);
		m_spIPicture->Render(pdc->m_hDC, 0, 0, ImageDimensions.cx, ImageDimensions.cy, 0, hmHeight, hmWidth, -hmHeight, NULL);
	}

	CVersionInfo version;

	int logpixelsy = pdc->GetDeviceCaps(LOGPIXELSY);

	int fontHeight = -MulDiv(VersionTextSize, logpixelsy, 72);
	HFont *font = HFont::Create(fontHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE,
		0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, FontName);

	HGdiObj *oldfont = pdc->SelectObject(font);
	String sVersion = version.GetProductVersion();
	String s = LanguageSelect.FormatMessage(IDS_VERSION_FMT, sVersion.c_str());
	pdc->SetBkMode(TRANSPARENT);
	RECT area = VersionTextArea;
	pdc->DrawText(s.c_str(), -1, &area, VersionTextStyle);
	pdc->SelectObject(oldfont);
	font->DeleteObject();

	fontHeight = -MulDiv(DevelopersTextSize, logpixelsy, 72);
	font = HFont::Create(fontHeight, 0, 0, 0, 0, FALSE, FALSE,
		0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, FontName);
	oldfont = pdc->SelectObject(font);
	String text = LanguageSelect.LoadString(IDS_SPLASH_DEVELOPERS);
	// Replace ", " with linefeed in developers list text to get
	// developers listed a name per line in the about dialog
	std::string::size_type pos = std::string::npos;
	while ((pos = text.rfind(_T(", "), pos)) != std::string::npos)
	{
		text.replace(pos, 2, _T("\n"), 1);
	}

	area = DevelopersArea;
	pdc->DrawText(text.c_str(), -1, &area, DevelopersTextStyle);
	pdc->SelectObject(oldfont);
	font->DeleteObject();

	fontHeight = -MulDiv(CopyrightTextSize, logpixelsy, 72);
	font = HFont::Create(fontHeight, 0, 0, 0, 0, FALSE, FALSE,
		0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, FontName);
	oldfont = pdc->SelectObject(font);

	text = LanguageSelect.LoadString(IDS_SPLASH_GPLTEXT);
	area = CopyrightArea;
	pdc->DrawText(text.c_str(), -1, &area, CopyrightTextStyle);
	pdc->SelectObject(oldfont);
	font->DeleteObject();

	return CDRF_DODEFAULT;
}
