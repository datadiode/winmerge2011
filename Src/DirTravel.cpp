/** 
 * @file  DirTravel.cpp
 *
 * @brief Implementation file for Directory traversal functions.
 *
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "paths.h"
#include "DiffContext.h"
#include "FileFilterHelper.h"
#include "DirItem.h"
#include "CompareStats.h"
#include "../BuildTmp/Merge/midl/LogParser_h.h"
#include "../BuildTmp/Merge/midl/LogParser_i.c"

void CDiffContext::InitCollect()
{
	if (m_piFilterGlobal->getSql() == NULL)
		return;

	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	OException::Check(CoCreateInstance(
		CLSID_LogQueryClass, NULL, CLSCTX_ALL,
		IID_ILogQuery, (void **)&m_piLogQuery));
	OException::Check(CoCreateInstance(
		CLSID_COMFileSystemInputContextClass, NULL, CLSCTX_ALL,
		IID_ICOMFileSystemInputContext, (void **)&m_piInputContext));

	m_piInputContext->put_recurse(0);
}

void CDiffContext::TermCollect()
{
	if (m_piFilterGlobal->getSql() == NULL)
		return;

	if (m_piLogQuery)
	{
		m_piLogQuery->Release();
		m_piLogQuery = NULL;
	}
	if (m_piInputContext)
	{
		m_piInputContext->Release();
		m_piInputContext = NULL;
	}

	CoUninitialize();
}

/**
 * @brief Load arrays with all directories & files in specified dir
 */
void CDiffContext::LoadAndSortFiles(LPCTSTR sDir, DirItemArray *dirs, DirItemArray *files) const
{
	LoadFiles(sDir, dirs, files);
	Sort(dirs);
	Sort(files);
	// If recursing the flat way, reset total count of items to 0
	if (m_nRecursive == 2)
		m_pCompareStats->SetTotalItems(0);
}

/**
 * @brief Find files and subfolders from given folder.
 * This function saves all files and subfolders in given folder to arrays.
 * We use 64-bit version of stat() to get times since find doesn't return
 * valid times for very old files (around year 1970). Even stat() seems to
 * give negative time values but we can live with that. Those around 1970
 * times can happen when file is created so that it  doesn't get valid
 * creation or modification dates.
 * @param [in] sDir Base folder for files and subfolders.
 * @param [in, out] dirs Array where subfolders are stored.
 * @param [in, out] files Array where files are stored.
 */
