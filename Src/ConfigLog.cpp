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
/** 
 * @file  ConfigLog.cpp
 *
 * @brief CConfigLog implementation
 */
#include "StdAfx.h"
#include "Constants.h"
#include "Common/version.h"
#include "Common/RegKey.h"
#include "UniFile.h"
#include "DiffWrapper.h"
#include "ConfigLog.h"
#include "paths.h"
#include "7zCommon.h"
#include "SettingStore.h"
#include "Environment.h"

CConfigLog::CConfigLog()
: m_diffOptions(NULL)
, m_pfile(new UniStdioFile())
{
}

CConfigLog::~CConfigLog()
{
	delete m_pfile;
}

/** 
 * @brief Return logfile name and path
 */
String CConfigLog::GetFileName() const
{
	return m_sFileName;
}

/**
 * @brief String wrapper around API call GetLocaleInfo
 */
static String GetLocaleString(LCID locid, LCTYPE lctype)
{
	TCHAR buffer[512];
	if (!GetLocaleInfo(locid, lctype, buffer, _countof(buffer)))
		buffer[0] = 0;
	return buffer;
}

/**
 * @brief Return Windows font charset constant name from constant value, eg, FontCharsetName() => "Hebrew"
 */
static LPCTSTR FontCharsetName(BYTE charset)
{
	switch (charset)
	{
	case ANSI_CHARSET: return _T("Ansi");
	case BALTIC_CHARSET: return _T("Baltic");
	case CHINESEBIG5_CHARSET: return _T("Chinese Big5");
	case DEFAULT_CHARSET: return _T("Default");
	case EASTEUROPE_CHARSET: return _T("East Europe");
	case GB2312_CHARSET: return _T("Chinese GB2312");
	case GREEK_CHARSET: return _T("Greek");
	case HANGUL_CHARSET: return _T("Korean Hangul");
	case MAC_CHARSET: return _T("Mac");
	case OEM_CHARSET: return _T("OEM");
	case RUSSIAN_CHARSET: return _T("Russian");
	case SHIFTJIS_CHARSET: return _T("Japanese Shift-JIS");
	case SYMBOL_CHARSET: return _T("Symbol");
	case TURKISH_CHARSET: return _T("Turkish");
	case VIETNAMESE_CHARSET: return _T("Vietnamese");
	case JOHAB_CHARSET: return _T("Korean Johab");
	case ARABIC_CHARSET: return _T("Arabic");
	case HEBREW_CHARSET: return _T("Hebrew");
	case THAI_CHARSET: return _T("Thai");
	default: return _T("Unknown");
	}
}

/**
 * @brief Write string item
 */
void CConfigLog::WriteItem(int indent, LPCTSTR key, LPCTSTR value)
{
	// do nothing if actually reading config file
	if (!m_writing)
		return;

	string_format text(value ? _T("%*.0s%s: %s\r\n") : _T("%*.0s%s:\r\n"), indent, key, key, value);
	m_pfile->WriteString(text);
}

/**
 * @brief Write int item
 */
void CConfigLog::WriteItem(int indent, LPCTSTR key, long value)
{
	// do nothing if actually reading config file
	if (!m_writing)
		return;

	string_format text(_T("%*.0s%s: %ld\r\n"), indent, key, key, value);
	m_pfile->WriteString(text);
}

/**
 * @brief Write boolean item using keywords (Yes|No)
 */
void CConfigLog::WriteItemYesNo(int indent, LPCTSTR key, bool *pvalue)
{
	if (m_writing)
	{
		string_format text(_T("%*.0s%s: %s\r\n"), indent, key, key, *pvalue ? _T("Yes") : _T("No"));
		m_pfile->WriteString(text);
	}
	else
	{
		std::map<String, String>::iterator it = m_settings.find(key);
		if (it != m_settings.end())
		{
			if (it->second == _T("Yes"))
			{
				*pvalue = true;
			}
			else if (it->second == _T("No"))
			{
				*pvalue = false;
			}
		}
	}
}

