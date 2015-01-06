/*
 *	Extended MFC message boxes -- Version 1.1a
 *	Copyright (c) 2004 Michael P. Mehl. All rights reserved.
 *	Copyright (c) 2011 Other contributors.
 *
 *	The contents of this file are subject to the Mozilla Public License
 *	Version 1.1a (the "License"); you may not use this file except in
 *	compliance with the License. You may obtain a copy of the License at 
 *	http://www.mozilla.org/MPL/.
 *
 *	Software distributed under the License is distributed on an "AS IS" basis,
 *	WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 *	for the specific language governing rights and limitations under the
 *	License. 
 *
 *	The Original Code is Copyright (c) 2004 Michael P. Mehl. All rights
 *	reserved. The Initial Developer of the Original Code is Michael P. Mehl
 *	<michael.mehl@web.de>.
 *
 *	Alternatively, the contents of this file may be used under the terms of
 *	the GNU Lesser General Public License Version 2.1 (the "LGPL License"),
 *	in which case the provisions of LGPL License are applicable instead of
 *	those above. If you wish to allow use of your version of this file only
 *	under the terms of the LGPL License and not to allow others to use your
 *	version of this file under the MPL, indicate your decision by deleting
 *	the provisions above and replace them with the notice and other provisions
 *	required by the LGPL License. If you do not delete the provisions above,
 *	a recipient may use your version of this file under either the MPL or
 *	the LGPL License.
 */
#pragma once

//////////////////////////////////////////////////////////////////////////////
// Message box style definitions (mostly taken from WinUser.h).

#ifndef MB_CANCELTRYCONTINUE
#define MB_CANCELTRYCONTINUE		0x00000006L	// Standard for Win 5.x.
#endif

#define MB_CONTINUEABORT			0x00000007L	// Additional style.
#define MB_SKIPSKIPALLCANCEL		0x00000008L	// Additional style.
#define MB_IGNOREIGNOREALLCANCEL	0x00000009L	// Additional style.

#define MB_DONT_DISPLAY_AGAIN		0x01000000L	// Additional style.
#define MB_DONT_ASK_AGAIN			0x02000000L	// Additional style.
#define MB_YES_TO_ALL				0x04000000L	// Additional style.
#define MB_NO_TO_ALL				0x08000000L	// Additional style.

#define MB_DEFAULT_CHECKED			0x10000000L // Additional style.
#define MB_RIGHT_ALIGN				0x20000000L	// Additional style.
#define MB_NO_SOUND					0x40000000L	// Additional style.
#define MB_IGNORE_IF_SILENCED		0x80000000L // Additional style.

#define MB_DEFBUTTON5				0x00000400L	// Additional style.
#define MB_DEFBUTTON6				0x00000500L	// Additional style.

//////////////////////////////////////////////////////////////////////////////
// Dialog element IDs.

#ifndef IDTRYAGAIN
#define IDTRYAGAIN					10			// Standard for Win 5.x.
#endif

#ifndef IDCONTINUE
#define IDCONTINUE					11			// Standard for Win 5.x.
#endif

#define IDYESTOALL					14			// Additional element.
#define IDNOTOALL					15			// Additional element.
#define IDSKIP						16			// Additional element.
#define IDSKIPALL					17			// Additional element.
#define IDIGNOREALL					18			// Additional element.
#define IDCHECKBOX					19			// Additional element.

//////////////////////////////////////////////////////////////////////////////
// Name of the registry section for storing the message box results.

#define REGISTRY_SECTION_MESSAGEBOX	_T("MessageBoxes")

//////////////////////////////////////////////////////////////////////////////
// Class definition.

class CMessageBoxDialog : public ODialog
{
public:
	// Constructor of the class for direct providing of the message strings.
	CMessageBoxDialog(LPCTSTR strMessage,
		LPCTSTR strTitle = NULL, UINT nStyle = MB_OK, UINT nHelp = 0);
	// Method for setting a timeout.
	void SetTimeout(UINT nSeconds, BOOL bDisabled = FALSE);
	// Method for displaying the dialog.
	int DoModal(HINSTANCE hInst, HWND hwndParent, HKEY hKey);
	// Method for initializing the dialog.
	virtual BOOL OnInitDialog();
	// Method for handling command messages.
	virtual BOOL OnCommand(WPARAM, LPARAM);

protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	void OnTimer(UINT_PTR nIDEvent);

private:
	//////////////////////////////////////////////////////////////////////////
	// Private member variables of this dialog.

	String		m_strMessage;		// Message to be displayed.
	String		m_strTitle;			// Title to be used.
	UINT		m_nStyle;			// Style of the message box.
	UINT		m_nHelp;			// Help context of the message box.

	HICON		m_hIcon;			// Icon to be displayed in the dialog.

	UINT		m_nTimeoutSeconds;	// Seconds for a timeout.
	BOOL		m_bTimeoutDisabled;	// Flag whether the timeout is disabled.
	BOOL		m_bDontDisplayAgain;

	typedef struct tagMSGBOXBTN
	{
		int		id;					// ID of a dialog button.
		String	title;				// title of a dialog button.
	} MSGBOXBTN;

	std::vector<MSGBOXBTN> m_aButtons;
									// List of all buttons in the dialog.
	int			m_nDefaultButton;	// ID of the default button.

	POINT		DialogUnitToPixel[41];

private:
	// Method for adding a button to the list of buttons.
	void AddButton(UINT nID, UINT nTitle);
	// Method for parsing the given style.
	void ParseStyle();
	// Method for creating the icon control.
	LPARAM CreateIconControl();
	// Method for creating the message control.
	LPARAM CreateMessageControl(HSurface *, int nXPosition, int nYPosition);
	// Method for creating the checkbox control.
	LPARAM CreateCheckboxControl(HSurface *, int nXPosition, int nYPosition);
	// Method for creating the button controls.
	LPARAM ComputeButtonSize(HSurface *);
	// Method for defining the layout of the dialog.
	void DefineLayout();
};
