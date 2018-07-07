/////////////////////////////////////////////////////////////////////////////
//    License (GPLv3+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 3 of the License, or (at
//    your option) any later version.
//    
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
/////////////////////////////////////////////////////////////////////////////
/** 
 * @file  DiffFileInfo.cpp
 *
 * @brief Implementation for DiffFileInfo routines
 */
#include "stdafx.h"
#include "DirItem.h"
#include "DiffFileInfo.h"
#include "unicoder.h"

/**
 * @brief Clears FileInfo data.
 */
void DiffFileInfo::ClearPartial()
{
	DirItem::ClearPartial();
	version.Clear();
	versionChecked = VersionInvalid;
	encoding.Clear();
	m_textStats.clear();
}

/**
 * @brief Return true if file is in any Unicode encoding
 */
bool DiffFileInfo::IsEditableEncoding() const
{
	return encoding.m_bom == false;
}
