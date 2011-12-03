/** 
 * @file  DirTravel.cpp
 *
 * @brief Implementation file for Directory traversal functions.
 *
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "paths.h"
#include "DirItem.h"
#include "DirTravel.h"

static void LoadFiles(LPCTSTR sDir, DirItemArray * dirs, DirItemArray * files);
static void Sort(DirItemArray * dirs, bool casesensitive);

/**
 * @brief Load arrays with all directories & files in specified dir
 */
void LoadAndSortFiles(LPCTSTR sDir, DirItemArray * dirs, DirItemArray * files, bool casesensitive)
{
	LoadFiles(sDir, dirs, files);
	Sort(dirs, casesensitive);
	Sort(files, casesensitive);
}

/**
 * @brief Find files and subfolders from given folder.
 * This function saves all files and subfolders in given folder to arrays.
 * We use 64-bit version of stat() to get times since find doesn't return
 * valid times for very old files (around year 1970). Even stat() seems to
 * give negative time values but we can live with that. Those around 1970
 * times can happen when file is created so that it  doesn't get valid
 * creation or modificatio dates.
 * @param [in] sDir Base folder for files and subfolders.
 * @param [in, out] dirs Array where subfolders are stored.
 * @param [in, out] files Array where files are stored.
 */
static void LoadFiles(LPCTSTR sDir, DirItemArray * dirs, DirItemArray * files)
{
	String sPattern = paths_ConcatPath(sDir, _T("*.*"));
	WIN32_FIND_DATA ff;
	HANDLE h = FindFirstFile(sPattern.c_str(), &ff);
	if (h != INVALID_HANDLE_VALUE)
	{
		do
		{
			bool bIsDirectory = (ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0;
			if (bIsDirectory && _tcsstr(_T(".."), ff.cFileName))
				continue;

			DirItem ent;

			// Save filetimes as seconds since January 1, 1970
			// Note that times can be < 0 if they are around that 1970..
			// Anyway that is not sensible case for normal files so we can
			// just use zero for their time.
			ent.ctime = ff.ftCreationTime;
			ent.mtime = ff.ftLastWriteTime;

			if ((ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				ent.size.Lo = ff.nFileSizeLow;
				ent.size.Hi = ff.nFileSizeHigh;
			}

			ent.path = sDir;
			ent.filename = ff.cFileName;
			ent.flags.attributes = ff.dwFileAttributes;

			(bIsDirectory ? dirs : files)->push_back(ent);
		} while (FindNextFile(h, &ff));
		FindClose(h);
	}
}

static int collate(const String &str1, const String &str2)
{
	return _tcscoll(str1.c_str(), str2.c_str());
}

/**
 * @brief case-sensitive collate function for qsorting an array
 */
static bool __cdecl cmpstring(const DirItem &elem1, const DirItem &elem2)
{
	return collate(elem1.filename, elem2.filename) < 0;
}

static int collate_ignore_case(const String &str1, const String &str2)
{
	return _tcsicoll(str1.c_str(), str2.c_str());
}

/**
 * @brief case-insensitive collate function for qsorting an array
 */
static bool __cdecl cmpistring(const DirItem &elem1, const DirItem &elem2)
{
	return collate_ignore_case(elem1.filename, elem2.filename) < 0;
}

/**
 * @brief sort specified array
 */
static void Sort(DirItemArray * dirs, bool casesensitive)
{
	if (casesensitive)
        stl::sort(dirs->begin(), dirs->end(), cmpstring);
	else
		stl::sort(dirs->begin(), dirs->end(), cmpistring);
}

/**
 * @brief  Compare (NLS aware) two strings, either case-sensitive or
 * case-insensitive as caller specifies
 */
int collstr(const String & s1, const String & s2, bool casesensitive)
{
	if (casesensitive)
		return collate(s1, s2);
	else
		return collate_ignore_case(s1, s2);
}
