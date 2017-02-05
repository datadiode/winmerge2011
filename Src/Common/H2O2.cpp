// H2O2.cpp
//
// Copyright (c) 2005-2010  David Nash (as of Win32++ v7.0.2)
// Copyright (c) 2011-2016  Jochen Neubeck
//
// Permission is hereby granted, free of charge, to
// any person obtaining a copy of this software and
// associated documentation files (the "Software"),
// to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify,
// merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom
// the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice
// shall be included in all copies or substantial portions
// of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
// SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "RegKey.h"
#include "SettingStore.h"

using namespace H2O;

LRESULT OWindow::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult = ::CallWindowProc(m_pfnSuper, m_hWnd, message, wParam, lParam);
	switch (message)
	{
	case WM_NCDESTROY:
		m_hWnd = NULL;
		m_pfnSuper = NULL;
		if (m_bAutoDelete)
			delete this;
		break;
	}
	return lResult;
}

OWindow::~OWindow()
{
	if (m_pfnSuper)
		DestroyWindow();
}

struct DrawItemStruct_WebLinkButton : DRAWITEMSTRUCT
{
	void DrawItem() const
	{
		TCHAR cText[INTERNET_MAX_PATH_LENGTH];
		int cchText = ::GetWindowText(hwndItem, cText, _countof(cText));
		COLORREF clrText = RGB(0,0,255);
		if (::GetWindowLong(hwndItem, GWL_STYLE) & BS_LEFTTEXT)
		{
			clrText = RGB(128,0,128);
		}
		RECT rcText = rcItem;
		::DrawText(hDC, cText, cchText, &rcText, DT_LEFT | DT_CALCRECT);
		::SetWindowPos(hwndItem, NULL, 0, 0, rcText.right, rcItem.bottom,
			SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW);
		switch (itemAction)
		{
		case ODA_DRAWENTIRE:
			::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rcItem, 0, 0, 0);
			::SetBkMode(hDC, TRANSPARENT);
			::SetTextColor(hDC, clrText);
			::DrawText(hDC, cText, cchText, &rcText, DT_LEFT);
			rcText.top = rcText.bottom - 1;
			::SetBkColor(hDC, clrText);
			::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rcText, 0, 0, 0);
			if (itemState & ODS_FOCUS)
			{
			case ODA_FOCUS:
				if (!(itemState & ODS_NOFOCUSRECT))
				{
					::SetTextColor(hDC, RGB(0,0,0));
					::SetBkColor(hDC, RGB(255,255,255));
					::SetBkMode(hDC, OPAQUE);
					rcText.top = rcText.bottom - 1;
					++rcText.bottom;
					::DrawFocusRect(hDC, &rcText);
				}
			}
			break;
		}
	}
};

template<>
LRESULT OWindow::MessageReflect_WebLinkButton<WM_DRAWITEM>(WPARAM, LPARAM lParam)
{
	reinterpret_cast<DrawItemStruct_WebLinkButton *>(lParam)->DrawItem();
	return 0;
}

template<>
LRESULT OWindow::MessageReflect_WebLinkButton<WM_SETCURSOR>(WPARAM, LPARAM lParam)
{
	HCURSOR hCursor = ::LoadCursor(NULL, MAKEINTRESOURCE(IDC_HAND));
	::SetCursor(hCursor);
	return TRUE;
}

template<>
LRESULT OWindow::MessageReflect_WebLinkButton<WM_COMMAND>(WPARAM, LPARAM lParam)
{
	HButton *button = reinterpret_cast<HButton *>(lParam);
	if (button->SetStyle(button->GetStyle() | BS_LEFTTEXT))
		button->Invalidate();
	return 0;
}

template<>
LRESULT OWindow::MessageReflect_ColorButton<WM_DRAWITEM>(WPARAM, LPARAM lParam)
{
	DRAWITEMSTRUCT *pdis = reinterpret_cast<DRAWITEMSTRUCT *>(lParam);
	DrawEdge(pdis->hDC, &pdis->rcItem, EDGE_SUNKEN,
		pdis->itemState & ODS_FOCUS ? BF_RECT|BF_ADJUST|BF_MONO : BF_RECT|BF_ADJUST);
	COLORREF cr = m_pWnd->GetDlgItemInt(pdis->CtlID);
	COLORREF crTmp = SetBkColor(pdis->hDC, cr);
	ExtTextOut(pdis->hDC, 0, 0, ETO_OPAQUE, &pdis->rcItem, 0, 0, 0);
	SetBkColor(pdis->hDC, crTmp);
	return 0;
}

