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
#include "LogFile.h"
#include "FileActionScript.h"
#include "LoadSaveCodepageDlg.h"
#include "IntToIntMap.h"
#include "FileOrFolderSelect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static bool CheckPathsExist(LPCTSTR path, int allow = IS_EXISTING);

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
			paths_UndoMagic(wcsdupa(path)), MB_ICONWARNING);
		return false;
	}
	return true;
}

/// Prompt & copy item from right to left, if legal
void CDirView::DoCopyRightToLeft()
{
	WaitStatusCursor waitstatus(IDS_STATUS_COPYFILES);
	// First we build a list of desired actions
	const CDiffContext *ctxt = m_pFrame->GetDiffContext();
	FileActionScript actionScript;
	int sel = -1;
	while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
	{
		const DIFFITEM *di = GetDiffItem(sel);
		if (di->isSideRightOrBoth())
		{
			FileActionItem act;
			GetItemFileNames(di, act.dest, act.src);
			
			// We must check that paths still exists
			if (!CheckPathsExist(act.src.c_str()))
				return;

			act.context = sel;
			act.dirflag = di->isDirectory();
			act.atype = FileAction::ACT_COPY;
			act.UIResult = FileActionItem::UI_SYNC;
			act.UIOrigin = FileActionItem::UI_RIGHT;
			act.UIDestination = FileActionItem::UI_LEFT;
			actionScript.AddActionItem(act);
		}
	}
	// Now we prompt, and execute actions
	PerformActionList(actionScript);
}

/// Prompt & copy item from left to right, if legal
void CDirView::DoCopyLeftToRight()
{
	WaitStatusCursor waitstatus(IDS_STATUS_COPYFILES);
	// First we build a list of desired actions
	const CDiffContext *ctxt = m_pFrame->GetDiffContext();
	FileActionScript actionScript;
	int sel = -1;
	while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
	{
		const DIFFITEM *di = GetDiffItem(sel);
		if (di->isSideLeftOrBoth())
		{
			FileActionItem act;
			GetItemFileNames(di, act.src, act.dest);

			// We must first check that paths still exists
			if (!CheckPathsExist(act.src.c_str()))
				return;

			act.context = sel;
			act.dirflag = di->isDirectory();
			act.atype = FileAction::ACT_COPY;
			act.UIResult = FileActionItem::UI_SYNC;
			act.UIOrigin = FileActionItem::UI_LEFT;
			act.UIDestination = FileActionItem::UI_RIGHT;
			actionScript.AddActionItem(act);
		}
	}
	// Now we prompt, and execute actions
	PerformActionList(actionScript);
}

/// Prompt & delete left, if legal
void CDirView::DoDelLeft()
{
	WaitStatusCursor waitstatus(IDS_STATUS_DELETEFILES);
	// First we build a list of desired actions
	FileActionScript actionScript;
	int sel = -1;
	const CDiffContext *ctxt = m_pFrame->GetDiffContext();
	while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
	{
		const DIFFITEM *di = GetDiffItem(sel);
		if (di->isDeletableOnLeft())
		{
			FileActionItem act;
			act.src = ctxt->GetLeftFilepathAndName(di);

			// We must check that path still exists
			if (!CheckPathsExist(act.src.c_str()))
				return;

			act.context = sel;
			act.dirflag = di->isDirectory();
			act.atype = FileAction::ACT_DEL;
			act.UIResult = FileActionItem::UI_DEL_LEFT;
			actionScript.AddActionItem(act);
		}
	}
	// Now we prompt, and execute actions
	PerformActionList(actionScript);
}

/// Prompt & delete right, if legal
void CDirView::DoDelRight()
{
	WaitStatusCursor waitstatus(IDS_STATUS_DELETEFILES);
	// First we build a list of desired actions
	const CDiffContext *ctxt = m_pFrame->GetDiffContext();
	FileActionScript actionScript;
	int sel = -1;
	while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
	{
		const DIFFITEM *di = GetDiffItem(sel);
		if (di->isDeletableOnRight())
		{
			FileActionItem act;
			act.src = ctxt->GetRightFilepathAndName(di);

			// We must first check that path still exists
			if (!CheckPathsExist(act.src.c_str()))
				return;

			act.context = sel;
			act.dirflag = di->isDirectory();
			act.atype = FileAction::ACT_DEL;
			act.UIResult = FileActionItem::UI_DEL_RIGHT;
			actionScript.AddActionItem(act);
		}
	}
	// Now we prompt, and execute actions
	PerformActionList(actionScript);
}

