/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997-2000  Thingamahoochie Software
//    Author: Dean Grimm
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
/////////////////////////////////////////////////////////////////////////////
/** 
 * @file  Merge.cpp
 *
 * @brief Defines the class behaviors for the application.
 *
 */
#include "stdafx.h"
#include <new.h> // _set_new_handler
#include <process.h> // _cexit
#include "Environment.h"
#include "Common/SettingStore.h"
#include "Merge.h"
#include "MainFrm.h"
#include "LogFile.h"
#include "coretools.h"
#include "paths.h"
#include "LanguageSelect.h"

// For shutdown cleanup
#include "charsets.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/**
 * @brief Remove the temp folder.
 * @param [in] path Folder to remove.
 * @return TRUE if removal succeeds, FALSE if fails.
 */
static void ClearTempfolder(LPCTSTR path)
{
	// SHFileOperation expects a ZZ terminated list of paths!
	String paths(path, static_cast<String::size_type>(_tcslen(path)) + 1);
	SHFILEOPSTRUCT fileop = { 0, FO_DELETE, paths.c_str(), 0,
		FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI, 0, 0, 0 };
	SHFileOperation(&fileop);
}

/**
 * @brief Remove orphaned temp folders.
 */
static void CleanupWMtemp()
{
	String pattern = env_GetTempPath();
	const String::size_type underline = pattern.rfind(_T('_')) + 1;
	if (underline == 0)
		return;
	pattern.replace(underline, String::npos, _T("*"));
	WIN32_FIND_DATA ff;
	HANDLE h = FindFirstFile(pattern.c_str(), &ff);
	if (h == INVALID_HANDLE_VALUE)
		return;
	do
	{
		if (ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (HANDLE hMutex = CreateMutex(NULL, TRUE, ff.cFileName))
			{
				DWORD dwMutex = GetLastError();
				if (dwMutex != ERROR_ALREADY_EXISTS)
				{
					String::size_type position = pattern.rfind(_T('\\')) + 1;
					pattern.replace(position, String::npos, ff.cFileName);
					ClearTempfolder(pattern.c_str());
				}
				CloseHandle(hMutex);
			}
		}
	} while (FindNextFile(h, &ff));
	FindClose(h);
}

// WinMerge registry settings are stored under HKEY_CURRENT_USER/Software/Thingamahoochie
// This is the name of the company of the original author (Dean Grimm)
CSettingStore SettingStore(_T("Thingamahoochie"), _T("WinMerge"));

CLanguageSelect LanguageSelect;

/////////////////////////////////////////////////////////////////////////////
// CMergeApp

/**
 * @brief custom new handler
 * extern "C" so it can be invoked from diffutils/src/util.c.
 */
