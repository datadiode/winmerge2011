/////////////////////////////////////////////////////////////////////////////
//    see Merge.cpp for license (GPLv2+) statement
//
/////////////////////////////////////////////////////////////////////////////
/**
 *  @file DirActions.cpp
 *
 *  @brief Implementation of methods of CDirView that copy/move/delete files
 */
// ID line follows -- this is updated by SVN
// $Id$

// It would be nice to make this independent of the UI (CDirView)
// but it needs access to the list of selected items.
// One idea would be to provide an iterator over them.
//

#include "stdafx.h"
#include "Merge.h"
#include "LanguageSelect.h"
#include "DirView.h"
#include "DirFrame.h"
#include "MainFrm.h"
#include "coretools.h"
#include "paths.h"
#include "7zCommon.h"
#include "ShellFileOperations.h"
#include "OptionsDef.h"
#include "LogFile.h"
#include "DiffItem.h"
#include "FileActionScript.h"
#include "LoadSaveCodepageDlg.h"
#include "IntToIntMap.h"
#include "FileOrFolderSelect.h"
#include "ConfirmFolderCopyDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static bool ConfirmCopy(int origin, int destination, int count,
		LPCTSTR src, LPCTSTR dest, BOOL destIsSide);
static bool ConfirmMove(int origin, int destination, int count,
		LPCTSTR src, LPCTSTR dest, BOOL destIsSide);
static bool ConfirmDialog(UINT caption, UINT question,
		int origin, int destination, int count,
		LPCTSTR src, LPCTSTR dest, BOOL destIsSide);
static bool CheckPathsExist(LPCTSTR path, int allow = IS_EXISTING);

/**
 * @brief Ask user a confirmation for copying item(s).
 * Shows a confirmation dialog for copy operation. Depending ont item count
 * dialog shows full paths to items (single item) or base paths of compare
 * (multiple items).
 * @param [in] origin Origin side of the item(s).
 * @param [in] destination Destination side of the item(s).
 * @param [in] count Number of items.
 * @param [in] src Source path.
 * @param [in] dest Destination path.
 * @param [in] destIsSide Is destination path either of compare sides?
 * @return TRUE if copy should proceed, FALSE if aborted.
 */
static bool ConfirmCopy(int origin, int destination, int count,
		LPCTSTR src, LPCTSTR dest, BOOL destIsSide)
{
	bool ret = ConfirmDialog(
		IDS_CONFIRM_COPY_CAPTION,
		count == 1 ? IDS_CONFIRM_SINGLE_COPY : IDS_CONFIRM_MULTIPLE_COPY,
		origin,
		destination, count,	src, dest, destIsSide);
	return ret;
}

/**
 * @brief Ask user a confirmation for moving item(s).
 * Shows a confirmation dialog for move operation. Depending ont item count
 * dialog shows full paths to items (single item) or base paths of compare
 * (multiple items).
 * @param [in] origin Origin side of the item(s).
 * @param [in] destination Destination side of the item(s).
 * @param [in] count Number of items.
 * @param [in] src Source path.
 * @param [in] dest Destination path.
 * @param [in] destIsSide Is destination path either of compare sides?
 * @return TRUE if copy should proceed, FALSE if aborted.
 */
static bool ConfirmMove(int origin, int destination, int count,
		LPCTSTR src, LPCTSTR dest, BOOL destIsSide)
{
	bool ret = ConfirmDialog(
		IDS_CONFIRM_MOVE_CAPTION,
		count == 1 ? IDS_CONFIRM_SINGLE_MOVE : IDS_CONFIRM_MULTIPLE_MOVE,
		origin,
		destination, count,	src, dest, destIsSide);
	return ret;
}

/**
 * @brief Show a (copy/move) confirmation dialog.
 * @param [in] caption Caption of the dialog.
 * @param [in] question Guestion to ask from user.
 * @param [in] origin Origin side of the item(s).
 * @param [in] destination Destination side of the item(s).
 * @param [in] count Number of items.
 * @param [in] src Source path.
 * @param [in] dest Destination path.
 * @param [in] destIsSide Is destination path either of compare sides?
 * @return TRUE if copy should proceed, FALSE if aborted.
 */
static bool ConfirmDialog(UINT caption, UINT question,
		int origin, int destination, int count,
		LPCTSTR src, LPCTSTR dest, BOOL destIsSide)
{
	ConfirmFolderCopyDlg dlg;
	dlg.m_caption = LanguageSelect.LoadString(caption);
	dlg.m_question = LanguageSelect.Format(question, count);
	dlg.m_fromText = LanguageSelect.LoadString(
		origin == FileActionItem::UI_LEFT ? IDS_FROM_LEFT : IDS_FROM_RIGHT);
	dlg.m_toText = LanguageSelect.LoadString(!destIsSide ? IDS_TO :
		destination == FileActionItem::UI_LEFT ? IDS_TO_LEFT : IDS_TO_RIGHT);

	dlg.m_fromPath = src;
	if (paths_DoesPathExist(src) == IS_EXISTING_DIR && !paths_EndsWithSlash(src))
		dlg.m_fromPath += _T("\\");
	dlg.m_toPath = dest;
	if (paths_DoesPathExist(dest) == IS_EXISTING_DIR && !paths_EndsWithSlash(dest))
		dlg.m_toPath += _T("\\");

	return LanguageSelect.DoModal(dlg) == IDYES;
}

/**
 * @brief Checks if path (to be operated) exists.
 * This function checks if the given paths exists and is of allowed type.
 * @return TRUE if path exists and is of allowed type.
 */
