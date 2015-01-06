/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997-2000  Thingamahoochie Software
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
 *  @file FileTransform.h
 *
 *  @brief Declaration of file transformations
 */ 
#pragma once

#include "Common/UniFile.h"
#include "Common/MyCom.h"

/**
 * @brief Unpacking/packing information for a given file
 *
 * @note Can be be copied between threads
 * Each thread really needs its own instance so that subcode is really defined
 * during the unpacking (open file) of the thread
 */
class PackingInfo
{
public:
	PackingInfo()
	: pfnCreateUniFile(UniMemFile::CreateInstance)
	, disallowMixedEOL(false)
	, readOnly(false)
	{
	}
	void SetXML();
	void SetPlugin(LPCTSTR);
public:
	/// "3rd path" where output saved if given
	String saveAsPath;
	/// text type to override syntax highlighting
	String textType;
	/// plugin moniker
	String pluginMoniker;
	/// plugin options
	CMyComPtr<IDispatch> pluginOptionsOwner;
	CMyComPtr<IDispatch> pluginOptions;
	/// custom UniFile
	UniFile *(*pfnCreateUniFile)(PackingInfo *);
	bool disallowMixedEOL;
	bool readOnly;
};
