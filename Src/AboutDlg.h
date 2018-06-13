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
 * @file  AboutDlg.h
 *
 * @brief Declaration file for CAboutDlg.
 *
 */
#pragma once

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