/**
 * @brief Same as WriteItemYesNo, except store Yes/No in reverse
 */
void CConfigLog::WriteItemYesNoInverted(int indent, LPCTSTR key, bool *pvalue)
{
	bool tempval = !(*pvalue);
	WriteItemYesNo(indent, key, &tempval);
	*pvalue = !(tempval);
}

/**
 * @brief Same as WriteItemYesNo, except store Yes/No in reverse
 */
void CConfigLog::WriteItemYesNoInverted(int indent, LPCTSTR key, int *pvalue)
{
	bool tempval = !(*pvalue);
	WriteItemYesNo(indent, key, &tempval);
	*pvalue = !(tempval);
}

/**
 * @brief Write out various possibly relevant windows locale information
 */
void CConfigLog::WriteLocaleSettings(LCID locid, LPCTSTR title)
{
	// do nothing if actually reading config file
	if (!m_writing)
		return;

	WriteItem(1, title);
	WriteItem(2, _T("Def ANSI codepage"), GetLocaleString(locid, LOCALE_IDEFAULTANSICODEPAGE).c_str());
	WriteItem(2, _T("Def OEM codepage"), GetLocaleString(locid, LOCALE_IDEFAULTCODEPAGE).c_str());
	WriteItem(2, _T("Country"), GetLocaleString(locid, LOCALE_SENGCOUNTRY).c_str());
	WriteItem(2, _T("Language"), GetLocaleString(locid, LOCALE_SENGLANGUAGE).c_str());
	WriteItem(2, _T("Language code"), GetLocaleString(locid, LOCALE_ILANGUAGE).c_str());
	WriteItem(2, _T("ISO Language code"), GetLocaleString(locid, LOCALE_SISO639LANGNAME).c_str());
}

/**
 * @brief Write version of a single executable file
 */
void CConfigLog::WriteVersionOf1(int indent, LPTSTR path, bool bDllGetVersion)
{
	// Do nothing if actually reading config file
	if (!m_writing)
		return;

	LPTSTR name = PathFindFileName(path);
	CVersionInfo vi(path);
	String sVersion = vi.GetFixedProductVersion();

	if (bDllGetVersion)
	{
		if (HINSTANCE hinstDll = LoadLibrary(path))
		{
			DLLGETVERSIONPROC DllGetVersion = (DLLGETVERSIONPROC) 
				GetProcAddress(hinstDll, "DllGetVersion");
			if (DllGetVersion)
			{
				DLLVERSIONINFO dvi;
				ZeroMemory(&dvi, sizeof dvi);
				dvi.cbSize = sizeof dvi;
				if (SUCCEEDED(DllGetVersion(&dvi)))
				{
					sVersion += string_format(_T(" dllversion=%u.%u dllbuild=%u"),
						dvi.dwMajorVersion, dvi.dwMinorVersion, dvi.dwBuildNumber);
				}
			}
			FreeLibrary(hinstDll);
		}
	}
	string_format text
	(
		name == path
	?	_T("%*s%-24s %s\r\n")
	:	_T("%*s%-24s %s path=%s\r\n"),
		indent,
		// Tilde prefix for modules currently mapped into WinMerge
		GetModuleHandle(path) ? _T("~") : _T(""),
		name,
		sVersion.c_str(),
		path
	);
	m_pfile->WriteString(text);
}

/**
 * @brief Write version of a set of executable files
 */
void CConfigLog::WriteVersionOf(int indent, LPTSTR path)
{
	LPTSTR name = PathFindFileName(path);
	WIN32_FIND_DATA ff;
	HANDLE h = FindFirstFile(path, &ff);
	if (h != INVALID_HANDLE_VALUE)
	{
		do
		{
			lstrcpy(name, ff.cFileName);
			WriteVersionOf1(indent, path);
		} while (FindNextFile(h, &ff));
		FindClose(h);
	}
}

