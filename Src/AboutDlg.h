/////////////////////////////////////////////////////////////////////////////
//    SPDX-License-Identifier: GPL-3.0-or-later
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

	CMyComPtr<IPicture> m_spIPicture;
	HFont *m_font_gnu_ascii;
};