static bool CheckPathsExist(LPCTSTR path, int allow)
{
	if (allow != 0 && (allow & paths_DoesPathExist(path)) == 0)
	{
		LanguageSelect.MsgBox(IDS_DIRCMP_NOTSYNC,
			paths_UndoMagic(&String(path).front()), MB_ICONWARNING);
		return false;
	}
	return true;
}

/// Prompt & copy item from right to left, if legal
void CDirView::DoCopyRightToLeft()
{
	WaitStatusCursor waitstatus(IDS_STATUS_COPYFILES);
	// First we build a list of desired actions
	FileActionScript actionScript;
	int selCount = 0;
	int sel = -1;
	String slFile, srFile;
	while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
	{
		const DIFFITEM& di = GetDiffItem(sel);
		if (di.diffcode.diffcode != 0 && IsItemCopyableToLeft(di))
		{
			GetItemFileNames(sel, slFile, srFile);
			
			// We must check that paths still exists
			if (!CheckPathsExist(srFile.c_str()))
				return;

			FileActionItem act;
			String sDest(slFile);

			if (m_pFrame->GetRecursive())
			{
				// If destination sides's relative path is empty it means we
				// are copying unique items and need to get the real relative
				// path from original side.
				if (di.left.path.empty())
				{
					sDest = m_pFrame->GetLeftBasePath();
					sDest = paths_ConcatPath(sDest, di.right.path);
					sDest = paths_ConcatPath(sDest, di.right.filename);
				}
			}

			act.src = srFile;
			act.dest = sDest;
			act.context = sel;
			act.dirflag = di.diffcode.isDirectory();
			act.atype = FileAction::ACT_COPY;
			act.UIResult = FileActionItem::UI_SYNC;
			act.UIOrigin = FileActionItem::UI_RIGHT;
			act.UIDestination = FileActionItem::UI_LEFT;
			actionScript.AddActionItem(act);
		}
		++selCount;
	}
	// Now we prompt, and execute actions
	ConfirmAndPerformActions(actionScript, selCount);
}

/// Prompt & copy item from left to right, if legal
void CDirView::DoCopyLeftToRight()
{
	WaitStatusCursor waitstatus(IDS_STATUS_COPYFILES);
	// First we build a list of desired actions
	FileActionScript actionScript;
	int selCount = 0;
	int sel = -1;
	String slFile, srFile;
	while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
	{
		const DIFFITEM& di = GetDiffItem(sel);
		if (di.diffcode.diffcode != 0 && IsItemCopyableToRight(di))
		{
			GetItemFileNames(sel, slFile, srFile);

			// We must first check that paths still exists
			if (!CheckPathsExist(slFile.c_str()))
				return;

			FileActionItem act;
			String sDest(srFile);

			if (m_pFrame->GetRecursive())
			{
				// If destination sides's relative path is empty it means we
				// are copying unique items and need to get the real relative
				// path from original side.
				if (di.right.path.empty())
				{
					sDest = m_pFrame->GetRightBasePath();
					sDest = paths_ConcatPath(sDest, di.left.path);
					sDest = paths_ConcatPath(sDest, di.left.filename);
				}
			}

			act.src = slFile;
			act.dest = sDest;
			act.dirflag = di.diffcode.isDirectory();
			act.context = sel;
			act.atype = FileAction::ACT_COPY;
			act.UIResult = FileActionItem::UI_SYNC;
			act.UIOrigin = FileActionItem::UI_LEFT;
			act.UIDestination = FileActionItem::UI_RIGHT;
			actionScript.AddActionItem(act);
		}
		++selCount;
	}
	// Now we prompt, and execute actions
	ConfirmAndPerformActions(actionScript, selCount);
}

/// Prompt & delete left, if legal
void CDirView::DoDelLeft()
{
	WaitStatusCursor waitstatus(IDS_STATUS_DELETEFILES);
	// First we build a list of desired actions
	FileActionScript actionScript;
	int selCount = 0;
	int sel = -1;
	String slFile, srFile;
	while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
	{
		const DIFFITEM& di = GetDiffItem(sel);
		if (di.diffcode.diffcode != 0 && IsItemDeletableOnLeft(di))
		{
			GetItemFileNames(sel, slFile, srFile);

			// We must check that path still exists
			if (!CheckPathsExist(slFile.c_str()))
				return;

			FileActionItem act;
			act.src = slFile;
			act.dirflag = di.diffcode.isDirectory();
			act.context = sel;
			act.atype = FileAction::ACT_DEL;
			act.UIResult = FileActionItem::UI_DEL_LEFT;
			actionScript.AddActionItem(act);
		}
		++selCount;
	}
	// Now we prompt, and execute actions
	ConfirmAndPerformActions(actionScript, selCount);
}
/// Prompt & delete right, if legal
void CDirView::DoDelRight()
{
	WaitStatusCursor waitstatus(IDS_STATUS_DELETEFILES);
	// First we build a list of desired actions
	FileActionScript actionScript;
	int selCount = 0;
	int sel = -1;
	String slFile, srFile;
	while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
	{
		const DIFFITEM& di = GetDiffItem(sel);
		if (di.diffcode.diffcode != 0 && IsItemDeletableOnRight(di))
		{
			GetItemFileNames(sel, slFile, srFile);

			// We must first check that path still exists
			if (!CheckPathsExist(srFile.c_str()))
				return;

			FileActionItem act;
			act.src = srFile;
			act.dirflag = di.diffcode.isDirectory();
			act.context = sel;
			act.atype = FileAction::ACT_DEL;
			act.UIResult = FileActionItem::UI_DEL_RIGHT;
			actionScript.AddActionItem(act);
		}
		++selCount;
	}
	// Now we prompt, and execute actions
	ConfirmAndPerformActions(actionScript, selCount);
}