extern "C" int NewHandler(size_t)
{
	OException::Throw(ERROR_OUTOFMEMORY);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CMergeApp construction

CMergeApp::CMergeApp()
: m_hInstance(NULL)
, m_pMainWnd(NULL)
, m_bNonInteractive(false)
, m_nActiveOperations(0)
{
	_set_new_handler(NewHandler);
}

CMergeApp::~CMergeApp()
{
}
/////////////////////////////////////////////////////////////////////////////
// The one and only CMergeApp object

CMergeApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CMergeApp initialization

/**
 * @brief Initialize WinMerge application instance.
 * @return TRUE if application initialization succeeds (and we'll run it),
 *   FALSE if something failed and we exit the instance.
 * @todo We could handle these failure situations more gratefully, i.e. show
 *  at least some error message to the user..
 */
HRESULT CMergeApp::InitInstance()
{
#ifndef SetDllDirectory
#error #define WINVER 0x0502 to include the definition of SetDllDirectory
#endif
	// Prevents DLL hijacking
	HMODULE hLibrary = GetModuleHandle(_T("kernel32.dll"));
	if (BOOL (WINAPI *SetSearchPathMode)(DWORD) = (BOOL (WINAPI *)(DWORD))GetProcAddress(hLibrary, "SetSearchPathMode"))
		SetSearchPathMode(0x00000001L /*BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE*/ | 0x00008000L /*BASE_SEARCH_PATH_PERMANENT*/);
	if (BOOL (WINAPI *SetDllDirectory)(LPCTSTR) = (BOOL (WINAPI *)(LPCTSTR))GetProcAddress(hLibrary, _CRT_STRINGIZE(SetDllDirectory)))
		SetDllDirectory(_T(""));

	InitCommonControls();    // initialize common control library

#ifdef _DEBUG
	// Runtime switch so programmer may set this in interactive debugger
	int dbgmem = 0;
	if (dbgmem)
	{
		// get current setting
		int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
		// Keep freed memory blocks in the heap's linked list and mark them as freed
		tmpFlag |= _CRTDBG_DELAY_FREE_MEM_DF;
		// Call _CrtCheckMemory at every allocation and deallocation request.
		// WARNING: This slows down WinMerge *A LOT*
		tmpFlag |= _CRTDBG_CHECK_ALWAYS_DF;
		// Set the new state for the flag
		_CrtSetDbgFlag( tmpFlag );
	}
#endif

	// Create exclusion mutex name
	TCHAR szDesktopName[MAX_PATH] = _T("Win9xDesktop");
	GetUserObjectInformation(GetThreadDesktop(GetCurrentThreadId()), UOI_NAME,
		szDesktopName, sizeof szDesktopName, NULL);

	TCHAR szMutexName[MAX_PATH + 40];
	// Combine window class name and desktop name to form a unique mutex name.
	// As the window class name is decorated to distinguish between ANSI and
	// UNICODE build, so will be the mutex name.
	wsprintf(szMutexName, _T("%s-%s"), WinMergeWindowClass, szDesktopName);
	const HANDLE hMutex = CreateMutex(NULL, FALSE, szMutexName);
	const DWORD dwMutex = GetLastError();

	if (hMutex)
		WaitForSingleObject(hMutex, INFINITE);

	HRESULT hr = E_FAIL;
	try
	{
		// Drag and Drop functionality needs OleInitialize
		OException::Check(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE));
		OException::Check(OleInitialize(NULL));

		// Parse command-line arguments.
		MergeCmdLineInfo cmdInfo = GetCommandLine();

		if (HKEY loadkey = SettingStore.GetAppRegistryKey())
		{
			IOptionDef::InitOptions(loadkey, NULL);
			SettingStore.RegCloseKey(loadkey);
		}

		if (int logging = COptionsMgr::Get(OPT_LOGGING))
		{
			LogFile.EnableLogging(TRUE);
			String logfile = env_GetMyDocuments();
			logfile = paths_ConcatPath(logfile, _T("WinMerge\\WinMerge.log"));
			LogFile.SetFile(logfile.c_str());
			LogFile.SetMaskLevel(logging); // See CLogFile for possible levels.
		}

		// If cmdInfo.m_invocationMode equals InvocationModeMergeTool, then
		// ClearCase waits for the process to produce an output file and
		// terminate, in which case single instance logic is not applicable.
		if (dwMutex == ERROR_ALREADY_EXISTS && cmdInfo.m_nSingleInstance != 0 &&
			(cmdInfo.m_nSingleInstance == 1 || COptionsMgr::Get(OPT_SINGLE_INSTANCE)))
		{
			// Send commandline to previous instance
			CMyComPtr<IUnknown> spUnknown;
			OException::Check(GetActiveObject(IID_IMergeApp, NULL, &spUnknown));
			CMyComPtr<IDispatch> spDispatch;
			OException::Check(spUnknown->QueryInterface(&spDispatch));
			CoAllowSetForegroundWindow(spDispatch, NULL);
			CMyDispId DispId;
			OException::Check(DispId.Init(spDispatch, L"ParseCmdLine"));
			OException::Check(DispId.Call(spDispatch, CMyDispParams<2>().Unnamed
				[GetCommandLine()][CurrentDirectory().c_str()]));

			hr = S_FALSE;
		}
		else
		{
			// Cleanup left over tempfiles from previous instances. Normally
			// this should do nothing. But if for some reason WinMerge missed
			// to delete temp files this makes sure they are removed.
			CleanupWMtemp();

			charsets_init();
			// Locate the supplement folder and read the Supplement.ini
			InitializeSupplements();

			CCrystalTextView::InitSharedResources();

			// Register the main window class. 
			const WNDCLASS wc =
			{
				CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
				::DefWindowProc, 0, 0, NULL, NULL,
				::LoadCursor(NULL, IDC_ARROW),
				::GetSysColorBrush(COLOR_BTNFACE),
				NULL, WinMergeWindowClass
			};
			::RegisterClass(&wc);

			// create main MDI Frame window
			HWindow *const pWndMainFrame = HWindow::CreateEx(
				WS_EX_ACCEPTFILES, WinMergeWindowClass, _T("WinMerge"),
				WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
				NULL, NULL);
			if (pWndMainFrame == NULL)
				OException::Throw(GetLastError());

			// subclass the window
			new CMainFrame(pWndMainFrame, cmdInfo);

			hr = S_OK;
		}
	}
	catch (OException *e)
	{
		e->ReportError(NULL);
		delete e;
	}

	if (hMutex)
		ReleaseMutex(hMutex);

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CMergeApp commands

/**
 * @brief Called when application is about to exit.
 * This functions is called when application is exiting, so this is
 * good place to do cleanups.
 * @return Application's exit value (returned from WinMain()).
 */
int CMergeApp::ExitInstance(HRESULT hr)
{
	if (hr == S_OK)
	{
		// Deallocate custom parser associations
		CCrystalTextView::FreeParserAssociations();
		CCrystalTextView::FreeSharedResources();
		charsets_term();
		// Remove tempfolder
		ClearTempfolder(env_GetTempPath());
	}
	// Stay balanced
	OleUninitialize();
	CoUninitialize();
	return FAILED(hr);
}

int CMergeApp::DoMessageBox(LPCTSTR lpszPrompt, UINT nType, UINT nIDPrompt)
{
	// This is a convenient point for breakpointing !!!
	if (m_bNonInteractive)
		return IDCANCEL;

	HWND owner = m_pMainWnd ? m_pMainWnd->GetLastActivePopup()->m_hWnd : NULL;

	// Use our own message box implementation, which adds the
	// do not show again checkbox, and implements it on subsequent calls
	// (if caller set the style)

	// Create the message box dialog.
	CMessageBoxDialog dlgMessage(lpszPrompt, _T("WinMerge"), nType, nIDPrompt);
	// Display the message box dialog and return the result.
	HKEY hKey = SettingStore.GetSectionKey(REGISTRY_SECTION_MESSAGEBOX);
	int nResult = dlgMessage.DoModal(m_hInstance, owner, hKey);
	SettingStore.RegCloseKey(hKey);
	return nResult;
}

/**
 * @brief Read various settings from Supplement.ini
 */
void CMergeApp::InitializeSupplements()
{
	String supplementFolder = COptionsMgr::Get(OPT_SUPPLEMENT_FOLDER);
	supplementFolder = env_ExpandVariables(supplementFolder.c_str());
	if (!globalFileFilter.SetUserFilterPath(supplementFolder))
	{
		supplementFolder = COptionsMgr::GetDefault(OPT_SUPPLEMENT_FOLDER);
		COptionsMgr::SaveOption(OPT_SUPPLEMENT_FOLDER, supplementFolder);
		globalFileFilter.SetUserFilterPath(supplementFolder);
	}
	SetEnvironmentVariable(_T("SupplementFolder"), supplementFolder.c_str());
	String ini = paths_ConcatPath(supplementFolder, _T("Supplement.ini"));
	TCHAR buffer[0x8000];
	// Synthesize ProgramFiles(x86) environment variable if running in 32-bit OS
	if (!GetEnvironmentVariable(_T("ProgramFiles(x86)"), buffer, _countof(buffer)))
	{
		GetEnvironmentVariable(_T("ProgramFiles"), buffer, _countof(buffer));
		SetEnvironmentVariable(_T("ProgramFiles(x86)"), buffer);
	}
	// Default values for process-level environment variables
	static const TCHAR defenv[] =
		_T("LogParser=%ProgramFiles(x86)%\\Log Parser 2.2\0")
		_T("xdoc2txt=C:\\xdoc2txt\0")
		_T("MediaInfo_CLI=C:\\MediaInfo_CLI\0");
	// First, remove any possibly conflicting variables from process environment
	memcpy(buffer, defenv, sizeof defenv);
	TCHAR *p = buffer;
	while (TCHAR *q = _tcschr(p, _T('=')))
	{
		const size_t r = _tcslen(q);
		*q++ = _T('\0');
		SetEnvironmentVariable(p, NULL);
		p = q + r;
	}
	// Second, assign environment variables provided through Supplement.ini
	// If no such exist, assign default values and write them to Supplement.ini
	if (GetPrivateProfileSection(_T("Environment"),
			buffer, _countof(buffer), ini.c_str()) ||
		WritePrivateProfileSection(_T("Environment"),
			static_cast<LPCTSTR>(memcpy(buffer, defenv, sizeof defenv)),
			ini.c_str()))
	{
		p = buffer;
		while (TCHAR *q = _tcschr(p, _T('=')))
		{
			const size_t r = _tcslen(q);
			*q++ = _T('\0');
			SetEnvironmentVariable(p, env_ExpandVariables(q).c_str());
			p = q + r;
		}
		// Assign, and write to Supplement.ini, any variables which are missing
		// from Supplement.ini, e.g. due to manual editing or a program update
		memcpy(buffer, defenv, sizeof defenv);
		p = buffer;
		while (TCHAR *q = _tcschr(p, _T('=')))
		{
			const size_t r = _tcslen(q);
			*q++ = _T('\0');
			if (!GetEnvironmentVariable(p, NULL, 0))
			{
				SetEnvironmentVariable(p, env_ExpandVariables(q).c_str());
				WritePrivateProfileString(_T("Environment"), p, q, ini.c_str());
			}
			p = q + r;
		}
	}
	// Preload any DLLs listed in Supplement.ini's [Preload] section
	if (GetPrivateProfileSection(_T("Preload"), buffer, _countof(buffer), ini.c_str()))
	{
		LPTSTR pch = buffer;
		while (const size_t cch = _tcslen(pch))
		{
			String path = env_ExpandVariables(pch);
			// Don't accept relative paths for security reasons.
			if (!PathIsRelative(path.c_str()))
				LoadLibrary(path.c_str());
			pch += cch + 1;
		}
	}
	// Create a [Parsers] section as per CCrystalTextView's default settings if not yet present
	if (GetPrivateProfileSection(_T("Parsers"), buffer, _countof(buffer), ini.c_str()))
	{
		CCrystalTextView::ScanParserAssociations(buffer);
	}
	else
	{
		CCrystalTextView::DumpParserAssociations(buffer);
		WritePrivateProfileSection(_T("Parsers"), buffer, ini.c_str());
	}
	// Create a [FileTransforms] section with some working defaults if not yet present
	if (!GetPrivateProfileSection(_T("FileTransforms"), buffer, _countof(buffer), ini.c_str()))
	{
		static const TCHAR buffer[] =
			_T("vcproj = script:\\FileTransforms\\msxml.wsc?transform='\\FileTransforms\\msxml-sort.xslt'\0")
			_T("vcxproj = script:\\FileTransforms\\msxml.wsc?transform='\\FileTransforms\\msxml-sort.xslt'\0");
		WritePrivateProfileSection(_T("FileTransforms"), buffer, ini.c_str());
	}
}

/**
 * @brief Get default editor path.
 * @return full path to the editor program executable.
 */
String CMergeApp::GetDefaultEditor()
{
	return _T("%windir%\\system32\\notepad.exe");
}

/**
 * @brief Get default supplement folder path.
 */
String CMergeApp::GetDefaultSupplementFolder()
{
	return paths_ConcatPath(env_GetMyDocuments(), WinMergeDocumentsFolder);
}

bool CMergeApp::PreTranslateMessage(MSG *pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (HWND hwndActive = ::GetActiveWindow())
		{
			if (::GetClassWord(hwndActive, GCW_ATOM) == (ATOM)WC_DIALOG)
			{
				if (::IsDialogMessage(hwndActive, pMsg))
				{
					return true;
				}
			}
		}
	}
	return m_pMainWnd && m_pMainWnd->PreTranslateMessage(pMsg);
}

