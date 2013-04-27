/* RunModal.cpp: Run external program in a modal fashion

Copyright (c) 2005 Jochen Tucht

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

Last change: 2013-04-27 by Jochen Neubeck
*/
#include "stdafx.h"
#include "RunModal.h"

/**
 * @brief Wait for process to terminate
 */
HRESULT NTAPI RunModal(HANDLE hProcess)
{
	DWORD dwExitCode = STILL_ACTIVE;
	while (::GetExitCodeProcess(hProcess, &dwExitCode) && dwExitCode == STILL_ACTIVE)
	{
		::WaitForSingleObject(hProcess, 500);
		MSG msg;
		while (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}
	::CloseHandle(hProcess);
	return dwExitCode;
}

/**
 * @brief Run program in a modal fashion using STARTUPINFO
 */
HRESULT NTAPI RunModal(STARTUPINFO &si, LPCTSTR lpCommandLine, LPCTSTR lpCurrentDirectory)
{
	HRESULT dwExitCode = CO_E_CREATEPROCESS_FAILURE;
	PROCESS_INFORMATION pi;
	ASSERT(si.cb == sizeof si);
	if (CreateProcess(NULL, (LPTSTR)lpCommandLine, NULL, NULL, TRUE,
		0, NULL, lpCurrentDirectory, &si, &pi))
	{
		CloseHandle(pi.hThread);
		dwExitCode = RunModal(pi.hProcess);
	}
	return dwExitCode;
}

/**
 * @brief Run program in a modal fashion using SHELLEXECUTEINFO
 */
HRESULT NTAPI RunModal(SHELLEXECUTEINFO &si)
{
	HRESULT dwExitCode = CO_E_CREATEPROCESS_FAILURE;
	ASSERT(si.cbSize == sizeof si);
	if (ShellExecuteEx(&si))
	{
		dwExitCode = RunModal(si.hProcess);
	}
	return dwExitCode;
}
