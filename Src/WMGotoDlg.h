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
 * @file  WMGotoDlg.h
 *
 * @brief Declaration file for WMGotoDlg dialog.
 *
 */
#pragma once

/**
 * @brief Class for Goto-dialog.
 * This dialog allows user to go to certain line or or difference in the file
 * compare. As there are two panels with different line numbers, there is a
 * choice for target panel. When dialog is opened, its values are initialized
 * for active file's line number.
 */
class WMGotoDlg
	: ZeroInit<WMGotoDlg>
	, public ODialog
{
// Construction
public:
	WMGotoDlg(CMergeEditView *); // constructor

// Dialog Data
	String m_strParam;   /**< Line/difference number. */
	int m_nFile;         /**< Target file number. */
	int m_nGotoWhat;     /**< Goto line or difference? */

// Implementation
protected:
	template<DDX_Operation>
			bool UpdateData();
	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
private:
	CMergeEditView *const m_pBuddy;
};
