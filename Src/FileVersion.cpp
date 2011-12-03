/** 
 * @file  FileVersion.cpp
 *
 * @brief Implementation for FileVersion
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "FileVersion.h"

/**
 * @brief Default constructor.
 */
FileVersion::FileVersion()
: m_bFileVersionSet(false)
, m_fileVersionMS(0)
, m_fileVersionLS(0)
, m_bProductVersionSet(false)
, m_productVersionMS(0)
, m_productVersionLS(0)
{
}

/**
 * @brief Reset version data to zeroes.
 */
void FileVersion::Clear()
{
	m_bFileVersionSet = false;
	m_fileVersionMS = 0;
	m_fileVersionLS = 0;
	m_bProductVersionSet = false;
	m_productVersionMS = 0;
	m_productVersionLS = 0;
}

/**
 * @brief Set file version number.
 * @param [in] versionMS Most significant dword for version.
 * @param [in] versionLS Least significant dword for version.
 */
void FileVersion::SetFileVersion(DWORD versionMS, DWORD versionLS)
{
	m_bFileVersionSet = true;
	m_fileVersionMS = versionMS;
	m_fileVersionLS = versionLS;
}

/**
 * @brief Set product version number.
 * @param [in] versionMS Most significant dword for version.
 * @param [in] versionLS Least significant dword for version.
 */
void FileVersion::SetProductVersion(DWORD versionMS, DWORD versionLS)
{
	m_bProductVersionSet = true;
	m_productVersionMS = versionMS;
	m_productVersionLS = versionLS;
}

/**
 * @brief Get file version as a string.
 * @return File version number as a string. Returns empty string if there is
 * no version number for the file.
 */
String FileVersion::GetFileVersionString()
{
	if (!m_bFileVersionSet)
		return _T("");

	TCHAR ver[30];
	_sntprintf(ver, _countof(ver), _T("%u.%u.%u.%u"),
		HIWORD(m_fileVersionMS), LOWORD(m_fileVersionMS),
		HIWORD(m_fileVersionLS), LOWORD(m_fileVersionLS));
	return ver;
}

/**
 * @brief Get product version as a string.
 * @return Product version number as a string.
 */
String FileVersion::GetProductVersionString()
{
	if (!m_bProductVersionSet)
		return _T("");

	TCHAR ver[30];
	_sntprintf(ver, _countof(ver), _T("%u.%u.%u.%u"),
		HIWORD(m_productVersionMS), LOWORD(m_productVersionMS),
		HIWORD(m_productVersionLS), LOWORD(m_productVersionLS));
	return ver;
}
