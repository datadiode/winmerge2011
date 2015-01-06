/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997-2006  Thingamahoochie Software
//    Author: Dean Grimm
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
/////////////////////////////////////////////////////////////////////////////
/** 
 * @file  FileOrFolderSelect.h
 *
 * @brief Declaration of the file and folder selection routines.
 */
#pragma once

BOOL SelectFile(HWND parent, String &path,
	UINT titleid = IDS_OPEN_TITLE, UINT filterid = IDS_ALLFILES,
	BOOL is_open = TRUE, LPCTSTR defaultExtension = NULL);

BOOL SelectFolder(HWND parent, String &path, UINT titleid);

BOOL SelectFileOrFolder(HWND parent, String &path, const String &filter);
