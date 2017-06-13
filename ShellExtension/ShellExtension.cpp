/////////////////////////////////////////////////////////////////////////////
// ShellExtension.cpp : Implementation of DLL Exports.
//
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
//

#include "StdAfx.h"
#include "resource.h"
#include <initguid.h>

#include <atliface_h.h>
#include <atliface_i.c>

#include "WinMergeShell.h"
#include "RegKey.h"
#include "findhta.h"

#include <TlHelp32.h>

#ifdef _WIN64
#define BITNESS_INFO L"[64-bit]"
#else
#define BITNESS_INFO L"[32-bit]"
#endif

DEFINE_GUID(CLSID_WinMergeShell,0x4E716236,0xAA30,0x4C65,0xB2,0x25,0xD6,0x8B,0xBA,0x81,0xE9,0xC2);

static CWinMergeShell *pModule = NULL;

static WCHAR const ModuleAtom[] = L"ModuleAtom:4e716236-aa30-4c65-b225-d68bba81e9c2";

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C" BOOL WINAPI _DllMainCRTStartup(HINSTANCE hInstance, DWORD dwReason, LPVOID)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		// Refuse to load into th process of the setup routine
		if (GetModuleHandleW(L"mshta.exe") && StrStrIW(GetCommandLineW(), ModuleAtom))
			return FALSE;
		AddAtomW(ModuleAtom);
		pModule = new CWinMergeShell(hInstance);
		DisableThreadLibraryCalls(hInstance);
		break;
	case DLL_PROCESS_DETACH:
		delete pModule;
		break;
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow()
{
	return pModule->GetLockCount() == 0 ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	if (rclsid == CLSID_WinMergeShell)
		return pModule->QueryInterface(riid, ppv);
	return CLASS_E_CLASSNOTAVAILABLE;
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer()
{
	return pModule->RegisterClassObject(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer()
{
	return pModule->RegisterClassObject(FALSE);
}

template<typename P>
class DllImport
{
	FARPROC const p;
public:
	DllImport(HMODULE module, LPCSTR name): p(GetProcAddress(module, name)) { }
	P operator *() const { return reinterpret_cast<P>(p); }
};

static BOOL KillHostingProcesses(HWND hWnd)
{
    HMODULE const kernel32 = GetModuleHandleW(L"kernel32");
	DllImport<BOOL (APIENTRY *)(HANDLE, PBOOL)> const IsWow64Process(kernel32, "IsWow64Process");

	HANDLE const hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
		return FALSE;

	int choice = 0;

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof pe32;
	if (Process32First(hProcessSnap, &pe32)) do
	{
		if (pe32.th32ProcessID == GetCurrentProcessId())
			continue;
		if (HANDLE const hProcess = OpenProcess(
			PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID))
		{
			int veto = 0;
			if (*IsWow64Process)
			{
				BOOL bIsWow46Process;
				if (!(*IsWow64Process)(hProcess, &bIsWow46Process))
					veto |= 2; // veto due to unclear situation
				else if (bIsWow46Process)
					veto ^= 1; // mutually canceling veto due to bitness mismatch
				if (!(*IsWow64Process)(GetCurrentProcess(), &bIsWow46Process))
					veto |= 2; // veto due to unclear situation
				else if (bIsWow46Process)
					veto ^= 1; // mutually canceling veto due to bitness mismatch
			}
			if (veto == 0)
			{
				DWORD dwExitCode = 0;
				if (LPVOID const pVirtual = VirtualAllocEx(
					hProcess, NULL, sizeof ModuleAtom, MEM_COMMIT, PAGE_READWRITE))
				{
					if (WriteProcessMemory(
						hProcess, pVirtual, ModuleAtom, sizeof ModuleAtom, NULL))
					{
						if (HANDLE const hThread = CreateRemoteThread(
							hProcess, NULL, 0,
							reinterpret_cast<LPTHREAD_START_ROUTINE>(FindAtomW),
							pVirtual, 0, NULL))
						{
							WaitForSingleObject(hThread, INFINITE);
							GetExitCodeThread(hThread, &dwExitCode);
							CloseHandle(hThread);
						}
					}
					VirtualFreeEx(hProcess, pVirtual, 0, MEM_RELEASE);
				}
				if (dwExitCode != 0)
				{
					WCHAR msg[1024];
					wsprintfW(msg,
						L"The WinMerge shell extension is used by the following process:"
						L"\n\n%s\n\n"
						L"Do you want to terminate this process?",
						pe32.szExeFile);
					choice = MessageBoxW(hWnd, msg,
						L"ShellExtension " BITNESS_INFO,
						MB_ICONWARNING | MB_YESNOCANCEL);
					if (choice == IDYES)
					{
						TerminateProcess(hProcess, dwExitCode);
						WaitForSingleObject(hProcess, 5000);
					}
				}
			}
			CloseHandle(hProcess);
		}
	} while (choice != IDCANCEL && Process32Next(hProcessSnap, &pe32));

	CloseHandle(hProcessSnap);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// DllInstall - Allows for restarting explorer.exe with /i:Shell_TrayWnd

STDAPI DllInstall(BOOL, LPCWSTR lpCmdLine)
{
	KillHostingProcesses(FindHTA(lpCmdLine));
	return S_OK;
}