/**
 * @brief Prompt & delete both, if legal.
 */
void CDirView::DoDelBoth()
{
	WaitStatusCursor waitstatus(IDS_STATUS_DELETEFILES);
	// First we build a list of desired actions
	FileActionScript actionScript;
	int sel = -1;
	while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
	{
		const DIFFITEM *di = GetDiffItem(sel);
		if (di->isDeletableOnBoth())
		{
			FileActionItem act;
			GetItemFileNames(di, act.dest, act.src);

			// We must first check that paths still exists
			if (!CheckPathsExist(act.src.c_str()) ||
				!CheckPathsExist(act.dest.c_str()))
			{
				return;
			}

			act.context = sel;
			act.dirflag = di->isDirectory();
			act.atype = FileAction::ACT_DEL;
			act.UIResult = FileActionItem::UI_DEL_BOTH;
			actionScript.AddActionItem(act);
		}
	}
	// Now we prompt, and execute actions
	PerformActionList(actionScript);
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
	int sel = -1;
	String slFile, srFile;
	const BOOL leftRO = m_pFrame->GetLeftReadOnly();
	const BOOL rightRO = m_pFrame->GetRightReadOnly();
	while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
	{
		const DIFFITEM *di = GetDiffItem(sel);
		if (di == NULL)
			break;
		GetItemFileNames(di, slFile, srFile);

		int leftFlags = DOES_NOT_EXIST;
		int rightFlags = DOES_NOT_EXIST;
		FileActionItem act;
		if (di->isDeletableOnBoth() && !leftRO && !rightRO)
		{
			leftFlags = IS_EXISTING;
			rightFlags = IS_EXISTING;
			act.src = srFile;
			act.dest = slFile;
			act.UIResult = FileActionItem::UI_DEL_BOTH;
		}
		else if (di->isDeletableOnLeft() && !leftRO)
		{
			leftFlags = IS_EXISTING;
			act.src = slFile;
			act.UIResult = FileActionItem::UI_DEL_LEFT;
		}
		else if (di->isDeletableOnRight() && !rightRO)
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

			act.dirflag = di->isDirectory();
			act.context = sel;
			act.atype = FileAction::ACT_DEL;
			actionScript.AddActionItem(act);
		}
	}
	// Now we prompt, and execute actions
	PerformActionList(actionScript);
}

/**
 * @brief Copy selected left-side files to user-specified directory
 *
 * When copying files from recursive compare file subdirectory is also
 * read so directory structure is preserved.
 */
void CDirView::DoCopyLeftTo()
{
	if (!SelectFolder(m_hWnd, m_lastCopyFolder, IDS_SELECT_DEST_LEFT))
		return;

	WaitStatusCursor waitstatus(IDS_STATUS_COPYFILES);

	FileActionScript actionScript;
	actionScript.m_destBase = m_lastCopyFolder + _T('\\');
	int sel = -1;
	String slFile, srFile;
	while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
	{
		const DIFFITEM *di = GetDiffItem(sel);
		if (di->isSideLeftOrBoth())
		{
			GetItemFileNames(di, slFile, srFile);

			// We must check that path still exists
			if (!CheckPathsExist(slFile.c_str()))
				return;

			FileActionItem act;
			act.dest = actionScript.m_destBase;
			if (!di->left.path.empty())
			{
				act.dest += di->left.path;
				act.dest += _T('\\');
			}
			act.dest += di->left.filename;
			act.src = slFile;
			act.dirflag = di->isDirectory();
			act.context = sel;
			act.atype = FileAction::ACT_COPY;
			act.UIResult = FileActionItem::UI_DONT_CARE;
			act.UIOrigin = FileActionItem::UI_LEFT;
			actionScript.AddActionItem(act);
		}
	}
	// Now we prompt, and execute actions
	PerformActionList(actionScript);
}

