/*
 * Copyright (c) 2016 Jochen Neubeck
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/**
 * @file  FileOperationsDlg.cpp
 *
 * @brief Implementation of FileOperationsDlg class.
 */
#include "StdAfx.h"
#include "resource.h"
#include "paths.h"
#include "LanguageSelect.h"
#include "FileOperationsDlg.h"
#include <process.h>

/** @brief ID for timer updating UI. */
static const UINT IDT_UPDATE = 1;

/** @brief Interval (in milliseconds) for UI updates. */
static const UINT UPDATE_INTERVAL = 100;

static const UINT WM_APP_MESSAGE_BOX_DIALOG = WM_APP + 1;

static const DWORD ExclusiveFileAttributes = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_NORMAL;
static const DWORD ProtectiveFileAttributes = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN;

static LPCWSTR GetNameFromPath(LPCWSTR path)
{
	LPCWSTR last = StrRChr(path, NULL, L'\\');
	return last == NULL || *++last ? last : NULL;
}

static LPWSTR AppendNameToPath(LPWSTR path, LPCWSTR name)
{
	LPWSTR last = NULL;
	int pathlen = lstrlenW(path);
	if (pathlen > 0 && pathlen < SHRT_MAX)
	{
		if (path[--pathlen] != L'\\')
			path[++pathlen] = L'\\';
		path[++pathlen] = L'\0';
		int namelen = lstrlenW(name);
		if (pathlen + namelen <= SHRT_MAX)
			last = lstrcpyW(path + pathlen, name);
	}
	return last;
}

static void CreateDirectoryPath(LPCWSTR path)
{
	WCHAR const x = L'\\';
	LPWSTR p = PathSkipRootW(path);
	if (p <= path)
		return;
	if (LPWSTR q = StrRChrW(p, NULL, x))
	{
		*q = L'\0';
		// Initialize status to ERROR_ALREADY_EXISTS iff the opposite is true.
		DWORD status = GetFileAttributesW(path) ^ ~ERROR_ALREADY_EXISTS;
		*q = x;
		// Loop as long as failing for ERROR_ALREADY_EXISTS.
		while (status == ERROR_ALREADY_EXISTS && (q = StrChrW(p, x)) != NULL)
		{
			*q = L'\0';
			if (!CreateDirectoryW(path, NULL))
				status = GetLastError();
			*q = x;
			p = q + 1;
		}
	}
}

static void ShrinkPath(String &shrinked, LPCWSTR complete)
{
	const int DisplayPathLimit = 2600;
	int len = lstrlenW(complete);
	if (len <= DisplayPathLimit)
	{
		shrinked.assign(complete, len);
	}
	else
	{
		LPCWSTR p = complete + DisplayPathLimit / 2;
		LPCWSTR q = complete + len - DisplayPathLimit / 2;
		if (LPCWSTR r = StrRChr(complete, p, L'\\'))
			p = r + 1;
		if (LPCWSTR r = StrChr(q, L'\\'))
			q = r;
		int u = static_cast<int>(p - complete);
		int v = static_cast<int>(complete + len - q);
		shrinked.resize(u + 1 + v);
		LPWSTR buf = const_cast<LPWSTR>(shrinked.c_str());
		memcpy(buf, complete, u * sizeof *buf);
		*(buf += u) = L'*';
		memcpy(++buf, q, v * sizeof *buf);
	}
}

FileOperationsDlg::FileOperationsDlg(SHFILEOPSTRUCT &fos)
	: ODialog(IDD_FILE_OPERATIONS)
	, m_fos(fos)
	, m_count(0)
	, m_hThread(NULL)
	, m_hContinue(NULL)
	, m_hCancel(NULL)
	, m_bContinue(TRUE)
	, m_bCancel(FALSE)
	, m_retry(0)
	, m_trash_file(0)
	, m_trash_file_attentive(0)
	, m_trash_folder(0)
	, m_trash_folder_attentive(0)
	, m_dwOSVersion(GetVersion())
	, m_dwCopyFlags(LOBYTE(m_dwOSVersion) >= 6 ? COPY_FILE_NO_BUFFERING : 0)
	, m_src(NULL)
	, m_dst(NULL)
{
}

