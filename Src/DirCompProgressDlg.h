/**
 * @file  DirCompProgressDlg.h
 *
 * @brief Declaration file for Directory compare statusdialog class
 */
#pragma once

#include "CompareStats.h"

class CDirFrame;
class CDirView;
class CompareStatisticsDlg;

/////////////////////////////////////////////////////////////////////////////
// DirCompProgressDlg dialog

/**
 * @brief Class for directory compare statusdialog.
 *
 * Implements non-modal dialog directory compares. We create this
 * modeless dialog when we start the compare and destroy it after
 * compare is ready. Dialog itself shows counts of total found items
 * and items compared so far. And nice progressbar for user to have some
 * idea how compare is going.
 *
 * Status updates are fired by periodic timer events. We have shared
 * datastructure with compare code. Compare code updates status information
 * to datastructure during compare. When timer event fires, dialog reads
 * that datastructure and updates the GUI.
 */
class DirCompProgressDlg : public ODialog
{
// Construction
public:
	DirCompProgressDlg(CDirView *);
	~DirCompProgressDlg();

// Implementation
protected:
	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

	void OnTimer(UINT_PTR nIDEvent);
	BOOL ReadyHandlerSetup();
	void ReadyHandlerReady();

private:
	CDirView *const m_pDirView;
	CDirFrame *const m_pDirDoc; /**< Pointer to dirdoc we are comparing */
	CompareStatisticsDlg *const m_pStatsDlg;
	String m_strPauseContinue;
	int m_rotPauseContinue;
	CMyComPtr<IDispatch> m_spReadyHandler;
};
