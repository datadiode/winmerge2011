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

/*
 *	The flag MB_DONT_DISPLAY_AGAIN or MB_DONT_ASK_AGAIN is stored in the registry
 *  See GenerateRegistryKey for the creation of the key
 *  The "normal" rule is to use the help Id as identifier
 *  And it is really simple to just repeat the text ID as help ID
 *  (for message formed with AfxFormatString, repeat the ID used to format the string)
 *
 *  Search for MB_DONT_DISPLAY_AGAIN and MB_DONT_ASK_AGAIN for all the
 *  concerned AfxMessageBox
 */

#include <StdAfx.h>
#include "resource.h"
#include <windowsx.h> // for GET_X_LPARAM, GET_Y_LPARAM
#include "LanguageSelect.h"
#include "MessageBoxDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using std::vector;

//////////////////////////////////////////////////////////////////////////////
// Layout values (in dialog units).

#define CX_BORDER					8		// Width of the border.
#define CY_BORDER					8		// Height of the border.

#define CX_CHECKBOX_ADDON			14		// Additional width of the checkbox.

#define CX_BUTTON					40		// Standard width of a button.
#define CY_BUTTON					14		// Standard height of a button.
#define CX_BUTTON_BORDER			4		// Standard border for a button.
#define CY_BUTTON_BORDER			1		// Standard border for a button.
#define CX_BUTTON_SPACE				4		// Standard space for a button.

//////////////////////////////////////////////////////////////////////////////
// Timer values.

#define MESSAGE_BOX_TIMER			2201	// Event identifier for the timer.

/*
 *	Constructor of the class.
 *
 *	This constructor is used to provide the strings directly without providing
 *	resource IDs from which these strings should be retrieved. If no title is
 *	given, the application name will be used as the title of the dialog.
 */
 CMessageBoxDialog::CMessageBoxDialog(LPCTSTR strMessage, 
	LPCTSTR strTitle, UINT nStyle, UINT nHelp) 
	: ODialog(IDD_MESSAGE_BOX)
	, m_strMessage(strMessage)
	, m_strTitle(strTitle)
	, m_nStyle(nStyle)
	, m_nHelp(nHelp)
	, m_hIcon(NULL)
	, m_nTimeoutSeconds(0)
	, m_bTimeoutDisabled(FALSE)
	, m_bDontDisplayAgain(FALSE)
	, m_nDefaultButton(-1)
{
	ASSERT(!m_strMessage.empty());
}

//////////////////////////////////////////////////////////////////////////////
// Methods for setting and retrieving dialog options.

/*
 *	Method for setting a timeout.
 *
 *	A timeout is a countdown, which starts, when the message box is displayed.
 *	There are two modes for a timeout: The "un-disabled" or "enabled" timeout
 *	means, that the user can choose any button, but if he doesn't choose one,
 *	the default button will be assumed as being chossen, when the countdown is
 *	finished. The other mode, a "disabled" countdown is something like a nag
 *	screen. All buttons will be disabled, until the countdown is finished.
 *	After that, the user can click any button.
 */
void CMessageBoxDialog::SetTimeout(UINT nSeconds, BOOL bDisabled)
{
	// Save the settings for the timeout.
	m_nTimeoutSeconds	= nSeconds;
	m_bTimeoutDisabled	= bDisabled;
}

//////////////////////////////////////////////////////////////////////////////
// Methods for handling common window functions.

/*
 *	Method for displaying the dialog.
 *
 *	If the MB_DONT_DISPLAY_AGAIN or MB_DONT_ASK_AGAIN flag is set, this
 *	method will check, whether a former result for this dialog was stored
 *	in the registry. If yes, the former result will be returned without
 *	displaying the dialog. Otherwise the message box will be displayed in
 *	the normal way.
 */
