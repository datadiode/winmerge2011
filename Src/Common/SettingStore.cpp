/// <copyright company="JetBrains">
/// Copyright © 2003-2008 JetBrains s.r.o.
/// You may distribute under the terms of the GNU General Public License, as published by the Free Software Foundation, version 2 (see License.txt in the repository root folder).
/// </copyright>

// © JetBrains, Inc, 2005
// Written by (H) Serge Baltic

// Copyright © 2011 WinMerge Team

#include <StdAfx.h>
#include "SettingStore.h"

CSettingStore::CSettingStore(LPCTSTR sCompanyName, LPCTSTR sApplicationName)
{
	// Store
	m_sCompanyName = sCompanyName;
	m_sApplicationName = sApplicationName;
}

int CSettingStore::GetProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int nDefault) const
{
	ASSERT(lpszSection != NULL);
	ASSERT(lpszEntry != NULL);

	HKEY hSecKey = GetSectionKey(lpszSection);
	if (hSecKey == NULL)
		return nDefault;
	DWORD dwValue;
	DWORD dwType;
	DWORD dwCount = sizeof(DWORD);
	LONG lResult = RegQueryValueEx(hSecKey, lpszEntry, NULL, &dwType,
		(LPBYTE)&dwValue, &dwCount);
	RegCloseKey(hSecKey);
	if (lResult == ERROR_SUCCESS)
	{
		ASSERT(dwType == REG_DWORD);
		ASSERT(dwCount == sizeof(dwValue));
		return (UINT)dwValue;
	}
	return nDefault;
}

String CSettingStore::GetProfileString(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszDefault) const
{
	ASSERT(lpszSection != NULL);
	ASSERT(lpszEntry != NULL);

	HKEY hSecKey = GetSectionKey(lpszSection);
	if (hSecKey == NULL)
		return lpszDefault;
	String strValue;
	DWORD dwType, dwCount;
	LONG lResult = RegQueryValueEx(hSecKey, lpszEntry, NULL, &dwType,
		NULL, &dwCount);
	if (lResult == ERROR_SUCCESS)
	{
		ASSERT(dwType == REG_SZ);
		strValue.resize(dwCount / sizeof(TCHAR));
		lResult = RegQueryValueEx(hSecKey, lpszEntry, NULL, &dwType,
			(LPBYTE)&strValue[0], &dwCount);
	}
	RegCloseKey(hSecKey);
	if (lResult == ERROR_SUCCESS)
	{
		ASSERT(dwType == REG_SZ);
		return strValue;
	}
	return lpszDefault;
}

BOOL CSettingStore::GetProfileBinary(LPCTSTR lpszSection, LPCTSTR lpszEntry, BYTE** ppData, UINT* pBytes) const
{
	ASSERT(lpszSection != NULL);
	ASSERT(lpszEntry != NULL);
	ASSERT(ppData != NULL);
	ASSERT(pBytes != NULL);
	*ppData = NULL;
	*pBytes = 0;

	HKEY hSecKey = GetSectionKey(lpszSection);
	if (hSecKey == NULL)
		return FALSE;

	DWORD dwType, dwCount;
	LONG lResult = RegQueryValueEx(hSecKey, lpszEntry, NULL, &dwType,
		NULL, &dwCount);
	*pBytes = dwCount;
	if (lResult == ERROR_SUCCESS)
	{
		ASSERT(dwType == REG_BINARY);
		*ppData = new BYTE[*pBytes];
		lResult = RegQueryValueEx(hSecKey, lpszEntry, NULL, &dwType,
			*ppData, &dwCount);
	}
	RegCloseKey(hSecKey);
	if (lResult == ERROR_SUCCESS)
	{
		ASSERT(dwType == REG_BINARY);
		return TRUE;
	}
	else
	{
		delete [] *ppData;
		*ppData = NULL;
	}
	return FALSE;

}

BOOL CSettingStore::WriteProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int nValue) const
{
	ASSERT(lpszSection != NULL);
	ASSERT(lpszEntry != NULL);

	HKEY hSecKey = GetSectionKey(lpszSection);
	if (hSecKey == NULL)
		return FALSE;
	LONG lResult = RegSetValueEx(hSecKey, lpszEntry, NULL, REG_DWORD,
		(LPBYTE)&nValue, sizeof(nValue));
	RegCloseKey(hSecKey);
	return lResult == ERROR_SUCCESS;
}