FileOperationsDlg::~FileOperationsDlg()
{
	SysFreeString(m_src);
	SysFreeString(m_dst);
}

LRESULT FileOperationsDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_APP_MESSAGE_BOX_DIALOG:
		// Post an IDCANCEL command to provide evidence that this won't cause
		// resumption of the worker thread while in WM_APP_MESSAGE_BOX_DIALOG.
		PostMessage(WM_COMMAND, IDCANCEL);
		if (CMessageBoxDialog *pDlg = reinterpret_cast<CMessageBoxDialog *>(lParam))
		{
			int ret = pDlg->DoModal(GetModuleHandle(NULL), m_hWnd, NULL);
			if (int *pRet = reinterpret_cast<int *>(wParam))
				*pRet = ret;
			// The worker thread may safely resume now.
			if (ret == IDCANCEL)
			{
				m_bCancel = TRUE;
				SetEvent(m_hCancel);
				PostMessage(WM_COMMAND, IDCANCEL);
			}
			else if (m_bContinue)
			{
				SetEvent(m_hContinue);
			}
		}
		break;
	case WM_COMMAND:
		switch (wParam)
		{
		case 0: // thread finished
		case IDCANCEL: // user canceled
			if (!IsWindowEnabled())
				break; // UI thread appears to be in WM_APP_MESSAGE_BOX_DIALOG.
			if (m_hThread)
			{
				m_bCancel = TRUE;
				SetEvent(m_hCancel);
				SetEvent(m_hContinue);
				WaitForSingleObject(m_hThread, INFINITE);
				// A nonzero exit code indicates that an exception occurred.
				DWORD code = STILL_ACTIVE;
				GetExitCodeThread(m_hThread, &code);
				if (code != 0)
					m_fos.fAnyOperationsAborted = TRUE;
				CloseHandle(m_hThread);
				m_hThread = NULL;
			}
			if (m_hContinue)
			{
				CloseHandle(m_hContinue);
				m_hContinue = NULL;
			}
			if (m_hCancel)
			{
				CloseHandle(m_hCancel);
				m_hCancel = NULL;
			}
			EndDialog(wParam);
			break;
		case IDC_PAUSE_CONTINUE:
			SetContinue(!m_bContinue);
			(m_bContinue ? SetEvent : ResetEvent)(m_hContinue);
			break;
		}
		break;
	case WM_TIMER:
		UpdateProgress();
		break;
	case WM_CTLCOLORSTATIC:
		switch (reinterpret_cast<HWindow *>(lParam)->GetDlgCtrlID())
		{
		case IDC_EDIT_DST_LOCATION:
			if (reinterpret_cast<HEdit *>(lParam)->GetModify())
				reinterpret_cast<HSurface *>(wParam)->SetTextColor(RGB(255, 0, 0));
			reinterpret_cast<HSurface *>(wParam)->SetBkColor(GetSysColor(COLOR_BTNFACE));
			return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_BTNFACE));
		}
		break;
	}
	return ODialog::WindowProc(message, wParam, lParam);
}

void FileOperationsDlg::SetContinue(BOOL bContinue)
{
	if (m_bContinue != bContinue)
	{
		std::rotate(m_strPauseContinue.begin(),
			m_strPauseContinue.begin() + m_strPauseContinue.find('\t') + 1,
			m_strPauseContinue.end());
		m_bContinue = bContinue;
	}
	SetDlgItemText(IDC_PAUSE_CONTINUE,
		m_strPauseContinue.substr(0, m_strPauseContinue.find('\t')).c_str());
}