int CMessageBoxDialog::DoModal(HINSTANCE hinst, HWND parent, HKEY hKey)
{
	TCHAR strRegistryKey[12];
	_ultot(m_nHelp, strRegistryKey, 10);
	// Check whether the result may be retrieved from the registry.
	if (m_nStyle & (MB_DONT_DISPLAY_AGAIN | MB_DONT_ASK_AGAIN))
	{
		// Try to read the former result of the message box from the registry.
		int nFormerResult = 0;
		DWORD dwType = 0;
		DWORD cbData = sizeof nFormerResult;
		LONG ret = RegQueryValueEx(hKey, strRegistryKey, NULL, &dwType, (LPBYTE)&nFormerResult, &cbData);
		if ((ret == ERROR_SUCCESS) && (dwType == REG_DWORD) && (cbData == sizeof nFormerResult))
		{
			if (m_nStyle & MB_IGNORE_IF_SILENCED)
				return IDIGNORE;
			// Return the former result without displaying the dialog.
			return nFormerResult;
		}
	}
	// Call the parent method.
	int nResult = static_cast<int>(ODialog::DoModal(hinst, parent));
	if ((m_nStyle & (MB_DONT_DISPLAY_AGAIN | MB_DONT_ASK_AGAIN)) && m_bDontDisplayAgain)
	{
		// Store the result of the dialog in the registry.
		RegSetValueEx(hKey, strRegistryKey, 0, REG_DWORD, (LPBYTE)&nResult, sizeof nResult);
	}
	return nResult;
}

/*
 *	Method for initializing the dialog.
 *
 *	This method is used for initializing the dialog. It will create the
 *	content of the dialog, which means it will create all controls and will
 *	size the dialog to fit it's content.
 */
BOOL CMessageBoxDialog::OnInitDialog()
{
	ODialog::OnInitDialog();

	LONG i = _countof(DialogUnitToPixel);
	do
	{
		RECT *prc = reinterpret_cast<RECT *>(DialogUnitToPixel + i - 2);
		prc->left = prc->top = 0;
		prc->right = prc->bottom = i;
		MapDialogRect(prc);
		--i;
	} while (i >= 2);

	// Set the title of the dialog.
	SetWindowText(m_strTitle.c_str());

	// Set the help ID of the dialog.
	ASSERT(((m_nStyle & (MB_DONT_DISPLAY_AGAIN | MB_DONT_ASK_AGAIN)) == 0) || (m_nHelp != 0));

	// Parse the style of the message box.
	ParseStyle();

	// Define the layout of the dialog.
	DefineLayout();

	// Check whether no sound should be generated.
	if (!(m_nStyle & MB_NO_SOUND))
	{
		// Do a beep.
		::MessageBeep(m_nStyle & MB_ICONMASK);
	}

	// Check whether to bring the window to the foreground.
	if (m_nStyle & MB_SETFOREGROUND)
	{
		// Bring the window to the foreground.
		SetForegroundWindow();
	}

	// Check whether the window should be the topmost window.
	if (m_nStyle & (MB_TOPMOST | MB_SYSTEMMODAL))
	{
		SetWindowPos(reinterpret_cast<HWindow *>(HWND_TOPMOST), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}

	// Disable SC_CLOSE if neither IDOK nor IDCANCEL are present.
	if (GetDlgItem(IDOK) == GetDlgItem(IDCANCEL))
	{
		// Disable the close item from the system menu.
		GetSystemMenu(FALSE)->EnableMenuItem(SC_CLOSE, MF_GRAYED);
	}

	// Set the focus to the default button.
	GotoDlgCtrl(GetDlgItem(m_nDefaultButton));
	// Check whether a timeout is set.
	if (m_nTimeoutSeconds > 0)
	{
		// Install a timer.
		SetTimer(MESSAGE_BOX_TIMER, 1000);
		// Check whether it's a disabled timeout.
		if (m_bTimeoutDisabled)
		{
			HWindow *pwndButton = NULL;
			while ((pwndButton = FindWindowEx(pwndButton, WC_BUTTON)) != NULL)
			{
				pwndButton->EnableWindow(FALSE);
			}
		}
	}
	// Return FALSE to retain focus as set above.
	return FALSE;
}

/*
 *	Method for handling command messages.
 *
 *	This method will handle command messages, which are those messages, which
 *	are generated, when a user clicks a button of the dialog.
 */
BOOL CMessageBoxDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (HIWORD(wParam) == BN_CLICKED)
	{
		switch (WORD id = LOWORD(wParam))
		{
		case IDHELP:
			// Display the help for this message box.
			//OnHelp();
			return TRUE;
		case IDCHECKBOX:
			m_bDontDisplayAgain = ::IsDlgButtonChecked(m_hWnd, IDCHECKBOX);
			return TRUE;
		default:
			// Check whether a disabled timeout is running.
			if (!m_bTimeoutDisabled || m_nTimeoutSeconds == 0)
			{
				::EndDialog(m_hWnd, id);
			}
			return TRUE;
		}
	}
	return FALSE;
}

