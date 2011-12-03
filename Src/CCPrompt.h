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
 * @file  CCPrompt.h
 *
 * @brief Declaration file for ClearCase dialog.
 *
 */
// ID line follows -- this is updated by SVN
// $Id$

#pragma once

/**
 * @brief A dialog for ClearCase checkout/checkin.
 */
class CCCPrompt : public ODialog
{
// Construction
public:
	CCCPrompt();   // standard constructor

	String	m_comments;

// Implementation
protected:
	template<DDX_Operation>
			bool UpdateData();
	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

public:
	BOOL m_bMultiCheckouts;
	BOOL m_bCheckin;
};
