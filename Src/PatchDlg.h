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
 * @file  PatchDlg.h
 *
 * @brief Declaration file for patch creation dialog
 */
#pragma once

#include "SuperComboBox.h"

struct PATCHFILES;

/////////////////////////////////////////////////////////////////////////////
// PatchDlg dialog

/** 
 * @brief Dialog class for Generate Patch -dialog.
 * This dialog allows user to select files from which to create a patch,
 * patch file's filename and several options related to patch.
 */
class CPatchDlg
	: ZeroInit<CPatchDlg>
	, public ODialog
{
// Construction
public:
	CPatchDlg();   // standard constructor

	// Functions to add and get selected files (as PATCHFILEs)
	void AddItem(const PATCHFILES& pf);
	int GetItemCount();
	const PATCHFILES& GetItemAt(int position);
	void ClearItems();

// Dialog Data
	BOOL m_ignoreCase;
	String	m_file1;
	String	m_file2;
	String	m_fileResult;
	BOOL m_ignoreBlankLines;
	BOOL m_applyLineFilters;
	int m_whitespaceCompare;
	BOOL m_appendFile;
	BOOL m_openToEditor;
	BOOL m_includeCmdLine;

	enum output_style m_outputStyle; /**< Patch style (context, unified etc.) */
	int m_contextLines; /**< How many context lines are added. */

	HComboBox			*m_pCbStyle;
	HSuperComboBox		*m_pCbContext;
	HSuperComboBox		*m_pCbFile1;
	HSuperComboBox		*m_pCbFile2;
	HSuperComboBox		*m_pCbResult;

// Implementation
protected:

	std::vector<PATCHFILES> m_fileList; /**< Source files to create patch from */

	void ChangeFile(LPCTSTR sFile, BOOL bLeft);
	void UpdateSettings();
	void LoadSettings();
	void SaveSettings();

	template<DDX_Operation>
			bool UpdateData();
	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

	void OnOK();
	void OnDiffBrowseFile1();
	void OnDiffBrowseFile2();
	void OnDiffBrowseResult();
	void OnSelchangeFile1Combo();
	void OnSelchangeFile2Combo();
	void OnSelchangeResultCombo();
	void OnSelchangeDiffStyle();
	void OnDiffSwapFiles();
	void OnDefaultSettings();

};
