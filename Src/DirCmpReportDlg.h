/** 
 * @file  DirCmpReportDlg.h
 *
 * @brief Declaration file for DirCmpReport Dialog.
 *
 */
#pragma once

#include "DirReportTypes.h"
#include "SuperComboBox.h"

/** 
 * @brief Folder compare dialog class.
 * This dialog (and class) showa folder-compare report's selections
 * for user. Also filename and path for report file can be chosen
 * with this dialog.
 */
class DirCmpReportDlg
	: ZeroInit<DirCmpReportDlg>
	, public ODialog
{
public:
	DirCmpReportDlg();   // standard constructor

	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	void OnOK();
	void OnSelchangeFile();
	void OnSelchangeStyle();
	void OnBtnClickReportBrowse();
	void OnBtnDblclickCopyClipboard();

	HSuperComboBox *m_pCbReportFile; /**< Report filename control */
	String m_sReportFile; /**< Report filename string */
	HComboBox *m_ctlStyle; /**< Report type control */
	REPORT_TYPE m_nReportType; /**< Report type integer */
	BOOL m_bCopyToClipboard; /**< Do we copy report to clipboard? */
};
