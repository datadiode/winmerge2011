/** 
 * @file  paths.h
 *
 * @brief Declaration file for path routines
 */
// ID line follows -- this is updated by SVN
// $Id$

#ifndef paths_h_included
#define paths_h_included

/**
 * @brief Possible values when checking for file/folder existence.
 */
typedef enum
{
	DOES_NOT_EXIST, /**< File or folder does not exist. */
	IS_EXISTING_FILE, /**< It is existing file */
	IS_EXISTING_DIR, /**< It is existing folder */
} PATH_EXISTENCE;

bool paths_EndsWithSlash(LPCTSTR);
PATH_EXISTENCE paths_DoesPathExist(LPCTSTR);
LPCTSTR paths_UndoMagic(LPTSTR);
String paths_GetLongPath(LPCTSTR);
bool paths_CreateIfNeeded(LPCTSTR);
bool paths_EnsurePathExist(LPCTSTR);
PATH_EXISTENCE GetPairComparability(LPCTSTR, LPCTSTR);
BOOL paths_IsShortcut(LPCTSTR);
String ExpandShortcut(LPCTSTR);
String paths_ConcatPath(const String & path, const String & subpath);
String paths_GetParentPath(LPCTSTR);
bool paths_PathIsExe(LPCTSTR);

#endif // paths_h_included