BOOL CSettingStore::WriteProfileString(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszValue) const
{
	ASSERT(lpszSection != NULL);

	LONG lResult;
	if (lpszEntry == NULL) //delete whole section
	{
		HKEY hAppKey = GetAppRegistryKey();
		if (hAppKey == NULL)
			return FALSE;
		lResult = ::RegDeleteKey(hAppKey, lpszSection);
		RegCloseKey(hAppKey);
	}
	else if (lpszValue == NULL)
	{
		HKEY hSecKey = GetSectionKey(lpszSection);
		if (hSecKey == NULL)
			return FALSE;
		// necessary to cast away const below
		lResult = ::RegDeleteValue(hSecKey, (LPTSTR)lpszEntry);
		RegCloseKey(hSecKey);
	}
	else
	{
		HKEY hSecKey = GetSectionKey(lpszSection);
		if (hSecKey == NULL)
			return FALSE;
		lResult = RegSetValueEx(hSecKey, lpszEntry, NULL, REG_SZ,
			(LPBYTE)lpszValue, (lstrlen(lpszValue)+1)*sizeof(TCHAR));
		RegCloseKey(hSecKey);
	}
	return lResult == ERROR_SUCCESS;
}

BOOL CSettingStore::WriteProfileBinary(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPBYTE pData, UINT nBytes) const
{
	ASSERT(lpszSection != NULL);

	LONG lResult;
	HKEY hSecKey = GetSectionKey(lpszSection);
	if (hSecKey == NULL)
		return FALSE;
	lResult = RegSetValueEx(hSecKey, lpszEntry, NULL, REG_BINARY,
		pData, nBytes);
	RegCloseKey(hSecKey);
	return lResult == ERROR_SUCCESS;
}

// returns key for HKEY_CURRENT_USER\"Software"\RegistryKey\ProfileName
// creating it if it doesn't exist
// responsibility of the caller to call RegCloseKey() on the returned HKEY
HKEY CSettingStore::GetAppRegistryKey() const
{
	ASSERT(!m_sCompanyName.empty());
	ASSERT(!m_sApplicationName.empty());

	HKEY hAppKey = NULL;
	HKEY hSoftKey = NULL;
	HKEY hCompanyKey = NULL;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("software"), 0, KEY_WRITE|KEY_READ,
		&hSoftKey) == ERROR_SUCCESS)
	{
		DWORD dw;
		if (RegCreateKeyEx(hSoftKey, m_sCompanyName.c_str(), 0, REG_NONE,
			REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL,
			&hCompanyKey, &dw) == ERROR_SUCCESS)
		{
			RegCreateKeyEx(hCompanyKey, m_sApplicationName.c_str(), 0, REG_NONE,
				REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL,
				&hAppKey, &dw);
		}
	}
	if (hSoftKey != NULL)
		RegCloseKey(hSoftKey);
	if (hCompanyKey != NULL)
		RegCloseKey(hCompanyKey);

	return hAppKey;
}

// returns key for:
//      HKEY_CURRENT_USER\"Software"\RegistryKey\AppName\lpszSection
// creating it if it doesn't exist.
// responsibility of the caller to call RegCloseKey() on the returned HKEY
HKEY CSettingStore::GetSectionKey(LPCTSTR lpszSection, DWORD dwCreationDisposition) const
{
	ASSERT(lpszSection != NULL);

	HKEY hSectionKey = NULL;
	HKEY hAppKey = GetAppRegistryKey();
	if (hAppKey == NULL)
		return NULL;

	if (dwCreationDisposition == CREATE_ALWAYS)
		SHDeleteKey(hAppKey, lpszSection);

	DWORD dw;
	RegCreateKeyEx(hAppKey, lpszSection, 0, REG_NONE,
		REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL,
		&hSectionKey, &dw);
	RegCloseKey(hAppKey);
	return hSectionKey;
}
