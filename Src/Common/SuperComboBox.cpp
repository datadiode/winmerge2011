// SuperComboBox.cpp : implementation file
//
#include "StdAfx.h"
#include "coretools.h"
#include "SettingStore.h"
#include "SuperComboBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// HSuperComboBox

int HSuperComboBox::FindString(int nIndexStart, LPCTSTR lpszString)
{
	String sItem;
	const int nCount = GetCount();
	while (++nIndexStart < nCount)
	{
		GetLBText(nIndexStart, sItem);
		if (EatPrefix(sItem.c_str(), lpszString))
			return nIndexStart;
	}
	return -1;
}

/**
 * @brief Adds a string to the list box of a combo box
 * @param lpszItem Pointer to the null-terminated string that is to be added.
 */
int HSuperComboBox::AddString(LPCTSTR lpszItem)
{
	return InsertString(-1, lpszItem);
}

/**
 * @brief Inserts a string into the list box of a combo box.
 * @param nIndex The zero-based index to the position in the list box that receives the string.
 * @param lpszItem Pointer to the null-terminated string that is to be added.
 */
int HSuperComboBox::InsertString(int nIndex, LPCTSTR lpszItem)
{
	if (GetEditControl())
	{
		COMBOBOXEXITEM cbitem;
		cbitem.mask = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
		cbitem.pszText = const_cast<LPTSTR>(lpszItem);
		cbitem.iItem = nIndex;
		cbitem.iImage = I_IMAGECALLBACK;
		cbitem.iSelectedImage = I_IMAGECALLBACK;
		return static_cast<int>(SendMessage(CBEM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&cbitem)));
	}
	else
	{
		return HComboBox::InsertString(nIndex, lpszItem);
	}
}

/////////////////////////////////////////////////////////////////////////////
// HSuperComboBox message handlers

void HSuperComboBox::LoadState(LPCTSTR szRegSubKey, UINT nMaxItems)
{
	if (HKEY hKey = SettingStore.GetSectionKey(szRegSubKey))
	{
		DWORD n = 0;
		DWORD bufsize = 0;
		SettingStore.RegQueryInfoKey(hKey, &n, &bufsize);
		if (n > nMaxItems)
			n = nMaxItems;
		TCHAR *const s = static_cast<TCHAR *>(_alloca(bufsize));
		for (DWORD i = 0 ; i < n ; ++i)
		{
			TCHAR name[20];
			wsprintf(name, _T("Item_%d"), i);
			DWORD cb = bufsize;
			DWORD type = REG_NONE;
			SettingStore.RegQueryValueEx(hKey, name, &type, reinterpret_cast<BYTE *>(s), &cb);
			if (type == REG_SZ)
			{
				if (FindStringExact(-1, s) == -1 && s[0] != _T('\0'))
				{
					AddString(s);
				}
			}
		}
		SetCurSel(0);
		SettingStore.RegCloseKey(hKey);
	}
}

/**
 * @brief Saves strings in combobox.
 * This function saves strings in combobox, in editbox and in dropdown.
 * Whitespace characters are stripped from begin and end of the strings
 * before saving. Empty strings are not saved. So strings which have only
 * whitespace characters aren't save either.
 * @param [in] szRegSubKey Registry subkey where to save strings.
 * @param [in] nMaxItems Max number of strings to save.
 */
void HSuperComboBox::SaveState(LPCTSTR szRegSubKey, UINT nMaxItems)
{
	String s;
	GetWindowText(s);
	string_trim_ws(s); // TODO: Is this really appropriate?
	DWORD n = GetCount();
	if (nMaxItems > n)
	{
		nMaxItems = n;
		if (FindStringExact(-1, s.c_str()) == -1)
			++nMaxItems;
	}
	if (HKEY hKey = SettingStore.GetSectionKey(szRegSubKey, nMaxItems))
	{
		String strItem;
		DWORD i = 0;
		DWORD j = 0;
		for (;;)
		{
			if (s != strItem)
			{
				TCHAR name[20];
				wsprintf(name, _T("Item_%d"), j++);
				SettingStore.RegSetValueEx(hKey, name, REG_SZ,
					reinterpret_cast<const BYTE *>(s.c_str()),
					(s.length() + 1) * sizeof(TCHAR));
				GetWindowText(strItem);
			}
			if (i >= n || j >= nMaxItems)
				break;
			GetLBText(i++, s);
			string_trim_ws(s); // TODO: Is this really appropriate?
		}
		SettingStore.RegCloseKey(hKey);
	}
}

void HSuperComboBox::AutoCompleteFromLB(int nIndexFrom)
{
	if (GetKeyState(VK_BACK) < 0 || GetKeyState(VK_DELETE) < 0)
		return;

	HEdit *const pEdit = GetEditControl();
	// Don't take action on programmatic changes to the control's content!
	if (!pEdit->GetModify())
		return;

	// Get the text in the edit box
	String s;
	GetWindowText(s);
	// bail out if no text
	if (s.empty())
		return;

	// look for the string that is prefixed by the typed text
	int idx = FindString(nIndexFrom, s.c_str());
	if (idx == CB_ERR)
		return;

	// get the current selection
	DWORD sel = pEdit->GetSel();
	SetCurSel(idx);
	// select the text after our typing
	pEdit->SetSel(MAKELONG(sel, -1));
}