/**
 * @brief Copy selected righ-side files to user-specified directory
 *
 * When copying files from recursive compare file subdirectory is also
 * read so directory structure is preserved.
 */
void CDirView::DoCopyRightTo()
{
	if (!SelectFolder(m_hWnd, m_lastCopyFolder, IDS_SELECT_DEST_RIGHT))
		return;

	WaitStatusCursor waitstatus(IDS_STATUS_COPYFILES);

	FileActionScript actionScript;
	actionScript.m_destBase = m_lastCopyFolder + _T('\\');
	int sel = -1;
	String slFile, srFile;
	while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
	{
		const DIFFITEM *di = GetDiffItem(sel);
		if (di->isSideRightOrBoth())
		{
			GetItemFileNames(di, slFile, srFile);

			// We must check that path still exists
			if (!CheckPathsExist(srFile.c_str()))
				return;

			FileActionItem act;
			act.dest = actionScript.m_destBase;
			if (!di->right.path.empty())
			{
				act.dest += di->right.path;
				act.dest += _T('\\');
			}
			act.dest += di->right.filename;
			act.src = srFile;
			act.dirflag = di->isDirectory();
			act.context = sel;
			act.atype = FileAction::ACT_COPY;
			act.UIResult = FileActionItem::UI_DONT_CARE;
			act.UIOrigin = FileActionItem::UI_RIGHT;
			actionScript.AddActionItem(act);
		}
	}
	// Now we prompt, and execute actions
	PerformActionList(actionScript);
}

/**
 * @brief Move selected left-side files to user-specified directory
 *
 * When moving files from recursive compare file subdirectory is also
 * read so directory structure is preserved.
 */
void CDirView::DoMoveLeftTo()
{
	if (!SelectFolder(m_hWnd, m_lastCopyFolder, IDS_SELECT_DEST_LEFT))
		return;

	WaitStatusCursor waitstatus(IDS_STATUS_MOVEFILES);

	FileActionScript actionScript;
	actionScript.m_destBase = m_lastCopyFolder + _T('\\');
	int sel = -1;
	String slFile, srFile;
	while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
	{
		const DIFFITEM *di = GetDiffItem(sel);
		if (di->isDeletableOnLeft())
		{
			GetItemFileNames(di, slFile, srFile);

			// We must check that path still exists
			if (!CheckPathsExist(slFile.c_str()))
				return;

			FileActionItem act;
			act.dest = actionScript.m_destBase;
			if (!di->left.path.empty())
			{
				act.dest += di->left.path;
				act.dest += _T('\\');
			}
			act.dest += di->left.filename;
			act.src = slFile;
			act.dirflag = di->isDirectory();
			act.context = sel;
			act.atype = FileAction::ACT_MOVE;
			act.UIOrigin = FileActionItem::UI_LEFT;
			act.UIResult = FileActionItem::UI_DEL_LEFT;
			actionScript.AddActionItem(act);
		}
	}
	// Now we prompt, and execute actions
	PerformActionList(actionScript);
}

/**
 * @brief Move selected right-side files to user-specified directory
 *
 * When moving files from recursive compare file subdirectory is also
 * read so directory structure is preserved.
 */
void CDirView::DoMoveRightTo()
{
	if (!SelectFolder(m_hWnd, m_lastCopyFolder, IDS_SELECT_DEST_RIGHT))
		return;

	WaitStatusCursor waitstatus(IDS_STATUS_MOVEFILES);

	FileActionScript actionScript;
	actionScript.m_destBase = m_lastCopyFolder + _T('\\');
	int sel = -1;
	String slFile, srFile;
	while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
	{
		const DIFFITEM *di = GetDiffItem(sel);
		if (di->isDeletableOnRight())
		{
			GetItemFileNames(di, slFile, srFile);

			// We must check that path still exists
			if (!CheckPathsExist(srFile.c_str()))
				return;

			FileActionItem act;
			act.dest = actionScript.m_destBase;
			if (!di->right.path.empty())
			{
				act.dest += di->right.path;
				act.dest += _T('\\');
			}
			act.dest += di->right.filename;
			act.src = srFile;
			act.dirflag = di->isDirectory();
			act.context = sel;
			act.atype = FileAction::ACT_MOVE;
			act.UIOrigin = FileActionItem::UI_RIGHT;
			act.UIResult = FileActionItem::UI_DEL_RIGHT;
			actionScript.AddActionItem(act);
		}
	}
	// Now we prompt, and execute actions
	PerformActionList(actionScript);
}

