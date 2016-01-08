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
/**
 * @file  ShellFileOperations.cpp
 *
 * @brief Implementation of ShellFileOperations class.
 */
#include "StdAfx.h"
#include "resource.h"
#include "paths.h"
#include "ShellFileOperations.h"
#include "LanguageSelect.h"
#include "FileOperationsDlg.h"

ShellFileOperations::ShellFileOperations(HWND hwnd, UINT wFunc,
	DWORD dwFlags, size_t fromMax, size_t toMax) : dwFlags(dwFlags)
{
	std::auto_ptr<TCHAR, true> apFrom(fromMax ? new TCHAR[fromMax + 1] : NULL);
	std::auto_ptr<TCHAR, true> apTo(toMax ? new TCHAR[toMax + 1] : NULL);
	SHFILEOPSTRUCT::hwnd = H2O::GetTopLevelParent(hwnd);
	SHFILEOPSTRUCT::wFunc = wFunc;
	SHFILEOPSTRUCT::fFlags = LOWORD(dwFlags);
	SHFILEOPSTRUCT::pFrom = pFrom = apFrom.release();
	SHFILEOPSTRUCT::pTo = pTo = apTo.release();
}

ShellFileOperations::~ShellFileOperations()
{
	delete[] SHFILEOPSTRUCT::pFrom;
	delete[] SHFILEOPSTRUCT::pTo;
}

bool ShellFileOperations::Run()
{
	int result = 0;
	if (pFrom > SHFILEOPSTRUCT::pFrom)
	{
		if (dwFlags & UseLowLevelFunctions)
		{
			FileOperationsDlg dlg(*this);
			result = LanguageSelect.DoModal(dlg, hwnd);
		}
		else
		{
			EnableWindow(hwnd, FALSE);
			result = SHFileOperation(this);
			EnableWindow(hwnd, TRUE);
		}
	}
	return result == 0 && !fAnyOperationsAborted;
}

LPTSTR ShellFileOperations::AddPath(LPTSTR p, LPCTSTR q)
{
	if (!(dwFlags & UseLowLevelFunctions))
	{
		_tcscpy(p, q);
		q = paths_UndoMagic(p);
	}
	size_t n = _tcslen(q);
	memmove(p, q, ++n * sizeof(TCHAR));
	*(p += n) = _T('\0');
	return p;
}