/**
 * @brief Prompt & delete both, if legal.
 */
void CDirView::DoDelBoth()
{
	WaitStatusCursor waitstatus(IDS_STATUS_DELETEFILES);
	// First we build a list of desired actions
	FileActionScript actionScript;
	int selCount = 0;
	int sel = -1;
	String slFile, srFile;
	while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
	{
		const DIFFITEM& di = GetDiffItem(sel);
		if (di.diffcode.diffcode != 0 && IsItemDeletableOnBoth(di))
		{
			GetItemFileNames(sel, slFile, srFile);

			// We must first check that paths still exists
			if (!CheckPathsExist(srFile.c_str()) ||
				!CheckPathsExist(slFile.c_str()))
			{
				return;
			}

			FileActionItem act;
			act.src = srFile;
			act.dest = slFile;
			act.dirflag = di.diffcode.isDirectory();
			act.context = sel;
			act.atype = FileAction::ACT_DEL;
			act.UIResult = FileActionItem::UI_DEL_BOTH;
			actionScript.AddActionItem(act);
		}
		++selCount;
	}
	// Now we prompt, and execute actions
	ConfirmAndPerformActions(actionScript, selCount);
}

/**
 * @brief Delete left, right or both items.
 * @note Usually we don't need to check for read-only in this level of code.
 *   Usually we can disable handling read-only items/sides by disabling GUI
 *   element. But in this case the GUI element effects to both sides and can
 *   be selected when another side is read-only.
 */
void CDirView::DoDelAll()
{
	WaitStatusCursor waitstatus(IDS_STATUS_DELETEFILES);
	// First we build a list of desired actions
	FileActionScript actionScript;
	int selCount = 0;
	int sel = -1;
	String slFile, srFile;
	const BOOL leftRO = m_pFrame->GetLeftReadOnly();
	const BOOL rightRO = m_pFrame->GetRightReadOnly();
	while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
	{
		const DIFFITEM& di = GetDiffItem(sel);
		if (di.diffcode.diffcode != 0)
		{
			GetItemFileNames(sel, slFile, srFile);

			int leftFlags = DOES_NOT_EXIST;
			int rightFlags = DOES_NOT_EXIST;
			FileActionItem act;
			if (IsItemDeletableOnBoth(di) && !leftRO && !rightRO)
			{
				leftFlags = IS_EXISTING;
				rightFlags = IS_EXISTING;
				act.src = srFile;
				act.dest = slFile;
				act.UIResult = FileActionItem::UI_DEL_BOTH;
			}
			else if (IsItemDeletableOnLeft(di) && !leftRO)
			{
				leftFlags = IS_EXISTING;
				act.src = slFile;
				act.UIResult = FileActionItem::UI_DEL_LEFT;
			}
			else if (IsItemDeletableOnRight(di) && !rightRO)
			{
				rightFlags = IS_EXISTING;
				act.src = srFile;
				act.UIResult = FileActionItem::UI_DEL_RIGHT;
			}
			// Check one of sides is actually being added to removal list
			if (leftFlags != DOES_NOT_EXIST || rightFlags != DOES_NOT_EXIST)
			{
				// We must first check that paths still exists
				if (!CheckPathsExist(slFile.c_str(), leftFlags) ||
					!CheckPathsExist(srFile.c_str(), rightFlags))
				{
					return;
				}

				act.dirflag = di.diffcode.isDirectory();
				act.context = sel;
				act.atype = FileAction::ACT_DEL;
				actionScript.AddActionItem(act);
				++selCount;
			}
		}
	}
	// Now we prompt, and execute actions
	ConfirmAndPerformActions(actionScript, selCount);
}

/**
 * @brief Copy selected left-side files to user-specified directory
 *
 * When copying files from recursive compare file subdirectory is also
 * read so directory structure is preserved.
 */
void CDirView::DoCopyLeftTo()
{
	String destPath;
	if (!SelectFolder(m_hWnd, destPath, m_lastCopyFolder.c_str(), IDS_SELECT_DEST_LEFT))
		return;
	m_lastCopyFolder = destPath;

	WaitStatusCursor waitstatus(IDS_STATUS_COPYFILES);

	FileActionScript actionScript;
	int selCount = 0;
	int sel = -1;
	String slFile, srFile;
	while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
	{
		const DIFFITEM& di = GetDiffItem(sel);
		if (di.diffcode.diffcode != 0 && IsItemCopyableToOnLeft(di))
		{
			GetItemFileNames(sel, slFile, srFile);

			// We must check that path still exists
			if (!CheckPathsExist(slFile.c_str()))
				return;

			FileActionItem act;
			String sFullDest = destPath + _T("\\");

			actionScript.m_destBase = sFullDest;

			if (m_pFrame->GetRecursive())
			{
				if (!di.left.path.empty())
				{
					sFullDest += di.left.path;
					sFullDest += _T("\\");
				}
			}
			sFullDest += di.left.filename;
			act.dest = sFullDest;

			act.src = slFile;
			act.dirflag = di.diffcode.isDirectory();
			act.context = sel;
			act.atype = FileAction::ACT_COPY;
			act.UIResult = FileActionItem::UI_DONT_CARE;
			act.UIOrigin = FileActionItem::UI_LEFT;
			actionScript.AddActionItem(act);
			++selCount;
		}
	}
	// Now we prompt, and execute actions
	ConfirmAndPerformActions(actionScript, selCount);
}