/**
 * @brief Confirm actions with user as appropriate
 * (type, whether single or multiple).
 */
bool CDirView::ConfirmActionList(FileActionScript &actionList)
{
	// TODO: We need better confirmation for file actions.
	// Maybe we should show a list of files with actions done..
	const CDiffContext *ctxt = m_pFrame->GetDiffContext();
	FileActionItem item = actionList.GetHeadActionItem();

	bool bDestIsSide = true;

	// special handling for the single item case, because it is probably the most common,
	// and we can give the user exact details easily for it
	switch (item.atype)
	{
	case FileAction::ACT_COPY:
		if (item.UIResult == FileActionItem::UI_DONT_CARE)
			bDestIsSide = false;

		if (actionList.GetActionItemCount() == 1)
		{
			if (!actionList.ConfirmCopy(item.UIOrigin, item.UIDestination,
				item.src.c_str(), item.dest.c_str(), bDestIsSide))
			{
				return false;
			}
		}
		else
		{
			String src;
			String dst;

			if (item.UIOrigin == FileActionItem::UI_LEFT)
				src = ctxt->GetLeftPath();
			else
				src = ctxt->GetRightPath();

			if (bDestIsSide)
			{
				if (item.UIDestination == FileActionItem::UI_LEFT)
					dst = ctxt->GetLeftPath();
				else
					dst = ctxt->GetRightPath();
			}
			else
			{
				if (!actionList.m_destBase.empty())
					dst = actionList.m_destBase;
				else
					dst = item.dest;
			}

			if (!actionList.ConfirmCopy(item.UIOrigin, item.UIDestination,
				src.c_str(), dst.c_str(), bDestIsSide))
			{
				return false;
			}
		}
		break;
		
	case FileAction::ACT_DEL:
		break;

	case FileAction::ACT_MOVE:
		bDestIsSide = false;
		if (actionList.GetActionItemCount() == 1)
		{
			if (!actionList.ConfirmMove(item.UIOrigin, item.UIDestination,
				item.src.c_str(), item.dest.c_str(), bDestIsSide))
			{
				return false;
			}
		}
		else
		{
			String src;
			String dst;

			if (item.UIOrigin == FileActionItem::UI_LEFT)
				src = ctxt->GetLeftPath();
			else
				src = ctxt->GetRightPath();

			if (!actionList.m_destBase.empty())
				dst = actionList.m_destBase;
			else
				dst = item.dest;

			if (!actionList.ConfirmMove(item.UIOrigin, item.UIDestination,
				src.c_str(), dst.c_str(), bDestIsSide))
			{
				return false;
			}
		}
		break;

	// Invalid operation
	default: 
		LogErrorString(_T("Unknown fileoperation in CDirView::ConfirmActionList()"));
		_RPTF0(_CRT_ERROR, "Unknown fileoperation in CDirView::ConfirmActionList()");
		break;
	}
	return true;
}

/**
 * @brief Perform an array of actions
 * @note There can be only COPY or DELETE actions, not both!
 * @sa CMainFrame::SaveToVersionControl()
 * @sa CMainFrame::SyncFilesToVCS()
 */
void CDirView::PerformActionList(FileActionScript &actionScript)
{
	if (actionScript.GetActionItemCount() == 0)
		return;

	if (!ConfirmActionList(actionScript))
		return;

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
	int curSel = GetNextItem(-1, LVNI_SELECTED);
	while (actionList.GetActionItemCount() > 0)
	{
		// Start handling from tail of list, so removing items
		// doesn't invalidate our item indexes.
		FileActionItem act = actionList.RemoveTailActionItem();
		// Update item
		m_pFrame->UpdateDiffAfterOperation(act, actionList.m_bMakeTargetItemWritable);
	}
	
	// Make sure selection is at sensible place if all selected items
	// were removed.
	UINT selected = GetSelectedCount();
	if (selected == 0)
	{
		if (curSel > 0)
			--curSel;
		MoveFocus(0, curSel);
	}
}

