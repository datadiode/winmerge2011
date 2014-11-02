/**
 *  @file version.cpp
 *
 *  @brief Implementation of CVersionInfo class
 */ 
#include "StdAfx.h"
#include "version.h"

/**
 * @brief Structure used to store language and codepage.
 */
struct LANGUAGEANDCODEPAGE
{
	WORD wLanguage;
	WORD wCodePage;
};

/** 
 * @brief Constructor.
 * @param [in] szFileToVersion Filename to read version from.
 * @param [in] bDllVersion If TRUE queries DLL version.
 */
CVersionInfo::CVersionInfo(LPCTSTR szFileName)
: m_pVffInfo(NULL)
, m_wLanguage(0)
, m_wCodepage(0)
{
	GetVersionInfo(szFileName);
}

/** 
 * @brief Constructor for asking version number from known module.
 * @param [in] hModule Handle to module for version info.
 */
CVersionInfo::CVersionInfo(HINSTANCE hModule)
: m_pVffInfo(NULL)
, m_wLanguage(0)
, m_wCodepage(0)
{
	TCHAR szFileName[MAX_PATH];
	GetModuleFileName(hModule, szFileName, MAX_PATH);
	GetVersionInfo(szFileName);
}

CVersionInfo::~CVersionInfo()
{
	delete[] m_pVffInfo;
}

/** 
 * @brief Return file version string.
 * @return File version as string.
 */
String CVersionInfo::GetFileVersion() const
{
	return QueryValue(_T("FileVersion"));
}

/** 
 * @brief Return private build number.
 * @return Private build number as string.
 */
String CVersionInfo::GetPrivateBuild() const
{
	return QueryValue(_T("PrivateBuild"));
}

/** 
 * @brief Return special build number.
 * @return Special build number as string.
 */
String CVersionInfo::GetSpecialBuild() const
{
	return QueryValue(_T("SpecialBuild"));
}

/** 
 * @brief Return company name.
 * @return Company name.
 */
String CVersionInfo::GetCompanyName() const
{
	return QueryValue(_T("CompanyName"));
}

/** 
 * @brief Return file description string.
 * @return File description string.
 */
String CVersionInfo::GetFileDescription() const
{
	return QueryValue(_T("FileDescription"));
}

/** 
 * @brief Return internal name.
 * @return Internal name.
 */
String CVersionInfo::GetInternalName() const
{
	return QueryValue(_T("InternalName"));
}

/** 
 * @brief Return copyright info.
 * @return Copyright info.
 */
String CVersionInfo::GetLegalCopyright() const
{
	return QueryValue(_T("LegalCopyright"));
}

/** 
 * @brief Return original filename.
 * @return Original filename.
 */
String CVersionInfo::GetOriginalFilename() const
{
	return QueryValue(_T("OriginalFilename"));
}

/** 
 * @brief Return product's version number.
 * @return Product's version number as string.
 */
String CVersionInfo::GetProductVersion() const
{
	return QueryValue(_T("ProductVersion"));
}

/** 
 * @brief Format version string from numbers.
 * Version number consists of four WORD (16-bit) numbers. This function
 * formats those numbers to string, where numbers are separated by
 * dots. If the last number is zero it is not printed.
 * @param [in] First two (most significant) numbers for version number.
 * @param [in] Last two numbers for version number.
 * @return Formatted version string.
 */
String CVersionInfo::MakeVersionString(DWORD hi, DWORD lo) const
{
	if (m_pVffInfo == NULL)
		return String();
	TCHAR ver[24];
	LPCTSTR fmt = _T("%u.%u.%u.%u");
	if (LOWORD(lo) == 0)
		fmt += 3;
	wnsprintf(ver, _countof(ver), fmt, HIWORD(hi), LOWORD(hi), HIWORD(lo), LOWORD(lo));
	return ver;
}

/** 
 * @brief Return numeric product's version number.
 * This function returns version number given as a number in version info.
 * @return Product's version number as string.
 */
String CVersionInfo::GetFixedProductVersion()
{
	return MakeVersionString(dwProductVersionMS, dwProductVersionLS);
}

/** 
 * @brief Return numeric file's version number.
 * This function returns version number given as a number in version info.
 * @return File's version number as string.
 */