void FileOperationsDlg::UpdateProgress()
{
	AutoLock lock(m_progress);
	if (m_progress.ItemChanged)
	{
		if (m_progress.Item < m_count)
		{
			m_pwndMessage->SetWindowText(LanguageSelect.FormatStrings(
				IDS_DIRVIEW_STATUS_FMT_FOCUS,
				NumToStr(m_progress.Item + 1).c_str(),
				NumToStr(m_count).c_str()));
		}
		m_pwndProgress->SendMessage(PBM_SETPOS, m_progress.Item);
		m_progress.ItemChanged = false;
	}
	if (m_progress.FileNameChanged)
	{
		LPCTSTR src = paths_CompactPath(m_pwndSrcLocation, m_progress.src);
		LPCTSTR dst = paths_CompactPath(m_pwndSrcLocation, m_progress.dst);
		LPCTSTR rem = paths_CompactPath(m_pwndSrcLocation, m_progress.rem, _T('~'));
		if (*rem)
			(*dst ? dst : src) = rem;
		m_pwndSrcLocation->SetWindowText(src);
		if (m_fos.wFunc != FO_DELETE)
			m_pwndDstLocation->SetWindowText(dst);
		m_progress.FileNameChanged = false;
	}
	if (m_progress.BytesChanged)
	{	
		while (m_progress.TotalFileSize.QuadPart > LONG_MAX)
		{
			m_progress.TotalFileSize.QuadPart >>= 1;
			m_progress.TotalBytesTransferred.QuadPart >>= 1;
		}
		m_pwndProgressFile->SendMessage(PBM_SETPOS, m_progress.TotalBytesTransferred.LowPart);
		m_pwndProgressFile->SendMessage(PBM_SETRANGE32, 0, m_progress.TotalFileSize.LowPart);
		m_progress.BytesChanged = false;
	}
}

BOOL FileOperationsDlg::OnInitDialog() 
{
	ODialog::OnInitDialog();
	LanguageSelect.TranslateDialog(m_hWnd);

	GetDlgItemText(IDC_PAUSE_CONTINUE, m_strPauseContinue);

	m_pwndMessage = GetDlgItem(IDC_MESSAGE);
	m_pwndProgress = GetDlgItem(IDC_PROGRESS);
	m_pwndProgressFile = GetDlgItem(IDC_PROGRESS_FILE);
	m_pwndSrcLocation = static_cast<HEdit *>(GetDlgItem(IDC_EDIT_SRC_LOCATION));
	m_pwndDstLocation = static_cast<HEdit *>(GetDlgItem(IDC_EDIT_DST_LOCATION));
	if (LPCTSTR pFrom = m_fos.pFrom)
	{
		while (size_t len = _tcslen(pFrom))
		{
			++m_count;
			pFrom += len + 1;
		}
	}
	m_pwndProgress->SendMessage(PBM_SETRANGE32, 0, m_count);
	m_progress.ItemChanged = true;

	LPTHREAD_START_ROUTINE proc = NULL;
	UINT idTitle = 0;
	BOOL bContinue = TRUE;
	switch (m_fos.wFunc)
	{
	case FO_MOVE:
		proc = OException::ThreadProc<FileOperationsDlg, &FileOperationsDlg::MoveThread>;
		idTitle = IDS_STATUS_MOVEFILES;
		break;
	case FO_COPY:
		proc = OException::ThreadProc<FileOperationsDlg, &FileOperationsDlg::CopyThread>;
		idTitle = IDS_STATUS_COPYFILES;
		break;
	case FO_DELETE:
		proc = OException::ThreadProc<FileOperationsDlg, &FileOperationsDlg::DeleteThread>;
		idTitle = IDS_STATUS_DELETEFILES;
		bContinue = FALSE;
		m_trash_file = IDYESTOALL;
		m_trash_file_attentive = IDYESTOALL;
		m_trash_folder = IDYESTOALL;
		m_trash_folder_attentive = IDYESTOALL;
		RECT rc[3];
		m_pwndDstLocation->GetWindowRect(rc + 1);
		m_pwndProgressFile->GetWindowRect(rc + 2);
		m_pwndProgressFile->ShowWindow(SW_HIDE);
		UnionRect(rc, rc + 1, rc + 2);
		ScreenToClient(rc);
		m_pwndDstLocation->SetExStyle(0);
		m_pwndDstLocation->SetWindowPos(NULL,
			rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top,
			SWP_NOZORDER | SWP_FRAMECHANGED);
		m_pwndDstLocation->GetClientRect(rc);
		m_pwndDstLocation->SetRect(rc);
		m_pwndDstLocation->SetWindowText(
			LanguageSelect.LoadString(IDS_TRASH_SELECTED_ITEMS).c_str());
		m_pwndDstLocation->SetModify(TRUE);
		break;
	}
	SetContinue(bContinue);
	if (proc)
	{
		SetWindowText(LanguageSelect.LoadString(idTitle).c_str());
		ShowWindow(SW_SHOW);
		m_hContinue = CreateEvent(NULL, TRUE, bContinue, NULL);
		m_hCancel = CreateEvent(NULL, TRUE, FALSE, NULL);
		m_hThread = BeginThreadEx(NULL, 0, proc, this, 0, NULL);
		SetTimer(IDT_UPDATE, UPDATE_INTERVAL);
	}
	return TRUE;
}

