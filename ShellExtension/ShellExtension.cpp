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
// $Id$

// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL,
//      run nmake -f ShellExtensionps.mk in the project directory.

#include "StdAfx.h"
#include "resource.h"
#include <initguid.h>

#include <atliface_h.h>
#include <atliface_i.c>

#include "WinMergeShell.h"
#include "RegKey.h"

DEFINE_GUID(CLSID_WinMergeShell,0x4E716236,0xAA30,0x4C65,0xB2,0x25,0xD6,0x8B,0xBA,0x81,0xE9,0xC2);

static CWinMergeShell *pModule = NULL;

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C" BOOL WINAPI _DllMainCRTStartup(HINSTANCE hInstance, DWORD dwReason, LPVOID)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		pModule = new CWinMergeShell(hInstance);
		DisableThreadLibraryCalls(hInstance);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		delete pModule;
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
	// If we're on NT 4.0, add ourselves to the list of approved shell extensions.

	// Note that you should *NEVER* use the overload of CRegKey::SetValue with
	// 4 parameters.  It lets you set a value in one call, without having to
	// call CRegKey::Open() first.  However, that version of SetValue() has a
	// bug in that it requests KEY_ALL_ACCESS to the key.  That will fail if the
	// user is not an administrator.  (The code should request KEY_WRITE, which
	// is all that's necessary.)

	// Read also:
	// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/shellcc/
	// platform/shell/programmersguide/shell_int/shell_int_extending/
	// extensionhandlers/shell_ext.asp

	// Special code for Windows NT 4.0 only
	if ((GetVersion() & 0x8000FFFFUL) == 4)
	{
		CRegKeyEx reg;
		LONG lRet;

		lRet = reg.Open(HKEY_LOCAL_MACHINE,
			_T("Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"));

		if (ERROR_SUCCESS != lRet)
			return E_ACCESSDENIED;

		lRet = reg.WriteString(_T("WinMerge_Shell Extension"),
			_T("{4E716236-AA30-4C65-B225-D68BBA81E9C2}"));

		if (ERROR_SUCCESS != lRet)
			return E_ACCESSDENIED;
	}
	return pModule->RegisterClassObject(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer()
{
	// If we're on NT 4.0, remove ourselves from the list of approved shell extensions.
	// Note that if we get an error along the way, I don't bail out since I want
	// to do the normal ATL unregistration stuff too.

	// Special code for Windows NT 4.0 only
	if ((GetVersion() & 0x8000FFFFUL) == 4)
	{
		CRegKeyEx reg;
		LONG lRet;

		lRet = reg.OpenWithAccess(HKEY_LOCAL_MACHINE,
			_T("Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"),
			KEY_SET_VALUE);

		if (ERROR_SUCCESS == lRet)
		{
			lRet = reg.DeleteValue(_T("{4E716236-AA30-4C65-B225-D68BBA81E9C2}"));
		}
	}
	return pModule->RegisterClassObject(FALSE);
}
