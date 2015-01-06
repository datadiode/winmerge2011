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
 * @file  DirItem.cpp
 *
 * @brief Implementation for DirItem routines
 */
#include "StdAfx.h"
#include "coretools.h"
#include "DirItem.h"
#include "paths.h"

/**
 * @brief Update fileinfo from given file.
 * This function updates file's information from given item. Function
 * does not set filename and path.
 * @param [in] sFilePath Full path to file/directory to update
 * @return TRUE if information was updated (item was found).
 */
BOOL DirItem::Update(LPCTSTR sFilePath)
{
	size.int64 = -1;
	flags.reset();
	mtime = 0;
	BOOL retVal = FALSE;
	HANDLE h = ::CreateFile(sFilePath, GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (h != INVALID_HANDLE_VALUE)
	{
		BY_HANDLE_FILE_INFORMATION fstats;
		if ((retVal = ::GetFileInformationByHandle(h, &fstats)) != FALSE)
		{
			// There can be files without modification date.
			// Then we must use creation date. Of course we assume
			// creation date then exists...
			mtime = fstats.ftLastWriteTime;
			if (mtime == 0)
				mtime = fstats.ftCreationTime;

			// No size for directory ( size remains as -1)
			if ((fstats.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				size.Lo = fstats.nFileSizeLow;
				size.Hi = fstats.nFileSizeHigh;
			}

			flags.attributes = fstats.dwFileAttributes;
		}
		::CloseHandle(h);
	}
	return retVal;
}

/**
 * @brief Update file's modification time.
 */
BOOL DirItem::ApplyFileTimeTo(LPCTSTR path) const
{
	HANDLE h = ::CreateFile(path, GENERIC_WRITE,
							FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
							OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	BOOL bRet = FALSE;
	if (h != INVALID_HANDLE_VALUE)
	{
		bRet = ::SetFileTime(h, NULL, NULL, &mtime);
		::CloseHandle(h);
	}
	return bRet;
}

/**
 * @brief Clears FileInfo data.
 */
/*void DirItem::Clear()
{
	ClearPartial();
	filename.clear();
	path.clear();
}*/

/**
 * @brief Clears FileInfo data except path/filename.
 */
void DirItem::ClearPartial()
{
	ctime = 0;
	mtime = 0;
	size.int64 = -1;
	flags.reset();
}