/**
 * @brief Copy selected righ-side files to user-specified directory
 *
 * When copying files from recursive compare file subdirectory is also
 * read so directory structure is preserved.
 */
void CDirView::DoCopyRightTo()
{
	String destPath;
	if (!SelectFolder(m_hWnd, destPath, m_lastCopyFolder.c_str(), IDS_SELECT_DEST_RIGHT))
		return;
	m_lastCopyFolder = destPath;

	WaitStatusCursor waitstatus(IDS_STATUS_COPYFILES);

	FileActionScript actionScript;
	int selCount = 0;
	int sel = -1;
	String slFile, srFile;
	while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
	{
		const DIFFITEM& di = GetDiffItem(sel);
		if (di.diffcode.diffcode != 0 && IsItemCopyableToOnRight(di))
		{
			GetItemFileNames(sel, slFile, srFile);

			// We must check that path still exists
			if (!CheckPathsExist(srFile.c_str()))
				return;

			FileActionItem act;
			String sFullDest = destPath + _T("\\");

			actionScript.m_destBase = sFullDest;

			if (m_pFrame->GetRecursive())
			{
				if (!di.right.path.empty())
				{
					sFullDest += di.right.path;
					sFullDest += _T("\\");
				}
			}
			sFullDest += di.right.filename;
			act.dest = sFullDest;

			act.src = srFile;
			act.dirflag = di.diffcode.isDirectory();
			act.context = sel;
			act.atype = FileAction::ACT_COPY;
			act.UIResult = FileActionItem::UI_DONT_CARE;
			act.UIOrigin = FileActionItem::UI_RIGHT;
			actionScript.AddActionItem(act);
			++selCount;
		}
	}
	// Now we prompt, and execute actions
	ConfirmAndPerformActions(actionScript, selCount);
}

/**
 * @brief Move selected left-side files to user-specified directory
 *
 * When moving files from recursive compare file subdirectory is also
 * read so directory structure is preserved.
 */
void CDirView::DoMoveLeftTo()
{
	String destPath;
	if (!SelectFolder(m_hWnd, destPath, m_lastCopyFolder.c_str(), IDS_SELECT_DEST_LEFT))
		return;
	m_lastCopyFolder = destPath;

	WaitStatusCursor waitstatus(IDS_STATUS_MOVEFILES);

	FileActionScript actionScript;
	int selCount = 0;
	int sel = -1;
	String slFile, srFile;
	while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
	{
		const DIFFITEM& di = GetDiffItem(sel);
		if (di.diffcode.diffcode != 0 && IsItemCopyableToOnLeft(di) && IsItemDeletableOnLeft(di))
		{
			GetItemFileNames(sel, slFile, srFile);

			// We must check that path still exists
			if (!CheckPathsExist(slFile.c_str()))
				return;

			FileActionItem act;
			String sFullDest = destPath + _T("\\");
			actionScript.m_destBase = sFullDest;
			if (m_pFrame->GetRecursive())
			{
				if (!di.left.path.empty())
				{
					sFullDest += di.left.path;
					sFullDest += _T("\\");
				}
			}
			sFullDest += di.left.filename;
			act.dest = sFullDest;

			act.src = slFile;
			act.dirflag = di.diffcode.isDirectory();
			act.context = sel;
			act.atype = FileAction::ACT_MOVE;
			act.UIOrigin = FileActionItem::UI_LEFT;
			act.UIResult = FileActionItem::UI_DEL_LEFT;
			actionScript.AddActionItem(act);
			++selCount;
		}
	}
	// Now we prompt, and execute actions
	ConfirmAndPerformActions(actionScript, selCount);
}

/**
 * @brief Move selected right-side files to user-specified directory
 *
 * When moving files from recursive compare file subdirectory is also
 * read so directory structure is preserved.
 */
void CDirView::DoMoveRightTo()
{
	String destPath;
	if (!SelectFolder(m_hWnd, destPath, m_lastCopyFolder.c_str(), IDS_SELECT_DEST_RIGHT))
		return;
	m_lastCopyFolder = destPath;

	WaitStatusCursor waitstatus(IDS_STATUS_MOVEFILES);

	FileActionScript actionScript;
	int selCount = 0;
	int sel = -1;
	String slFile, srFile;
	while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
	{
		const DIFFITEM& di = GetDiffItem(sel);
		if (di.diffcode.diffcode != 0 && IsItemCopyableToOnRight(di) && IsItemDeletableOnRight(di))
		{
			GetItemFileNames(sel, slFile, srFile);

			// We must check that path still exists
			if (!CheckPathsExist(srFile.c_str()))
				return;

			FileActionItem act;
			String sFullDest = destPath + _T("\\");
			actionScript.m_destBase = sFullDest;
			if (m_pFrame->GetRecursive())
			{
				if (!di.right.path.empty())
				{
					sFullDest += di.right.path;
					sFullDest += _T("\\");
				}
			}
			sFullDest += di.right.filename;
			act.dest = sFullDest;

			act.src = srFile;
			act.dirflag = di.diffcode.isDirectory();
			act.context = sel;
			act.atype = FileAction::ACT_MOVE;
			act.UIOrigin = FileActionItem::UI_RIGHT;
			act.UIResult = FileActionItem::UI_DEL_RIGHT;
			actionScript.AddActionItem(act);
			++selCount;
		}
	}
	// Now we prompt, and execute actions
	ConfirmAndPerformActions(actionScript, selCount);
}

