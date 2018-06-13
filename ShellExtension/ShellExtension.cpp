/////////////////////////////////////////////////////////////////////////////
// ShellExtension.cpp : Implementation of DLL Exports.
//
/////////////////////////////////////////////////////////////////////////////
//    License (GPLv3+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 3 of the License, or (at
//    your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

static DWORD QueryJobObjectUiRestrictions()
{
	JOBOBJECT_BASIC_UI_RESTRICTIONS BasicUIRestrictions = { 0 };
	if (QueryInformationJobObject(NULL, JobObjectBasicUIRestrictions, &BasicUIRestrictions, sizeof BasicUIRestrictions, NULL))
		return BasicUIRestrictions.UIRestrictionsClass;
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C" BOOL WINAPI _DllMainCRTStartup(HINSTANCE hInstance, DWORD dwReason, LPVOID)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		// Refuse to load into the process of the setup routine
		if (GetModuleHandleW(L"mshta.exe") && StrStrIW(GetCommandLineW(), ModuleAtom))
			return FALSE;
		// Refuse to load into ShellExperienceHost.exe to work around Windows 10
		if (GetModuleHandleW(L"ShellExperienceHost.exe"))
			return FALSE;
		// Refuse to load into a process which is part of a job object
		if (QueryJobObjectUiRestrictions() & JOB_OBJECT_UILIMIT_GLOBALATOMS)
			return FALSE;
		if (GlobalFindAtomW(ModuleAtom))
		{
			TCHAR exe[MAX_PATH];
			GetModuleFileNameW(NULL, exe, _countof(exe));
			TCHAR dll[MAX_PATH];
			// hInstance = GetModuleHandle(L"kernel32"); // for testing
			GetModuleFileNameW(hInstance, dll, _countof(dll));
			if (int prefix = PathCommonPrefixW(exe, dll, NULL))
				if (dll[prefix] == L'\\' && PathIsFileSpecW(dll + prefix + 1))
					AddAtomW(ModuleAtom);
			return FALSE;
		}
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
		return S_FALSE;

	ATOM const globalAtom = GlobalAddAtomW(ModuleAtom);

	WCHAR path[MAX_PATH];
	GetModuleFileName(pModule->m_hInstance, path, _countof(path));

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
					hProcess, NULL, 1024, MEM_COMMIT, PAGE_READWRITE))
				{
					if (WriteProcessMemory(
						hProcess, pVirtual, path, sizeof path, NULL))
					{
						if (HANDLE const hThread = CreateRemoteThread(
							hProcess, NULL, 0,
							reinterpret_cast<LPTHREAD_START_ROUTINE>(LoadLibraryW),
							pVirtual, 0, NULL))
						{
							// Have suspended processes time out after a moderate delay
							WaitForSingleObject(hThread, 100);
							GetExitCodeThread(hThread, &dwExitCode);
							if (dwExitCode == STILL_ACTIVE)
								TerminateThread(hThread, 0);
							CloseHandle(hThread);
						}
					}
					if (WriteProcessMemory(
						hProcess, pVirtual, ModuleAtom, sizeof ModuleAtom, NULL))
					{
						if (HANDLE const hThread = CreateRemoteThread(
							hProcess, NULL, 0,
							reinterpret_cast<LPTHREAD_START_ROUTINE>(FindAtomW),
							pVirtual, 0, NULL))
						{
							// Have suspended processes time out after a moderate delay
							WaitForSingleObject(hThread, 100);
							GetExitCodeThread(hThread, &dwExitCode);
							if (dwExitCode == STILL_ACTIVE)
								TerminateThread(hThread, 0);
							CloseHandle(hThread);
						}
					}
					VirtualFreeEx(hProcess, pVirtual, 0, MEM_RELEASE);
				}
				if (dwExitCode >= 0xC000)
				{
					WCHAR msg[1024];
					wsprintfW(msg,
						L"The following process belongs to or uses part of WinMerge:"
						L"\n\n%s\n\n"
						L"Do you want to terminate this process?",
						pe32.szExeFile);
					choice = MessageBoxW(hWnd, msg,
						L"ShellExtension " BITNESS_INFO,
						MB_ICONWARNING | MB_YESNOCANCEL);
					if (choice == IDYES)
					{
						TerminateProcess(hProcess, 0);
						WaitForSingleObject(hProcess, 5000);
					}
				}
			}
			CloseHandle(hProcess);
		}
	} while (choice != IDCANCEL && Process32Next(hProcessSnap, &pe32));

	GlobalDeleteAtom(globalAtom);
	CloseHandle(hProcessSnap);
	return choice == IDCANCEL ? E_ABORT : S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// DllInstall - Allows for restarting explorer.exe with /i:Shell_TrayWnd

STDAPI DllInstall(BOOL, LPCWSTR lpCmdLine)
{
	return KillHostingProcesses(FindHTA(lpCmdLine));
}
