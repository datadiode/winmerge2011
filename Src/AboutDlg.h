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
 * @file  AboutDlg.h
 *
 * @brief Declaration file for CAboutDlg.
 *
 */
// RCS ID line follows -- this is updated by CVS
// $Id$

#ifndef _ABOUTDLG_H_
#define _ABOUTDLG_H_

/** 
 * @brief About-dialog class.
 * 
 * Shows About-dialog bitmap and draws version number and other
 * texts into it.
 */
class CAboutDlg : public ODialog
{
public:
	CAboutDlg();

// Dialog Data

// Implementation
protected:
	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	void OnBnClickedOpenContributors();
};

#endif // _ABOUTDLG_H_
