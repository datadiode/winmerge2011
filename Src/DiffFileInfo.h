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
 * @file  DiffFileInfo.h
 *
 * @brief Declaration file for DiffFileInfo
 */
#pragma once

#include "DirItem.h"
#include "FileVersion.h"
#include "FileTextEncoding.h"
#include "FileTextStats.h"

/**
 * @brief Class for fileflags and coding info.
 */
struct DiffFileFlags : public FileFlags
{
	String ToString() const;
};

/**
 * @brief Information for file.
 * This class expands DirItem class with encoding information and
 * text stats information.
 * @sa DirItem.
 */
struct DiffFileInfo : public DirItem
{
// data
	enum VersionChecked
	{
		VersionMissing,
		VersionPresent,
		VersionInvalid
	} versionChecked;
	FileVersion version; /**< string of fixed file version, eg, 1.2.3.4 */
	DiffFileFlags flags; /**< file attributes */
	FileTextEncoding encoding; /**< unicode or codepage info */
	FileTextStats m_textStats; /**< EOL, zero-byte etc counts */

// methods

	DiffFileInfo(): versionChecked(VersionInvalid) { }
	void ClearPartial();
	bool IsEditableEncoding() const;
};