// Confirm with user, then perform the action list
void CDirView::ConfirmAndPerformActions(FileActionScript & actionList, int selCount)
{
	if (selCount == 0) // Not sure it is possible to get right-click menu without
		return;    // any selected items, but may as well be safe

	ASSERT(actionList.GetActionItemCount() > 0); // Or else the update handler got it wrong

	if (!ConfirmActionList(actionList, selCount))
		return;

	PerformActionList(actionList);
}

/**
 * @brief Confirm actions with user as appropriate
 * (type, whether single or multiple).
 */
BOOL CDirView::ConfirmActionList(const FileActionScript & actionList, int selCount)
{
	// TODO: We need better confirmation for file actions.
	// Maybe we should show a list of files with actions done..
	FileActionItem item = actionList.GetHeadActionItem();

	BOOL bDestIsSide = TRUE;

	// special handling for the single item case, because it is probably the most common,
	// and we can give the user exact details easily for it
	switch (item.atype)
	{
	case FileAction::ACT_COPY:
		if (item.UIResult == FileActionItem::UI_DONT_CARE)
			bDestIsSide = FALSE;

		if (actionList.GetActionItemCount() == 1)
		{
			if (!ConfirmCopy(item.UIOrigin, item.UIDestination,
				actionList.GetActionItemCount(), item.src.c_str(), item.dest.c_str(),
				bDestIsSide))
			{
				return FALSE;
			}
		}
		else
		{
			String src;
			String dst;

			if (item.UIOrigin == FileActionItem::UI_LEFT)
				src = m_pFrame->GetLeftBasePath();
			else
				src = m_pFrame->GetRightBasePath();

			if (bDestIsSide)
			{
				if (item.UIDestination == FileActionItem::UI_LEFT)
					dst = m_pFrame->GetLeftBasePath();
				else
					dst = m_pFrame->GetRightBasePath();
			}
			else
			{
				if (!actionList.m_destBase.empty())
					dst = actionList.m_destBase;
				else
					dst = item.dest;
			}

			if (!ConfirmCopy(item.UIOrigin, item.UIDestination,
				actionList.GetActionItemCount(), src.c_str(), dst.c_str(), bDestIsSide))
			{
				return FALSE;
			}
		}
		break;
		
	case FileAction::ACT_DEL:
		break;

	case FileAction::ACT_MOVE:
		bDestIsSide = FALSE;
		if (actionList.GetActionItemCount() == 1)
		{
			if (!ConfirmMove(item.UIOrigin, item.UIDestination,
				actionList.GetActionItemCount(), item.src.c_str(), item.dest.c_str(),
				bDestIsSide))
			{
				return FALSE;
			}
		}
		else
		{
			String src;
			String dst;

			if (item.UIOrigin == FileActionItem::UI_LEFT)
				src = m_pFrame->GetLeftBasePath();
			else
				src = m_pFrame->GetRightBasePath();

			if (!actionList.m_destBase.empty())
				dst = actionList.m_destBase;
			else
				dst = item.dest;

			if (!ConfirmMove(item.UIOrigin, item.UIDestination,
				actionList.GetActionItemCount(), src.c_str(), dst.c_str(), bDestIsSide))
			{
				return FALSE;
			}
		}
		break;

	// Invalid operation
	default: 
		LogErrorString(_T("Unknown fileoperation in CDirView::ConfirmActionList()"));
		_RPTF0(_CRT_ERROR, "Unknown fileoperation in CDirView::ConfirmActionList()");
		break;
	}
	return TRUE;
}

/**
 * @brief Perform an array of actions
 * @note There can be only COPY or DELETE actions, not both!
 * @sa CMainFrame::SaveToVersionControl()
 * @sa CMainFrame::SyncFilesToVCS()
 */
void CDirView::PerformActionList(FileActionScript & actionScript)
{
	// Reset suppressing VSS dialog for multiple files.
	// Set in CMainFrame::SaveToVersionControl().
	m_pFrame->m_pMDIFrame->m_CheckOutMulti = FALSE;
	m_pFrame->m_pMDIFrame->m_bVssSuppressPathCheck = false;

	theApp.AddOperation();
	try
	{
		const FILEOP_FLAGS operFlags = COptionsMgr::Get(OPT_USE_RECYCLE_BIN) ? FOF_ALLOWUNDO : 0;
		if (actionScript.Run(m_hWnd, operFlags))
			UpdateAfterFileScript(actionScript);
	}
	catch (OException *e)
	{
		e->ReportError(m_hWnd);
		delete e;
	}
	theApp.RemoveOperation();
}

/**
 * @brief Update results after running FileActionScript.
 * This functions is called after script is finished to update
 * results (including UI).
 * @param [in] actionlist Script that was run.
 */
void CDirView::UpdateAfterFileScript(FileActionScript &actionList)
{
	int curSel = GetFirstSelectedInd();
	while (actionList.GetActionItemCount() > 0)
	{
		// Start handling from tail of list, so removing items
		// doesn't invalidate our item indexes.
		FileActionItem act = actionList.RemoveTailActionItem();
		// Update item
		m_pFrame->UpdateDiffAfterOperation(act);
	}
	
	// Make sure selection is at sensible place if all selected items
	// were removed.
	UINT selected = GetSelectedCount();
	if (selected == 0)
	{
		if (curSel < 1)
			++curSel;
		MoveFocus(0, curSel - 1, selected);
	}
}

