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
 * @file  CompareStatisticsDlg.cpp
 *
 * @brief Implementation file for CompareStatisticsDlg dialog
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "Merge.h"
#include "LanguageSelect.h"
#include "CompareStatisticsDlg.h"
#include "CompareStats.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// SaveClosingDlg dialog

CompareStatisticsDlg::CompareStatisticsDlg(const CompareStats *pCompareStats)
	: ODialog(IDD_COMPARE_STATISTICS)
	, m_pCompareStats(pCompareStats)
{
}

/**
 * @brief Initialize the dialog, set statistics and load icons.
 */
BOOL CompareStatisticsDlg::OnInitDialog()
{
	ODialog::OnInitDialog();
	LanguageSelect.TranslateDialog(m_hWnd);

	// Load icons
	if (HANDLE hIcon = LanguageSelect.LoadSmallIcon(IDI_LFOLDER))
		SendDlgItemMessage(IDC_STAT_ILUNIQFOLDER, STM_SETICON, (WPARAM)hIcon);
	if (HANDLE hIcon = LanguageSelect.LoadSmallIcon(IDI_RFOLDER))
		SendDlgItemMessage(IDC_STAT_IRUNIQFOLDER, STM_SETICON, (WPARAM)hIcon);
	if (HANDLE hIcon = LanguageSelect.LoadSmallIcon(IDI_FOLDER))
		SendDlgItemMessage(IDC_STAT_IIDENTICFOLDER, STM_SETICON, (WPARAM)hIcon);
	if (HANDLE hIcon = LanguageSelect.LoadSmallIcon(IDI_NOTEQUALFILE))
		SendDlgItemMessage(IDC_STAT_INOTEQUAL, STM_SETICON, (WPARAM)hIcon);
	if (HANDLE hIcon = LanguageSelect.LoadSmallIcon(IDI_BINARYDIFF))
		SendDlgItemMessage(IDC_STAT_IDIFFBINFILE, STM_SETICON, (WPARAM)hIcon);
	if (HANDLE hIcon = LanguageSelect.LoadSmallIcon(IDI_LFILE))
		SendDlgItemMessage(IDC_STAT_ILUNIQFILE, STM_SETICON, (WPARAM)hIcon);
	if (HANDLE hIcon = LanguageSelect.LoadSmallIcon(IDI_RFILE))
		SendDlgItemMessage(IDC_STAT_IRUNIQFILE, STM_SETICON, (WPARAM)hIcon);
	if (HANDLE hIcon = LanguageSelect.LoadSmallIcon(IDI_EQUALFILE))
		SendDlgItemMessage(IDC_STAT_IEQUALFILE, STM_SETICON, (WPARAM)hIcon);
	if (HANDLE hIcon = LanguageSelect.LoadSmallIcon(IDI_EQUALBINARY))
		SendDlgItemMessage(IDC_STAT_IEQUALBINFILE, STM_SETICON, (WPARAM)hIcon);
	if (HANDLE hIcon = LanguageSelect.LoadSmallIcon(IDI_SIGMA))
	{
		SendDlgItemMessage(IDC_STAT_ITOTALFOLDERS, STM_SETICON, (WPARAM)hIcon);
		SendDlgItemMessage(IDC_STAT_ITOTALFILES, STM_SETICON, (WPARAM)hIcon);
	}

	Update();
	return FALSE; // Do not set focus
}

void CompareStatisticsDlg::Update()
{
	int totalFiles = 0;
	int totalFolders = 0;

	// Identical
	int count = m_pCompareStats->GetCount(CompareStats::DIFFIMG_DIR);
	totalFolders += count;
	SetDlgItemInt(IDC_STAT_IDENTICFOLDER, count);
	count = m_pCompareStats->GetCount(CompareStats::DIFFIMG_TEXTSAME);
	totalFiles += count;
	SetDlgItemInt(IDC_STAT_IDENTICFILE, count);
	count = m_pCompareStats->GetCount(CompareStats::DIFFIMG_BINSAME);
	totalFiles += count;
	SetDlgItemInt(IDC_STAT_IDENTICBINARY, count);

	// Different
	count = m_pCompareStats->GetCount(CompareStats::DIFFIMG_TEXTDIFF);
	totalFiles += count;
	SetDlgItemInt(IDC_STAT_DIFFFILE, count);
	count = m_pCompareStats->GetCount(CompareStats::DIFFIMG_BINDIFF);
	totalFiles += count;
	SetDlgItemInt(IDC_STAT_DIFFBINARY, count);

	// Unique
	count = m_pCompareStats->GetCount(CompareStats::DIFFIMG_LDIRUNIQUE);
	totalFolders += count;
	SetDlgItemInt(IDC_STAT_LUNIQFOLDER, count);
	count = m_pCompareStats->GetCount(CompareStats::DIFFIMG_LUNIQUE);
	totalFiles += count;
	SetDlgItemInt(IDC_STAT_LUNIQFILE, count);
	count = m_pCompareStats->GetCount(CompareStats::DIFFIMG_RDIRUNIQUE);
	totalFolders += count;
	SetDlgItemInt(IDC_STAT_RUNIQFOLDER, count);
	count = m_pCompareStats->GetCount(CompareStats::DIFFIMG_RUNIQUE);
	totalFiles += count;
	SetDlgItemInt(IDC_STAT_RUNIQFILE, count);

	// Total
	SetDlgItemInt(IDC_STAT_TOTALFOLDER, totalFolders);
	SetDlgItemInt(IDC_STAT_TOTALFILE, totalFiles);
}

LRESULT CompareStatisticsDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
		case IDCANCEL:
			EndDialog(wParam);
			break;
		}
		break;
	}
	return ODialog::WindowProc(message, wParam, lParam);
}
