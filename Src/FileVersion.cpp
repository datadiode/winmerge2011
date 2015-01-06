/** 
 * @file  FileVersion.cpp
 *
 * @brief Implementation for FileVersion
 */
#include "StdAfx.h"
#include "FileVersion.h"

/**
 * @brief Get version as a string.
 */
String FileVersion::GetVersionString() const
{
	TCHAR ver[30];
	_sntprintf(ver, _countof(ver), _T("%u.%u.%u.%u"),
		HIWORD(m_versionMS), LOWORD(m_versionMS),
		HIWORD(m_versionLS), LOWORD(m_versionLS));
	return ver;
}
