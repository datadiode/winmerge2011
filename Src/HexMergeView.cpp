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
 * @file  HexMergeView.cpp
 *
 * @brief Implementation file for CHexMergeDoc
 *
 */
#include "stdafx.h"
#include "Merge.h"
#include "MainFrm.h"
#include "HexMergeFrm.h"
#include "HexMergeView.h"
#include "LanguageSelect.h"
#include "paths.h"
#include "Environment.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/**
 * @brief Turn bool api result into success/error code
 */
static HRESULT NTAPI SE(BOOL f)
{
	if (f)
		return S_OK;
	HRESULT hr = (HRESULT)::GetLastError();
	ASSERT(hr);
	if (hr == 0)
		hr = E_UNEXPECTED;
	return hr;
}

static UINT64 NTAPI GetLastWriteTime(HANDLE h)
{
	UINT64 ft;
	return ::GetFileTime(h, 0, 0, reinterpret_cast<FILETIME *>(&ft)) ? ft : 0;
}

static void NTAPI SetLastWriteTime(HANDLE h, UINT64 ft)
{
	::SetFileTime(h, 0, 0, reinterpret_cast<FILETIME *>(&ft));
}

static int nCBCodeLast = SB_ENDSCROLL;

static void SynchronizeScrollPos(HWindow *pSender, int nBar)
{
	int nPos = pSender->GetScrollPos(nBar);
	LPCTSTR pcwAtom = MAKEINTATOM(pSender->GetClassAtom());
	HWindow *pParent = pSender->GetParent();
	HWindow *pChild = NULL;
	while ((pChild = pParent->FindWindowEx(pChild, pcwAtom)) != NULL)
	{
		if (pChild == pSender)
			continue;
		pChild->SetScrollPos(nBar, nPos);
		pChild->SendMessage(WM_HSCROLL + nBar, MAKEWPARAM(SB_THUMBTRACK, nPos), (LPARAM)pSender);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CHexMergeView

LRESULT CHexMergeView::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_SETFOCUS:
		m_pDocument->m_wndFilePathBar.SetActive(m_nThisPane, true);
		break;
	case WM_KILLFOCUS:
		m_pDocument->m_wndFilePathBar.SetActive(m_nThisPane, false);
		break;
	case WM_HSCROLL:
		OWindow::WindowProc(uMsg, wParam, lParam);
		if (lParam == 0)
		{
			SynchronizeScrollPos(reinterpret_cast<HWindow *>(m_hWnd), SB_HORZ);
			nCBCodeLast = LOWORD(wParam);
		}
		return 0;
	case WM_VSCROLL:
		OWindow::WindowProc(uMsg, wParam, lParam);
		if (lParam == 0)
		{
			nCBCodeLast = LOWORD(wParam);
			SynchronizeScrollPos(reinterpret_cast<HWindow *>(m_hWnd), SB_VERT);
		}
		return 0;
	case WM_LBUTTONDOWN:
		SetFocus();
		break;
	case WM_WINDOWPOSCHANGED:
		if ((reinterpret_cast<WINDOWPOS *>(lParam)->flags & SWP_NOSIZE) == 0)
			OnSize();
		break;
	case WM_ERASEBKGND:
		OnEraseBkgnd();
		break;
	case WM_PAINT:
		PostMessage(WM_APP);
		break;
	case WM_APP:
		if (GetModified())
		{
			if (!m_pDocument->m_wndFilePathBar.GetModify(m_nThisPane))
			{
				m_pDocument->m_wndFilePathBar.SetModify(m_nThisPane, TRUE);
				m_pDocument->SetTitle();
			}
			m_pDocument->UpdateCmdUI();
		}
		break;
	}
	return OWindow::WindowProc(uMsg, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////
// CHexMergeView construction/destruction

/**
 * @brief Constructor.
 */
CHexMergeView::CHexMergeView(CHexMergeFrame *pDocument, int nThisPane)
: m_pif(NULL)
, m_nThisPane(nThisPane)
, m_pDocument(pDocument)
, m_mtime(0)
, m_pStatusBar(NULL)
, m_style(0)
{
	m_bAutoDelete = true;
}

/**
 * @brief Load heksedit.dll and setup window class name
 */
LPCTSTR CHexMergeView::RegisterClass()
{
	static void *pv = NULL;
	if (pv == NULL)
	{
		static const CLSID clsid = { 0xBCA3CA6B, 0xCC6B, 0x4F79,
			{ 0xA2, 0xC2, 0xDD, 0xBE, 0x86, 0x4B, 0x1C, 0x90 } };
		if (FAILED(::CoGetClassObject(clsid, CLSCTX_INPROC_SERVER, NULL, IID_IUnknown, &pv)))
			pv = LoadLibrary(_T("Frhed\\hekseditU.dll"));
		WNDCLASS wc;
		if (!::GetClassInfo(NULL, _T("heksedit"), &wc))
			OException::Throw(GetLastError());
		wc.cbWndExtra = sizeof(CHexMergeView *);
		wc.lpszClassName = _T("HexMergeView");
		::RegisterClass(&wc);
	}
	return _T("HexMergeView");
}

/**
 * @brief Prevent side effects on scroll bar visibility
 */
void CHexMergeView::OnSize()
{
	// Undo any side effects on scroll bar visibility
	ShowScrollBar(SB_HORZ, (m_style & WS_HSCROLL) != 0);
	ShowScrollBar(SB_VERT, (m_style & WS_VSCROLL) != 0);
	// Adjust status bar panels
	if (HStatusBar *const pBar = m_pStatusBar)
	{
		RECT rect;
		GetWindowRect(&rect);
		rect.right -= rect.left;
		if (pBar->GetStyle() & SBS_SIZEGRIP)
			rect.right -= ::GetSystemMetrics(SM_CXVSCROLL);
		int parts[3];
		parts[2] = rect.right;
		rect.right -= 80;
		parts[1] = rect.right;
		rect.right -= 90;
		parts[0] = rect.right;
		pBar->SetParts(3, parts);
	}
	m_pDocument->RecalcBytesPerLine();
}

void CHexMergeView::OnEraseBkgnd()
{
	if (nCBCodeLast == SB_ENDSCROLL)
	{
		SynchronizeScrollPos(reinterpret_cast<HWindow *>(m_hWnd), SB_VERT);
		SynchronizeScrollPos(reinterpret_cast<HWindow *>(m_hWnd), SB_HORZ);
		nCBCodeLast = SB_ENDSCROLL;
	}
}

/**
 * @brief Get pointer to control's content buffer
 */
BYTE *CHexMergeView::GetBuffer(int length)
{
	return m_pif->get_buffer(length);
}

/**
 * @brief Get length of control's content buffer
 */
int CHexMergeView::GetLength()
{
	return m_pif->get_length();
}

/**
 * @brief Checks if file has changed since last update
 * @param [in] path File to check
 * @return TRUE if file is changed.
 */
BOOL CHexMergeView::IsFileChangedOnDisk(LPCTSTR path)
{
	// NB: FileTimes are measured in 100 nanosecond intervals since 1601-01-01.
	BOOL bChanged = FALSE;
	HANDLE h = CreateFile(path, FILE_READ_ATTRIBUTES, FILE_SHARE_READ|FILE_SHARE_WRITE,
		0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (h != INVALID_HANDLE_VALUE)
	{
		UINT64 mtime = GetLastWriteTime(h);
		UINT64 lower = min(mtime, m_mtime);
		UINT64 upper = max(mtime, m_mtime);
		bool bIgnoreSmallDiff = COptionsMgr::Get(OPT_IGNORE_SMALL_FILETIME);
		UINT64 tolerance = bIgnoreSmallDiff ? SmallTimeDiff * FileTime::TicksPerSecond : 0;
		bChanged = upper - lower > tolerance || m_size != GetFileSize(h, 0);
		CloseHandle(h);
	}
	return bChanged;
}

/**
 * @brief Load file
 */
HRESULT CHexMergeView::LoadFile(LPCTSTR path)
{
	HANDLE h = CreateFile(path, GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	HRESULT hr = SE(h != INVALID_HANDLE_VALUE);
	if (hr != S_OK)
		return hr;
	m_mtime = GetLastWriteTime(h);
	DWORD length = m_size = GetFileSize(h, 0);
	hr = SE(length != INVALID_FILE_SIZE);
	if (hr == S_OK)
	{
		if (void *buffer = GetBuffer(length))
		{
			DWORD cb = 0;
			hr = SE(ReadFile(h, buffer, length, &cb, 0) && cb == length);
			if (hr != S_OK)
				GetBuffer(0);
		}
		else if (length != 0)
		{
			hr = E_OUTOFMEMORY;
		}
	}
	CloseHandle(h);
	return hr;
}

/**
 * @brief Save file
 */
HRESULT CHexMergeView::SaveFile(LPCTSTR path)
{
	// Warn user in case file has been changed by someone else
	if (IsFileChangedOnDisk(path))
	{
		int response = LanguageSelect.FormatStrings(
			IDS_FILECHANGED_ONDISK, paths_UndoMagic(wcsdupa(path))
		).MsgBox(MB_HIGHLIGHT_ARGUMENTS | MB_ICONWARNING | MB_YESNO);
		if (response == IDNO)
			return S_OK;
	}
	// Ask user what to do about FILE_ATTRIBUTE_READONLY
	String strPath = path;
	if (DWORD attr = paths_IsReadonlyFile(strPath.c_str()))
		if (theApp.m_pMainWnd->HandleReadonlySave(attr, strPath) == IDCANCEL)
			return S_OK;
	// Take a chance to create a backup
	if (!theApp.m_pMainWnd->CreateBackup(false, path))
		return S_OK;
	// Write data to an intermediate file
	String sIntermediateFilename = env_GetTempFileName(env_GetTempPath(), _T("MRG_"));
	if (sIntermediateFilename.empty())
		return E_FAIL; //Nothing to do if even tempfile name fails
	HANDLE h = CreateFile(sIntermediateFilename.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
		0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	HRESULT hr = SE(h != INVALID_HANDLE_VALUE);
	if (hr != S_OK)
		return hr;
	DWORD length = GetLength();
	void *buffer = GetBuffer(length);
	if (buffer == 0)
		return E_POINTER;
	DWORD cb = 0;
	hr = SE(WriteFile(h, buffer, length, &cb, 0) && cb == length);
	UINT64 mtime = GetLastWriteTime(h);
	CloseHandle(h);
	if (hr != S_OK)
		return hr;
	hr = SE(CopyFile(sIntermediateFilename.c_str(), strPath.c_str(), FALSE));
	if (hr != S_OK)
		return hr;
	m_mtime = mtime;
	m_size = length;
	SetModified(FALSE);
	hr = SE(DeleteFile(sIntermediateFilename.c_str()));
	if (hr != S_OK)
	{
		LogErrorString(_T("DeleteFile(%s) failed: %s"),
			sIntermediateFilename.c_str(), GetSysError(hr).c_str());
	}
	return S_OK;
}

/**
 * @brief Get status
 */
IHexEditorWindow::Status *CHexMergeView::GetStatus()
{
	return m_pif->get_status();
}

/**
 * @brief Get modified flag
 */
BOOL CHexMergeView::GetModified()
{
	return m_pif->get_status()->iFileChanged;
}

/**
 * @brief Set modified flag
 */
void CHexMergeView::SetModified(BOOL bModified)
{
	m_pif->get_status()->iFileChanged = bModified;
}

/**
 * @brief Get readonly flag
 */
BOOL CHexMergeView::GetReadOnly()
{
	return m_pif->get_settings()->bReadOnly;
}

/**
 * @brief Set readonly flag
 */
void CHexMergeView::SetReadOnly(BOOL bReadOnly)
{
	m_pif->get_settings()->bReadOnly = bReadOnly;
}

/**
 * @brief Allow the control to update all kinds of things that need to be updated when
 * the window or content buffer have been resized or certain settings have been changed.
 */
void CHexMergeView::ResizeWindow()
{
	m_pif->resize_window();
}

/**
 * @brief Repaint a range of bytes
 */
void CHexMergeView::RepaintRange(int i, int j)
{
	int iBytesPerLine = m_pif->get_settings()->iBytesPerLine;
	m_pif->repaint(i / iBytesPerLine, j / iBytesPerLine);
}

/**
 * @brief Find a sequence of bytes
 */
void CHexMergeView::OnEditFind()
{
	m_pif->CMD_find();
}

/**
 * @brief Find & replace a sequence of bytes
 */
void CHexMergeView::OnEditReplace()
{
	m_pif->CMD_replace();
}

/**
 * @brief Repeat last find in one or another direction
 */
void CHexMergeView::OnEditRepeat()
{
	if (GetKeyState(VK_SHIFT) < 0)
		m_pif->CMD_findprev();
	else
		m_pif->CMD_findnext();
}

/**
 * @brief Cut selected content
 */
void CHexMergeView::OnEditCut()
{
	m_pif->CMD_edit_cut();
}

/**
 * @brief Copy selected content
 */
void CHexMergeView::OnEditCopy()
{
	m_pif->CMD_edit_copy();
}

/**
 * @brief Paste clipboard content over selected content
 */
void CHexMergeView::OnEditPaste()
{
	m_pif->CMD_edit_paste();
}

/**
 * @brief Select entire content
 */
void CHexMergeView::OnEditSelectAll()
{
	m_pif->CMD_select_all();
}

/**
 * @brief Clear selected content
 */
void CHexMergeView::OnEditClear()
{
	m_pif->CMD_edit_clear();
}

/**
 * @brief Check for keyboard commands
 */
BOOL CHexMergeView::PreTranslateMessage(MSG* pMsg)
{
	const int length = m_pif->get_length();
	if (!m_pif->translate_accelerator(pMsg))
		return FALSE;
	if (length != m_pif->get_length())
		m_pif->resize_window();
	return TRUE;
}

/**
 * @brief Go to first diff
 */
void CHexMergeView::OnFirstdiff()
{
	m_pif->select_next_diff(TRUE);
}

/**
 * @brief Go to last diff
 */
void CHexMergeView::OnLastdiff()
{
	m_pif->select_prev_diff(TRUE);
}

/**
 * @brief Go to next diff
 */
void CHexMergeView::OnNextdiff()
{
	m_pif->select_next_diff(FALSE);
}

/**
 * @brief Go to previous diff
 */
void CHexMergeView::OnPrevdiff()
{
	m_pif->select_prev_diff(FALSE);
}

void CHexMergeView::ZoomText(int amount)
{
	m_pif->CMD_zoom(amount);
}
