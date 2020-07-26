/* Merge7z907.cpp
Copyright (c) Jochen Neubeck
SPDX-License-Identifier: LGPL-2.1-or-later
Last change: 2018-12-28 by Jochen Neubeck */

#include "stdafx.h"

#define INITGUID
#include <initguid.h>

#include "Merge7zCommon.h"

#include "7zip/UI/FileManager/OpenCallback.h"
#include "7zip/UI/FileManager/ExtractCallback.h"
#include "7zip/UI/FileManager/resourceGui.h"

#include "7zip/UI/Common/ArchiveExtractCallback.h"
#include "7zip/UI/GUI/UpdateCallbackGUI.h"

#include "Windows/Thread.h"

/**
 * @brief Extraction thread
 */
class CThreadExtracting : public CProgressThreadVirt
{
protected:
	virtual HRESULT ProcessVirt()
	{
		HRESULT result = Archive->Extract(indices, numItems, false, ArchiveExtractCallback);
		return result;
	}
	CExtractCallbackImp *ExtractCallbackSpec;
	IInArchive *Archive;
	IArchiveExtractCallback *ArchiveExtractCallback;
	const UINT32 *indices;
	UINT32 numItems;
public:
	//HRESULT result;

	CThreadExtracting
	(
		CExtractCallbackImp *ExtractCallbackSpec,
		IInArchive *Archive,
		IArchiveExtractCallback *ArchiveExtractCallback,
		const CSysString &title,
		HWND hwndParent,
		const UINT32 *indices = 0,
		UINT32 numItems = -1
	):
		ExtractCallbackSpec(ExtractCallbackSpec),
		Archive(Archive),
		ArchiveExtractCallback(ArchiveExtractCallback),
		indices(indices),
		numItems(numItems)
	{
		ExtractCallbackSpec->ProgressDialog = this;
		CompressingMode = false;
		//result = E_FAIL;
		if (HRESULT hr = Create(GetUnicodeString(title), hwndParent))
		{
			Complain(hr, NULL);
		}
	}
};

/**
 * @brief Initialize Inspector
 */
Format7zDLL::Interface::Inspector::Inspector(Format7zDLL::Interface *format, LPCTSTR path)
: format(format), archive(0), file(0), callback(0), path(path), ustrDefaultName(GetUnicodeString(path))
{
}

/**
 * @brief Open archive from separate thread
 */
unsigned Format7zDLL::Interface::Inspector::OpenThread()
{
	COpenArchiveCallback *const callbackImpl = static_cast<COpenArchiveCallback *>(callback);
	CProgressCloser closer(callbackImpl->ProgressDialog);
	if COMPLAIN(!NFile::NFind::CFindFile().FindFirst(path, fileInfo))
		return ERROR_FILE_NOT_FOUND;
	if COMPLAIN(!file->Open(path))
		return ERROR_OPEN_FAILED;
	static const UInt64 maxCheckStartPosition = ULLONG_MAX;
	if COMPLAIN(archive->Open(file, &maxCheckStartPosition, callback) != S_OK)
		return ERROR_CANT_ACCESS_FILE;
	return ERROR_SUCCESS;
}

/**
 * @brief Wrap the above in a callback accepted by NWindows::CThread::Create()
 */
unsigned Format7zDLL::Interface::Inspector::OpenThreadStatic(void *p)
{
	return static_cast<Inspector *>(p)->OpenThread();
}

/**
 * @brief Initialize Inspector
 */
