// SuperComboBox.cpp : implementation file
//
#include "StdAfx.h"
#include "coretools.h"
#include "SettingStore.h"
#include "SuperComboBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
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
		TCHAR *const s = reinterpret_cast<TCHAR *>(_alloca(bufsize));
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
	if (HKEY hKey = SettingStore.GetSectionKey(szRegSubKey, CREATE_ALWAYS))
	{
		String strItem, s;
		GetWindowText(s);
		UINT i = 0;
		UINT j = 0;
		UINT n = GetCount();
		for (;;)
		{
			string_trim_ws(s);
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
	case WM_CTLCOLORLISTBOX:
		::SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(DefWndProcDropping));
		DefWndProcDropping = NULL;
		if (HListBox *pLb = reinterpret_cast<HListBox *>(lParam))
		{
			RECT rc;
			pLb->GetWindowRect(&rc);
			RECT rcBounds;
			H2O::GetDesktopWorkArea(hWnd, &rcBounds);
			rcBounds.right -= rc.right - rc.left;
			rcBounds.bottom -= rc.bottom - rc.top;
			// Keep the dropped control within the work area
			if (rc.left < rcBounds.left)
				rc.left = rcBounds.left;
			if (rc.left > rcBounds.right)
				rc.left = rcBounds.right;
			if (rc.top < rcBounds.top)
				rc.top = rcBounds.top;
			if (rc.top > rcBounds.bottom)
				rc.top = rcBounds.bottom;
			pLb->SetWindowPos(NULL, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
		}
		break;
	}
	return lResult;
}

void HSuperComboBox::AdjustDroppedWidth()
{
	int cxMax = 0;
	int cchTextMax = 0;
	const int nCount = GetCount();
	const int cxExtra = GetSystemMetrics(SM_CXVSCROLL) + 2 * GetSystemMetrics(SM_CXBORDER);
	const int cxAvail = GetSystemMetrics(SM_CXSCREEN) - cxExtra;
	int nIndex;
	for (nIndex = 0 ; nIndex < nCount ; ++nIndex)
	{
		int cchText = GetLBTextLen(nIndex);
		if (cchTextMax < cchText)
			cchTextMax = cchText;
	}
	LPTSTR const pszText = reinterpret_cast<LPTSTR>(_alloca((cchTextMax + 1) * sizeof *pszText));
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
	if (DefWndProcDropping == NULL)
	{
		DefWndProcDropping = reinterpret_cast<WNDPROC>(
			SetWindowLongPtr(m_hWnd, GWLP_WNDPROC,
			reinterpret_cast<LONG_PTR>(WndProcDropping)));
	}
}

void HSuperComboBox::EnsureSelection()
{
	int sel = GetCurSel();
	String text;
	GetWindowText(text);
	SetCurSel(sel != CB_ERR && !text.empty() ? sel : 0);
	SetWindowText(text.c_str());
}