int FileOperationsDlg::Invoke(CMessageBoxDialog &dlg)
{
	ResetEvent(m_hContinue);
	int ret = 0;
	PostMessage(WM_APP_MESSAGE_BOX_DIALOG, reinterpret_cast<WPARAM>(&ret), reinterpret_cast<LPARAM>(&dlg));
	HANDLE const handles[] = { m_hContinue, m_hCancel };
	WaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE);
	return ret;
}

BOOL FileOperationsDlg::WantRetry(int &choice, UINT fmt, LPCWSTR src, LPCWSTR dst)
{
	if (choice != IDIGNOREALL)
	{
		OException what = GetLastError();
		src = paths_UndoMagic(wcsdupa(src));
		dst = paths_UndoMagic(wcsdupa(dst));
		CMessageBoxDialog dlg(
			LanguageSelect.FormatStrings(fmt, src, dst, what.msg),
			MB_HIGHLIGHT_ARGUMENTS | MB_ICONEXCLAMATION, fmt);
		dlg.AddButton(IDRETRY, IDS_MESSAGEBOX_RETRY);
		dlg.AddButton(IDIGNORE, IDS_MESSAGEBOX_IGNORE);
		dlg.AddButton(IDIGNOREALL, IDS_MESSAGEBOX_IGNOREALL);
		dlg.AddButton(IDCANCEL, IDS_MESSAGEBOX_CANCEL);
		choice = Invoke(dlg);
	}
	if (choice == IDRETRY)
		return TRUE;
	m_fos.fAnyOperationsAborted = TRUE;
	return FALSE;
}

BOOL FileOperationsDlg::WantTrash(int &choice, UINT fmt, LPCWSTR src, LPCWSTR dst)
{
	if (m_bCancel)
		return FALSE;
	if (choice != IDYESTOALL && choice != IDNOTOALL)
	{
		src = paths_UndoMagic(wcsdupa(src));
		dst = paths_UndoMagic(wcsdupa(dst));
		CMessageBoxDialog dlg(
			LanguageSelect.FormatStrings(fmt, src, dst),
			MB_HIGHLIGHT_ARGUMENTS | MB_ICONEXCLAMATION, fmt);
		dlg.AddButton(IDYES, IDS_MESSAGEBOX_YES);
		dlg.AddButton(IDYESTOALL, IDS_MESSAGEBOX_YES_TO_ALL);
		dlg.AddButton(IDNO, IDS_MESSAGEBOX_NO);
		dlg.AddButton(IDNOTOALL, IDS_MESSAGEBOX_NO_TO_ALL);
		dlg.AddButton(IDCANCEL, IDS_MESSAGEBOX_CANCEL);
		choice = Invoke(dlg);
	}
	if (choice == IDYES || choice == IDYESTOALL)
		return TRUE;
	m_fos.fAnyOperationsAborted = TRUE;
	return FALSE;
}

