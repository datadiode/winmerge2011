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
#include "SettingStore.h"
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
CMessageBoxDialog::CMessageBoxDialog(LPCTSTR strMessage, UINT nStyle, UINT nHelp)
	: ODialog(IDD_MESSAGE_BOX)
	, m_nStyle(nStyle)
	, m_nHelp(nHelp)
	, m_hIcon(NULL)
	, m_nTimeoutSeconds(0)
	, m_bTimeoutDisabled(FALSE)
	, m_bDontDisplayAgain(FALSE)
	, m_nDefaultButton(-1)
{
	int length = 0;
	// First, compute length of canonicalized string and reserve memory for it.
	LPCTSTR p = strMessage;
	while (p)
	{
		LPCTSTR q = _tcschr(p, _T('\n'));
		if (int i = static_cast<int>(q ? ++q - p : _tcslen(p)))
		{
			int j = i;
			int k = p[--i] != '\n' ? 0 : i == 0 || p[--i] != '\r' ? 1 : 2;
			length += k != 0 ? j - k + 2 : j;
		}
		p = q;
	}
	m_strMessage.reserve(length);
	// Second, canonicalize the string while copying it to the reserved memory.
	p = strMessage;
	while (p)
	{
		LPCTSTR q = _tcschr(p, _T('\n'));
		if (int i = static_cast<int>(q ? ++q - p : _tcslen(p)))
		{
			int j = i;
			int k = p[--i] != '\n' ? 0 : i == 0 || p[--i] != '\r' ? 1 : 2;
			m_strMessage.append(p, k != 0 ? j - k + 2 : j);
			if (k != 0)
			{
				String::iterator r = m_strMessage.end();
				*--r = '\n';
				*--r = '\r';
			}
		}
		p = q;
	}
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
		LONG ret = SettingStore.RegQueryValueEx(hKey, strRegistryKey, &dwType, (LPBYTE)&nFormerResult, &cbData);
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
	if (m_bDontDisplayAgain)
	{
		// Store the result of the dialog in the registry.
		SettingStore.RegSetValueEx(hKey, strRegistryKey, REG_DWORD, (LPBYTE)&nResult, sizeof nResult);
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

LRESULT CMessageBoxDialog::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_TIMER:
		OnTimer(wParam);
		break;
	case WM_CTLCOLORSTATIC:
		if (reinterpret_cast<HWND>(lParam) == m_edit.m_hWnd)
		{
			reinterpret_cast<HSurface *>(wParam)->SetBkMode(TRANSPARENT);
			return reinterpret_cast<LRESULT>(GetStockObject(NULL_BRUSH));
		}
		break;
	case WM_PARENTNOTIFY:
		if (LOWORD(wParam) == WM_CREATE)
		{
			// Set the font of the control.
			HWindow *const pwndCtrl = reinterpret_cast<HWindow *>(lParam);
			WPARAM wFont = SendMessage(WM_GETFONT);
			pwndCtrl->SendMessage(WM_SETFONT, wFont, TRUE);
		}
		break;
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			switch (WORD id = LOWORD(wParam))
			{
			case IDHELP:
				// Display the help for this message box.
				//OnHelp();
				break;
			case IDCHECKBOX:
				m_bDontDisplayAgain = IsDlgButtonChecked(IDCHECKBOX);
				break;
			default:
				// Check whether a disabled timeout is running.
				if (!m_bTimeoutDisabled || m_nTimeoutSeconds == 0)
				{
					EndDialog(id);
				}
				break;
			}
		}
		break;
	}
	return ODialog::WindowProc(uMsg, wParam, lParam);
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
		// Check whether the timeout is finished.
		if (--m_nTimeoutSeconds == 0)
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
		if (m_aButtons.empty())
		{
			// Add just one button: "Ok".
			AddButton(IDOK, IDS_MESSAGEBOX_OK);
		}
		break;
	}

	// Check whether to add a help button.
	if (m_nStyle & MB_HELP)
	{
		// Add the help button.
		AddButton(IDHELP, IDS_MESSAGEBOX_HELP);
	}

	// Set the default button.
	C_ASSERT(MB_DEFBUTTON(1) == MB_DEFBUTTON1 && (MB_DEFBUTTON1 >> 8) == 0);
	C_ASSERT(MB_DEFBUTTON(2) == MB_DEFBUTTON2 && (MB_DEFBUTTON2 >> 8) == 1);
	C_ASSERT(MB_DEFBUTTON(3) == MB_DEFBUTTON3 && (MB_DEFBUTTON3 >> 8) == 2);
	C_ASSERT(MB_DEFBUTTON(4) == MB_DEFBUTTON4 && (MB_DEFBUTTON4 >> 8) == 3);
	UINT nDefaultIndex = (m_nStyle & MB_DEFMASK) >> 8;
	if (nDefaultIndex >= m_aButtons.size())
		nDefaultIndex = 0;
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
	RECT rect = { 0, 0, 0, 0 };
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
	// Create a variable with the style of the control.
	DWORD style = WS_CHILD | WS_VISIBLE | WS_GROUP | ES_MULTILINE | ES_READONLY;
	// Define the maximum width of the message.
	int const cx = MulDiv(GetSystemMetrics(SM_CXSCREEN), 2, 3);
	int const cy = MulDiv(GetSystemMetrics(SM_CYSCREEN), 2, 3);

	const UINT flags = DT_NOPREFIX | DT_WORDBREAK | DT_EXPANDTABS | DT_EDITCONTROL | DT_CALCRECT;

	RECT rect = { 0, 0, cx, 0 };
	m_edit.m_nLineHeight = pdc->DrawText(_T("%"), 1, &rect, flags);
	rect.right = rect.bottom = 0;

	String fmt;
	if (m_nStyle & MB_HIGHLIGHT_ARGUMENTS)
		LanguageSelect.LoadString(m_nHelp).swap(fmt);

	LPCTSTR f = fmt.c_str();
	LPCTSTR p = m_strMessage.c_str();
	while (p)
	{
		LPCTSTR q = _tcschr(p, _T('\n'));
		int n = static_cast<int>(q ? ++q - p : _tcslen(p));
		RECT linerect = { 0, 0, cx, 0 };
		int h = pdc->DrawText(p, n, &linerect, flags);
		if (rect.right < linerect.right)
			rect.right = linerect.right;
		rect.bottom += h;
		p = q;
		m_edit.m_aStripes.push_back(
			*f == _T('%') &&
			_tcstol(f + 1, const_cast<LPTSTR *>(&f), 10) &&
			*f == _T('\n') ? -h : h);
		if (LPCTSTR g = _tcschr(f, _T('\n')))
			f = g + 1;
	}
	if (rect.bottom > cy)
	{
		rect.bottom = cy;
		style |= WS_VSCROLL;
		rect.right += GetSystemMetrics(SM_CXVSCROLL);
	}
	// Check whether the text should be right aligned.
	if (m_nStyle & MB_RIGHT)
		style |= ES_RIGHT;
	// Create the static control for the message.
	m_edit.Subclass(HEdit::Create(style,
		nXPosition, nYPosition, rect.right, rect.bottom, m_pWnd, 0,
		m_strMessage.c_str(), m_nStyle & MB_RTLREADING ? WS_EX_RTLREADING : 0));
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
	HButton::Create(
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP | BS_AUTOCHECKBOX,
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
	SIZE sButton = { DialogUnitToPixel[CX_BUTTON].x, DialogUnitToPixel[CY_BUTTON].y };

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

	// Set the dimensions of the dialog.
	RECT rc = { 0, 0, DialogUnitToPixel[CX_BORDER].x, DialogUnitToPixel[CY_BORDER].y };

	// Check whether an icon is defined.
	LPARAM sIcon = 0;
	if (m_hIcon != NULL)
	{
		sIcon = CreateIconControl();
		rc.right += GET_X_LPARAM(sIcon) + DialogUnitToPixel[CX_BORDER].x;
	}

	LPARAM sMessage = CreateMessageControl(pdc, rc.right, rc.bottom);
	rc.bottom += GET_Y_LPARAM(sMessage);
	rc.bottom += DialogUnitToPixel[CY_BORDER].y;
	rc.bottom += DialogUnitToPixel[CY_BORDER / 2].y;

	// Check whether an checkbox is defined.
	LPARAM sCheckbox = 0;
	if (m_nStyle & (MB_DONT_DISPLAY_AGAIN | MB_DONT_ASK_AGAIN))
	{
		sCheckbox = CreateCheckboxControl(pdc, rc.right, rc.bottom);
		rc.bottom += GET_Y_LPARAM(sCheckbox);
		rc.bottom += DialogUnitToPixel[CY_BORDER].y;
	}
	rc.right += max(GET_X_LPARAM(sMessage), GET_X_LPARAM(sCheckbox));
	rc.right += DialogUnitToPixel[CX_BORDER].x;

	LPARAM sButton = ComputeButtonSize(pdc);
	// Calculate the width of the buttons.
	LONG align =
		(m_aButtons.size() - 1) * DialogUnitToPixel[CX_BUTTON_SPACE].x +
		m_aButtons.size() * GET_X_LPARAM(sButton);

	rc.right = max(rc.right, 2 * DialogUnitToPixel[CX_BORDER].x + align);
	rc.bottom = max(rc.bottom, 2 * DialogUnitToPixel[CY_BORDER].y + GET_Y_LPARAM(sIcon)); 

	align = rc.right - align;
	// Check button alignment.
	if (m_nStyle & MB_RIGHT_ALIGN)
	{
		// Right align the buttons.
		align -= DialogUnitToPixel[CX_BORDER].x;
	}
	else
	{
		// Center align the buttons.
		align /= 2;
	}

	// Run through all buttons.
	DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP;
	for (vector<MSGBOXBTN>::iterator iter = m_aButtons.begin(); iter != m_aButtons.end(); ++iter)
	{
		// Create the button.
		HButton::Create(style, align, rc.bottom,
			GET_X_LPARAM(sButton), GET_Y_LPARAM(sButton),
			m_pWnd, iter->id, iter->title.c_str());
		align += GET_X_LPARAM(sButton) + DialogUnitToPixel[CX_BUTTON_SPACE].x;
		style = WS_CHILD | WS_VISIBLE | WS_TABSTOP;
	}

	rc.bottom += GET_Y_LPARAM(sButton) + DialogUnitToPixel[CY_BORDER].y;

	ReleaseDC(pdc);

	// Calculate the window rect.
	::AdjustWindowRect(&rc, GetStyle(), FALSE);
	SetWindowPos(NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER);
	H2O::CenterWindow(m_pWnd, GetParent());
}