/// is it possible to copy item to left ?
BOOL CDirView::IsItemCopyableToLeft(const DIFFITEM & di)
{
	// don't let them mess with error items
	if (di.diffcode.isResultError()) return FALSE;
	// can't copy same items
	if (di.diffcode.isResultSame()) return FALSE;
	// impossible if only on left
	if (di.diffcode.isSideLeftOnly()) return FALSE;

	// everything else can be copied to left
	return TRUE;
}
/// is it possible to copy item to right ?
BOOL CDirView::IsItemCopyableToRight(const DIFFITEM & di)
{
	// don't let them mess with error items
	if (di.diffcode.isResultError()) return FALSE;
	// can't copy same items
	if (di.diffcode.isResultSame()) return FALSE;
	// impossible if only on right
	if (di.diffcode.isSideRightOnly()) return FALSE;

	// everything else can be copied to right
	return TRUE;
}
/// is it possible to delete left item ?
BOOL CDirView::IsItemDeletableOnLeft(const DIFFITEM & di)
{
	// don't let them mess with error items
	if (di.diffcode.isResultError()) return FALSE;
	// impossible if only on right
	if (di.diffcode.isSideRightOnly()) return FALSE;
	// everything else can be deleted on left
	return TRUE;
}
/// is it possible to delete right item ?
BOOL CDirView::IsItemDeletableOnRight(const DIFFITEM & di)
{
	// don't let them mess with error items
	if (di.diffcode.isResultError()) return FALSE;
	// impossible if only on right
	if (di.diffcode.isSideLeftOnly()) return FALSE;

	// everything else can be deleted on right
	return TRUE;
}
/// is it possible to delete both items ?
BOOL CDirView::IsItemDeletableOnBoth(const DIFFITEM & di)
{
	// don't let them mess with error items
	if (di.diffcode.isResultError()) return FALSE;
	// impossible if only on right or left
	if (di.diffcode.isSideLeftOnly() || di.diffcode.isSideRightOnly()) return FALSE;

	// everything else can be deleted on both
	return TRUE;
}

/**
 * @brief Determine if item can be opened.
 * Basically we only disable opening unique files at the moment.
 * Unique folders can be opened since we ask for creating matching folder
 * to another side.
 * @param [in] di DIFFITEM for item to check.
 * @return TRUE if the item can be opened, FALSE otherwise.
 */
BOOL CDirView::IsItemOpenable(const DIFFITEM & di) const
{
	if (m_bTreeMode && m_pFrame->GetRecursive())
	{
		if (di.diffcode.isDirectory() ||
			(di.diffcode.isSideRightOnly() || di.diffcode.isSideLeftOnly()))
		{
			return FALSE;
		}
	}
	else 
	{
		if (!di.diffcode.isDirectory() &&
			(di.diffcode.isSideRightOnly() || di.diffcode.isSideLeftOnly()))
		{
			return FALSE;
		}
	}
	return TRUE;
}
/// is it possible to compare these two items?
BOOL CDirView::AreItemsOpenable(const DIFFITEM & di1, const DIFFITEM & di2) const
{
	String sLeftBasePath = m_pFrame->GetLeftBasePath();
	String sRightBasePath = m_pFrame->GetRightBasePath();

	// Must be both directory or neither
	if (di1.diffcode.isDirectory() != di2.diffcode.isDirectory()) return FALSE;

	// Must be on different sides, or one on one side & one on both
	if (di1.diffcode.isSideLeftOnly() && (di2.diffcode.isSideRightOnly() ||
			di2.diffcode.isSideBoth()))
		return TRUE;
	if (di1.diffcode.isSideRightOnly() && (di2.diffcode.isSideLeftOnly() ||
			di2.diffcode.isSideBoth()))
		return TRUE;
	if (di1.diffcode.isSideBoth() && (di2.diffcode.isSideLeftOnly() ||
			di2.diffcode.isSideRightOnly()))
		return TRUE;

	// Allow to compare items if left & right path refer to same directory
	// (which means there is effectively two files involved). No need to check
	// side flags. If files weren't on both sides, we'd have no DIFFITEMs.
	if (lstrcmpi(sLeftBasePath.c_str(), sRightBasePath.c_str()) == 0)
		return TRUE;

	return FALSE;
}
/// is it possible to open left item ?
BOOL CDirView::IsItemOpenableOnLeft(const DIFFITEM & di) const
{
	// impossible if only on right
	if (di.diffcode.isSideRightOnly()) return FALSE;

	// everything else can be opened on right
	return TRUE;
}
/// is it possible to open right item ?
BOOL CDirView::IsItemOpenableOnRight(const DIFFITEM & di) const
{
	// impossible if only on left
	if (di.diffcode.isSideLeftOnly()) return FALSE;

	// everything else can be opened on left
	return TRUE;
}
/// is it possible to open left ... item ?
BOOL CDirView::IsItemOpenableOnLeftWith(const DIFFITEM & di) const
{
	return (!di.diffcode.isDirectory() && IsItemOpenableOnLeft(di));
}
/// is it possible to open with ... right item ?
BOOL CDirView::IsItemOpenableOnRightWith(const DIFFITEM & di) const
{
	return (!di.diffcode.isDirectory() && IsItemOpenableOnRight(di));
}
/// is it possible to copy to... left item?
BOOL CDirView::IsItemCopyableToOnLeft(const DIFFITEM & di)
{
	// impossible if only on right
	if (di.diffcode.isSideRightOnly()) return FALSE;

	// everything else can be copied to from left
	return TRUE;
}
/// is it possible to copy to... right item?
BOOL CDirView::IsItemCopyableToOnRight(const DIFFITEM & di)
{
	// impossible if only on left
	if (di.diffcode.isSideLeftOnly()) return FALSE;

	// everything else can be copied to from right
	return TRUE;
}

