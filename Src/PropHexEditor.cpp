/** 
 * @file  PropHexEditor.cpp
 *
 * @brief Implementation of PropHexEditor propertysheet
 */
#include "StdAfx.h"
#include "OptionsPanel.h"
#include "resource.h"
#include "LanguageSelect.h"
#include "PropHexEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/** @brief Maximum number of bytes per line. */
#define MAX_BPL 16384

/** 
 * @brief Constructor.
 */
PropHexEditor::PropHexEditor()
: OptionsPanel(IDD_PROPPAGE_HEX_EDITOR, sizeof *this)
{
}

/** 
 * @brief Function handling dialog data exchange between GUI and variables.
 */
template<ODialog::DDX_Operation op>
bool PropHexEditor::UpdateData()
{
	DDX_Text<op>(IDC_BPL_EDIT, m_nBytesPerLine);
	DDX_Check<op>(IDC_BPL_AUTO, m_bAutomaticBPL);
	return true;
}

LRESULT PropHexEditor::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		if (IsUserInputCommand(wParam))
			UpdateData<Get>();
		switch (wParam)
		{
		case MAKEWPARAM(IDC_BPL_EDIT, EN_CHANGE):
			m_nBytesPerLine = ValidateNumber(reinterpret_cast<HEdit *>(lParam), 1, MAX_BPL);
			break;
		}
		break;
	}
	return OptionsPanel::WindowProc(message, wParam, lParam);
}

/** 
 * @brief Reads options values from storage to UI.
 */
void PropHexEditor::ReadOptions()
{
	m_nBytesPerLine = COptionsMgr::Get(OPT_BYTES_PER_LINE);
	m_bAutomaticBPL = COptionsMgr::Get(OPT_AUTOMATIC_BPL);
}

/** 
 * @brief Writes options values from UI to storage.
 */
void PropHexEditor::WriteOptions()
{
	COptionsMgr::SaveOption(OPT_BYTES_PER_LINE, (int)m_nBytesPerLine);
	COptionsMgr::SaveOption(OPT_AUTOMATIC_BPL, m_bAutomaticBPL != FALSE);
}

/** 
 * @brief Called before propertysheet is drawn.
 */
BOOL PropHexEditor::OnInitDialog()
{
	if (HEdit *edit = static_cast<HEdit *>(GetDlgItem(IDC_BPL_EDIT)))
	{
		edit->SetLimitText(sizeof _CRT_STRINGIZE(MAX_BPL) - 1);
	}
	return OptionsPanel::OnInitDialog();
}

/** 
 * @brief Update availability of line difference controls.
 */
void PropHexEditor::UpdateScreen()
{
	UpdateData<Set>();
}
