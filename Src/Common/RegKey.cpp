/** 
 * @file  RegKey.cpp
 *
 * @brief Implementation of CRegKeyEx C++ wrapper class for reading Windows registry
 */
#include "StdAfx.h"
#include "RegKey.h"
#include "SettingStore.h"

static const CSettingStore NullSettingStore(NULL, NULL);

CRegKeyEx::CRegKeyEx()
: m_hKey(NULL)
, m_store(NullSettingStore)
{
}

/**
 * @brief Default constructor.
 */
CRegKeyEx::CRegKeyEx(HKEY hKey)
: m_hKey(hKey)
, m_store(SettingStore)
{
}

/**
 * @brief Default destructor.
 */
CRegKeyEx::~CRegKeyEx()
{
	Close();
}

/**
 * @brief Closes the key.
 */
void CRegKeyEx::Close()
{
	if (m_hKey) 
	{
		m_store.RegCloseKey(m_hKey);
		m_hKey = NULL;
	}
}

/**
 * @brief Opens or creates a key in given path.
 * @param [in] hKeyRoot Root key to open, HKLM, HKCU..
 * @param [in] pszPath Path to actual registry key to access.
 * @return ERROR_SUCCESS or error value.
 */
LONG CRegKeyEx::Open(HKEY hKeyRoot, LPCTSTR pszPath)
{
	return OpenWithAccess(hKeyRoot, pszPath, KEY_ALL_ACCESS);
}

/**
 * @brief Opens or creates a key in given path with access control.
 * @param [in] hKeyRoot Root key to open, HKLM, HKCU..
 * @param [in] pszPath Path to actual registry key to access.
 * @param [in] regsam Registry access parameter.
 * @return ERROR_SUCCESS or error value.
 */
LONG CRegKeyEx::OpenWithAccess(HKEY hKeyRoot, LPCTSTR pszPath, REGSAM regsam)
{
	Close();
	return m_store.RegCreateKeyEx(hKeyRoot, pszPath, REG_OPTION_NON_VOLATILE, regsam, &m_hKey);
}

/**
 * @brief Opens key in given path.
 * @param [in] hKeyRoot Root key to open, HKLM, HKCU..
 * @param [in] pszPath Path to actual registry key to access.
 * @param [in] regsam Registry access parameter.
 * @return ERROR_SUCCESS or error value.
 */
LONG CRegKeyEx::OpenNoCreateWithAccess(HKEY hKeyRoot, LPCTSTR pszPath, REGSAM regsam)
{
	Close();
	return RegOpenKeyEx(hKeyRoot, pszPath, 0, regsam, &m_hKey);
}

/**
 * @brief Opens registry key from HKEY_LOCAL_MACHINE for reading.
 * @param [in] key Path to actual registry key to access.
 * @return true on success, false otherwise.
 */
LONG CRegKeyEx::QueryRegMachine(LPCTSTR key)
{
	return OpenNoCreateWithAccess(HKEY_LOCAL_MACHINE, key, KEY_QUERY_VALUE);
}

/**
 * @brief Opens registry key from HKEY_CURRENT_USER for reading.
 * @param [in] key Path to actual registry key to access.
 * @return true on success, false otherwise.
 */
LONG CRegKeyEx::QueryRegUser(LPCTSTR key)
{
	return OpenNoCreateWithAccess(HKEY_CURRENT_USER, key, KEY_QUERY_VALUE);
}

/**
 * @brief Write DWORD value to registry.
 * @param [in] pszKey Path to actual registry key to access.
 * @param [in] dwVal Value to write.
 * @return ERROR_SUCCESS on success, or error value.
 */
LONG CRegKeyEx::WriteDword(LPCTSTR pszKey, DWORD dwVal) const
{
	assert(m_hKey);
	assert(pszKey);
	return m_store.RegSetValueEx(m_hKey, pszKey, REG_DWORD,
		(const LPBYTE) &dwVal, sizeof(DWORD));
}

/**
 * @brief Write string value to registry.
 * @param [in] pszKey Path to actual registry key to access.
 * @param [in] pszData Value to write.
 * @return ERROR_SUCCESS on success, or error value.
 */
LONG CRegKeyEx::WriteString(LPCTSTR pszKey, LPCTSTR pszData) const
{
	assert(m_hKey);
	assert(pszKey);
	assert(pszData);

	return m_store.RegSetValueEx(m_hKey, pszKey, REG_SZ,
		(const LPBYTE) pszData, (lstrlen(pszData) + 1) * sizeof(TCHAR));
}

/**
 * @brief Read DWORD value from registry.
 * @param [in] pszKey Path to actual registry key to access.
 * @param [in] defval Default value to return if reading fails.
 * @return Read DWORD value.
 */
DWORD CRegKeyEx::ReadDword(LPCTSTR pszKey, DWORD defval) const
{
	assert(m_hKey);
	assert(pszKey);

	DWORD dwType;
	DWORD dwSize = sizeof (DWORD);

	DWORD value;
	LONG ret = m_store.RegQueryValueEx(m_hKey, pszKey,
		&dwType, (LPBYTE)&value, &dwSize);

	if (ret != ERROR_SUCCESS || dwType != REG_DWORD)
		return defval;

	return value;
}

/**
 * @brief Read String value from registry.
 * @param [in] pszKey Path to actual registry key to access.
 * @param [in] defval Default value to return if reading fails.
 * @param [in] blob Memory in the caller's stack frame to receive the result.
 * @return Read String value.
 */
LPCTSTR CRegKeyEx::ReadString(LPCTSTR pszKey, LPCTSTR defval, BLOB *blob) const
{
	assert(m_hKey);
	assert(pszKey);
	DWORD dwType;
	LONG ret = m_store.RegQueryValueEx(m_hKey, pszKey, &dwType, blob->pBlobData, &blob->cbSize);
	return ret == ERROR_SUCCESS && dwType == REG_SZ ?
		reinterpret_cast<TCHAR *>(blob->pBlobData) : defval;
}

LONG CRegKeyEx::DeleteValue(LPCTSTR pszKey) const
{
	return m_store.RegDeleteValue(m_hKey, pszKey);
}
