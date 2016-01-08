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
 * @file  FileOperationsDlg.h
 *
 * @brief Declaration file for FileOperationsDlg class.
 */
#pragma once

class FileOperationsDlg : public ODialog
{
public:
	FileOperationsDlg(SHFILEOPSTRUCT &);
	~FileOperationsDlg();

private:
	SHFILEOPSTRUCT &m_fos;
	int m_count;
	String m_strPauseContinue;
	BOOL m_bContinue;
	BOOL volatile m_bCancel;
	int m_retry; // whether to retry a failed operation
	int m_trash_file; // whether to trash an already existing file
	int m_trash_file_attentive; // whether to trash a read-only or hidden file
	int m_trash_folder; // whether to trash an already existing folder
	int m_trash_folder_attentive; // whether to trash a nonempty folder
	HWindow *m_pwndMessage;
	HWindow *m_pwndProgress;
	HWindow *m_pwndProgressFile;
	HEdit *m_pwndSrcLocation;
	HEdit *m_pwndDstLocation;
	HANDLE m_hThread;
	HANDLE m_hContinue;
	HANDLE m_hCancel;
	DWORD const m_dwOSVersion;
	DWORD const m_dwCopyFlags;
	BSTR m_src;
	BSTR m_dst;

	class Progress
		: ZeroInit<Progress>
		, CRITICAL_SECTION
	{
	public:
		Progress() { InitializeCriticalSection(this); }
		~Progress() { DeleteCriticalSection(this); }
		operator CRITICAL_SECTION *() { return this; }
		bool FileNameChanged;
		bool ItemChanged;
		bool BytesChanged;
		String src;
		String dst;
		String rem;
		int Item;
		LARGE_INTEGER TotalFileSize;
		LARGE_INTEGER TotalBytesTransferred;
	} m_progress;

	class AutoLock
	{
		CRITICAL_SECTION *const p;
	public:
		AutoLock(CRITICAL_SECTION *p): p(p) { EnterCriticalSection(p); }
		~AutoLock() { LeaveCriticalSection(p); }
	};

	class AutoWait
	{
		HANDLE const h;
	public:
		AutoWait(HANDLE h): h(h) { }
		~AutoWait() { WaitForSingleObject(h, INFINITE); }
	};

	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	void SetContinue(BOOL);
	void UpdateProgress();
	int Invoke(CMessageBoxDialog &);
	BOOL WantRetry(int &, UINT, LPCWSTR, LPCWSTR);
	BOOL WantTrash(int &, UINT, LPCWSTR, LPCWSTR);
	static LPWSTR AssignPath(BSTR &, LPCWSTR);
	LPVOID ClearTarget(LPCWSTR, LPWSTR);
	void MoveItem(LPCWSTR, LPCWSTR);
	void CopyLeaf(LPCWSTR, LPCWSTR);
	void CopyTree(LPWSTR, LPWSTR);
	void DeleteItem(LPCWSTR);
	void Item(int);
	void FromTo(LPCWSTR, LPCWSTR);
	void Removing(LPCWSTR);
	LPCWSTR ToWhere(LPCWSTR, LPCWSTR);
	static DWORD WINAPI ProgressRoutine(LARGE_INTEGER, LARGE_INTEGER,
		LARGE_INTEGER, LARGE_INTEGER, DWORD, DWORD, HANDLE, HANDLE, LPVOID)
		throw();
	DWORD MoveThread();
	DWORD CopyThread();
	DWORD DeleteThread();
};