LRESULT CDocFrame::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NCACTIVATE:
		if (wParam == 0)
		{
			// Window is going inactive => save current focus
			m_pWndLastFocus = HWindow::GetFocus();
			if (m_pWndLastFocus && !IsChild(m_pWndLastFocus))
				m_pWndLastFocus = NULL;
		}
		else if (HTabCtrl *pTc = m_pMDIFrame->GetTabBar())
		{
			// Window is going active => update TabBar selection
			int index = pTc->GetItemCount();
			while (index)
			{
				--index;
				TCITEM item;
				item.mask = TCIF_PARAM;
				pTc->GetItem(index, &item);
				if (item.lParam == reinterpret_cast<LPARAM>(m_hWnd))
				{
					pTc->SetCurSel(index);
				}
			}
		}
		break;
	case WM_MDIACTIVATE:
		if (reinterpret_cast<HWND>(lParam) == m_hWnd)
		{
			m_pMDIFrame->SetActiveMenu(m_pHandleSet->m_pMenuShared);
			m_pMDIFrame->UpdateDocFrameSettings(this);
		}
		else if (lParam == 0)
		{
			m_pMDIFrame->SetActiveMenu(m_pMDIFrame->m_pMenuDefault);
			WINDOWPLACEMENT wp;
			wp.length = sizeof wp;
			GetWindowPlacement(&wp);
			SettingStore.WriteProfileInt(_T("Settings"), _T("ActiveFrameMax"), wp.showCmd == SW_MAXIMIZE);
			m_pMDIFrame->UpdateDocFrameSettings(NULL);
		}
		break;
	case WM_SETFOCUS:
		if (m_pWndLastFocus == NULL)
			m_pWndLastFocus = GetNextDlgTabItem(NULL);
		m_pWndLastFocus->SetFocus();
		break;
	case WM_SETTEXT:
		if (HTabCtrl *pTc = m_pMDIFrame->GetTabBar())
		{
			int index = pTc->GetItemCount();
			while (index)
			{
				--index;
				TCITEM item;
				item.mask = TCIF_PARAM;
				pTc->GetItem(index, &item);
				if (item.lParam == reinterpret_cast<LPARAM>(m_hWnd))
				{
					item.mask = TCIF_TEXT;
					item.pszText = reinterpret_cast<LPTSTR>(lParam);
					pTc->SetItem(index, &item);
				}
			}
		}
		break;
	case WM_NCDESTROY:
		return OWindow::WindowProc(uMsg, wParam, lParam);
	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED)
		{
		case WM_CAPTURECHANGED:
			RecalcLayout();
			if (::GetCapture() == NULL)
				SavePosition();
		}
		break;
	case WM_GETMINMAXINFO:
		::DefMDIChildProc(m_hWnd, uMsg, wParam, lParam);
		MINMAXINFO *lpMMI = reinterpret_cast<MINMAXINFO *>(lParam);
		if (lpMMI->ptMaxTrackSize.x < lpMMI->ptMaxSize.x)
			lpMMI->ptMaxTrackSize.x = lpMMI->ptMaxSize.x;
		if (lpMMI->ptMaxTrackSize.y < lpMMI->ptMaxSize.y)
			lpMMI->ptMaxTrackSize.y = lpMMI->ptMaxSize.y;
		return 0;
	}
	return ::DefMDIChildProc(m_hWnd, uMsg, wParam, lParam);
}