/// get file name on specified side for first selected item
String CDirView::GetSelectedFileName(SIDE_TYPE stype)
{
	String left, right;
	int sel = GetNextItem(-1, LVNI_SELECTED);
	if (sel != -1 && GetNextItem(sel, LVNI_SELECTED) == -1)
		GetItemFileNames(sel, left, right);
	return stype == SIDE_LEFT ? left : right;
}
/**
 * @brief Get the file names on both sides for specified item.
 * @note Return empty strings if item is special item.
 */
void CDirView::GetItemFileNames(int sel, String& strLeft, String& strRight)
{
	UINT_PTR diffpos = GetItemKey(sel);
	if (diffpos == SPECIAL_ITEM_POS)
	{
		strLeft.clear();
		strRight.clear();
	}
	else
	{
		const DIFFITEM &di = m_pFrame->GetDiffByKey(diffpos);
		const String leftrelpath = paths_ConcatPath(di.left.path, di.left.filename);
		const String rightrelpath = paths_ConcatPath(di.right.path, di.right.filename);
		const String &leftpath = m_pFrame->GetLeftBasePath();
		const String &rightpath = m_pFrame->GetRightBasePath();
		strLeft = paths_ConcatPath(leftpath, leftrelpath);
		strRight = paths_ConcatPath(rightpath, rightrelpath);
	}
}

/**
 * @brief Open selected file with registered application.
 * Uses shell file associations to open file with registered
 * application. We first try to use "Edit" action which should
 * open file to editor, since we are more interested editing
 * files than running them (scripts).
 * @param [in] stype Side of file to open.
 */
void CDirView::DoOpen(SIDE_TYPE stype)
{
	String file = GetSelectedFileName(stype);
	if (file.empty()) return;
	int rtn = (int)ShellExecute(::GetDesktopWindow(), _T("edit"), file.c_str(), 0, 0, SW_SHOWNORMAL);
	if (rtn==SE_ERR_NOASSOC)
		rtn = (int)ShellExecute(::GetDesktopWindow(), _T("open"), file.c_str(), 0, 0, SW_SHOWNORMAL);
	if (rtn==SE_ERR_NOASSOC)
		DoOpenWith(stype);
}

/// Open with dialog for file on selected side
void CDirView::DoOpenWith(SIDE_TYPE stype)
{
	String file = GetSelectedFileName(stype);
	if (!file.empty())
		m_pFrame->m_pMDIFrame->OpenFileWith(file.c_str());
}

/// Open selected file  on specified side to external editor
void CDirView::DoOpenWithEditor(SIDE_TYPE stype)
{
	String file = GetSelectedFileName(stype);
	if (!file.empty())
		m_pFrame->m_pMDIFrame->OpenFileToExternalEditor(file.c_str());
}

/**
 * @brief Mark selected items as needing for rescan.
 * @return Count of items to rescan.
 */
UINT CDirView::MarkSelectedForRescan()
{
	int sel = -1;
	int items = 0;
	while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
	{
		// Don't try to rescan special items
		if (GetItemKey(sel) == SPECIAL_ITEM_POS)
			continue;

		DIFFITEM &di = GetDiffItem(sel);
		di.diffcode.diffcode &= ~(DIFFCODE::TEXTFLAGS | DIFFCODE::SIDEFLAGS | DIFFCODE::COMPAREFLAGS | DIFFCODE::SCANFLAGS);
		di.diffcode.diffcode |= DIFFCODE::NEEDSCAN;
		++items;
	}
	return items;
}

/**
 * @brief Return string such as "15 of 30 Files Affected" or "30 Files Affected"
 */
static String FormatFilesAffectedString(int n1, int n2)
{
	String str = LanguageSelect.LoadString(IDS_FILES_AFFECTED_FMT);
	String::size_type pct1 = str.find(_T("%1"));
	String::size_type pct2 = str.find(_T("%2"));
	if ((n1 == n2) && (pct1 != String::npos) && (pct2 != String::npos))
	{
		assert(pct2 > pct1); // rethink if that happens...
		str.erase(pct1 + 2, pct2 - pct1);
		pct2 = String::npos;
	}
	if (pct2 != String::npos)
		str.replace(pct2, 2, NumToStr(n2).c_str());
	if (pct1 != String::npos)
		str.replace(pct1, 2, NumToStr(n1).c_str());
	return str;
}

/**
 * @brief Count left & right files, and number with editable text encoding
 * @param nLeft [out]  #files on left side selected
 * @param nLeftAffected [out]  #files on left side selected which can have text encoding changed
 * @param nRight [out]  #files on right side selected
 * @param nRightAffected [out]  #files on right side selected which can have text encoding changed
 *
 * Affected files include all except unicode files
 */