LPWSTR FileOperationsDlg::AssignPath(BSTR &p, LPCWSTR q)
{
	if (!SysReAllocStringLen(&p, NULL, SHRT_MAX))
		OException::Throw(ERROR_OUTOFMEMORY);
	if (p != q)
		lstrcpynW(p, q, SHRT_MAX + 1);
	return p;
}

DWORD FileOperationsDlg::ProgressRoutine(
	LARGE_INTEGER TotalFileSize, LARGE_INTEGER TotalBytesTransferred,
	LARGE_INTEGER StreamSize, LARGE_INTEGER StreamBytesTransferred,
	DWORD, DWORD dwCallbackReason, HANDLE, HANDLE, LPVOID pv)
{
	FileOperationsDlg *const p = static_cast<FileOperationsDlg *>(pv);
	AutoWait wait(p->m_hContinue);
	AutoLock lock(p->m_progress);
	p->m_progress.TotalFileSize.QuadPart = TotalFileSize.QuadPart;
	p->m_progress.TotalBytesTransferred.QuadPart = TotalBytesTransferred.QuadPart;
	p->m_progress.BytesChanged = true;
	return PROGRESS_CONTINUE;
}

void FileOperationsDlg::Item(int nItem)
{
	AutoLock lock(m_progress);
	m_progress.Item = nItem;
	m_progress.ItemChanged = true;
}

void FileOperationsDlg::FromTo(LPCWSTR src, LPCWSTR dst)
{
	AutoWait wait(m_hContinue);
	AutoLock lock(m_progress);
	ShrinkPath(m_progress.src, src);
	ShrinkPath(m_progress.dst, dst);
	m_progress.FileNameChanged = true;
}

void FileOperationsDlg::Removing(LPCWSTR rem)
{
	AutoWait wait(m_hContinue);
	AutoLock lock(m_progress);
	ShrinkPath(m_progress.rem, rem);
	m_progress.FileNameChanged = true;
}

LPCWSTR FileOperationsDlg::ToWhere(LPCWSTR pFrom, LPCWSTR pTo)
{
	if (!GetNameFromPath(pTo) && (pFrom = GetNameFromPath(pFrom)) != NULL)
	{
		LPWSTR pToWhere = AssignPath(m_dst, pTo);
		pTo = AppendNameToPath(pToWhere, pFrom) ? pToWhere : NULL;
	}
	return pTo;
}

LPVOID FileOperationsDlg::ClearTarget(LPCWSTR src, LPWSTR dst)
{
	if (LPWSTR dstname = AppendNameToPath(dst, L"*.*"))
	{
		WIN32_FIND_DATAW fd;
		HANDLE h = FindFirstFileW(dst, &fd);
		*--dstname = L'\0';
		if (h != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (fd.cFileName[0] && (fd.cFileName[0] != L'.' ||
					fd.cFileName[1] && (fd.cFileName[1] != L'.' ||
					fd.cFileName[2])))
				{
					if (src) // we have not yet asked the user
					{
						if (!WantTrash(m_trash_folder_attentive, IDS_TRASH_FOLDER_ATTENTIVE, src, dst))
						{
							src = dst = NULL; // keep your hands off and don't ask again
							break;
						}
						src = NULL; // don't ask again
					}
					if (LPWSTR dstname = AppendNameToPath(dst, fd.cFileName))
					{
						if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
						{
							ClearTarget(src, dst);
							if (fd.dwFileAttributes & ProtectiveFileAttributes)
								SetFileAttributesW(dst, FILE_ATTRIBUTE_DIRECTORY);
							RemoveDirectoryW(dst);
						}
						else
						{
							Removing(dst);
							if (fd.dwFileAttributes & ProtectiveFileAttributes)
								SetFileAttributesW(dst, FILE_ATTRIBUTE_NORMAL);
							DeleteFileW(dst);
						}
						*--dstname = L'\0';
					}
				}
			} while (!m_bCancel && FindNextFileW(h, &fd));
			FindClose(h);
			if (src) // we have not yet asked the user
			{
				if (!WantTrash(m_trash_folder, IDS_TRASH_FOLDER, src, dst))
				{
					dst = NULL; // keep your hands off
				}
			}
		}
	}
	Removing(dst);
	return dst;
}

