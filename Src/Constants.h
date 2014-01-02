/** 
 * @file Constants.h
 *
 * @brief WinMerge constants, URLs, paths etc.
 *
 */
// ID line follows -- this is updated by SVN
// $Id$

#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

/** @brief URL for hyperlink in About-dialog. */
const TCHAR WinMergeURL[] = _T("http://winmerge.org/");

/**
 * @brief URL to help index in internet.
 * We use internet help when local help file is not found (not installed).
 */
const TCHAR DocsURL[] = _T("http://manual.winmerge.org/index.html");

/** @brief URL to translations page in internet. */
const TCHAR TranslationsUrl[] = _T("http://winmerge.org/translations/");

/** @brief URL of the GPL license. */
const TCHAR LicenceUrl[] = _T("http://www.gnu.org/licenses/gpl-2.0.html");

/** @brief WinMerge download page URL. */
const TCHAR DownloadUrl[] = _T("http://winmerge.org/downloads/");

/** @brief Relative (to WinMerge executable ) path to local help file. */
const TCHAR DocsPath[] = _T("\\Docs\\WinMerge.chm");

/** @brief Contributors list. */
const TCHAR ContributorsPath[] = _T("\\contributors.txt");

/** @brief Release notes in HTML format. */
const TCHAR RelNotes[] = _T("\\Docs\\ReleaseNotes.html");

/** @brief GPL Licence local file name. */
const TCHAR LicenseFile[] = _T("\\Copying");

/** @brief WinMerge folder in My Folders-folder. */
const TCHAR WinMergeDocumentsFolder[] = _T("WinMerge");

/** @brief Executable Filename for ANSI build. */
const TCHAR ExecutableFilename[] = _T("WinMerge.exe");
/** @brief Executable Filename for Unicode build. */
const TCHAR ExecutableFilenameU[] = _T("WinMergeU.exe");

/**
 * @brief Flags used when opening files
 */
enum
{
	FFILEOPEN_NONE		= 0x0000,
	FFILEOPEN_NOMRU		= 0x0001, /**< Do not add this path to MRU list */
	FFILEOPEN_READONLY	= 0x0002, /**< Open this path as read-only */
	FFILEOPEN_MISSING	= 0x0008, /**< The file is known to be missing */
	FFILEOPEN_CMDLINE	= 0x0010, /**< Path is read from commandline */
	FFILEOPEN_PROJECT	= 0x0020, /**< Path is read from project-file */
	FFILEOPEN_DETECTBIN	= 0x0040, /**< Auto-detect binary files */
	FFILEOPEN_DETECTZIP	= 0x0080, /**< Auto-detect archive files */
	FFILEOPEN_DETECT	= FFILEOPEN_DETECTBIN | FFILEOPEN_DETECTZIP
};

#endif // _CONSTANTS_H_
