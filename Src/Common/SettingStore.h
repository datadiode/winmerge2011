/// <copyright company="JetBrains">
/// Copyright © 2003-2008 JetBrains s.r.o.
/// You may distribute under the terms of the GNU General Public License, as published by the Free Software Foundation, version 2 (see License.txt in the repository root folder).
/// </copyright>

// JetIe Setting Store
//
// Generic implementation for a Registry setting store,
// which has a strict list of settings and default values so that they could be accessed
// via an ID to avoid disagreement in default values, names and locations.
//
// © JetBrains, Inc, 2005
// Written by (H) Serge Baltic

// Copyright © 2011-2015 WinMerge Team
// Adaptions for WinMerge sum up to undeprecating the deprecated methods and
// removing access through numeric IDs. Being basically a degradation, they aim
// at nothing but providing those MFC-like methods our code happens to rely on.

#pragma once

#include "UnicodeString.h"

// TODO: implement critical sections to synchronize access to this object from different threads.
extern class CSettingStore
{
public:
	/// Initializes the instance by specifying the registry location which will point to the HKCR\Software\<CompanyName>\<ApplicationName> key as the settints store root key.
	CSettingStore(LPCTSTR sCompanyName, LPCTSTR sApplicationName);
	~CSettingStore();

	BOOL SetFileName(LPCTSTR sFileName);
// Operations

	/// Returns an integer option value (or a default value, if absent), identified by the section specifying the Registry key (use a backslash to separate subkeys) and the entry which defines the value name within that key, which must be of the appropriate type.
	int GetProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int nDefault) const;

	/// Returns a string option value (or a default value, if absent), identified by the section specifying the Registry key (use a backslash to separate subkeys) and the entry which defines the value name within that key, which must be of the appropriate type.
	String GetProfileString(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszDefault = _T("")) const;

	/// Persists an integer option value, identified by the section specifying the Registry key (use a backslash to separate subkeys) and the entry which defines the value name within that key.
	BOOL WriteProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int nValue) const;

	/// Persists a string option value, identified by the section specifying the Registry key (use a backslash to separate subkeys) and the entry which defines the value name within that key.
	BOOL WriteProfileString(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszValue) const;

	/// Returns the application's Registry Key as a WinAPI entity, which must be explicitly closed by the caller.
	HKEY GetAppRegistryKey() const;

	/// Returns the registry key to the section where the application settings are stored, which must be explicitly closed by the caller.
	HKEY GetSectionKey(LPCTSTR lpszSection, DWORD dwCount = 0xFFFFFFFF) const;

	LONG RegCreateKeyEx(HKEY, LPCTSTR, DWORD, REGSAM, PHKEY) const;
	LONG RegCloseKey(HKEY) const;
	LONG RegQueryValueEx(HKEY, LPCTSTR, LPDWORD, LPBYTE, LPDWORD) const;
	LONG RegSetValueEx(HKEY, LPCTSTR, DWORD, CONST BYTE *, DWORD) const;
	LONG SHDeleteKey(HKEY, LPCTSTR) const;
	LONG RegDeleteValue(HKEY, LPCTSTR) const;
	LONG RegQueryInfoKey(HKEY, LPDWORD, LPDWORD = NULL) const;
private:
	/// Handle to external registry hive if mounted
	HKEY m_hHive;
	/// Access permissions to request (either KEY_READ | KEY_WRITE or KEY_READ)
	REGSAM m_regsam;;
	/// Whether any settings were changed after loading
	mutable bool m_bDirty;
	/// Filename for portable media setup
	String m_sFileName;
	/// Company name in the registry key (under HKCU/Software).
	LPCTSTR m_sCompanyName;
	/// Application name in the registry (under the company key).
	LPCTSTR m_sApplicationName;	
#ifdef _DEBUG
	/// Help catch accidental calls from a different thread
	class CThreadIntegrity
	{
		DWORD const id;
	public:
		CThreadIntegrity() : id(GetCurrentThreadId()) { }
		operator bool() const { return id == GetCurrentThreadId(); }
	} ThreadIntegrity;
#endif
	/// Disallow copy construction.
	CSettingStore(const CSettingStore &other);
	/// Disallow assignment.
	CSettingStore &operator=(const CSettingStore &other);
} SettingStore;