template<>
LRESULT OWindow::MessageReflect_TopLevelWindow<WM_ACTIVATE>(WPARAM wParam, LPARAM lParam)
{
	if (HWindow *pwndOther = reinterpret_cast<HWindow *>(lParam))
	{
		if (LOWORD(wParam) == WA_INACTIVE && pwndOther->GetParent()->m_hWnd == m_hWnd && !pwndOther->IsWindowVisible())
		{
			CenterWindow(pwndOther);
			HICON icon = pwndOther->GetIcon(ICON_BIG);
			if (icon == NULL)
			{
				icon = GetIcon(ICON_BIG);
				if (icon != NULL)
				{
					pwndOther->SetIcon(icon, ICON_BIG);
				}
			}
		}
	}
	return 0;
}

void OWindow::SwapPanes(UINT id_0, UINT id_1)
{
	struct
	{
		HWindow *pWindow;
		HWindow *pPrevWindow;
		LONG style;
		WINDOWPLACEMENT wp;
	} rg[2];
	rg[0].pWindow = m_pWnd->GetDlgItem(id_0);
	rg[0].pPrevWindow = rg[0].pWindow->GetWindow(GW_HWNDPREV);
	rg[0].style = rg[0].pWindow->GetStyle();
	rg[0].wp.length = sizeof(WINDOWPLACEMENT);
	rg[0].pWindow->GetWindowPlacement(&rg[0].wp);
	rg[1].pWindow = m_pWnd->GetDlgItem(id_1);
	rg[1].pPrevWindow = rg[1].pWindow->GetWindow(GW_HWNDPREV);
	rg[1].style = rg[1].pWindow->GetStyle();
	rg[1].wp.length = sizeof(WINDOWPLACEMENT);
	rg[1].pWindow->GetWindowPlacement(&rg[1].wp);
	rg[0].pWindow->SetDlgCtrlID(id_1);
	rg[1].pWindow->SetDlgCtrlID(id_0);
	rg[0].pWindow->SetStyle(rg[1].style);
	rg[1].pWindow->SetStyle(rg[0].style);
	rg[0].pWindow->SetWindowPlacement(&rg[1].wp);
	rg[1].pWindow->SetWindowPlacement(&rg[0].wp);
	const UINT flags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOCOPYBITS;
	if (rg[0].pWindow != rg[1].pPrevWindow)
		rg[0].pWindow->SetWindowPos(rg[1].pPrevWindow, 0, 0, 0, 0, flags);
	if (rg[1].pWindow != rg[0].pPrevWindow)
		rg[1].pWindow->SetWindowPos(rg[0].pPrevWindow, 0, 0, 0, 0, flags);
}

BOOL ODialog::OnInitDialog()
{
	return TRUE;
}

INT_PTR ODialog::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_INITDIALOG)
	{
		lParam = reinterpret_cast<PROPSHEETPAGE *>(lParam)->lParam;
		SetWindowLongPtr(hWnd, DWLP_USER, lParam);
		SetWindowLongPtr(hWnd, DWLP_DLGPROC, NULL);
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc));
		ODialog *pThis = reinterpret_cast<ODialog *>(lParam);
		const_cast<HWND &>(pThis->m_hWnd) = hWnd;
		return pThis->OnInitDialog();
	}
	return FALSE;
}

LRESULT ODialog::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ODialog *const pThis = FromHandle(reinterpret_cast<HWindow *>(hWnd));
	switch (uMsg)
	{
	case WM_ACTIVATE:
		pThis->MessageReflect_TopLevelWindow<WM_ACTIVATE>(wParam, lParam);
		break;
	}
	LRESULT lResult = 0;
	try
	{
		lResult = pThis->WindowProc(uMsg, wParam, lParam);
	}
	catch (OException *e)
	{
		e->ReportError(hWnd);
		delete e;
	}
	return lResult;
}

LRESULT ODialog::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return ::DefDlgProc(m_hWnd, uMsg, wParam, lParam);
}

INT_PTR ODialog::DoModal(HINSTANCE hinst, HWND parent)
{
	PROPSHEETPAGE psp;
	psp.lParam = reinterpret_cast<LPARAM>(this);
	return DialogBoxParam(hinst, m_idd, parent, DlgProc, reinterpret_cast<LPARAM>(&psp));
}