LRESULT CMessageBoxDialog::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_TIMER:
		OnTimer(wParam);
		break;
	case WM_PARENTNOTIFY:
		if (LOWORD(wParam) == WM_CREATE)
		{
			// Set the font of the control.
			HWND hwndCtrl = reinterpret_cast<HWND>(lParam);
			WPARAM wFont = ::SendMessage(m_hWnd, WM_GETFONT, 0, 0);
			::SendMessage(hwndCtrl, WM_SETFONT, wFont, TRUE);
		}
		break;
	case WM_COMMAND:
		if (OnCommand(wParam, lParam))
			return TRUE;
		break;
	}
	return ODialog::WindowProc(message, wParam, lParam);
}

/*
 *	Method for handling a timer event.
 *
 *	When a timeout for the message box is set, this method will perform the
 *	update of the dialog controls every second.
 */
void CMessageBoxDialog::OnTimer(UINT_PTR nIDEvent)
{
	// Check whether the event is interesting for us.
	if (nIDEvent == MESSAGE_BOX_TIMER)
	{
		// Decrease the remaining seconds.
		m_nTimeoutSeconds--;
		// Check whether the timeout is finished.
		if (m_nTimeoutSeconds == 0)
		{
			// Kill the timer for this event and reset the handle.
			KillTimer(MESSAGE_BOX_TIMER);
			// Check whether it has been a disabled timeout.
			if (m_bTimeoutDisabled)
			{
				HWindow *pwndButton = NULL;
				while ((pwndButton = FindWindowEx(pwndButton, WC_BUTTON)) != NULL)
				{
					pwndButton->EnableWindow(TRUE);
				}
				// Set the focus to the default button.
				GotoDlgCtrl(GetDlgItem(m_nDefaultButton));
			}
			else
			{
				// End the dialog with the default button.
				EndDialog(m_nDefaultButton);
			}
		}
		TCHAR title[100];
		if (GetDlgItemText(m_nDefaultButton, title, _countof(title) - 15))
		{
			if (LPTSTR equals = _tcsrchr(title, _T('=')))
			{
				wsprintf(equals, _T("= %d"), m_nTimeoutSeconds);
				SetDlgItemText(m_nDefaultButton, title);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
// Helper methods.

/*
 *	Method for adding a button to the list of buttons.
 *
 *	This method adds a button to the list of buttons, which will be created in
 *	the dialog, but it will not create the button control itself.
 */
void CMessageBoxDialog::AddButton(UINT nID, UINT nTitle)
{
	// Create a new structure to store the button information.
	MSGBOXBTN bButton = { nID, LanguageSelect.LoadString(nTitle) };
	// Add the button to the list of buttons.
    m_aButtons.push_back(bButton);
}

/*
 *	Method for parsing the given style.
 *
 *	This method will parse the given style for the message box and will create
 *	the elements of the dialog box according to it. If you want to add more
 *	user defined styles, simply modify this method.
 */
void CMessageBoxDialog::ParseStyle()
{
	// Switch the style of the buttons.
	switch (m_nStyle & MB_TYPEMASK)
	{
	case MB_OKCANCEL:
		// Add two buttons: "Ok" and "Cancel".
		AddButton(IDOK, IDS_MESSAGEBOX_OK);
		AddButton(IDCANCEL, IDS_MESSAGEBOX_CANCEL);
		break;

	case MB_ABORTRETRYIGNORE:
		// Add three buttons: "Abort", "Retry" and "Ignore".
		AddButton(IDABORT, IDS_MESSAGEBOX_ABORT);
		AddButton(IDRETRY, IDS_MESSAGEBOX_RETRY);
		AddButton(IDIGNORE, IDS_MESSAGEBOX_IGNORE);
		break;

	case MB_YESNOCANCEL:
		// Add three buttons: "Yes", "No" and "Cancel".
		AddButton(IDYES, IDS_MESSAGEBOX_YES);
		// Check whether to add a "Yes to all" button.
		if (m_nStyle & MB_YES_TO_ALL)
		{
			// Add the additional button.
			AddButton(IDYESTOALL, IDS_MESSAGEBOX_YES_TO_ALL);
		}
		AddButton(IDNO, IDS_MESSAGEBOX_NO);
		// Check whether to add a "No to all" button.
		if (m_nStyle & MB_NO_TO_ALL)
		{
			// Add the additional button.
			AddButton(IDNOTOALL, IDS_MESSAGEBOX_NO_TO_ALL);
		}
		AddButton(IDCANCEL, IDS_MESSAGEBOX_CANCEL);
		break;

	case MB_YESNO:
		// Add two buttons: "Yes" and "No".
		AddButton(IDYES, IDS_MESSAGEBOX_YES);
		// Check whether to add a "Yes to all" button.
		if (m_nStyle & MB_YES_TO_ALL)
		{
			// Add the additional button.
			AddButton(IDYESTOALL, IDS_MESSAGEBOX_YES_TO_ALL);
		}
		AddButton(IDNO, IDS_MESSAGEBOX_NO);
		// Check whether to add a "No to all" button.
		if (m_nStyle & MB_NO_TO_ALL)
		{
			// Add the additional button.
			AddButton(IDNOTOALL, IDS_MESSAGEBOX_NO_TO_ALL);
		}
		break;

	case MB_RETRYCANCEL:
		// Add two buttons: "Retry" and "Cancel".
		AddButton(IDRETRY, IDS_MESSAGEBOX_RETRY);
		AddButton(IDCANCEL, IDS_MESSAGEBOX_CANCEL);
		break;

	case MB_CANCELTRYCONTINUE:
		// Add three buttons: "Cancel", "Try again" and "Continue".
		AddButton(IDCANCEL, IDS_MESSAGEBOX_CANCEL);
		AddButton(IDTRYAGAIN, IDS_MESSAGEBOX_RETRY);
		AddButton(IDCONTINUE, IDS_MESSAGEBOX_CONTINUE);
		break;

	case MB_CONTINUEABORT:
		// Add two buttons: "Continue" and "Abort".
		AddButton(IDCONTINUE, IDS_MESSAGEBOX_CONTINUE);
		AddButton(IDABORT, IDS_MESSAGEBOX_ABORT);
		break;

	case MB_SKIPSKIPALLCANCEL:
		// Add three buttons: "Skip", "Skip all" and "Cancel".
		AddButton(IDSKIP, IDS_MESSAGEBOX_SKIP);
		AddButton(IDSKIPALL, IDS_MESSAGEBOX_SKIPALL);
		AddButton(IDCANCEL, IDS_MESSAGEBOX_CANCEL);
		break;

	case MB_IGNOREIGNOREALLCANCEL:
		// Add three buttons: "Ignore", "Ignore all" and "Cancel".
		AddButton(IDIGNORE, IDS_MESSAGEBOX_IGNORE);
		AddButton(IDIGNOREALL, IDS_MESSAGEBOX_IGNOREALL);
		AddButton(IDCANCEL, IDS_MESSAGEBOX_CANCEL);
		break;

	default:
		ASSERT(FALSE);
		// fall through
	case MB_OK:
		// Add just one button: "Ok".
		AddButton(IDOK, IDS_MESSAGEBOX_OK);
		break;
	}

	// Check whether to add a help button.
	if (m_nStyle & MB_HELP)
	{
		// Add the help button.
		AddButton(IDHELP, IDS_MESSAGEBOX_HELP);
	}

	// Check whether a default button was defined.
	// Switch the default button.
	C_ASSERT((MB_DEFBUTTON1 >> 8) == 0);
	C_ASSERT((MB_DEFBUTTON2 >> 8) == 1);
	C_ASSERT((MB_DEFBUTTON3 >> 8) == 2);
	C_ASSERT((MB_DEFBUTTON4 >> 8) == 3);
	C_ASSERT((MB_DEFBUTTON5 >> 8) == 4);
	C_ASSERT((MB_DEFBUTTON6 >> 8) == 5);
	vector<MSGBOXBTN>::size_type nDefaultIndex = (m_nStyle & MB_DEFMASK) >> 8;

	// Check whether enough buttons are available.
	if (nDefaultIndex >= m_aButtons.size())
		nDefaultIndex = 0;
	// Set the new default button.
	m_nDefaultButton = m_aButtons[nDefaultIndex].id;
	SetDefID(m_nDefaultButton);

	if (m_nTimeoutSeconds > 0)
	{
		// Add the remaining seconds to the text of the button.
		TCHAR szTimeoutSeconds[40];
		wsprintf(szTimeoutSeconds, _T(" = %d"), m_nTimeoutSeconds);
		m_aButtons[nDefaultIndex].title += szTimeoutSeconds;
	}

	// Check whether an icon was specified.
	if ((m_nStyle & MB_ICONMASK) && (m_hIcon == NULL))
	{
		// Switch the icon.
		switch (m_nStyle & MB_ICONMASK)
		{
		case MB_ICONEXCLAMATION:
			// Load the icon with the exclamation mark.
			m_hIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_EXCLAMATION));
			break;

		case MB_ICONHAND:
			// Load the icon with the error symbol.
			m_hIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_HAND));
			break;

		case MB_ICONQUESTION:
			// Load the icon with the question mark.
			m_hIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_QUESTION));
			break;

		case MB_ICONASTERISK:
			// Load the icon with the information symbol.
			m_hIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_ASTERISK));
			break;
		}
	}
}