void CDocFrame::ActivateFrame()
{
	BOOL bMaximized = FALSE;
	// get the active child frame, and a flag whether it is maximized
	if (!m_pMDIFrame->GetActiveDocFrame(&bMaximized))
	{
		// for the first frame, get the restored/maximized state from the registry
		bMaximized = SettingStore.GetProfileInt(_T("Settings"), _T("ActiveFrameMax"), FALSE);
	}
	int nCmdShow = bMaximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL;
	if (bMaximized)
	{
		// TODO: Check if the SetRedraw() is still needed.
		HWindow *const pWndMDIClient = m_pMDIFrame->m_pWndMDIClient;
		pWndMDIClient->SetRedraw(FALSE);
		ShowWindow(nCmdShow);
		BringWindowToTop();
		pWndMDIClient->SetRedraw(TRUE);
		pWndMDIClient->RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
	}
	else
	{
		ShowWindow(nCmdShow);
		BringWindowToTop();
	}
}

void CDocFrame::DestroyFrame()
{
	GetParent()->SendMessage(WM_MDIDESTROY, reinterpret_cast<WPARAM>(m_hWnd));
}

void CDocFrame::RecalcLayout()
{
	RECT rect;
	GetClientRect(&rect);
	m_wndFilePathBar.MoveWindow(rect.left, rect.top, rect.right, rect.bottom);
}