HWND ODialog::Create(HINSTANCE hinst, HWND parent)
{
	PROPSHEETPAGE psp;
	psp.lParam = reinterpret_cast<LPARAM>(this);
	return CreateDialogParam(hinst, m_idd, parent, DlgProc, reinterpret_cast<LPARAM>(&psp));
}

BOOL ODialog::IsUserInputCommand(WPARAM wParam)
{
	UINT id = LOWORD(wParam);
	UINT code = HIWORD(wParam);
	UINT dlgcode = (UINT)SendDlgItemMessage(id, WM_GETDLGCODE);
	if (dlgcode & DLGC_HASSETSEL)
		return code == EN_CHANGE && SendDlgItemMessage(id, EM_GETMODIFY) != 0;
	if (dlgcode & DLGC_BUTTON)
		return code == BN_CLICKED;
	if (dlgcode & DLGC_WANTARROWS)
	{
		switch (code)
		{
		case CBN_SELCHANGE:
			// force the selected item's text into the edit control
			SendDlgItemMessage(id, CB_SETCURSEL, SendDlgItemMessage(id, CB_GETCURSEL));
			// fall through
		case CBN_EDITCHANGE:
			return TRUE;
		}
	}
	return FALSE;
}

void ODialog::Update3StateCheckBoxLabel(UINT id)
{
	String text;
	HButton *const pButton = static_cast<HButton *>(GetDlgItem(id));
	// Remember the initial text of the label in an invisible child window.
	if (pButton->GetDlgItemText(1, text) == 0)
	{
		pButton->GetWindowText(text);
		HWindow::CreateEx(0, WC_STATIC, text.c_str(), WS_CHILD, 0, 0, 0, 0, pButton, 1);
	}
	// If there is a \t to split the text in twice, the shorter part shows up
	// for BST_(UN)CHECKED, and the longer part shows up for BST_INDETERMINATE.
	// The shorter part is assumed to be a prefix of the longer part.
	String::size_type i = text.find(_T('\t'));
	if (i != String::npos)
	{
		String::size_type j = text.length() - i;
		if (pButton->GetCheck() == BST_INDETERMINATE ? i <= j : i > j)
		{
			// If there is an &, move it where it belongs.
			j = text.find(_T('&'));
			if (j != String::npos)
			{
				text.erase(j, 1);
				text.insert(i + j, 1, _T('&'));
				--i;
			}
			// Swap texts before and after the \t.
			TCHAR *const buf = &text.front();
			buf[i] = _T('\0');
			_tcsrev(buf);
			_tcsrev(buf + i + 1);
			buf[i] = _T('\t');
			_tcsrev(buf);
		}
		// Exclude excess text from \t onwards. (In other words: Do not rely on
		// Visual Styles being in effect, and empirics about their behaviors.)
		text.resize(text.find(_T('\t')));
		pButton->SetWindowText(text.c_str());
	}
}

BOOL OResizableDialog::OnInitDialog()
{
	ODialog::OnInitDialog();
	CFloatState::Clear();
	TCHAR entry[8];
	GetAtomName(reinterpret_cast<ATOM>(m_idd), entry, _countof(entry));
	if (CRegKeyEx rk = SettingStore.GetSectionKey(_T("ScreenLayout")))
	{
		TCHAR value[1024];
		if (LPCTSTR pch = rk.ReadString(entry, NULL, CRegKeyEx::StringRef(value, _countof(value))))
		{
			int const cx = _tcstol(pch, const_cast<LPTSTR *>(&pch), 10);
			int const cy = _tcstol(&pch[*pch == 'x'], const_cast<LPTSTR *>(&pch), 10);
			SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
			CSplitState::Scan(m_hWnd, pch);
		}
	}
	return TRUE;
}

LRESULT OResizableDialog::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TCHAR entry[8];
	RECT rect;
	switch (uMsg)
	{
	case WM_DESTROY:
		GetAtomName(reinterpret_cast<ATOM>(m_idd), entry, _countof(entry));
		if (CRegKeyEx rk = SettingStore.GetSectionKey(_T("ScreenLayout")))
		{
			GetWindowRect(&rect);
			int const cx = rect.right - rect.left;
			int const cy = rect.bottom - rect.top;
			TCHAR value[1024];
			int cch = wsprintf(value, _T("%dx%d"), cx, cy);
			CSplitState::Dump(m_hWnd, value + cch);
			rk.WriteString(entry, value);
		}
		break;
	}
	return CFloatState::CallWindowProc(::DefDlgProc, m_hWnd, uMsg, wParam, lParam);
}