void CDirView::FormatEncodingDialogDisplays(CLoadSaveCodepageDlg &dlg)
{
	IntToIntMap currentCodepages;
	int nLeft=0, nLeftAffected=0, nRight=0, nRightAffected=0;
	int i = -1;
	while ((i = GetNextItem(i, LVNI_SELECTED)) != -1)
	{
		const DIFFITEM& di = GetDiffItem(i);
		if (di.diffcode.diffcode == 0) // Invalid value, this must be special item
			continue;
		if (di.diffcode.isDirectory())
			continue;

		if (di.diffcode.isSideLeftOrBoth())
		{
			// exists on left
			++nLeft;
			if (di.left.IsEditableEncoding())
				++nLeftAffected;
			int codepage = di.left.encoding.m_codepage;
			currentCodepages.Increment(codepage);
		}
		if (di.diffcode.isSideRightOrBoth())
		{
			++nRight;
			if (di.right.IsEditableEncoding())
				++nRightAffected;
			int codepage = di.right.encoding.m_codepage;
			currentCodepages.Increment(codepage);
		}
	}

	// Format strings such as "25 of 30 Files Affected"
	dlg.m_sAffectsLeftString = FormatFilesAffectedString(nLeftAffected, nLeft);
	dlg.m_sAffectsRightString = FormatFilesAffectedString(nRightAffected, nRight);
	int codepage = currentCodepages.FindMaxKey();
	dlg.m_nLoadCodepage = codepage;
	dlg.m_nSaveCodepage = codepage;
}

/**
 * @brief Display file encoding dialog to user & handle user's choices
 *
 * This handles DirView invocation, so multiple files may be affected
 */
void CDirView::DoFileEncodingDialog()
{
	CLoadSaveCodepageDlg dlg;
	// set up labels about what will be affected
	FormatEncodingDialogDisplays(dlg);
	dlg.m_bEnableSaveCodepage = false; // disallow setting a separate codepage for saving

	// Invoke dialog
	int rtn = LanguageSelect.DoModal(dlg);
	if (rtn != IDOK)
		return;

	int i = -1;
	while ((i = GetNextItem(i, LVNI_SELECTED)) != -1)
	{
		DIFFITEM &di = GetDiffItem(i);
		if (di.diffcode.diffcode == 0) // Invalid value, this must be special item
			continue;
		if (di.diffcode.isDirectory())
			continue;

		// Does it exist on left? (ie, right or both)
		if (dlg.m_bAffectsLeft && di.diffcode.isSideLeftOrBoth() && di.left.IsEditableEncoding())
		{
			di.left.encoding.SetCodepage(dlg.m_nLoadCodepage);
		}
		// Does it exist on right (ie, left or both)
		if (dlg.m_bAffectsRight && di.diffcode.isSideRightOrBoth() && di.right.IsEditableEncoding())
		{
			di.right.encoding.SetCodepage(dlg.m_nLoadCodepage);
		}
	}
	InvalidateRect(NULL);
	UpdateWindow();

	// TODO: We could loop through any active merge windows belonging to us
	// and see if any of their files are affected
	// but, if they've been edited, we cannot throw away the user's work?
}

/**
 * @brief Rename a file without moving it to different directory.
 *
 * @param szOldFileName [in] Full path of file to rename.
 * @param szNewFileName [in] New file name (without the path).
 *
 * @return TRUE if file was renamed successfully.
 */
bool CDirView::RenameOnSameDir(LPCTSTR szOldFileName, LPCTSTR szNewFileName)
{
	ASSERT(NULL != szOldFileName);
	ASSERT(NULL != szNewFileName);

	bool bSuccess = false;

	if (DOES_NOT_EXIST != paths_DoesPathExist(szOldFileName))
	{
		String sFullName;

		SplitFilename(szOldFileName, &sFullName, NULL, NULL);
		sFullName = paths_ConcatPath(sFullName, szNewFileName);

		// No need to rename if new file already exist.
		if ((sFullName.compare(szOldFileName)) ||
			(DOES_NOT_EXIST == paths_DoesPathExist(sFullName.c_str())))
		{
			ShellFileOperations fileOp(m_hWnd, FO_RENAME, 0,
				_tcslen(szOldFileName) + 1, sFullName.length() + 1);
			fileOp.AddSource(szOldFileName);
			fileOp.AddDestination(sFullName.c_str());
			bSuccess = fileOp.Run();
		}
		else
		{
			bSuccess = true;
		}
	}

	return bSuccess;
}

/**
 * @brief Rename selected item on both left and right sides.
 *
 * @param szNewItemName [in] New item name.
 *
 * @return TRUE if at least one file was renamed successfully.
 */
bool CDirView::DoItemRename(int iItem, LPCTSTR szNewItemName)
{
	ASSERT(NULL != szNewItemName);
	
	String sLeftFile, sRightFile;
	GetItemFileNames(iItem, sLeftFile, sRightFile);

	// We must check that paths still exists
	String failpath;
	DIFFITEM &di = GetDiffItem(iItem);
	if (di.diffcode.isSideLeftOrBoth() && !CheckPathsExist(sLeftFile.c_str()) ||
		di.diffcode.isSideRightOrBoth() && !CheckPathsExist(sRightFile.c_str()))
	{
		return FALSE;
	}

	bool bRenameLeft = false;
	bool bRenameRight = false;
	if (di.diffcode.isSideLeftOrBoth())
		bRenameLeft = RenameOnSameDir(sLeftFile.c_str(), szNewItemName);
	if (di.diffcode.isSideRightOrBoth())
		bRenameRight = RenameOnSameDir(sRightFile.c_str(), szNewItemName);

	if (bRenameLeft && bRenameRight)
	{
		di.left.filename = szNewItemName;
		di.right.filename = szNewItemName;
	}
	else if (bRenameLeft)
	{
		di.left.filename = szNewItemName;
		di.right.filename.clear();
	}
	else if (bRenameRight)
	{
		di.left.filename.clear();
		di.right.filename = szNewItemName;
	}

	return bRenameLeft || bRenameRight;
}
