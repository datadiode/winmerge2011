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
 * @file  SaveClosingDlg.h
 *
 * @brief Declaration file for SaveClosingDlg dialog
 */
#pragma once

/////////////////////////////////////////////////////////////////////////////
// SaveClosingDlg dialog

/**
 * @brief Dialog asking if user wants to save modified left and/or right
 * files.
 *
 * The dialog has separate frames for both files and unneeded frame and
 * controls inside it are disabled. Asked file(s) are selected using
 * DoAskFor() function.
 */
class SaveClosingDlg : public OResizableDialog
{
public:

	/** @brief Choices for modified files: save/discard changes. */
	enum SAVECLOSING_CHOICE
	{
		SAVECLOSING_SAVE,     /**< Save changes */
		SAVECLOSING_DISCARD,  /**< Discard changes */
	};

	SaveClosingDlg();   // standard constructor

// Dialog Data
	BOOL m_bAskForLeft; /**< Is left file modified? */
	BOOL m_bAskForRight; /**< Is right file modified? */
	String m_sLeftFile; /**< Path to left-file to save. */
	String m_sRightFile; /**< Path to right-side file to save. */
	int m_leftSave; /**< User's choice for left-side save. */
	int m_rightSave; /**< User's choice for righ-side save. */
	BOOL m_bDisableCancel; /**< Should we disable Cancel-button? */

protected:
	template<DDX_Operation>
			bool UpdateData();
	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

	void OnDiscardAll();
	void OnOK();
};