/*
 *	Method for creating the icon control.
 *
 *	This method will check whether the handle for an icon was defined and if
 *	yes it will create an control in the dialog to display that icon.
 */
LPARAM CMessageBoxDialog::CreateIconControl()
{
	// Create the control for the icon.
	RECT rect;
	::SetRectEmpty(&rect);
	if (HStatic *pwndIcon = HStatic::Create(
		WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_ICON,
		DialogUnitToPixel[CX_BORDER].x, DialogUnitToPixel[CY_BORDER].y,
		0, 0, m_pWnd, 0))
	{
		pwndIcon->SetIcon(m_hIcon);
		pwndIcon->GetClientRect(&rect);
	}

	// Return the size of the icon.
	return MAKELPARAM(rect.right, rect.bottom);
}

/*
 *	Method for creating the text control.
 *
 *	This method create the control displaying the text of the message for the
 *	message box. It will also try to determine the size required for the
 *	message.
 */
LPARAM CMessageBoxDialog::CreateMessageControl(HSurface *pdc, int nXPosition, int nYPosition)
{
	ASSERT(!m_strMessage.empty());
	// Define the maximum width of the message.
	RECT rect = { 0, 0, GetSystemMetrics(SM_CXSCREEN) / 2 + 100, 0 };
	// Check whether an icon is displayed.
	if (nXPosition > DialogUnitToPixel[CX_BORDER].x)
		rect.right -= nXPosition; // Decrease the maximum width.
	// Draw the text and retrieve the size of the text.
	pdc->DrawText(m_strMessage, &rect, DT_NOPREFIX | DT_WORDBREAK | DT_EXPANDTABS | DT_CALCRECT);
	// Create a variable with the style of the control.
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | SS_NOPREFIX;
	// Check whether the text should be right aligned.
	if (m_nStyle & MB_RIGHT)
		dwStyle |= SS_RIGHT;
	// Create the static control for the message.
	::CreateWindowEx(
		m_nStyle & MB_RTLREADING ? WS_EX_RTLREADING : 0,
		WC_STATIC, m_strMessage.c_str(), dwStyle,
		nXPosition, nYPosition, rect.right, rect.bottom,
		m_hWnd, NULL, NULL, NULL);

	return MAKELPARAM(rect.right, rect.bottom);
}

