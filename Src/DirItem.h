/////////////////////////////////////////////////////////////////////////////
//    License (GPLv2+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
/////////////////////////////////////////////////////////////////////////////
/** 
 * @file  DirItem.h
 *
 * @brief Declaration file for DirItem
 */
#pragma once

#include "Common/coretypes.h"

/**
 * @brief Class for fileflags.
 */
struct FileFlags
{
	DWORD attributes; /**< Fileattributes for item */
	FileFlags() : attributes(0) { }
	void reset() { attributes = 0; } /// Reset fileattributes
};

/**
 * @brief Information for file.
 * This class stores basic information from a file or folder.
 * Information consists of item name, times, size and attributes.
 * Also version info can be get for files supporting it.
 *
 * @note times in are seconds since January 1, 1970.
 * See Dirscan.cpp/fentry and Dirscan.cpp/LoadFiles()
 */
struct DirItem
{
	FileTime ctime; /**< time of creation */
	FileTime mtime; /**< time of last modify */
	CY size; /**< file size in bytes, -1 means file does not exist*/
	String filename; /**< filename for this item */
	String path; /**< full path (excluding filename) for the item */
	FileFlags flags; /**< file attributes */

	DirItem() { size.int64 = -1; }
	BOOL Update(LPCTSTR);
	BOOL ApplyFileTimeTo(LPCTSTR) const;
	void ClearPartial();
};