OPropertySheet::OPropertySheet()
{
	ZeroMemory(&m_psh, sizeof m_psh);
	m_psh.dwSize = sizeof m_psh;
	m_psh.dwFlags = PSH_PROPSHEETPAGE | PSH_USECALLBACK;
	m_psh.pfnCallback = PropSheetProc;
}

PROPSHEETPAGE *OPropertySheet::AddPage(ODialog &page)
{
	std::vector<PROPSHEETPAGE>::size_type index = m_pages.size();
	m_pages.resize(index + 1);
	PROPSHEETPAGE *psp = &m_pages.back();
	ZeroMemory(psp, sizeof *psp);
	psp->dwSize = sizeof *psp;
	psp->pszTemplate = page.m_idd;
	psp->pfnDlgProc = ODialog::DlgProc;
	psp->lParam = reinterpret_cast<LPARAM>(&page);
	return psp;
}

INT_PTR OPropertySheet::DoModal(HINSTANCE hinst, HWND parent)
{
	m_psh.ppsp = &m_pages.front();
	m_psh.nPages = m_pages.size();
	m_psh.hInstance = hinst;
	m_psh.hwndParent = parent;
	m_psh.pszCaption = m_caption.c_str();
	return ::PropertySheet(&m_psh);
}

int CALLBACK OPropertySheet::PropSheetProc(HWND hWnd, UINT uMsg, LPARAM lParam)
{
	switch (uMsg)
	{
	case PSCB_INITIALIZED:
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc));
		break;
	}
	return 0;
}

LRESULT OPropertySheet::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ACTIVATE:
		OWindow(hWnd).MessageReflect_TopLevelWindow<WM_ACTIVATE>(wParam, lParam);
		break;
	}
	return ::DefDlgProc(hWnd, uMsg, wParam, lParam);
}

HWND H2O::GetTopLevelParent(HWND hWnd)
{
	while (HWND hWndParent = ::GetParent(hWnd))
		hWnd = hWndParent;
	return hWnd;
}

HIMAGELIST H2O::Create3StateImageList()
{
	HIMAGELIST himlState = ImageList_Create(16, 16, ILC_COLOR | ILC_MASK, 3, 0);

	RECT rc = { 0, 0, 48, 16 };

	HDC hdcScreen = GetDC(0);

	HDC hdc = CreateCompatibleDC(hdcScreen);
	HBITMAP hbm = CreateCompatibleBitmap(hdcScreen, 48, 16);
	HGDIOBJ hbmOld = SelectObject(hdc, hbm);
	SetBkColor(hdc, RGB(255,255,255));
	ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

	SetRect(&rc, 1, 1, 14, 14);
	DrawFrameControl(hdc, &rc, DFC_BUTTON,
					 DFCS_FLAT|DFCS_BUTTONCHECK);

	OffsetRect(&rc, 16, 0);
	DrawFrameControl(hdc, &rc, DFC_BUTTON,
					 DFCS_FLAT|DFCS_BUTTONCHECK|DFCS_CHECKED);

	OffsetRect(&rc, 16, 0);
	DrawFrameControl(hdc, &rc, DFC_BUTTON,
					 DFCS_FLAT|DFCS_BUTTONCHECK);

	InflateRect(&rc, -4, -4);
	FillRect(hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));

	SelectObject(hdc, hbmOld);
	ImageList_AddMasked(himlState, hbm, RGB(255,255,255));

	DeleteObject(hbm);
	DeleteDC(hdc);
	ReleaseDC(0, hdcScreen);

	return himlState;
}