/*
 *	Method for creating the checkbox control.
 *
 *	If the user either specified the MB_DONT_DISPLAY_AGAIN or
 *	MB_DONT_ASK_AGAIN flag, this method will create a checkbox in the dialog
 *	for enabling the user to disable this dialog in the future.
 */
LPARAM CMessageBoxDialog::CreateCheckboxControl(HSurface *pdc, int nXPosition, int nYPosition)
{
	// Create a variable for storing the title of the checkbox.
	String strCheckboxTitle;
	// Check which style is used.
	if (m_nStyle & MB_DONT_DISPLAY_AGAIN)
	{
		// Load the string for the checkbox.
		strCheckboxTitle = LanguageSelect.LoadString(IDS_MESSAGEBOX_DONT_DISPLAY_AGAIN);
	}
	else if (m_nStyle & MB_DONT_ASK_AGAIN)
	{
		// Load the string for the checkbox.
		strCheckboxTitle = LanguageSelect.LoadString(IDS_MESSAGEBOX_DONT_ASK_AGAIN);
	}

	ASSERT(!strCheckboxTitle.empty());

	RECT rect = { 0, 0, 0, 0 };

	// Retrieve the size of the text.
	pdc->DrawText(strCheckboxTitle, &rect, DT_SINGLELINE | DT_CALCRECT);

	// Add the additional value to the width of the checkbox.
	rect.right += DialogUnitToPixel[CX_CHECKBOX_ADDON].x;

	// Create the checkbox.
	HButton::Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
		nXPosition, nYPosition, rect.right, rect.bottom,
		m_pWnd, IDCHECKBOX, strCheckboxTitle.c_str());

	// Check whether the checkbox should be marked checked at startup.
	if (m_nStyle & MB_DEFAULT_CHECKED)
	{
		CheckDlgButton(IDCHECKBOX, BST_CHECKED);
		m_bDontDisplayAgain = TRUE;
	}
	return MAKELPARAM(rect.right, rect.bottom);
}

