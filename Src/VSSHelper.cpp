/////////////////////////////////////////////////////////////////////////////
//    License (GPLv2+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
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
 * @file  VSSHelper.cpp
 *
 * @brief Implementation file for VSSHelper class
 */
#include "StdAfx.h"
#include "VSSHelper.h"
#include "coretools.h"

const String &VSSHelper::GetProjectBase()
{
	return m_strVssProjectBase;
}

BOOL VSSHelper::SetProjectBase(String strPath)
{
	if (strPath.size() < 2)
		return FALSE;

	m_strVssProjectBase = strPath;
	string_replace(m_strVssProjectBase, _T('/'), _T('\\'));

	// Check if m_strVssProjectBase has leading $\\, if not put them in:
	if (EatPrefix(m_strVssProjectBase.c_str(), _T("$\\")) == NULL)
		m_strVssProjectBase.insert(0, _T("$\\"));

	// Exclude any trailing backslashes
	m_strVssProjectBase.resize(m_strVssProjectBase.find_last_not_of(_T('\\')) + 1);
	return TRUE;
}