void Format7zDLL::Interface::Inspector::Init(HWND hwndParent)
{
	format->GetDefaultName(hwndParent, ustrDefaultName);
	COpenArchiveCallback *callbackImpl = new COpenArchiveCallback;
	(archive = format->GetInArchive()) -> AddRef();
	(file = new CInFileStream) -> AddRef();
	(callback = callbackImpl) -> AddRef();
	callbackImpl->PasswordIsDefined = false;
	callbackImpl->ParentWindow = hwndParent;
	UString progressTitle = LangString(IDS_OPENNING);
	callbackImpl->ProgressDialog.MainWindow = hwndParent;
	callbackImpl->ProgressDialog.MainTitle = L"7-Zip";
	callbackImpl->ProgressDialog.MainAddTitle = progressTitle + L' ';
	callbackImpl->ProgressDialog.WaitMode = true;
	NWindows::CThread thread;
	thread.Create(OpenThreadStatic, this);
	callbackImpl->StartProgressDialog(progressTitle, thread);
	DWORD dwError = ERROR_INVALID_HANDLE;
	GetExitCodeThread(thread, &dwError);
	if (dwError != ERROR_SUCCESS)
		Complain(dwError, path);
	passwordIsDefined = callbackImpl->PasswordIsDefined;
	password = callbackImpl->Password;
}

/**
 * @brief Extract set of items specified by index
 */
HRESULT Format7zDLL::Interface::Inspector::Extract(HWND hwndParent, LPCTSTR folder, const UINT32 *indices, UINT32 numItems)
{
	CArchiveExtractCallback *extractCallbackSpec = 0;
	CExtractCallbackImp *extractCallbackSpec2 = 0;
	HRESULT result = 0;
	try
	{
		if (*folder)
		{
			if COMPLAIN(!NFile::NDir::CreateComplexDir(folder))
			{
				Complain(ERROR_CANNOT_MAKE, folder);
			}
		}

		(extractCallbackSpec2 = new CExtractCallbackImp) -> AddRef();

		extractCallbackSpec2->Init();

		extractCallbackSpec2->OverwriteMode = NExtract::NOverwriteMode::kOverwrite;
		extractCallbackSpec2->PasswordIsDefined = passwordIsDefined;
		extractCallbackSpec2->Password = password;

		(extractCallbackSpec = new CArchiveExtractCallback) -> AddRef();

		extractCallbackSpec->InitForMulti
		(
			false,
			NExtract::NPathMode::kFullPaths,
			NExtract::NOverwriteMode::kOverwrite,
			false
		);

		CExtractNtOptions ntOptions;
		CArc arc;
		arc.Archive = archive;
		arc.Path = GetUnicodeString(path);
		arc.DefaultName = ustrDefaultName;
		arc.FormatIndex = 0;
		arc.SubfileIndex = 0;
		extractCallbackSpec->Init
		(
			ntOptions,
			NULL,
			&arc,
			extractCallbackSpec2,
			false,											//stdOutMode
			false,											//testMode
			GetUnicodeString(folder),
			UStringVector(), false,
			(UInt64)(Int64)-1
		);

		result = CThreadExtracting
		(
			extractCallbackSpec2,
			archive,
			extractCallbackSpec,
			PathFindFileName(path),
			hwndParent,
			indices,
			numItems
		).Result;

		//.result;

		/*if COMPLAIN(extractCallbackSpec->_numErrors)
		{
			//	There is no canned system message for this one, so it won't
			//	localize. I can't help it.
			Complain(_T("%s:\n%I64u error(s)"), path, extractCallbackSpec->_numErrors);
		}*/
		if COMPLAIN(result != S_OK && result != E_ABORT)
		{
			Complain(result, path);
		}
	}
	catch (Complain *complain)
	{
		result = complain->Alert(hwndParent);
	}
	Release(static_cast<IArchiveExtractCallback*>(extractCallbackSpec));
	Release(static_cast<IFolderArchiveExtractCallback*>(extractCallbackSpec2));
	return result;
}

/**
 * @brief Compression thread
 */