/*
 *	Method for creating the button controls.
 *
 *	According to the list of buttons, which should be displayed in this
 *	message box, this method will create them and add them to the dialog.
 */
LPARAM CMessageBoxDialog::ComputeButtonSize(HSurface *pdc)
{
	SIZE sButton =
	{ DialogUnitToPixel[CX_BUTTON].x, DialogUnitToPixel[CY_BUTTON].y };

	RECT rect = { 0, 0, 0, 0 };

	vector<MSGBOXBTN>::iterator iter;
	// Run through all buttons defined in the list of the buttons.
	for (iter = m_aButtons.begin(); iter != m_aButtons.end(); ++iter)
	{
		// Retrieve the size of the text.
		pdc->DrawText(iter->title, &rect, DT_SINGLELINE | DT_CALCRECT);
		// Resize the button.
		if (sButton.cx < rect.right)
			sButton.cx = rect.right;
		if (sButton.cy < rect.bottom)
			sButton.cy = rect.bottom;
	}

	// Add margins to the button size.
	sButton.cx += 2 * DialogUnitToPixel[CX_BUTTON_BORDER].x;
	sButton.cy += 2 * DialogUnitToPixel[CY_BUTTON_BORDER].y; // .x?

	return MAKELPARAM(sButton.cx, sButton.cy);
}

/*
 *	Method for defining the layout of the dialog.
 *
 *	This method will define the actual layout of the dialog. This layout is
 *	based on the created controls for the dialog.
 */
