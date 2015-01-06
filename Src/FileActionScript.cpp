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
#include "StdAfx.h"
#include "Merge.h"
#include "LanguageSelect.h"
#include "MainFrm.h"
#include "FileActionScript.h"
#include "ShellFileOperations.h"
#include "paths.h"
#include "ConfirmFolderCopyDlg.h"

using std::vector;

/**
 * @brief Standard constructor.
 */
FileActionScript::FileActionScript()
: m_bMakeTargetItemWritable(false)
{
}

/**
 * @brief Destructor.
 */
FileActionScript::~FileActionScript()
{
}

/**
 * @brief Ask user a confirmation for copying item(s).
 * Shows a confirmation dialog for copy operation. Depending ont item count
 * dialog shows full paths to items (single item) or base paths of compare
 * (multiple items).
 * @param [in] origin Origin side of the item(s).
 * @param [in] destination Destination side of the item(s).
 * @param [in] src Source path.
 * @param [in] dest Destination path.
 * @param [in] destIsSide Is destination path either of compare sides?
 * @return TRUE if copy should proceed, FALSE if aborted.
 */
bool FileActionScript::ConfirmCopy(
	int origin, int destination, LPCTSTR src, LPCTSTR dest, bool destIsSide)
{
	bool ret = ConfirmDialog(IDS_CONFIRM_COPY_CAPTION,
		GetActionItemCount() == 1 ? IDS_CONFIRM_SINGLE_COPY : IDS_CONFIRM_MULTIPLE_COPY,
		origin, destination, src, dest, destIsSide);
	return ret;
}

/**
 * @brief Ask user a confirmation for moving item(s).
 * Shows a confirmation dialog for move operation. Depending ont item count
 * dialog shows full paths to items (single item) or base paths of compare
 * (multiple items).
 * @param [in] origin Origin side of the item(s).
 * @param [in] destination Destination side of the item(s).
 * @param [in] src Source path.
 * @param [in] dest Destination path.
 * @param [in] destIsSide Is destination path either of compare sides?
 * @return TRUE if copy should proceed, FALSE if aborted.
 */
bool FileActionScript::ConfirmMove(
	int origin, int destination, LPCTSTR src, LPCTSTR dest, bool destIsSide)
{
	bool ret = ConfirmDialog(IDS_CONFIRM_MOVE_CAPTION,
		GetActionItemCount() == 1 ? IDS_CONFIRM_SINGLE_MOVE : IDS_CONFIRM_MULTIPLE_MOVE,
		origin, destination, src, dest, destIsSide);
	return ret;
}

/**
 * @brief Show a (copy/move) confirmation dialog.
 * @param [in] caption Caption of the dialog.
 * @param [in] question Guestion to ask from user.
 * @param [in] origin Origin side of the item(s).
 * @param [in] destination Destination side of the item(s).
 * @param [in] src Source path.
 * @param [in] dest Destination path.
 * @param [in] destIsSide Is destination path either of compare sides?
 * @return TRUE if copy should proceed, FALSE if aborted.
 */
bool FileActionScript::ConfirmDialog(
	UINT caption, UINT question,
	int origin, int destination, LPCTSTR src, LPCTSTR dest, bool destIsSide)
{
	ConfirmFolderCopyDlg dlg;
	dlg.m_caption = LanguageSelect.LoadString(caption);
	dlg.m_question = LanguageSelect.Format(question, GetActionItemCount());
	dlg.m_fromText = LanguageSelect.LoadString(
		origin == FileActionItem::UI_LEFT ? IDS_FROM_LEFT : IDS_FROM_RIGHT);
	dlg.m_toText = LanguageSelect.LoadString(!destIsSide ? IDS_TO :
		destination == FileActionItem::UI_LEFT ? IDS_TO_LEFT : IDS_TO_RIGHT);

	dlg.m_fromPath = src;
	if (paths_DoesPathExist(src) == IS_EXISTING_DIR && !paths_EndsWithSlash(src))
		dlg.m_fromPath.push_back(_T('\\'));
	dlg.m_toPath = dest;
	if (paths_DoesPathExist(dest) == IS_EXISTING_DIR && !paths_EndsWithSlash(dest))
		dlg.m_toPath.push_back(_T('\\'));

	bool ret = LanguageSelect.DoModal(dlg) == IDYES;
	m_bMakeTargetItemWritable = dlg.m_bMakeTargetItemWritable != FALSE;
	return ret;
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
		if (String::size_type len = iter->src.length())
			cchSource[iter->atype] += len + 1;
		if (String::size_type len = iter->dest.length())
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

	int choice = IDYES;
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
					ASSERT(choice != 0); // or else expect a side effect on iter->dest
					choice = theApp.m_pMainWnd->HandleReadonlySave(
						const_cast<String &>(iter->dest), choice);
					if (choice == IDCANCEL)
						break;
					if (choice == IDNO) // Skip this item
						continue; // NB: This also advances the iterator!
				}
				if (!theApp.m_pMainWnd->CreateBackup(true, iter->dest.c_str()))
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