void FileOperationsDlg::MoveItem(LPCWSTR src, LPCWSTR dst)
{
	do
	{
		DWORD const attr = GetFileAttributesW(dst);
		if (attr != INVALID_FILE_ATTRIBUTES)
		{
			if (attr & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (!ClearTarget(src, AssignPath(m_dst, dst)))
					return;
				if (attr & ProtectiveFileAttributes)
					SetFileAttributesW(dst, FILE_ATTRIBUTE_DIRECTORY);
				RemoveDirectoryW(dst);
				Removing(NULL);
			}
			else
			{
				if (!WantTrash(m_trash_file, IDS_TRASH_FILE, src, dst))
					return;
				if (attr & ProtectiveFileAttributes)
					SetFileAttributesW(dst, FILE_ATTRIBUTE_NORMAL);
				DeleteFileW(dst);
			}
		}
		if (MoveFileWithProgressW(src, dst, &ProgressRoutine, this, MOVEFILE_COPY_ALLOWED))
			return;
	} while (!m_bCancel && WantRetry(m_retry, IDS_ERROR_FILEMOVE, src, dst));
}

void FileOperationsDlg::CopyLeaf(LPCWSTR src, LPCWSTR dst)
{
	do
	{
		DWORD const attr = GetFileAttributesW(dst);
		if (attr != INVALID_FILE_ATTRIBUTES)
		{
			if (attr & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (!ClearTarget(src, AssignPath(m_dst, dst)))
					return;
				if (attr & ProtectiveFileAttributes)
					SetFileAttributesW(dst, FILE_ATTRIBUTE_DIRECTORY);
				RemoveDirectoryW(dst);
				Removing(NULL);
			}
			else if (attr & ProtectiveFileAttributes)
			{
				if (!WantTrash(m_trash_file_attentive, IDS_TRASH_FILE_ATTENTIVE, src, dst))
					return;
				SetFileAttributesW(dst, FILE_ATTRIBUTE_NORMAL);
			}
		}
		if (CopyFileExW(src, dst, &ProgressRoutine, this, const_cast<BOOL *>(&m_bCancel), m_dwCopyFlags))
			return;
	} while (!m_bCancel && WantRetry(m_retry, IDS_ERROR_FILECOPY, src, dst));
}

void FileOperationsDlg::CopyTree(LPWSTR src, LPWSTR dst)
{
	DWORD const attr = GetFileAttributesW(dst);
	if ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0)
	{
		if (attr & ProtectiveFileAttributes)
		{
			if (!WantTrash(m_trash_file_attentive, IDS_TRASH_FILE_ATTENTIVE, src, dst))
				return;
			SetFileAttributesW(dst, FILE_ATTRIBUTE_NORMAL);
		}
		else
		{
			if (!WantTrash(m_trash_file, IDS_TRASH_FILE, src, dst))
				return;
		}
		DeleteFileW(dst);
	}
	CreateDirectoryW(dst, NULL);
	if (LPWSTR srcname = AppendNameToPath(src, L"*.*"))
	{
		WIN32_FIND_DATAW fd;
		HANDLE h = FindFirstFileW(src, &fd);
		*--srcname = L'\0';
		if (h != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (fd.cFileName[0] && (fd.cFileName[0] != L'.' ||
					fd.cFileName[1] && (fd.cFileName[1] != L'.' ||
					fd.cFileName[2])))
				{
					if (LPWSTR srcname = AppendNameToPath(src, fd.cFileName))
					{
						if (LPWSTR dstname = AppendNameToPath(dst, fd.cFileName))
						{
							FromTo(src, dst);
							if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
								CopyTree(src, dst);
							else
								CopyLeaf(src, dst);
							*--dstname = L'\0';
						}
						*--srcname = L'\0';
					}
				}
			} while (!m_bCancel && FindNextFileW(h, &fd));
			FindClose(h);
		}
	}
}

