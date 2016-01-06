/** 
 * @file  paths.h
 *
 * @brief Declaration file for path routines
 */
#pragma once

/**
 * @brief Possible values when checking for file/folder existence.
 */
typedef enum
{
	DOES_NOT_EXIST = 0, /**< File or folder does not exist. */
	IS_EXISTING_FILE = 1, /**< It is existing file */
	IS_EXISTING_DIR = 2, /**< It is existing folder */
	IS_EXISTING = IS_EXISTING_FILE | IS_EXISTING_DIR
} PATH_EXISTENCE;

bool paths_EndsWithSlash(LPCTSTR);
PATH_EXISTENCE paths_DoesPathExist(LPCTSTR);
DWORD paths_IsReadonlyFile(LPCTSTR);
LPTSTR paths_UndoMagic(LPTSTR);
void paths_UndoMagic(String &);
String paths_GetLongPath(LPCTSTR);
bool paths_CreateIfNeeded(LPCTSTR, bool bExcludeLeaf = false);
PATH_EXISTENCE GetPairComparability(LPCTSTR, LPCTSTR);
BOOL paths_IsShortcut(LPCTSTR);
String ExpandShortcut(LPCTSTR);
String paths_ConcatPath(const String &, const String &);
String paths_GetParentPath(LPCTSTR);

struct CurrentDirectory: String { CurrentDirectory(); };

bool paths_PathIsExe(LPCTSTR);
LPCTSTR paths_CompactPath(HEdit *, String &, TCHAR marker = _T('\0'));