void H2O::GetDesktopWorkArea(HWND hWnd, LPRECT prcDesktop)
{
	// Get screen dimensions excluding task bar
	::SystemParametersInfo(SPI_GETWORKAREA, 0, prcDesktop, 0);
#ifndef _WIN32_WCE
	// Import the GetMonitorInfo and MonitorFromWindow functions
	if (HMODULE hUser32 = GetModuleHandle(_T("USER32.DLL")))
	{
		typedef BOOL (WINAPI* LPGMI)(HMONITOR, LPMONITORINFO);
		typedef HMONITOR (WINAPI* LPMFW)(HWND, DWORD);
		LPMFW pfnMonitorFromWindow = (LPMFW)::GetProcAddress(hUser32, "MonitorFromWindow");
		LPGMI pfnGetMonitorInfo = (LPGMI)::GetProcAddress(hUser32, _CRT_STRINGIZE(GetMonitorInfo));
		// Take multi-monitor systems into account
		if (pfnGetMonitorInfo && pfnMonitorFromWindow)
		{
			HMONITOR hActiveMonitor = pfnMonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi = { sizeof mi };

			if (pfnGetMonitorInfo(hActiveMonitor, &mi))
				*prcDesktop = mi.rcWork;
		}
	}
#endif
}

void H2O::CenterWindow(HWindow *pWnd, HWindow *pParent)
// Centers this window over its parent
{
	RECT rc;
	pWnd->GetWindowRect(&rc);
	RECT rcBounds;
	GetDesktopWorkArea(pWnd->m_hWnd, &rcBounds);
	// Get the parent window dimensions (parent could be the desktop)
	RECT rcParent = rcBounds;
	if ((pParent != NULL) || (pParent = pWnd->GetParent()) != NULL)
		pParent->GetWindowRect(&rcParent);
	// Calculate point to center the dialog over the portion of parent window on this monitor
	::IntersectRect(&rcParent, &rcParent, &rcBounds);
	rc.right -= rc.left;
	rc.bottom -= rc.top;
	rc.left = rcParent.left + (rcParent.right - rcParent.left - rc.right) / 2;
	rc.top = rcParent.top + (rcParent.bottom - rcParent.top - rc.bottom) / 2;
	rcBounds.right -= rc.right;
	rcBounds.bottom -= rc.bottom;
	// Keep the dialog within the work area
	if (rc.left < rcBounds.left)
		rc.left = rcBounds.left;
	if (rc.left > rcBounds.right)
		rc.left = rcBounds.right;
	if (rc.top < rcBounds.top)
		rc.top = rcBounds.top;
	if (rc.top > rcBounds.bottom)
		rc.top = rcBounds.bottom;
	pWnd->SetWindowPos(NULL, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

int OException::ReportError(HWND hwnd, UINT type) const
{
	return msg ? ::MessageBox(hwnd, msg, NULL, type) : 0;
}

OException::OException(LPCTSTR str)
{
	lstrcpyn(msg, str, _countof(msg));
}

OException::OException(DWORD err, LPCTSTR fmt)
{
	static DWORD const WinInetFlags =
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE;
	static DWORD const DefaultFlags =
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
	switch (HIWORD(err))
	{
	case 0x8007: // LogParser error
	case 0x800A: // VBScript or JScript error
		err = DISP_E_EXCEPTION;
		// fall through
	default:
		HMODULE const WinInet = ::GetModuleHandle(_T("WININET"));
		if (DWORD const len = ::FormatMessage(
			WinInet ? WinInetFlags : DefaultFlags, WinInet,
			err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			msg, _countof(msg), NULL))
		{
			if (err == DISP_E_EXCEPTION)
			{
				IErrorInfo *perrinfo;
				if (SUCCEEDED(GetErrorInfo(0, &perrinfo)) && perrinfo)
				{
					BSTR bstr;
					if (SUCCEEDED(perrinfo->GetSource(&bstr)) && bstr)
					{
						wnsprintf(msg + len, _countof(msg) - len, _T("\n%ls - "), bstr);
						SysFreeString(bstr);
					}
					if (SUCCEEDED(perrinfo->GetDescription(&bstr)) && bstr)
					{
						wnsprintf(msg + len, _countof(msg) - len, _T("%ls"), bstr);
						SysFreeString(bstr);
					}
					perrinfo->Release();
				}
			}
		}
		else
		{
			wnsprintf(msg, _countof(msg), fmt ? fmt : _T("Error 0x%08lX = %ld"), err, err);
		}
	}
}

void OException::Throw(LPCTSTR str)
{
	OException e(str);
	throw &e;
}

void OException::Throw(DWORD err, LPCTSTR fmt)
{
	OException e(err, fmt);
	throw &e;
}

void OException::ThrowSilent()
{
	throw static_cast<OException *>(0);
}

void OException::Check(HRESULT hr)
{
	if (FAILED(hr))
		Throw(hr);
}
