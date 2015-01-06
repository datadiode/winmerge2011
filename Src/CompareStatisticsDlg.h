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
 * @file  CompareStatisticsDlg.h
 *
 * @brief Declaration file for CompareStatisticsDlg dialog
 */
#pragma once

class CompareStats;

/**
 * @brief A dialog showing folder compare statistics.
 * This dialog shows statistics about current folder compare. It shows amounts
 * if items in different compare status categories.
 */
class CompareStatisticsDlg : public ODialog
{
public:
	CompareStatisticsDlg(const CompareStats *);
	void Update();

protected:
	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

// Implementation data
private:
	const CompareStats *const m_pCompareStats; /**< Compare statistics structure. */
};
