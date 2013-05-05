/** 
 * @file  PropArchive.cpp
 *
 * @brief Implementation of PropArchive propertysheet
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "OptionsPanel.h"
#include "resource.h"
#include "PropArchive.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// PropArchive dialog

PropArchive::PropArchive()
: OptionsPanel(IDD_PROP_ARCHIVE, sizeof *this)
{
}

/** 
 * @brief Sets update handlers for dialog controls.
 */
template<ODialog::DDX_Operation op>
bool PropArchive::UpdateData()
{
	DDX_Check<op>(IDC_ARCHIVE_ENABLE, m_bEnableSupport);
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
	m_bEnableSupport = COptionsMgr::Get(OPT_ARCHIVE_ENABLE);
	m_bProbeType = COptionsMgr::Get(OPT_ARCHIVE_PROBETYPE);
}

/** 
 * @brief Writes options values from UI to storage.
 */
void PropArchive::WriteOptions()
{
	COptionsMgr::SaveOption(OPT_ARCHIVE_ENABLE, m_bEnableSupport != FALSE);
	COptionsMgr::SaveOption(OPT_ARCHIVE_PROBETYPE, m_bProbeType != FALSE);
}

/** 
 * @brief Called Updates controls enabled/disables state.
 */
void PropArchive::UpdateScreen()
{
	UpdateData<Set>();
	GetDlgItem(IDC_ARCHIVE_DETECTTYPE)->EnableWindow(m_bEnableSupport);
}
