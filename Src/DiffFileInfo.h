/////////////////////////////////////////////////////////////////////////////
//    License (GPLv3+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
	FileTextEncoding encoding; /**< unicode or codepage info */
	FileTextStats m_textStats; /**< EOL, zero-byte etc counts */

// methods

	DiffFileInfo(): versionChecked(VersionInvalid) { }
	void ClearPartial();
	bool IsEditableEncoding() const;
};
