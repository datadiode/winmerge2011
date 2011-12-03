/** 
 * @file  PropArchive.cpp
 *
 * @brief Implementation of PropArchive propertysheet
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "OptionsPanel.h"
#include "Constants.h"
#include "resource.h"
#include "PropArchive.h"
#include "OptionsDef.h"
#include "OptionsMgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// PropArchive dialog

PropArchive::PropArchive()
: OptionsPanel(IDD_PROP_ARCHIVE)
, m_bEnableSupport(false)
, m_nInstallType(0)
, m_bProbeType(false)
{
}

/** 
 * @brief Sets update handlers for dialog controls.
 */
template<ODialog::DDX_Operation op>
bool PropArchive::UpdateData()
{
	DDX_Check<op>(IDC_ARCHIVE_ENABLE, m_bEnableSupport);
	DDX_Check<op>(IDC_ARCHIVE_INSTALSTANDALONE, m_nInstallType, 0);
	DDX_Check<op>(IDC_ARCHIVE_INSTALLOCAL, m_nInstallType, 1);
	DDX_Check<op>(IDC_ARCHIVE_DETECTTYPE, m_bProbeType);
	return true;
}

LRESULT PropArchive::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		if (IsUserInputCommand(wParam))
			UpdateData<Get>();
		switch (wParam)
		{
		case MAKEWPARAM(IDC_ARCHIVE_ENABLE, BN_CLICKED):
			UpdateScreen();
			break;
		case MAKEWPARAM(IDC_ARCHIVE_WWW, BN_CLICKED):
			if ((UINT)ShellExecute(NULL, _T("open"), DownloadUrl, NULL, NULL, SW_SHOWNORMAL) > 32)
				MessageReflect_WebLinkButton<WM_COMMAND>(wParam, lParam);
			else
				MessageBeep(0); // unable to execute file!
			break;
		}
		break;
	case WM_DRAWITEM:
		switch (wParam)
		{
		case IDC_ARCHIVE_WWW:
			return MessageReflect_WebLinkButton<WM_DRAWITEM>(wParam, lParam);
		}
		break;
	case WM_SETCURSOR:
		switch (::GetDlgCtrlID(reinterpret_cast<HWND>(wParam)))
		{
		case IDC_ARCHIVE_WWW:
			return MessageReflect_WebLinkButton<WM_SETCURSOR>(wParam, lParam);
		}
		break;
	}
	return OptionsPanel::WindowProc(message, wParam, lParam);
}


/** 
 * @brief Reads options values from storage to UI.
 */
void PropArchive::ReadOptions()
{
	int enable = COptionsMgr::Get(OPT_ARCHIVE_ENABLE);
	m_bEnableSupport = enable > 0;
	m_nInstallType = enable > 1 ? enable - 1 : 0;
	m_bProbeType = COptionsMgr::Get(OPT_ARCHIVE_PROBETYPE);
}

/** 
 * @brief Writes options values from UI to storage.
 */
void PropArchive::WriteOptions()
{
	if (m_bEnableSupport)
		COptionsMgr::SaveOption(OPT_ARCHIVE_ENABLE, m_nInstallType + 1);
	else
		COptionsMgr::SaveOption(OPT_ARCHIVE_ENABLE, (int)0);
	COptionsMgr::SaveOption(OPT_ARCHIVE_PROBETYPE, m_bProbeType == TRUE);
}

/** 
 * @brief Called Updates controls enabled/disables state.
 */
void PropArchive::UpdateScreen()
{
	UpdateData<Set>();
	GetDlgItem(IDC_ARCHIVE_INSTALLOCAL)->EnableWindow(m_bEnableSupport);
	GetDlgItem(IDC_ARCHIVE_INSTALSTANDALONE)->EnableWindow(m_bEnableSupport);
	GetDlgItem(IDC_ARCHIVE_DETECTTYPE)->EnableWindow(m_bEnableSupport);
}