void FileOperationsDlg::DeleteItem(LPCWSTR src)
{
	do
	{
		DWORD const attr = GetFileAttributesW(src);
		BOOL bDone = FALSE;
		if (attr != INVALID_FILE_ATTRIBUTES)
		{
			if (attr & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (!ClearTarget(src, AssignPath(m_src, src)))
					return;
				if (attr & ProtectiveFileAttributes)
					SetFileAttributesW(src, FILE_ATTRIBUTE_DIRECTORY);
				bDone = RemoveDirectoryW(src);
			}
			else
			{
				Removing(src);
				if (attr & ProtectiveFileAttributes)
				{
					if (!WantTrash(m_trash_file_attentive, IDS_TRASH_FILE_ATTENTIVE, src, src))
						return;
					SetFileAttributesW(src, FILE_ATTRIBUTE_NORMAL);
				}
				else
				{
					if (!WantTrash(m_trash_file, IDS_TRASH_FILE, src, src))
						return;
				}
				bDone = DeleteFileW(src);
			}
		}
		if (bDone)
			return;
	} while (!m_bCancel && WantRetry(m_retry, IDS_ERROR_FILEDELETE, src, src));
}

DWORD FileOperationsDlg::MoveThread()
{
	LPCWSTR pFrom = m_fos.pFrom;
	LPCWSTR pTo = m_fos.pTo;
	if (pFrom && pTo)
	{
		int nItem = 0;
		while (!m_bCancel && *pFrom)
		{
			FromTo(pFrom, pTo);
			if (LPCWSTR pToWhere = ToWhere(pFrom, pTo))
			{
				CreateDirectoryPath(pToWhere);
				MoveItem(pFrom, pToWhere);
			}
			Item(++nItem);
			pFrom += lstrlenW(pFrom) + 1;
			if (m_fos.fFlags & FOF_MULTIDESTFILES)
				pTo += lstrlenW(pTo) + 1;
		}
	}
	if (!m_bCancel)
		PostMessage(WM_COMMAND);
	return 0;
}

DWORD FileOperationsDlg::CopyThread()
{
	LPCWSTR pFrom = m_fos.pFrom;
	LPCWSTR pTo = m_fos.pTo;
	if (pFrom && pTo)
	{
		int nItem = 0;
		while (!m_bCancel && *pFrom)
		{
			FromTo(pFrom, pTo);
			if (LPCWSTR pToWhere = ToWhere(pFrom, pTo))
			{
				DWORD attr = GetFileAttributesW(pFrom);
				if ((attr & ExclusiveFileAttributes) == FILE_ATTRIBUTE_DIRECTORY)
				{
					CopyTree(AssignPath(m_src, pFrom), AssignPath(m_dst, pToWhere));
				}
				else
				{
					CreateDirectoryPath(pToWhere);
					CopyLeaf(pFrom, pToWhere);
				}
			}
			Item(++nItem);
			pFrom += lstrlenW(pFrom) + 1;
			if (m_fos.fFlags & FOF_MULTIDESTFILES)
				pTo += lstrlenW(pTo) + 1;
		}
	}
	if (!m_bCancel)
		PostMessage(WM_COMMAND);
	return 0;
}

DWORD FileOperationsDlg::DeleteThread()
{
	LPCWSTR pFrom = m_fos.pFrom;
	if (pFrom)
	{
		int nItem = 0;
		while (!m_bCancel && *pFrom)
		{
			DeleteItem(pFrom);
			Item(++nItem);
			pFrom += lstrlenW(pFrom) + 1;
		}
	}
	if (!m_bCancel)
		PostMessage(WM_COMMAND);
	return 0;
}