void CMessageBoxDialog::DefineLayout()
{
	HSurface *pdc = GetDC();
	pdc->SelectObject(GetFont());
	// Create a variable for storing the size of the dialog.
	SIZE sClient =
	{ 2 * DialogUnitToPixel[CX_BORDER].x, 2 * DialogUnitToPixel[CY_BORDER].y };

	// Create a variable to store the left position for a control element.
	int nXPosition = DialogUnitToPixel[CX_BORDER].x;
	int nYPosition = DialogUnitToPixel[CY_BORDER].y;

	// Check whether an icon is defined.
	if (m_hIcon != NULL)
	{
		
		LPARAM sIcon = CreateIconControl();
		// Add the size of the icon to the size of the dialog.
		sClient.cx += GET_X_LPARAM(sIcon) + DialogUnitToPixel[CX_BORDER].x;
		sClient.cy += GET_Y_LPARAM(sIcon) + DialogUnitToPixel[CY_BORDER].y;
		// Increase the x position for following control elements.
		nXPosition += GET_X_LPARAM(sIcon) + DialogUnitToPixel[CX_BORDER].x;
	}

	LPARAM sMessage = CreateMessageControl(pdc, nXPosition, nYPosition);
	// Change the size of the dialog according to the size of the message.
	sClient.cx += GET_X_LPARAM(sMessage) + DialogUnitToPixel[CX_BORDER].x;
	sClient.cy = max(sClient.cy, GET_Y_LPARAM(sMessage) + 2 *
		DialogUnitToPixel[CY_BORDER].y + DialogUnitToPixel[CY_BORDER / 2].y);

	// Define the new y position.
	nYPosition += GET_Y_LPARAM(sMessage) + DialogUnitToPixel[CY_BORDER].y +
		DialogUnitToPixel[CY_BORDER / 2].y;

	// Check whether an checkbox is defined.
	if (m_nStyle & (MB_DONT_DISPLAY_AGAIN | MB_DONT_ASK_AGAIN))
	{
		LPARAM sCheckbox = CreateCheckboxControl(pdc, nXPosition, nYPosition);
		// Try to determine the control element for the checkbox.
		// Resize the dialog if necessary.
		sClient.cx = max(sClient.cx, nXPosition + GET_X_LPARAM(sCheckbox) +
			DialogUnitToPixel[CX_BORDER].x);
		sClient.cy = max(sClient.cy, nYPosition + GET_Y_LPARAM(sCheckbox) +
			DialogUnitToPixel[CY_BORDER].y);

		// Define the y positions.
		nYPosition += GET_Y_LPARAM(sCheckbox) + DialogUnitToPixel[CY_BORDER].y;
	}

	LPARAM sButton = ComputeButtonSize(pdc);
	// Calculate the width of the buttons.
	int cxButtons =
		(m_aButtons.size() - 1) * DialogUnitToPixel[CX_BUTTON_SPACE].x +
		m_aButtons.size() * GET_X_LPARAM(sButton);
	int cyButtons = GET_Y_LPARAM(sButton);

	// Add the size of the buttons to the dialog.
	sClient.cx = max(sClient.cx, 2 * DialogUnitToPixel[CX_BORDER].x + cxButtons);
	sClient.cy += cyButtons + DialogUnitToPixel[CY_BORDER].y;

	// Calculate the start y position for the buttons.
	int nXButtonPosition = (sClient.cx - cxButtons) / 2;
	int nYButtonPosition = sClient.cy - DialogUnitToPixel[CY_BORDER].y - cyButtons;

	// Check whether the buttons should be right aligned.
	if (m_nStyle & MB_RIGHT_ALIGN)
	{
		// Right align the buttons.
		nXButtonPosition = sClient.cx - cxButtons - 
			DialogUnitToPixel[CX_BORDER].x;
	}

	// Run through all buttons.
	for (vector<MSGBOXBTN>::iterator iter = m_aButtons.begin(); iter != m_aButtons.end(); ++iter)
	{
		// Create the button.
		HButton::Create(
			WS_CHILD | WS_VISIBLE | WS_TABSTOP,
			nXButtonPosition, nYButtonPosition,
			GET_X_LPARAM(sButton), GET_Y_LPARAM(sButton),
			m_pWnd, iter->id, iter->title.c_str());

		nXButtonPosition += GET_X_LPARAM(sButton) + DialogUnitToPixel[CX_BUTTON_SPACE].x;
	}

	// Set the dimensions of the dialog.
	RECT rcClient = { 0, 0, sClient.cx, sClient.cy };
	// Calculate the window rect.
	::AdjustWindowRect(&rcClient, GetStyle(), FALSE);
	RECT rcParent;
	if (HWindow *pParent = GetParent())
		pParent->GetWindowRect(&rcParent);
	else
		H2O::GetDesktopWorkArea(NULL, &rcParent);
	// Move the window.
	MoveWindow(
		(rcParent.left + rcParent.right) / 2 - (rcClient.right - rcClient.left) / 2,
		(rcParent.top + rcParent.bottom) / 2 - (rcClient.bottom - rcClient.top) / 2,
		rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
	ReleaseDC(pdc);
}