String CVersionInfo::GetFixedFileVersion()
{
	return MakeVersionString(dwFileVersionMS, dwFileVersionLS);
}

/** 
 * @brief Return numeric file's version number.
 * This function returns version number given as two DWORDs.
 * @param [out] versionMS High DWORD for version number.
 * @param [out] versionLS Low DWORD for version number.
 * @return TRUE if version info was found, FALSE otherwise.
 */
BOOL CVersionInfo::GetFixedFileVersion(DWORD &versionMS, DWORD &versionLS)
{
	if (m_pVffInfo)
	{
		versionMS = dwFileVersionMS;
		versionLS = dwFileVersionLS;
		return TRUE;
	}
	return FALSE;
}

/** 
 * @brief Return comment string.
 * @return Comment string.
 */
String CVersionInfo::GetComments() const
{
	return QueryValue(_T("Comments"));
}

/** 
 * @brief Read version info from file.
 * This function reads version information from file's version resource
 * to member variables.
 */
void CVersionInfo::GetVersionInfo(LPCTSTR szFileName)
{
	ZeroMemory(static_cast<VS_FIXEDFILEINFO *>(this), sizeof(VS_FIXEDFILEINFO));
	DWORD dwVerHnd = 0; // An 'ignored' parameter, always '0'
	DWORD dwVerInfoSize = GetFileVersionInfoSize(szFileName, &dwVerHnd);
	if (dwVerInfoSize == 0)
		return;
	m_pVffInfo = new BYTE[dwVerInfoSize];
	if (!GetFileVersionInfo(szFileName, dwVerHnd, dwVerInfoSize, m_pVffInfo))
	{
		delete[] m_pVffInfo;
		m_pVffInfo = NULL;
		return;
	}
	VS_FIXEDFILEINFO *pffi;
	UINT len = sizeof *pffi;
	VerQueryValue(m_pVffInfo, _T("\\"), (LPVOID *)&pffi, &len);
	*static_cast<VS_FIXEDFILEINFO *>(this) = *pffi;
	if (m_wLanguage != 0)
	{
		m_wCodepage = GetCodepageForLanguage(m_wLanguage);
	}
	else
	{
		LANGUAGEANDCODEPAGE *lpTranslate;
		UINT langLen;
		if (VerQueryValue(m_pVffInfo,
			_T("\\VarFileInfo\\Translation"),
			(LPVOID *)&lpTranslate, &langLen))
		{
			m_wLanguage = lpTranslate[0].wLanguage;
			m_wCodepage = lpTranslate[0].wCodePage;
		}
	}
}

/** 
 * @brief Read value from version info data.
 * @param [in] szId Name of value/string to read.
 * @param [out] Value read.
 */
String CVersionInfo::QueryValue(LPCTSTR id) const
{
	assert(m_pVffInfo != NULL);
	LPVOID pv = NULL;
	UINT len = 0;
	TCHAR selector[256];
	wnsprintf(selector, _countof(selector),
		_T("\\StringFileInfo\\%08lx\\%s"),
		MAKELONG(m_wCodepage, m_wLanguage), id);
	return VerQueryValue(m_pVffInfo, selector, &pv, &len) && (len > 0) ?
		String(reinterpret_cast<LPCTSTR>(pv), len - 1) : String();
}

/** 
 * @brief Get codepage for given language.
 * This function finds codepage value for given language from version info.
 * That is, we have certain combinations of language-codepage in version info.
 * This function tells which codepage exists with given language, so we can
 * find existing version info data.
 * @param [in] wLanguage Language ID for which we need matching codepage.
 * @return wCodePage Found codepage.
 */
WORD CVersionInfo::GetCodepageForLanguage(WORD wLanguage)
{
	LANGUAGEANDCODEPAGE *lpTranslate;
	UINT cbTranslate;
	// Read the list of languages and code pages.

	VerQueryValue(m_pVffInfo, 
				_T("\\VarFileInfo\\Translation"),
				(LPVOID*)&lpTranslate,
				&cbTranslate);

	// Read the file description for each language and code page.

	int i = cbTranslate / sizeof(LANGUAGEANDCODEPAGE);
	while (i--)
	{
		if (lpTranslate[i].wLanguage == wLanguage)
			return lpTranslate[i].wCodePage;
	}
	return FALSE;
}