CDocFrame::CDocFrame(
	CMainFrame *pMDIFrame,
	HandleSet *pHandleSet,
	const LONG *FloatScript,
	const LONG *SplitScript
) : m_pMDIFrame(pMDIFrame),
	m_sConfigFile(SettingStore.GetFileName()),
	m_wndFilePathBar(FloatScript, SplitScript),
	m_pHandleSet(pHandleSet),
	m_pWndLastFocus(NULL)
{
	// Don't set m_bAutoDelete here because if derived constructor throws then
	// it is up to C++ to delete the instance. Instead, derived constructor is
	// responsible for setting m_bAutoDelete once instance state is mature.
	if (InterlockedIncrement(&m_pHandleSet->m_cRef) == 1)
	{
		const UINT id = m_pHandleSet->m_id;
		m_pHandleSet->m_pMenuShared = LanguageSelect.LoadMenu(id);
		m_pHandleSet->m_hAccelShared = LanguageSelect.LoadAccelerators(id);
	}
}

CDocFrame::~CDocFrame()
{
	if (m_pfnSuper)
	{
		DestroyFrame();
	}
	if (InterlockedDecrement(&m_pHandleSet->m_cRef) == 0)
	{
		m_pHandleSet->m_pMenuShared->DestroyMenu();
		m_pHandleSet->m_pMenuShared = NULL;
		m_pHandleSet->m_hAccelShared = NULL;
	}
}