void CDiffContext::LoadFiles(LPCTSTR sDir, DirItemArray *dirs, DirItemArray *files) const
{
	if (m_piInputContext)
	{
		CMyComBSTR sql = L"SELECT Name, CreationTime, LastWriteTime, Attributes, Size FROM '";
		sql += paths_ConcatPath(sDir, _T("*")).c_str();
		sql += L"' ";
		sql += m_piFilterGlobal->getSql();
		CMyComPtr<ILogRecordset> spLogRecordset;
		OException::Check(m_piLogQuery->Execute(sql, m_piInputContext, &spLogRecordset));
		VARIANT_BOOL bAtEnd;
		while (OException::Check(spLogRecordset->atEnd(&bAtEnd)), bAtEnd == VARIANT_FALSE)
		{
			CMyComPtr<ILogRecord> spLogRecord;
			OException::Check(spLogRecordset->getRecord(&spLogRecord));
			DirItem ent;
			ent.path = sDir;
			CMyVariant var;
			OException::Check(spLogRecord->getValueEx(var = 0L, &var));
			ent.filename = V_BSTR(&var);
			OException::Check(spLogRecord->getValueEx(var = 1L, &var));
			reinterpret_cast<PROPVARIANT *>(&var)->uhVal.QuadPart -= 1992223368000000000UI64;
			ent.ctime = reinterpret_cast<PROPVARIANT *>(&var)->filetime;
			OException::Check(spLogRecord->getValueEx(var = 2L, &var));
			reinterpret_cast<PROPVARIANT *>(&var)->uhVal.QuadPart -= 1992223368000000000UI64;
			ent.mtime = reinterpret_cast<PROPVARIANT *>(&var)->filetime;
			OException::Check(spLogRecord->getValueEx(var = 3L, &var));
			UINT i = SysStringLen(V_BSTR(&var));
			while (i)
			{
				switch (V_BSTR(&var)[--i])
				{
				case 'D':
					ent.flags.attributes |= FILE_ATTRIBUTE_DIRECTORY;
					break;
				case 'R':
					ent.flags.attributes |= FILE_ATTRIBUTE_READONLY;
					break;
				case 'A':
					ent.flags.attributes |= FILE_ATTRIBUTE_ARCHIVE;
					break;
				case 'H':
					ent.flags.attributes |= FILE_ATTRIBUTE_HIDDEN;
					break;
				case 'S':
					ent.flags.attributes |= FILE_ATTRIBUTE_SYSTEM;
					break;
				}
			}
			if ((ent.flags.attributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				// Ensure attributes to be nonzero for existing files
				OException::Check(spLogRecord->getValueEx(var = 4L, &var));
				ent.size = V_CY(&var);
				ent.flags.attributes |= FILE_ATTRIBUTE_NORMAL;
				files->push_back(ent);
				// If recursing the flat way, increment total count of items
				if (m_nRecursive == 2)
					m_pCompareStats->IncreaseTotalItems();
			}
			else if (_tcsstr(_T(".."), ent.filename.c_str()) == NULL)
			{
				if (m_nRecursive == 2)
				{
					// Allow user to abort scanning
					if (ShouldAbort())
						break;
					String sDir = paths_ConcatPath(ent.path, ent.filename);
					if (m_piFilterGlobal->includeDir(sDir.c_str()))
						LoadFiles(sDir.c_str(), dirs, files);
				}
				else
				{
					dirs->push_back(ent);
				}
			}

			OException::Check(spLogRecordset->moveNext());
		}
	}
	else
	{
		WIN32_FIND_DATA ff;
		HANDLE h = FindFirstFile(paths_ConcatPath(sDir, _T("*.*")).c_str(), &ff);
		if (h != INVALID_HANDLE_VALUE)
		{
			do
			{
				DirItem ent;
				ent.path = sDir;
				ent.ctime = ff.ftCreationTime;
				ent.mtime = ff.ftLastWriteTime;
				ent.filename = ff.cFileName;
				ent.flags.attributes = ff.dwFileAttributes;
				if ((ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
				{
					// Ensure attributes to be nonzero for existing files
					ent.flags.attributes |= FILE_ATTRIBUTE_NORMAL;
					ent.size.Lo = ff.nFileSizeLow;
					ent.size.Hi = ff.nFileSizeHigh;
					files->push_back(ent);
					// If recursing the flat way, increment total count of items
					if (m_nRecursive == 2)
						m_pCompareStats->IncreaseTotalItems();
				}
				else if (_tcsstr(_T(".."), ff.cFileName) == NULL)
				{
					if (m_nRecursive == 2)
					{
						// Allow user to abort scanning
						if (ShouldAbort())
							break;
						String sDir = paths_ConcatPath(ent.path, ent.filename);
						if (m_piFilterGlobal->includeDir(sDir.c_str()))
							LoadFiles(sDir.c_str(), dirs, files);
					}
					else
					{
						dirs->push_back(ent);
					}
				}
			} while (FindNextFile(h, &ff));
			FindClose(h);
		}
	}
}

/**
 * @brief case-sensitive collate function for qsorting an array
 */
static bool __cdecl cmpstring(const DirItem &elem1, const DirItem &elem2)
{
	int cmp = _tcscoll(elem1.filename.c_str(), elem2.filename.c_str());
	if (cmp == 0)
		cmp = _tcscoll(elem1.path.c_str(), elem2.path.c_str());
	return cmp < 0;
}

/**
 * @brief case-insensitive collate function for qsorting an array
 */
static bool __cdecl cmpistring(const DirItem &elem1, const DirItem &elem2)
{
	if (int cmp = _tcsicoll(elem1.filename.c_str(), elem2.filename.c_str()))
		return cmp < 0;
	if (elem1.size.int64 != elem2.size.int64)
		return elem1.size.int64 < elem2.size.int64;
	if (elem1.mtime != elem2.mtime)
		return elem1.mtime < elem2.mtime;
	return _tcsicoll(elem1.path.c_str(), elem2.path.c_str()) < 0;
}

/**
 * @brief sort specified array
 */
void CDiffContext::Sort(DirItemArray *dirs) const
{
	stl::sort(dirs->begin(), dirs->end(),
		m_piFilterGlobal->isCaseSensitive() ? cmpstring : cmpistring);
}