static WNDPROC DefWndProcDropping = NULL;

static LRESULT CALLBACK WndProcDropping(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult = ::CallWindowProc(DefWndProcDropping, hWnd, uMsg, wParam, lParam);
	switch (uMsg)
	{
	case WM_CAPTURECHANGED:
		::SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(DefWndProcDropping));
		DefWndProcDropping = NULL;
		break;
	case WM_WINDOWPOSCHANGING:
		WINDOWPOS *pwp = reinterpret_cast<WINDOWPOS *>(lParam);
		if ((pwp->flags & (SWP_NOMOVE | SWP_NOSIZE)) == 0)
		{
			COMBOBOXINFO info;
			info.cbSize = sizeof info;
			if (::GetComboBoxInfo(hWnd, &info))
			{
				HComboBox *pCb = reinterpret_cast<HComboBox *>(info.hwndCombo);
				RECT rc;
				pCb->GetWindowRect(&rc);
				RECT rcClip;
				H2O::GetDesktopWorkArea(pCb->m_hWnd, &rcClip);
				rcClip.right -= pwp->cx;
				rcClip.bottom -= pwp->cy;
				// Keep the dropped control within the work area
				if (pwp->x < rcClip.left)
					pwp->x = rcClip.left;
				if (pwp->x > rcClip.right)
					pwp->x = rcClip.right;
				if (pwp->y > rcClip.bottom)
					pwp->y = rc.top - pwp->cy;
				if (pwp->y < rcClip.top)
					pwp->y = rcClip.top;
			}
		}
		break;
	}
	return lResult;
}

void HSuperComboBox::AdjustDroppedWidth()
{
	COMBOBOXINFO info;
	info.cbSize = sizeof info;
	HComboBox *pCb = GetComboControl();
	if (!::GetComboBoxInfo(pCb ? pCb->m_hWnd : m_hWnd, &info))
		return;
	HListBox *pLb = reinterpret_cast<HListBox *>(info.hwndList);
	RECT rc;
	pLb->GetWindowRect(&rc);
	int cyEdge = rc.bottom - rc.top + info.rcButton.bottom + 1;
	pLb->GetClientRect(&rc);
	cyEdge -= rc.bottom;
	H2O::GetDesktopWorkArea(m_hWnd, &rc);
	int const cyItem = pLb->GetItemHeight(0);
	int const cyClip = (rc.bottom - rc.top - cyEdge) / 2 / cyItem * cyItem + cyEdge;
	GetWindowRect(&rc);
	SetWindowPos(NULL, 0, 0, rc.right - rc.left, cyClip, SWP_NOMOVE | SWP_NOZORDER);
	int cxMax = 0;
	int cchTextMax = 0;
	int const nCount = GetCount();
	int const cxExtra = GetSystemMetrics(SM_CXVSCROLL) + 2 * GetSystemMetrics(SM_CXBORDER);
	int const cxAvail = GetSystemMetrics(SM_CXSCREEN) - cxExtra;
	int nIndex;
	for (nIndex = 0 ; nIndex < nCount ; ++nIndex)
	{
		int cchText = GetLBTextLen(nIndex);
		if (cchTextMax < cchText)
			cchTextMax = cchText;
	}
	LPTSTR const pszText = static_cast<LPTSTR>(_alloca((cchTextMax + 1) * sizeof *pszText));
	if (HSurface *pdc = GetDC())
	{
		pdc->SelectObject(GetFont());
		for (nIndex = 0 ; nIndex < nCount ; ++nIndex)
		{
			int cchText = GetLBText(nIndex, pszText);
			SIZE size;
			if (pdc->GetTextExtent(pszText, cchText, &size) && (cxMax < size.cx))
				cxMax = size.cx;
		}
		ReleaseDC(pdc);
		if (cxMax > cxAvail)
			cxMax = cxAvail;
	}
	HWindow *pWndInner = this;
	if (HWindow *pWnd = GetEditControl())
		pWndInner = pWnd;
	else if (HWindow *pWnd = GetDlgItem(1001))
		pWndInner = pWnd;
	POINT pt = { cxMax + cxExtra, 0};
	pWndInner->MapWindowPoints(this, &pt, 1);
	SetDroppedWidth(pt.x);
	DefWndProcDropping = reinterpret_cast<WNDPROC>(
		::SetWindowLongPtr(info.hwndList, GWLP_WNDPROC,
		reinterpret_cast<LONG_PTR>(WndProcDropping)));
}

void HSuperComboBox::RestoreClientArea()
{
	if (wine_version)
		return; // It seems they missed to implement this glitch...
	// Trigger a WM_WINDOWPOSCHANGING to recover from some obscure corruption
	// which obstructs further drawing to a ComboBoxEx control's client area.
	SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE);
}