struct NameMap { int ival; LPCTSTR sval; };

/**
 * @brief Write boolean item using keywords (Yes|No)
 */
void CConfigLog::WriteItemWhitespace(int indent, LPCTSTR key, int *pvalue)
{
	static const NameMap namemap[] =
	{
		{ WHITESPACE_COMPARE_ALL, _T("Compare all") },
		{ WHITESPACE_IGNORE_CHANGE, _T("Ignore change") },
		{ WHITESPACE_IGNORE_ALL, _T("Ignore all") },
	};
	if (m_writing)
	{
		LPCTSTR text = _T("Unknown");
		for (int i = 0; i < _countof(namemap); ++i)
		{
			if (*pvalue == namemap[i].ival)
				text = namemap[i].sval;
		}
		WriteItem(indent, key, text);
	}
	else
	{
		*pvalue = namemap[0].ival;
		std::map<String, String>::iterator it = m_settings.find(key);
		if (it != m_settings.end())
		{
			for (int i = 0 ; i < _countof(namemap) ; ++i)
			{
				if (it->second == namemap[i].sval)
					*pvalue = namemap[i].ival;
			}
		}
	}
}

/** 
 * @brief Write logfile
 */
bool CConfigLog::DoFile(bool writing, String &sError)
{
	CVersionInfo version;

	m_writing = writing;

	if (writing)
	{
		String sFileName = paths_ConcatPath(env_GetMyDocuments(), WinMergeDocumentsFolder);
		paths_CreateIfNeeded(sFileName.c_str());
		m_sFileName = paths_ConcatPath(sFileName, _T("WinMerge.txt"));

		if (!m_pfile->OpenCreateUtf8(m_sFileName.c_str()))
		{
			const UniFile::UniError &err = m_pfile->GetLastUniError();
			sError = err.GetError();
			return false;
		}
		m_pfile->SetBom(true);
		m_pfile->WriteBom();
	}

// Begin log
	FileWriteString(_T("WinMerge configuration log\r\n"));
	FileWriteString(_T("--------------------------\r\n"));
	FileWriteString(_T("Saved to: "));
	FileWriteString(m_sFileName.c_str());
	FileWriteString(_T("\r\n* Please add this information (or attach this file)\r\n"));
	FileWriteString(_T("* when reporting bugs.\r\n"));
	FileWriteString(_T("Module names prefixed with tilda (~) are currently loaded in WinMerge process.\r\n"));

// Platform stuff
	FileWriteString(_T("\r\n\r\nVersion information:\r\n"));
	FileWriteString(_T(" WinMerge.exe: "));
	FileWriteString(version.GetFixedProductVersion().c_str());

	String privBuild = version.GetPrivateBuild();
	if (!privBuild.empty())
	{
		FileWriteString(_T(" - Private build: "));
		FileWriteString(privBuild.c_str());
	}

	FileWriteString(
		_T("\r\n Build config:")
#		ifdef _DEBUG
		_T(" _DEBUG")
#		endif
#		ifdef NDEBUG
		_T(" NDEBUG")
#		endif
#		ifdef _WIN64
		_T(" _WIN64")
#		endif
	);

	LPCTSTR szCmdLine = ::GetCommandLine();
	ASSERT(szCmdLine != NULL);
	// Skip the quoted executable file name.
	if (szCmdLine != NULL)
	{
		szCmdLine = ::PathGetArgs(szCmdLine);
	}

	// The command line include a space after the executable file name,
	// which mean that empty command line will have length of one.
	if (szCmdLine == NULL || szCmdLine[0] == _T('\0'))
	{
		szCmdLine = _T("none");
	}

	FileWriteString(_T("\r\n Command Line: "));
	FileWriteString(szCmdLine);

	FileWriteString(_T("\r\n Windows: "));
	FileWriteString(GetWindowsVer().c_str());

	FileWriteString(_T("\r\n Wine: %hs\r\n"), wine_version ? wine_version : "N/A");

	FileWriteString(_T("\r\n"));
	WriteVersionOf1(1, _T("COMCTL32.dll"));
	WriteVersionOf1(1, _T("shlwapi.dll"));
	WriteVersionOf1(1, _T("MergeLang.dll"));
	WriteVersionOf1(1, _T("ShellExtensionU.dll"), false);
	WriteVersionOf1(1, _T("ShellExtensionX64.dll"), false);
	WriteVersionOf1(1, _T("Frhed\\hekseditU.dll"), false);

	TCHAR path[MAX_PATH];
	GetModuleFileName(0, path, _countof(path));
	LPTSTR pattern = PathFindFileName(path);
	lstrcpy(pattern, _T("Merge7z\\*.dll"));
	WriteVersionOf(1, path);

// WinMerge settings
	FileWriteString(_T("\r\nWinMerge configuration:\r\n"));
	FileWriteString(_T(" Compare settings:\r\n"));

	WriteItemYesNo(2, _T("Ignore blank lines"), &m_diffOptions.bIgnoreBlankLines);
	WriteItemYesNo(2, _T("Ignore case"), &m_diffOptions.bIgnoreCase);
	WriteItemYesNo(2, _T("Ignore carriage return differences"), &m_diffOptions.bIgnoreEol);

	WriteItemWhitespace(2, _T("Whitespace compare"), &m_diffOptions.nIgnoreWhitespace);

	WriteItemYesNo(2, _T("Detect moved blocks"), &m_miscSettings.bMovedBlocks);
	WriteItem(2, _T("Compare method"), m_compareSettings.nCompareMethod);
	WriteItemYesNo(2, _T("Stop after first diff"), &m_compareSettings.bStopAfterFirst);

	FileWriteString(_T("\r\n Other settings:\r\n"));
	WriteItemYesNo(2, _T("Automatic rescan"), &m_miscSettings.bAutomaticRescan);
	WriteItemYesNoInverted(2, _T("Simple EOL"), &m_miscSettings.bAllowMixedEol);
	WriteItemYesNo(2, _T("Automatic scroll to 1st difference"), &m_miscSettings.bScrollToFirst);
	WriteItemYesNo(2, _T("Backup original file"), &m_miscSettings.bBackup);
	WriteItemYesNo(2, _T("Enable archive file support"), &m_miscSettings.bMerge7zEnable);
	WriteItemYesNo(2, _T("Detect archive type from file signature"), &m_miscSettings.bMerge7zProbeSignature);

	FileWriteString(_T("\r\n Folder compare:\r\n"));
	WriteItemYesNo(2, _T("Identical files"), &m_viewSettings.bShowIdent);
	WriteItemYesNo(2, _T("Different files"), &m_viewSettings.bShowDiff);
	WriteItemYesNo(2, _T("Left Unique files"), &m_viewSettings.bShowUniqueLeft);
	WriteItemYesNo(2, _T("Right Unique files"), &m_viewSettings.bShowUniqueRight);
	WriteItemYesNo(2, _T("Binary files"), &m_viewSettings.bShowBinaries);
	WriteItemYesNo(2, _T("Skipped files"), &m_viewSettings.bShowSkipped);
	WriteItemYesNo(2, _T("Tree-mode enabled"), &m_viewSettings.bTreeView);

	FileWriteString(_T("\r\n File compare:\r\n"));
	WriteItemYesNo(2, _T("Preserve filetimes"), &m_miscSettings.bPreserveFiletimes);
	WriteItemYesNo(2, _T("Match similar lines"), &m_miscSettings.bMatchSimilarLines);
	WriteItem(2, _T("MatchSimilarLinesMax"), m_miscSettings.nMatchSimilarLinesMax);

	FileWriteString(_T("\r\n Editor settings:\r\n"));
	WriteItemYesNo(2, _T("View Whitespace"), &m_miscSettings.bViewWhitespace);
	WriteItemYesNo(2, _T("Merge Mode enabled"), &m_miscSettings.bMergeMode);
	WriteItemYesNo(2, _T("Show linenumbers"), &m_miscSettings.bShowLinenumbers);
	WriteItemYesNo(2, _T("Wrap lines"), &m_miscSettings.bWrapLines);
	WriteItemYesNo(2, _T("Syntax Highlight"), &m_miscSettings.bSyntaxHighlight);
	WriteItem(2, _T("Tab size"), m_miscSettings.nTabSize);
	WriteItemYesNoInverted(2, _T("Insert tabs"), &m_miscSettings.nInsertTabs);
	
// Font settings
	FileWriteString(_T("\r\n Font:\r\n"));
	FileWriteString(_T("  Font facename: %s\r\n"), m_fontSettings.sFacename.c_str());
	FileWriteString(_T("  Font charset: %d (%s)\r\n"), m_fontSettings.nCharset,
		FontCharsetName(m_fontSettings.nCharset));

// System settings
	FileWriteString(_T("\r\nSystem settings:\r\n"));
	FileWriteString(_T(" codepage settings:\r\n"));
	WriteItem(2, _T("ANSI codepage"), GetACP());
	WriteItem(2, _T("OEM codepage"), GetOEMCP());
	WriteLocaleSettings(GetThreadLocale(), _T("Locale (Thread)"));
	WriteLocaleSettings(LOCALE_USER_DEFAULT, _T("Locale (User)"));
	WriteLocaleSettings(LOCALE_SYSTEM_DEFAULT, _T("Locale (System)"));

// Codepage settings
	WriteItemYesNo(1, _T("Detect codepage automatically for RC and HTML files"), &m_cpSettings.bDetectCodepage);
	WriteItem(1, _T("unicoder codepage"), FileTextEncoding::GetDefaultCodepage());

	CloseFile();

	return true;
}

