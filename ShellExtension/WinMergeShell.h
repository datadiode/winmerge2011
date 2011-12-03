/////////////////////////////////////////////////////////////////////////////
//    License (GPLv2+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or (at
//    your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
/////////////////////////////////////////////////////////////////////////////
// Look at http://www.codeproject.com/shell/ for excellent guide
// to Windows Shell programming by Michael Dunn.
/**
 * @file  WinMergeShell.h
 *
 * @brief Declaration file for ShellExtension class
 */
// ID line follows -- this is updated by SVN
// $Id$

/**
 * @brief Class for handling shell extension tasks
 */
class CWinMergeShell : IShellExtInit, IContextMenu, IClassFactory
{
public:
	CWinMergeShell(HINSTANCE hInstance);
	~CWinMergeShell();
	void *operator new(size_t);
	void operator delete(void *) { }
	HRESULT RegisterClassObject(BOOL);
	ULONG GetLockCount() { return m_cRef; }

protected:
	HINSTANCE const m_hInstance;
	LONG m_cRef;
	LCID m_lcid;
	CMyComBSTR m_strPaths[2]; /**< Paths for selected items */
	CMyComBSTR m_strPreviousPath; /**< Previously selected path */
	HBITMAP m_MergeBmp; /**< Icon */
	UINT m_nSelectedItems; /**< Amount of selected items */
	DWORD m_dwContextMenuEnabled; /**< Is context menu enabled and in which mode? */
	DWORD m_dwMenuState; /**< Shown menuitems */

	int DrawSimpleMenu(HMENU hmenu, UINT uMenuIndex, UINT uidFirstCmd);
	int DrawAdvancedMenu(HMENU hmenu, UINT uMenuIndex, UINT uidFirstCmd);
	void SetWinMergeLocale();
	CMyComBSTR GetResourceString(UINT resourceID);
	CMyComBSTR GetHelpText(UINT_PTR idCmd);
	CMyComBSTR FormatCmdLine(LPCTSTR winmergePath,
		LPCTSTR path1, LPCTSTR path2, DWORD dwAlter);

public:
	// IUnknown
	STDMETHOD(QueryInterface)(REFIID, void **);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();
	// IClassFactory
	STDMETHOD(CreateInstance)(IUnknown *, const IID &, void **);
	STDMETHOD(LockServer)(BOOL);
	// IShellExtInit
	STDMETHOD(Initialize)(LPCITEMIDLIST, LPDATAOBJECT, HKEY);
	// IContextMenu
	STDMETHOD(GetCommandString)(UINT_PTR, UINT, UINT*, LPSTR, UINT);
	STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO);
	STDMETHOD(QueryContextMenu)(HMENU, UINT, UINT, UINT, UINT);
};