/**
 * @brief Determine if item can be opened.
 * Basically we only disable opening unique files at the moment.
 * Unique folders can be opened since we ask for creating matching folder
 * to another side.
 * @param [in] di DIFFITEM for item to check.
 * @return TRUE if the item can be opened, FALSE otherwise.
 */
BOOL CDirView::IsItemOpenable(const DIFFITEM *di) const
{
	if (di == NULL)
		return FALSE;

	if (m_bTreeMode && m_pFrame->GetRecursive() == 1)
	{
		if (di->isDirectory() || (di->isSideRightOnly() || di->isSideLeftOnly()))
		{
			return FALSE;
		}
	}
	else 
	{
		if (!di->isDirectory() &&
			(di->isSideRightOnly() || di->isSideLeftOnly()))
		{
			return FALSE;
		}
	}
	return TRUE;
}
/// is it possible to compare these two items?
BOOL CDirView::AreItemsOpenable(const DIFFITEM *di1, const DIFFITEM *di2) const
{
	if (di1 == NULL || di2 == NULL)
		return FALSE;

	// Must be both directory or neither
	if (di1->isDirectory() != di2->isDirectory())
		return FALSE;

	// Must be on different sides, or one on one side & one on both
	if (di1->isSideLeftOnly() && di2->isSideRightOrBoth())
		return TRUE;
	if (di1->isSideRightOnly() && di2->isSideLeftOrBoth())
		return TRUE;
	if (di1->isSideBoth() && (di2->isSideLeftOnly() || di2->isSideRightOnly()))
		return TRUE;

	// Allow to compare items if left & right path refer to same directory
	// (which means there is effectively two files involved). No need to check
	// side flags. If files weren't on both sides, we'd have no DIFFITEMs.
	const CDiffContext *ctxt = m_pFrame->GetDiffContext();
	if (lstrcmpi(ctxt->GetLeftPath().c_str(), ctxt->GetRightPath().c_str()) == 0)
		return TRUE;

	return FALSE;
}

/// get file name on specified side for first selected item
String CDirView::GetSelectedFileName(SIDE_TYPE stype)
{
	String left, right;
	int sel = GetNextItem(-1, LVNI_SELECTED);
	if (sel != -1 && GetNextItem(sel, LVNI_SELECTED) == -1)
	{
		DIFFITEM *di = GetDiffItem(sel);
		GetItemFileNames(di, left, right);
	}
	return stype == SIDE_LEFT ? left : right;
}

/**
 * @brief Get the file names on both sides for specified item.
 * @note Return empty strings if item is special item.
 */