/** 
 * @brief Parse Windows version data to string.
 * @return String describing Windows version.
 */
string_format CConfigLog::GetWindowsVer() const
{
	CRegKeyEx key;
	if (ERROR_SUCCESS == key.QueryRegMachine(_T("Software\\Microsoft\\Windows NT\\CurrentVersion")))
	{
		return string_format(_T("%s (BuildLab: %s)"),
			key.ReadString(_T("ProductName"), _T("Unknown OS")),
			key.ReadString(_T("BuildLab"), _T("Unknown")));
	}
	return _T("Unknown OS");
}

bool CConfigLog::WriteLogFile(String &sError)
{
	CloseFile();
	return DoFile(true, sError);
}

void CConfigLog::ReadLogFile(LPCTSTR Filepath)
{
	CloseFile();
	String sError;
	if (!ParseSettings(Filepath))
		return;
	DoFile(false, sError);
}

/// Write line to file (if writing configuration log)
void CConfigLog::FileWriteString(LPCTSTR lpsz)
{
	if (m_writing)
		m_pfile->WriteString(lpsz);
}

/**
 * @brief Close any open file
 */
void CConfigLog::CloseFile()
{
	m_settings.clear();
	if (m_pfile->IsOpen())
		m_pfile->Close();
}

/**
 * @brief  Store all name:value strings from file into m_pCfgSettings
 */
bool CConfigLog::ParseSettings(LPCTSTR Filepath)
{
	UniMemFile file;
	if (!file.OpenReadOnly(Filepath))
		return false;

	String sLine, sEol;
	bool lossy;
	while (file.ReadString(sLine, sEol, &lossy))
	{
		int colon = sLine.find(_T(":"));
		if (colon > 0)
		{
			String name = sLine.substr(0, colon);
			String value = sLine.substr(colon + 1);
			string_trim_ws(name);
			string_trim_ws(value);
			m_settings[name] = value;
		}
	}
	file.Close();
	return true;
}
