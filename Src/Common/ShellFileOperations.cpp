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
#include "paths.h"
#include "ShellFileOperations.h"

ShellFileOperations::ShellFileOperations(
	HWND hwnd, UINT wFunc, FILEOP_FLAGS fFlags, size_t fromMax, size_t toMax)
{
	stl::auto_ptr<TCHAR, true> apFrom(fromMax ? new TCHAR[fromMax + 1] : NULL);
	stl::auto_ptr<TCHAR, true> apTo(toMax ? new TCHAR[toMax + 1] : NULL);
	SHFILEOPSTRUCT::hwnd = H2O::GetTopLevelParent(hwnd);
	SHFILEOPSTRUCT::wFunc = wFunc;
	SHFILEOPSTRUCT::fFlags = fFlags;
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
	bool succeeded = true;
	if (pFrom != NULL)
	{
		EnableWindow(hwnd, FALSE);
		succeeded = SHFileOperation(this) == 0 && !fAnyOperationsAborted;
		EnableWindow(hwnd, TRUE);
	}
	return succeeded;
}

LPTSTR ShellFileOperations::AddPath(LPTSTR p, LPCTSTR path)
{
	_tcscpy(p, path);
	LPCTSTR q = paths_UndoMagic(p);
	size_t n = _tcslen(q);
	if (n >= MAX_PATH)
	{
		if (DWORD k = GetShortPathName(path, p, MAX_PATH))
		{
			q = paths_UndoMagic(p);
			n = k - (q - p);
		}
	}
	memmove(p, q, ++n * sizeof(TCHAR));
	*(p += n) = _T('\0');
	return p;
}