class CThreadUpdating : public CProgressThreadVirt
{
protected:
	virtual HRESULT ProcessVirt()
	{
		HRESULT res = outArchive->UpdateItems(file, numItems, updateCallbackSpec);
		return res;
	}
	CUpdateCallbackGUI *updateCallbackGUI;
	IOutArchive *outArchive;
	CArchiveUpdateCallback *updateCallbackSpec;
	COutFileStream *file;
public:
	HRESULT result;
	UINT32 numItems;
	CThreadUpdating
	(
		CUpdateCallbackGUI *updateCallbackGUI,
		IOutArchive *outArchive,
		CArchiveUpdateCallback *updateCallbackSpec,
		UINT32 numItems,
		COutFileStream *file,
		const CSysString &title,
		HWND hwndParent
	):
		updateCallbackGUI(updateCallbackGUI),
		outArchive(outArchive),
		updateCallbackSpec(updateCallbackSpec),
		numItems(numItems),
		file(file)
	{
		result = E_FAIL;
		updateCallbackGUI->ProgressDialog = this;
		if (HRESULT hr = Create(GetUnicodeString(title), hwndParent))
		{
			Complain(hr, NULL);
		}
	}
};

/**
 * @brief Construct Updater
 */
Format7zDLL::Interface::Updater::Updater(Format7zDLL::Interface *format, LPCTSTR path)
: format(format), outArchive(0), file(0), path(path)
{
}

/**
 * @brief Initialize Updater
 */
void Format7zDLL::Interface::Updater::Init(HWND hwndParent)
{
	(outArchive = format->GetOutArchive()) -> AddRef();
	(file = new COutFileStream) -> AddRef();
	if COMPLAIN(!file->Create(path, true))
	{
		Complain(ERROR_CANNOT_MAKE, path);
	}
}

/**
 * @brief Commit update
 */
HRESULT Format7zDLL::Interface::Updater::Commit(HWND hwndParent)
{
	CArchiveUpdateCallback *updateCallbackSpec = 0;
	CUpdateCallbackGUI *updateCallbackGUI = 0;
	HRESULT result = 0;
	try
	{
		//	Ref counts are not always accurate with 7-Zip.
		//	An extra AddRef() ensures that interfaces remain valid until they
		//	are explicitly released at the end of this function.
		(updateCallbackSpec = new CArchiveUpdateCallback) -> AddRef();
		(updateCallbackGUI = new CUpdateCallbackGUI);// -> AddRef();

		updateCallbackGUI->Init();

		// No items in dest archive. We always recreate the dest archive
		// Build the operationChain. One element per item
		CRecordVector<CUpdatePair2> operationChain;
		CUpdatePair2 pair2;
		pair2.NewData = pair2.NewProps = true;

		operationChain.Reserve(dirItems.Items.Size());
		//
		unsigned i;
		for (i = 0 ; i < dirItems.Items.Size() ; i++)
		{
			pair2.DirIndex = i;
			operationChain.Add(pair2);
		}
		operationChain.ReserveDown();

		/* The following code may or may not help implement update on existing
		/* archives, which is currently not supported.
		UINT32 fileTimeType;
		outArchive->GetFileTimeType(&fileTimeType);

		CObjectVector<CUpdatePair> updatePairs;
		GetUpdatePairInfoList(dirItems, archiveItems, NFileTimeType::EEnum(fileTimeType), updatePairs); // must be done only once!!!

		CObjectVector<CUpdatePair2> operationChain;
		UpdateProduce(dirItems, archiveItems, updatePairs, NUpdateArchive::kAddActionSet, operationChain);*/

		// Now compress...
		updateCallbackSpec->DirItems = &dirItems;
		updateCallbackSpec->ArcItems = &archiveItems;
		updateCallbackSpec->UpdatePairs = &operationChain;
		updateCallbackSpec->Callback = updateCallbackGUI;

		result = CThreadUpdating
		(
			updateCallbackGUI,
			outArchive,
			updateCallbackSpec,
			operationChain.Size(),
			file,
			PathFindFileName(path),
			hwndParent
		).Result;

		if COMPLAIN(result != S_OK && result != E_ABORT)
		{
			Complain(result, path);
		}
	}
	catch (Complain *complain)
	{
		result = complain->Alert(hwndParent);
	}
	//	Always release interfaces in this order, or else all hell will break
	//	loose!
	Release(static_cast<IArchiveUpdateCallback*>(updateCallbackSpec));
	delete updateCallbackGUI;
	return result;
}