void CDocFrame::EnableModeless(BOOL bEnable)
{
	m_pMDIFrame->EnableModeless(bEnable);
	HWindow *pChild = NULL;
	while ((pChild = m_pMDIFrame->m_pWndMDIClient->FindWindowEx(pChild, WinMergeWindowClass)) != NULL)
	{
		if (pChild != m_pWnd)
			pChild->EnableWindow(bEnable);
	}
}

void CDocFrame::LoadIcon(CompareStats::RESULT iImage)
{
	HICON hIcon = LanguageSelect.LoadIcon(CompareStats::m_rgIDI[iImage]);
	if (GetIcon() != hIcon)
	{
		SetIcon(hIcon);
		if (HTabCtrl *pTc = m_pMDIFrame->GetTabBar())
		{
			int index = pTc->GetItemCount();
			while (index)
			{
				--index;
				TCITEM item;
				item.mask = TCIF_PARAM;
				pTc->GetItem(index, &item);
				if (item.lParam == reinterpret_cast<LPARAM>(m_hWnd))
				{
					item.mask = TCIF_IMAGE;
					item.iImage = iImage;
					pTc->SetItem(index, &item);
				}
			}
		}
		BOOL bMaximized;
		m_pMDIFrame->GetActiveDocFrame(&bMaximized);
		// When MDI maximized the window icon is drawn on the menu bar, so we
		// need to notify it that our icon has changed.
		if (bMaximized)
		{
			m_pMDIFrame->DrawMenuBar();
		}
	}
}

int CDocFrame::OnViewSubFrame(COptionDef<int> &opt)
{
	int value = COptionsMgr::Get(opt);
	if (GetKeyState(VK_SHIFT) < 0)
		value = value ? -value : -1;
	else
		value = value ? 0 : 1;
	COptionsMgr::SaveOption(opt, value);
	return value;
}

CSubFrame::CSubFrame(CDocFrame *pDocFrame, const LONG *FloatScript, UINT uHitTestCode)
	: CFloatState(FloatScript)
	, m_pDocFrame(pDocFrame)
	, m_uHitTestCode(uHitTestCode)
{
	CFloatState::Clear();
}

LRESULT CSubFrame::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_WINDOWPOSCHANGING:
		CFloatState::Float(reinterpret_cast<WINDOWPOS *>(lParam));
		break;
	case WM_SIZE:
		if (::GetCapture() == m_hWnd)
		{
		case WM_CAPTURECHANGED:
			m_pDocFrame->PostMessage(WM_CAPTURECHANGED);
		}
		break;
	case WM_NCHITTEST:
		return m_uHitTestCode;
	case WM_NCDESTROY:
		return OWindow::WindowProc(uMsg, wParam, lParam);
	}
	return ::DefFrameProc(m_hWnd, NULL, uMsg, wParam, lParam);
}

// Get user language description of error, if available
String GetSysError(int nerr)
{
	return OException(nerr).msg;
}

// Send message to log and debug window
void LogErrorString(LPCTSTR sz)
{
	LogFile.Write(CLogFile::LERROR, sz);
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR lpCmdLine, int nCmdShow)
{
	theApp.m_hInstance = hInstance;
	HRESULT hr = theApp.InitInstance();
	if (hr == S_OK)
	{
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0))
		{
			if (theApp.PreTranslateMessage(&msg))
				continue;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	int nExitCode = theApp.ExitInstance(hr);
#ifdef _DEBUG
	_strdup("INTENTIONAL LEAK");
	_cexit();
	_CrtDumpMemoryLeaks();
	ExitProcess(nExitCode);
#endif
	return nExitCode;
}
