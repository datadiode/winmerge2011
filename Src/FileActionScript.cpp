/////////////////////////////////////////////////////////////////////////////
//    License (GPLv2+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or (at
//    your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
/////////////////////////////////////////////////////////////////////////////
/**
 * @file  FileActionScript.cpp
 *
 * @brief Implementation of FileActionScript and related classes
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "Merge.h"
#include "LanguageSelect.h"
#include "OptionsDef.h"
#include "MainFrm.h"
#include "FileActionScript.h"
#include "ShellFileOperations.h"
#include "paths.h"

using stl::vector;

/**
 * @brief Standard constructor.
 */
FileActionScript::FileActionScript()
{
}

/**
 * @brief Standard destructor.
 */
FileActionScript::~FileActionScript()
{
}

/**
 * @brief Remove last action item from the list.
 * @return Item removed from the list.
 */
FileActionItem FileActionScript::RemoveTailActionItem()
{
	FileActionItem item = m_actions.back();
	m_actions.pop_back();
	return item;
}

bool FileActionScript::Run(HWND hwnd, FILEOP_FLAGS flags)
{
	// Now process files/directories that got added to list
	vector<FileActionItem>::const_iterator iter;
	DWORD cchSource[FileAction::ACT_TYPE] = { 0, 0, 0 };
	DWORD cchDestination[FileAction::ACT_TYPE] = { 0, 0, 0 };

	for (iter = m_actions.begin() ; iter != m_actions.end() ; ++iter)
	{
		if (size_t len = iter->src.length())
			cchSource[iter->atype] += len + 1;
		if (size_t len = iter->dest.length())
			cchDestination[iter->atype] += len + 1;
	}

	ShellFileOperations CopyOperations(hwnd, FO_COPY,
		flags | FOF_MULTIDESTFILES | FOF_NOCONFIRMMKDIR | FOF_NOCONFIRMATION,
		cchSource[FileAction::ACT_COPY], cchDestination[FileAction::ACT_COPY]);
	ShellFileOperations MoveOperations(hwnd, FO_MOVE,
		flags | FOF_MULTIDESTFILES,
		cchSource[FileAction::ACT_MOVE], cchDestination[FileAction::ACT_MOVE]);
	ShellFileOperations DelOperations(hwnd, FO_DELETE,
		flags,
		cchSource[FileAction::ACT_DEL] + cchDestination[FileAction::ACT_DEL], 0);

	BOOL bApplyToAll = FALSE;
	for (iter = m_actions.begin() ; iter != m_actions.end() ; ++iter)
	{
		if (iter->atype == FileAction::ACT_COPY)
		{
			if (iter->dirflag)
			{
				paths_CreateIfNeeded(iter->dest.c_str());
			}
			else
			{
				// Handle VCS checkout
				// Before we can write over destination file, we must unlock
				// (checkout) it. This also notifies VCS system that the file
				// has been modified.
				if (COptionsMgr::Get(OPT_VCS_SYSTEM) != VCS_NONE)
				{
					String strErr;
					int retVal = theApp.m_pMainWnd->SyncFileToVCS(
						iter->dest.c_str(), bApplyToAll, &strErr);
					if (retVal == -1)
					{
						theApp.DoMessageBox(strErr.c_str(), MB_ICONERROR);
						break;
					}
					if (retVal == IDCANCEL)
						break;
					if (retVal == IDNO) // Skip this item
						continue; // NB: This also advances the iterator!
				}
				if (!theApp.m_pMainWnd->CreateBackup(TRUE, iter->dest.c_str()))
				{
					LanguageSelect.MsgBox(IDS_ERROR_BACKUP, MB_ICONERROR);
					break;
				}
			}
			CopyOperations.AddSource(iter->src.c_str());
			CopyOperations.AddDestination(iter->dest.c_str());
		}
		else if (iter->atype == FileAction::ACT_MOVE)
		{
			MoveOperations.AddSource(iter->src.c_str());
			MoveOperations.AddDestination(iter->dest.c_str());
		}
		else if (iter->atype == FileAction::ACT_DEL)
		{
			DelOperations.AddSource(iter->src.c_str());
			if (!iter->dest.empty())
				DelOperations.AddSource(iter->dest.c_str());
		}
	}

	return
	(
		iter == m_actions.end() // if above loop did not terminate prematurely
	&&	CopyOperations.Run()
	&&	MoveOperations.Run()
	&&	DelOperations.Run()
	);
}