void CMessageBoxDialog::Edit::Subclass(HEdit *pwnd)
{
	OEdit::Subclass(pwnd);
	RECT rect;
	GetClientRect(&rect);
	SetRect(&rect);
}

LRESULT CMessageBoxDialog::Edit::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ERASEBKGND:
		if (HSurface *const pdc = reinterpret_cast<HSurface *>(wParam))
		{
			RECT rc;
			GetRect(&rc);
			rc.bottom = rc.top = -GetScrollPos(SB_VERT) * m_nLineHeight;
			COLORREF const dimmed = GetSysColor(COLOR_BTNFACE);
			COLORREF const bright = GetSysColor(COLOR_WINDOW);
			UINT const n = static_cast<UINT>(m_aStripes.size());
			for (UINT i = 0; i <= n; ++i)
			{
				int const h = i < n ? m_aStripes[i] : m_nLineHeight;
				rc.bottom += abs(h);
				if (pdc->RectVisible(&rc))
				{
					pdc->SetBkColor(h < 0 ? bright : dimmed);
					pdc->ExtTextOut(rc.left, rc.top, ETO_OPAQUE, &rc, NULL, 0, NULL);
					if (h < 0)
						pdc->DrawFocusRect(&rc);
				}
				rc.top = rc.bottom;
			}
		}
		return TRUE;
	}
	return OEdit::WindowProc(uMsg, wParam, lParam);
}
