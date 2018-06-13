/////////////////////////////////////////////////////////////////////////////
//    License (GPLv3+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
/////////////////////////////////////////////////////////////////////////////
/**
 * @file  ShellFileOperations.h
 *
 * @brief Declaration file for ShellFileOperations class.
 */
#pragma once

/**
 * @brief A wrapper class for SHFileOperation().
 * This class offers easier way to use SHFileOperation(). User can just add
 * paths without needing to care about adding correct amount of NULLs.
 */
class ShellFileOperations
	: ZeroInit<ShellFileOperations>
	, public SHFILEOPSTRUCT
{
public:
	static const DWORD UseLowLevelFunctions = 0x10000;

	ShellFileOperations(HWND, UINT, DWORD, size_t fromMax, size_t toMax);
	~ShellFileOperations();

	bool Run();

	void AddSource(LPCTSTR source)
	{
		pFrom = AddPath(pFrom, source);
	}
	void AddDestination(LPCTSTR destination)
	{
		pTo = AddPath(pTo, destination);
	}

private:
	LPTSTR AddPath(LPTSTR, LPCTSTR);
	DWORD const dwFlags;
	TCHAR *pFrom;
	TCHAR *pTo;
};