void CDirView::GetItemFileNames(const DIFFITEM *di, String &strLeft, String &strRight)
{
	if (di != NULL)
	{
		const CDiffContext *ctxt = m_pFrame->GetDiffContext();
		const String leftrelpath = paths_ConcatPath(di->left.path, di->left.filename);
		const String rightrelpath = paths_ConcatPath(di->right.path, di->right.filename);
		const String &leftpath = ctxt->GetLeftPath();
		const String &rightpath = ctxt->GetRightPath();
		strLeft = paths_ConcatPath(leftpath, leftrelpath);
		strRight = paths_ConcatPath(rightpath, rightrelpath);
	}
	else
	{
		strLeft.clear();
		strRight.clear();
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
	if (file.empty())
		return;
	int rtn = (int)ShellExecute(NULL, _T("edit"), file.c_str(), 0, 0, SW_SHOWNORMAL);
	if (rtn != SE_ERR_NOASSOC)
		return;
	rtn = (int)ShellExecute(NULL, _T("open"), file.c_str(), 0, 0, SW_SHOWNORMAL);
	if (rtn != SE_ERR_NOASSOC)
		return;
	DoOpenWith(stype);
}

/// Open with dialog for file on selected side
void CDirView::DoOpenWith(SIDE_TYPE stype)
{
	String file = GetSelectedFileName(stype);
	if (!file.empty())
		m_pFrame->m_pMDIFrame->OpenFileWith(file.c_str());
}

/// Open selected file on specified side to external editor
void CDirView::DoOpenWithEditor(SIDE_TYPE stype, LPCTSTR editor)
{
	String file = GetSelectedFileName(stype);
	if (!file.empty())
		m_pFrame->m_pMDIFrame->OpenFileToExternalEditor(file.c_str(), editor);
}

/// Open selected file on specified side to Frhed
void CDirView::DoOpenWithFrhed(SIDE_TYPE stype)
{
	String editor = GetModulePath() + _T("\\Frhed\\Frhed.exe");
	DoOpenWithEditor(stype, editor.c_str());
}

/**
 * @brief Mark selected items as needing for rescan.
 * @return Count of items to rescan.
 */
int CDirView::MarkSelectedForRescan()
{
	int sel = -1;
	int items = 0;
	while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
	{
		DIFFITEM *di = GetDiffItem(sel);
		// Don't try to rescan special items
		if (di == NULL)
			continue;

		di->diffcode &= ~(DIFFCODE::TEXTFLAGS | DIFFCODE::SIDEFLAGS | DIFFCODE::COMPAREFLAGS | DIFFCODE::SCANFLAGS);
		di->diffcode |= DIFFCODE::NEEDSCAN;
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
		const DIFFITEM *di = GetDiffItem(i);
		if (di == NULL) // Invalid value, this must be special item
			continue;
		if (di->isDirectory())
			continue;

		if (di->isSideLeftOrBoth())
		{
			// exists on left
			++nLeft;
			if (di->left.IsEditableEncoding())
				++nLeftAffected;
			int codepage = di->left.encoding.m_codepage;
			currentCodepages.Increment(codepage);
		}
		if (di->isSideRightOrBoth())
		{
			++nRight;
			if (di->right.IsEditableEncoding())
				++nRightAffected;
			int codepage = di->right.encoding.m_codepage;
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
	if (LanguageSelect.DoModal(dlg) != IDOK)
		return;

	int i = -1;
	while ((i = GetNextItem(i, LVNI_SELECTED)) != -1)
	{
		DIFFITEM *di = GetDiffItem(i);
		if (di == NULL) // Invalid value, this must be special item
			continue;
		if (di->isDirectory())
			continue;

		// Does it exist on left? (ie, right or both)
		if (dlg.m_bAffectsLeft && di->isSideLeftOrBoth() && di->left.IsEditableEncoding())
		{
			di->left.encoding.SetCodepage(dlg.m_nLoadCodepage);
		}
		// Does it exist on right (ie, left or both)
		if (dlg.m_bAffectsRight && di->isSideRightOrBoth() && di->right.IsEditableEncoding())
		{
			di->right.encoding.SetCodepage(dlg.m_nLoadCodepage);
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
		String sFullName = paths_GetParentPath(szOldFileName);
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

	// We must check that paths still exists
	String failpath;
	DIFFITEM *di = GetDiffItem(iItem);
	GetItemFileNames(di, sLeftFile, sRightFile);
	if (di->isSideLeftOrBoth() && !CheckPathsExist(sLeftFile.c_str()) ||
		di->isSideRightOrBoth() && !CheckPathsExist(sRightFile.c_str()))
	{
		return FALSE;
	}

	bool bRenameLeft = false;
	bool bRenameRight = false;
	if (di->isSideLeftOrBoth())
		bRenameLeft = RenameOnSameDir(sLeftFile.c_str(), szNewItemName);
	if (di->isSideRightOrBoth())
		bRenameRight = RenameOnSameDir(sRightFile.c_str(), szNewItemName);

	if (bRenameLeft && bRenameRight)
	{
		di->left.filename = szNewItemName;
		di->right.filename = szNewItemName;
	}
	else if (bRenameLeft)
	{
		di->left.filename = szNewItemName;
		di->right.filename.clear();
	}
	else if (bRenameRight)
	{
		di->left.filename.clear();
		di->right.filename = szNewItemName;
	}

	return bRenameLeft || bRenameRight;
}
