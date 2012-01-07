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
 * @file  DirView.cpp
 *
 * @brief Main implementation file for CDirView
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "Constants.h"
#include "Merge.h"
#include "SettingStore.h"
#include "LanguageSelect.h"
#include "DirView.h"
#include "DirFrame.h"  // StatePane
#include "HexMergeFrm.h"
#include "MainFrm.h"
#include "resource.h"
#include "coretools.h"
#include "FileTransform.h"
#include "paths.h"
#include "7zCommon.h"
#include "OptionsDef.h"
#include "DirCmpReport.h"
#include "DirCompProgressDlg.h"
#include "CompareStatisticsDlg.h"
#include "ShellContextMenu.h"
#include "PidlContainer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/**
 * @brief Limit (in seconds) to signal compare is ready for user.
 * If compare takes longer than this value (in seconds) we inform
 * user about it. Current implementation uses MessageBeep(IDOK).
 */
const int TimeToSignalCompare = 3;

/**
 * @brief Folder compare icon indexes.
 * This enum defines indexes for imagelist used for folder compare icons.
 * Note that this enum must be in synch with code in OnInitialUpdate() and
 * GetColImage(). Also remember that icons are in resource file...
 */
static enum
{
	DIFFIMG_LUNIQUE,
	DIFFIMG_RUNIQUE,
	DIFFIMG_DIFF,
	DIFFIMG_SAME,
	DIFFIMG_BINSAME,
	DIFFIMG_BINDIFF,
	DIFFIMG_LDIRUNIQUE,
	DIFFIMG_RDIRUNIQUE,
	DIFFIMG_SKIP,
	DIFFIMG_DIRSKIP,
	DIFFIMG_DIRDIFF,
	DIFFIMG_DIRSAME,
	DIFFIMG_DIR,
	DIFFIMG_ERROR,
	DIFFIMG_DIRUP,
	DIFFIMG_DIRUP_DISABLE,
	DIFFIMG_ABORT,
	DIFFIMG_TEXTDIFF,
	DIFFIMG_TEXTSAME,
	N_DIFFIMG
};

// The resource ID constants/limits for the Shell context menu
static const UINT LeftCmdFirst	= 0x9000;
static const UINT LeftCmdLast	= 0xAFFF;
static const UINT RightCmdFirst	= 0xB000;
static const UINT RightCmdLast	= 0xCFFF;

/////////////////////////////////////////////////////////////////////////////
// CDirView

HFont *CDirView::m_font = NULL;
HImageList *CDirView::m_imageList = NULL;
HImageList *CDirView::m_imageState = NULL;

CDirView::CDirView(CDirFrame *pFrame)
	: m_numcols(-1)
	, m_dispcols(-1)
	, m_pFrame(pFrame)
	, m_nHiddenItems(0)
	, m_pCmpProgressDlg(NULL)
	, m_compareStart(0)
	, m_bTreeMode(false)
	, m_pShellContextMenuLeft(new CShellContextMenu(LeftCmdFirst, LeftCmdLast))
	, m_hShellContextMenuLeft(NULL)
	, m_pShellContextMenuRight(new CShellContextMenu(RightCmdFirst, RightCmdLast))
	, m_hShellContextMenuRight(NULL)
	, m_pCompareAsScriptMenu(NULL)
{
	m_bTreeMode =  COptionsMgr::Get(OPT_TREE_MODE);
}

CDirView::~CDirView()
{
	delete m_pShellContextMenuRight;
	delete m_pShellContextMenuLeft;
	delete m_pCmpProgressDlg;
}

LRESULT CDirView::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		switch (HIWORD(wParam))
		{
		case EN_CHANGE:
			// Update the tool bar's "Undo" icon. It should be enabled when
			// renaming an item and undo is possible.
			GetMainFrame()->UpdateCmdUI<ID_EDIT_UNDO>(
				reinterpret_cast<HEdit *>(lParam)->CanUndo() ? MF_ENABLED : MF_GRAYED);
			break;
		case EN_KILLFOCUS:
			GetMainFrame()->UpdateCmdUI<ID_EDIT_UNDO>(MF_GRAYED);
			break;
		}
		break;
	case WM_NOTIFY:
		if (LRESULT lResult = OnNotify(lParam))
			return lResult;
		break;
	case WM_WINDOWPOSCHANGED:
		if ((reinterpret_cast<WINDOWPOS *>(lParam)->flags & SWP_NOSIZE) == 0)
			m_pFrame->AlignStatusBar(reinterpret_cast<WINDOWPOS *>(lParam)->cx);
		break;
	case WM_DESTROY:
		OnDestroy();
		break;
	case MSG_UI_UPDATE:
		OnUpdateUIMessage();
		return 0;
	case WM_DRAWITEM:
		if (reinterpret_cast<DRAWITEMSTRUCT *>(lParam)->CtlType == ODT_HEADER)
			m_ctlSortHeader.DrawItem(reinterpret_cast<DRAWITEMSTRUCT *>(lParam));
		return 0;
	}
	return OListView::WindowProc(uMsg, wParam, lParam);
}

void CDirView::AcquireSharedResources()
{
	// Load user-selected font
	m_font = HFont::CreateIndirect(&GetMainFrame()->m_lfDir);
	// Load the icons used for the list view (to reflect diff status)
	// NOTE: these must be in the exactly the same order than in enum
	// definition in begin of this file!
	const int iconCX = 16;
	const int iconCY = 16;
	m_imageList = HImageList::Create(iconCX, iconCY, ILC_COLOR32 | ILC_MASK, N_DIFFIMG, 0);
	static const WORD rgIDI[N_DIFFIMG] =
	{
		IDI_LFILE,
		IDI_RFILE,
		IDI_NOTEQUALFILE,
		IDI_EQUALFILE,
		IDI_EQUALBINARY,
		IDI_BINARYDIFF,
		IDI_LFOLDER,
		IDI_RFOLDER,
		IDI_FILESKIP,
		IDI_FOLDERSKIP,
		IDI_NOTEQUALFOLDER,
		IDI_EQUALFOLDER,
		IDI_FOLDER,
		IDI_COMPARE_ERROR,
		IDI_FOLDERUP,
		IDI_FOLDERUP_DISABLE,
		IDI_COMPARE_ABORTED,
		IDI_NOTEQUALTEXTFILE,
		IDI_EQUALTEXTFILE
	};
	int i = 0;
	do
	{
		VERIFY(-1 != m_imageList->Add(LanguageSelect.LoadIcon(rgIDI[i])));
	} while (++i < N_DIFFIMG);
	// Load the icons used for the list view (expanded/collapsed state icons)
	m_imageState = LanguageSelect.LoadImageList(IDB_TREE_STATE, 16);
}

void CDirView::ReleaseSharedResources()
{
	m_font->DeleteObject();
	m_font = NULL;
	m_imageList->Destroy();
	m_imageList = NULL;
	m_imageState->Destroy();
	m_imageState = NULL;
}

HFont *CDirView::ReplaceFont(const LOGFONT *lf)
{
	HFont *font = m_font;
	m_font = HFont::CreateIndirect(lf);
	return font;
}

void CDirView::UpdateFont()
{
	if (!OPT_FONT_DIRCMP_LOGFONT.IsDefault())
	{
		SetFont(m_font);
	}
	else
	{
		SetFont(NULL);
	}
}

void CDirView::OnInitialUpdate()
{
	// Load user-selected font
	UpdateFont();
	// Replace standard header with sort header
	if (HHeaderCtrl *pHeaderCtrl = GetHeaderCtrl())
		m_ctlSortHeader.SubclassWindow(pHeaderCtrl);

	SetImageList(m_imageList->m_hImageList, LVSIL_SMALL);

	// Restore column orders as they had them last time they ran
	LoadColumnOrders();

	// Display column headers (in appropriate order)
	ReloadColumns();

	// Show selection across entire row.
	// Also allow user to rearrange columns via drag&drop of headers.
	// Also enable infotips.
	DWORD exstyle = LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_INFOTIP;
	SetExtendedStyle(exstyle);
}

/**
 * @brief Return image index appropriate for this row
 */
int CDirView::GetColImage(const DIFFITEM & di) const
{
	// Must return an image index into image list created above in OnInitDialog
	if (di.diffcode.isResultError())
		return DIFFIMG_ERROR;
	if (di.diffcode.isResultAbort())
		return DIFFIMG_ABORT;
	if (di.diffcode.isResultFiltered())
		return (di.diffcode.isDirectory() ? DIFFIMG_DIRSKIP : DIFFIMG_SKIP);
	if (di.diffcode.isSideLeftOnly())
		return (di.diffcode.isDirectory() ? DIFFIMG_LDIRUNIQUE : DIFFIMG_LUNIQUE);
	if (di.diffcode.isSideRightOnly())
		return (di.diffcode.isDirectory() ? DIFFIMG_RDIRUNIQUE : DIFFIMG_RUNIQUE);
	if (di.diffcode.isResultSame())
	{
		if (di.diffcode.isDirectory())
			return DIFFIMG_DIRSAME;
		else
		{
			if (di.diffcode.isText())
				return DIFFIMG_TEXTSAME;
			else if (di.diffcode.isBin())
				return DIFFIMG_BINSAME;
			else
				return DIFFIMG_SAME;
		}
	}
	// diff
	if (di.diffcode.isResultDiff())
	{
		if (di.diffcode.isDirectory())
			return DIFFIMG_DIRDIFF;
		else
		{
			if (di.diffcode.isText())
				return DIFFIMG_TEXTDIFF;
			else if (di.diffcode.isBin())
				return DIFFIMG_BINDIFF;
			else
				return DIFFIMG_DIFF;
		}
	}
	return (di.diffcode.isDirectory() ? DIFFIMG_DIR : DIFFIMG_ABORT);
}

/**
 * @brief Get default folder compare status image.
 */
int CDirView::GetDefaultColImage() const
{
	return DIFFIMG_ERROR;
}

/**
 * @brief Called before compare is started.
 * CDirDoc calls this function before new compare is started, so this
 * is good place to setup GUI for compare.
 * @param [in] pCompareStats Pointer to class having current compare stats.
 */
HWND CDirView::StartCompare(CompareStats *pCompareStats)
{
	if (m_pCmpProgressDlg == NULL)
		m_pCmpProgressDlg = new DirCompProgressDlg(m_pFrame);
	m_pCmpProgressDlg->ShowWindow(SW_SHOW);
	m_pCmpProgressDlg->StartUpdating();
	m_compareStart = clock();
	return m_pCmpProgressDlg->m_hWnd;
}

/**
 * @brief Called when folder compare row is double-clicked with mouse.
 * Selected item is opened to folder or file compare.
 */
void CDirView::ReflectItemActivate(NMITEMACTIVATE *pNM)
{
	if (pNM->iItem >= 0)
	{
		const DIFFITEM& di = GetDiffItem(pNM->iItem);
		if (m_bTreeMode && GetDocument()->GetRecursive() && di.diffcode.isDirectory())
		{
			if (di.customFlags1 & ViewCustomFlags::EXPANDED)
				CollapseSubdir(pNM->iItem);
			else
				ExpandSubdir(pNM->iItem);
		}
		else
		{
			OpenSelection();
		}
	}
}

/**
 * @brief Load or reload the columns (headers) of the list view
 */
void CDirView::ReloadColumns()
{
	LoadColumnHeaderItems();

	UpdateColumnNames();
	SetColumnWidths();
	SetColAlignments();
}

/**
 * @brief Redisplay items in subfolder
 * @param [in] diffpos First item position in subfolder.
 * @param [in] level Indent level
 * @param [in,out] index Index of the item to be inserted.
 * @param [in,out] alldiffs Number of different items
 */
void CDirView::RedisplayChildren(UINT_PTR diffpos, int level, int &index, int &alldiffs)
{
	const CDiffContext *ctxt = m_pFrame->GetDiffContext();
	while (diffpos)
	{
		UINT_PTR curdiffpos = diffpos;
		const DIFFITEM &di = ctxt->GetNextSiblingDiffPosition(diffpos);

		if (!di.diffcode.isResultSame())
			++alldiffs;

		if (m_pFrame->IsShowable(di))
		{
			if (m_bTreeMode)
			{
				AddNewItem(index, curdiffpos, I_IMAGECALLBACK, level);
				index++;
				if (di.HasChildren())
				{
					SetItemState(index - 1, INDEXTOSTATEIMAGEMASK((di.customFlags1 & ViewCustomFlags::EXPANDED) ? 2 : 1), LVIS_STATEIMAGEMASK);
					if (di.customFlags1 & ViewCustomFlags::EXPANDED)
						RedisplayChildren(ctxt->GetFirstChildDiffPosition(curdiffpos), level + 1, index, alldiffs);
				}
			}
			else
			{
				if (!m_pFrame->GetRecursive() ||
					!di.diffcode.isDirectory() ||
					di.diffcode.isSideLeftOnly() ||
					di.diffcode.isSideRightOnly())
				{
					AddNewItem(index, curdiffpos, I_IMAGECALLBACK, 0);
					index++;
				}
				if (di.HasChildren())
				{
					RedisplayChildren(ctxt->GetFirstChildDiffPosition(curdiffpos), level + 1, index, alldiffs);
				}
			}
		}
	}
}

/**
 * @brief Redisplay folder compare view.
 * This function clears folder compare view and then adds
 * items from current compare to it.
 */
void CDirView::Redisplay()
{
	const CDiffContext *ctxt = m_pFrame->GetDiffContext();

	int cnt = 0;
	// Disable redrawing while adding new items
	SetRedraw(FALSE);

	DeleteAllItems();

	SetImageList(m_bTreeMode && m_pFrame->GetRecursive() ?
		m_imageState->m_hImageList : NULL, LVSIL_STATE);

	// If non-recursive compare, add special item(s)
	String leftParent, rightParent;
	if (!m_pFrame->GetRecursive() ||
		m_pFrame->AllowUpwardDirectory(leftParent, rightParent) == CDirFrame::AllowUpwardDirectory::ParentIsTempPath)
	{
		cnt += AddSpecialItems();
	}

	int alldiffs = 0;
	RedisplayChildren(ctxt->GetFirstDiffPosition(), 0, cnt, alldiffs);
	theApp.SetLastCompareResult(alldiffs);
	SortColumnsAppropriately();
	SetRedraw(TRUE);
}

void CDirView::DoScript(LPCWSTR text)
{
	WaitStatusCursor waitstatus(IDS_STATUS_OPENING_SELECTION);
	PackingInfo packingInfo;
	packingInfo.SetPlugin(text);
	OpenSelection(&packingInfo, 0);
}

/**
 * @brief Format context menu string and disable item if it cannot be applied.
 */
static void NTAPI FormatContextMenu(HMenu *pPopup, UINT uIDItem, int n1, int n2)
{
	TCHAR fmt[INFOTIPSIZE];
	pPopup->GetMenuString(uIDItem, fmt, _countof(fmt));
	String str = fmt;
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
	MENUITEMINFO mii;
	mii.cbSize = sizeof mii;
	mii.fMask = MIIM_STRING;
	mii.dwTypeData = const_cast<LPTSTR>(str.c_str());
	pPopup->SetMenuItemInfo(uIDItem, FALSE, &mii);
	if (n1 == 0)
		pPopup->EnableMenuItem(uIDItem, MF_GRAYED);
}

/**
 * @brief User right-clicked in listview rows
 */
void CDirView::ListContextMenu(POINT point)
{
	const BOOL leftRO = m_pFrame->GetLeftReadOnly();
	const BOOL rightRO = m_pFrame->GetRightReadOnly();
	// TODO: It would be more efficient to set
	// all the popup items now with one traverse over selected items
	// instead of using updates, in which we make a traverse for every item
	// Perry, 2002-12-04

	//2003-12-17 Jochen:
	//-	Archive related menu items follow the above suggestion.
	//-	For disabling to work properly, the tracking frame's m_bAutoMenuEnable
	//	member has to temporarily be turned off.
	int nTotal = 0; // total #items (includes files & directories, either side)
	int nCopyableToLeft = 0;
	int nCopyableToRight = 0;
	int nDeletableOnLeft = 0;
	int nDeletableOnRight = 0;
	int nDeletableOnBoth = 0;
	int nOpenableOnLeft = 0;
	int nOpenableOnRight = 0;
	int nOpenableOnBoth = 0;
	int nOpenableOnLeftWith = 0;
	int nOpenableOnRightWith = 0;
	int nFileNames = 0;
	int nDiffItems = 0;
	int i = -1;
	while ((i = GetNextItem(i, LVNI_SELECTED)) != -1)
	{
		const DIFFITEM& di = GetDiffItem(i);
		if (di.diffcode.diffcode == 0) // Invalid value, this must be special item
			continue;

		if (!di.diffcode.isDirectory())
			++nFileNames;

		if (IsItemDeletableOnBoth(di) && !leftRO && !rightRO)
			++nDeletableOnBoth;

		if (IsItemOpenableOnLeft(di))
		{
			++nOpenableOnLeft;
			if (!di.diffcode.isDirectory())
				++nOpenableOnLeftWith;
			if (IsItemCopyableToRight(di) && !rightRO)
				++nCopyableToRight;
			if (IsItemDeletableOnLeft(di) && !leftRO)
				++nDeletableOnLeft;
		}

		if (IsItemOpenableOnRight(di))
		{
			++nOpenableOnRight;
			if (!di.diffcode.isDirectory())
				++nOpenableOnRightWith;
			if (IsItemCopyableToLeft(di) && !leftRO)
				++nCopyableToLeft;
			if (IsItemDeletableOnRight(di) && !rightRO)
				++nDeletableOnRight;
		}

		if (IsItemNavigableDiff(di))
			++nDiffItems;
		if (IsItemOpenableOnLeft(di) || IsItemOpenableOnRight(di))
			++nOpenableOnBoth;

		++nTotal;
	}

	if (nTotal == 0)
		return;

	HMenu *const pMenu = LanguageSelect.LoadMenu(IDR_POPUP_DIRVIEW);

	// 1st submenu of IDR_POPUP_DIRVIEW is for item popup
	HMenu *const pPopup = pMenu->GetSubMenu(0);

	FormatContextMenu(pPopup, ID_DIR_COPY_PATHNAMES_LEFT, nOpenableOnLeft, nTotal);
	FormatContextMenu(pPopup, ID_DIR_COPY_PATHNAMES_RIGHT, nOpenableOnRight, nTotal);
	FormatContextMenu(pPopup, ID_DIR_COPY_PATHNAMES_BOTH, nOpenableOnBoth, nTotal);
	FormatContextMenu(pPopup, ID_DIR_COPY_FILENAMES, nFileNames, nTotal);

	FormatContextMenu(pPopup, ID_DIR_ZIP_LEFT, nOpenableOnLeft, nTotal);
	FormatContextMenu(pPopup, ID_DIR_ZIP_RIGHT, nOpenableOnRight, nTotal);
	FormatContextMenu(pPopup, ID_DIR_ZIP_BOTH, nOpenableOnBoth, nTotal);
	FormatContextMenu(pPopup, ID_DIR_ZIP_BOTH_DIFFS_ONLY, nDiffItems, nTotal);

	FormatContextMenu(pPopup, ID_DIR_COPY_LEFT_TO_RIGHT, nCopyableToRight, nTotal);
	FormatContextMenu(pPopup, ID_DIR_COPY_RIGHT_TO_LEFT, nCopyableToLeft, nTotal);
	FormatContextMenu(pPopup, ID_DIR_COPY_LEFT_TO_BROWSE, nOpenableOnLeft, nTotal);
	FormatContextMenu(pPopup, ID_DIR_COPY_RIGHT_TO_BROWSE, nOpenableOnRight, nTotal);
	FormatContextMenu(pPopup, ID_DIR_MOVE_LEFT_TO_BROWSE, nDeletableOnLeft, nTotal);
	FormatContextMenu(pPopup, ID_DIR_MOVE_RIGHT_TO_BROWSE, nDeletableOnRight, nTotal);

	FormatContextMenu(pPopup, ID_DIR_DEL_LEFT, nDeletableOnLeft, nTotal);
	FormatContextMenu(pPopup, ID_DIR_DEL_RIGHT, nDeletableOnRight, nTotal);
	FormatContextMenu(pPopup, ID_DIR_DEL_BOTH, nDeletableOnBoth, nTotal);
	FormatContextMenu(pPopup, ID_DIR_OPEN_LEFT, nOpenableOnLeft, nTotal);
	FormatContextMenu(pPopup, ID_DIR_OPEN_LEFT_WITH, nOpenableOnLeftWith, nTotal);
	FormatContextMenu(pPopup, ID_DIR_OPEN_RIGHT, nOpenableOnRight, nTotal);
	FormatContextMenu(pPopup, ID_DIR_OPEN_RIGHT_WITH, nOpenableOnRightWith, nTotal);
	FormatContextMenu(pPopup, ID_DIR_OPEN_LEFT_WITHEDITOR, nOpenableOnLeftWith, nTotal);
	FormatContextMenu(pPopup, ID_DIR_OPEN_RIGHT_WITHEDITOR, nOpenableOnRightWith, nTotal);
	
	pPopup->EnableMenuItem(ID_DIR_ITEM_RENAME,
		nTotal == 1 && !IsItemSelectedSpecial() ? MF_ENABLED : MF_GRAYED);

	bool bEnableShellContextMenu = COptionsMgr::Get(OPT_DIRVIEW_ENABLE_SHELL_CONTEXT_MENU);
	if (bEnableShellContextMenu)
	{
		if (nOpenableOnLeft || nOpenableOnRight)
		{
			pPopup->AppendMenu(MF_SEPARATOR, 0, NULL);
			if (nOpenableOnLeft)
			{
				String s = LanguageSelect.LoadString(IDS_SHELL_CONTEXT_MENU_LEFT);
				m_hShellContextMenuLeft = ::CreatePopupMenu();
				pPopup->AppendMenu(MF_POPUP, (UINT_PTR)m_hShellContextMenuLeft, s.c_str());
			}
			if (nOpenableOnRight)
			{
				String s = LanguageSelect.LoadString(IDS_SHELL_CONTEXT_MENU_RIGHT);
				m_hShellContextMenuRight = ::CreatePopupMenu();
				pPopup->AppendMenu(MF_POPUP, (UINT_PTR)m_hShellContextMenuRight, s.c_str());
			}
		}
	}

	m_pCompareAsScriptMenu = pPopup->GetSubMenu(1);
	// Disable "Compare As" if multiple items are selected, or if the selected
	// item is unique or represents a folder.
	if (nTotal != 1 || nFileNames != 1 || nOpenableOnLeft != 1 || nOpenableOnRight != 1)
		pPopup->EnableMenuItem(1, MF_GRAYED | MF_BYPOSITION);

	CMainFrame *pMDIFrame = m_pFrame->m_pMDIFrame;
	// invoke context menu
	// this will invoke all the OnUpdate methods to enable/disable the individual items
	int nCmd = pPopup->TrackPopupMenu(
		TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
		point.x, point.y, pMDIFrame->m_hWnd);

	HMenu *pScriptMenu = pMDIFrame->SetScriptMenu(m_pCompareAsScriptMenu, NULL);

	if (nCmd >= IDC_SCRIPT_FIRST && nCmd <= IDC_SCRIPT_LAST)
	{
		if (pScriptMenu)
		{
			WCHAR text[MAX_PATH];
			pScriptMenu->GetMenuStringW(nCmd, text, _countof(text));
			DoScript(text);
		}
	}
	else if (nCmd)
	{
		(m_pShellContextMenuLeft->InvokeCommand(nCmd, pMDIFrame->m_hWnd))
		|| (m_pShellContextMenuRight->InvokeCommand(nCmd, pMDIFrame->m_hWnd))
		// we have called TrackPopupMenu with TPM_RETURNCMD flag so we have to post message ourselves
		|| m_pFrame->PostMessage(WM_COMMAND, nCmd);
	}
	m_hShellContextMenuLeft = NULL;
	m_hShellContextMenuRight = NULL;

	if (pScriptMenu)
		pScriptMenu->DestroyMenu();

	m_pCompareAsScriptMenu = NULL;
	pMenu->DestroyMenu();
}

/**
 * @brief User right-clicked on specified logical column
 */
void CDirView::HeaderContextMenu(POINT point)
{
	HMenu *const pMenu = LanguageSelect.LoadMenu(IDR_POPUP_DIRVIEW);
	// 2nd submenu of IDR_POPUP_DIRVIEW is for header popup
	HMenu *const pSubMenu = pMenu->GetSubMenu(1);
	// invoke context menu
	// this will invoke all the OnUpdate methods to enable/disable the individual items
	CMainFrame *pFrame = m_pFrame->m_pMDIFrame;
	pSubMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
		point.x, point.y, pFrame->m_hWnd);
	pMenu->DestroyMenu();
}

/**
 * @brief Gets Explorer's context menu for a group of selected files.
 *
 * @param [in] Side whether to get context menu for the files from the left or
 *   right side.
 * @retval The HMENU.
 */
HMENU CDirView::ListShellContextMenu(SIDE_TYPE side)
{
	CMyComPtr<IShellFolder> pDesktop;
	HRESULT hr = SHGetDesktopFolder(&pDesktop);
	if (FAILED(hr))
		return NULL;

	String parentDir; // use it to track that all selected files are in the same parent directory
	CMyComPtr<IShellFolder> pCurrFolder;
	CPidlContainer pidls;

	int i = -1;
	while ((i = GetNextItem(i, LVNI_SELECTED)) != -1)
	{
		const DIFFITEM& di = GetDiffItem(i);
		if (di.diffcode.diffcode == 0) // Invalid value, this must be special item
			continue;
		String currentDir = (side == SIDE_LEFT) ?
			di.GetLeftFilepath(m_pFrame->GetLeftBasePath()) :
			di.GetRightFilepath(m_pFrame->GetRightBasePath());
		String filename = (side == SIDE_LEFT) ? di.left.filename : di.right.filename;
		if (parentDir.empty()) // first iteration, initialize parentDir and pCurrFolder
		{
			parentDir = currentDir;
			LPITEMIDLIST dirPidl;
			LPCTSTR displayName = paths_UndoMagic(&currentDir.front());
			hr = pDesktop->ParseDisplayName(NULL, NULL, const_cast<LPOLESTR>(displayName), NULL, &dirPidl, NULL);
			if (FAILED(hr))
				return NULL;
			hr = pDesktop->BindToObject(dirPidl, NULL, IID_IShellFolder, reinterpret_cast<void**>(&pCurrFolder));
			pidls.Free(dirPidl);
			if (FAILED(hr))
				return NULL;
		}
		else if (currentDir != parentDir) // check whether file belongs to the same parentDir, break otherwise
		{
			return NULL;
		}
		LPITEMIDLIST pidl;
		hr = pCurrFolder->ParseDisplayName(NULL, NULL, const_cast<LPOLESTR>(filename.c_str()), NULL, &pidl, NULL);
		if (FAILED(hr))
			return NULL;
		pidls.Add(pidl);
	}

	if (pCurrFolder == NULL)
		return NULL;

	CMyComPtr<IContextMenu> pContextMenu;
	hr = pCurrFolder->GetUIObjectOf(NULL,
		pidls.Size(), pidls.GetList(), IID_IContextMenu, 0,
		reinterpret_cast<void**>(&pContextMenu));
	if (FAILED(hr))
		return NULL;

	CShellContextMenu *pscm = (side == SIDE_LEFT) ?
		m_pShellContextMenuLeft : m_pShellContextMenuRight;
	return pscm->QueryShellContextMenu(pContextMenu);
}

/**
 * @brief Update any resources necessary after a GUI language change
 */
void CDirView::UpdateResources()
{
	UpdateColumnNames();
	m_pFrame->UpdateResources();
}

/**
 * @brief User just clicked a column, so perform sort
 */
void CDirView::ReflectColumnClick(NM_LISTVIEW *pNMListView)
{
	// set sort parameters and handle ascending/descending
	int oldSortColumn = COptionsMgr::Get(OPT_DIRVIEW_SORT_COLUMN);
	int sortcol = m_invcolorder[pNMListView->iSubItem];
	if (sortcol == oldSortColumn)
	{
		// Swap direction
		bool bSortAscending = COptionsMgr::Get(OPT_DIRVIEW_SORT_ASCENDING);
		COptionsMgr::SaveOption(OPT_DIRVIEW_SORT_ASCENDING, !bSortAscending);
	}
	else
	{
		COptionsMgr::SaveOption(OPT_DIRVIEW_SORT_COLUMN, sortcol);
		// most columns start off ascending, but not dates
		bool bSortAscending = IsDefaultSortAscending(sortcol);
		COptionsMgr::SaveOption(OPT_DIRVIEW_SORT_ASCENDING, bSortAscending);
	}

	SortColumnsAppropriately();
}

void CDirView::SortColumnsAppropriately()
{
	int sortCol = COptionsMgr::Get(OPT_DIRVIEW_SORT_COLUMN);
	if (sortCol == -1)
		return;

	bool bSortAscending = COptionsMgr::Get(OPT_DIRVIEW_SORT_ASCENDING);
	m_ctlSortHeader.SetSortImage(ColLogToPhys(sortCol), bSortAscending);
	//sort using static CompareFunc comparison function
	CompareState cs(this, sortCol, bSortAscending);
	SortItems(cs.CompareFunc, reinterpret_cast<DWORD_PTR>(&cs));
}

/// Do any last minute work as view closes
void CDirView::OnDestroy()
{
	ValidateColumnOrdering();
	SaveColumnOrders();
	SaveColumnWidths();
}

/**
 * @brief Open selected item when user presses ENTER key.
 */
void CDirView::OnReturn()
{
	int sel = GetFocusedItem();
	if (sel >= 0)
	{
		const DIFFITEM& di = GetDiffItem(sel);
		if (m_bTreeMode && GetDocument()->GetRecursive() && di.diffcode.isDirectory())
		{
			if (di.customFlags1 & ViewCustomFlags::EXPANDED)
				CollapseSubdir(sel);
			else
				ExpandSubdir(sel);
		}
		else
		{
			OpenSelection();
		}
	}
}

/**
 * @brief Expand/collapse subfolder when "+/-" icon is clicked.
 */
void CDirView::ReflectClick(NMITEMACTIVATE *pNM)
{
	LVHITTESTINFO lvhti;
	lvhti.pt = pNM->ptAction;
	SubItemHitTest(&lvhti);
	if (lvhti.flags == LVHT_ONITEMSTATEICON)
		ReflectItemActivate(pNM);
}

/**
 * @brief Expand collapsed folder in tree-view mode.
 */
void CDirView::OnExpandFolder()
{
	const int nSelItem = GetNextItem(-1, LVNI_SELECTED);
	if (nSelItem == -1)
		return;
	const DIFFITEM &di = GetItemAt(nSelItem);
	if (di.diffcode.isDirectory() && (di.customFlags1 &
			ViewCustomFlags::EXPANDED) == 0)
	{
		ExpandSubdir(nSelItem);
	}
}

/**
 * @brief Collapse expanded folder in tree-view mode.
 */
void CDirView::OnCollapseFolder()
{
	const int nSelItem = GetNextItem(-1, LVNI_SELECTED);
	if (nSelItem == -1)
		return;
	const DIFFITEM &di = GetItemAt(nSelItem);
	if (di.diffcode.isDirectory() && (di.customFlags1 &
			ViewCustomFlags::EXPANDED))
	{
		CollapseSubdir(nSelItem);
	}
}

/**
 * @brief Collapse subfolder
 * @param [in] sel Folder item index in listview.
 */
void CDirView::CollapseSubdir(int i)
{
	DIFFITEM& dip = GetDiffItemRef(i);
	if (!m_bTreeMode || !(dip.customFlags1 & ViewCustomFlags::EXPANDED) || !dip.HasChildren())
		return;

	SetRedraw(FALSE);	// Turn off updating (better performance)

	dip.customFlags1 &= ~ViewCustomFlags::EXPANDED;
	SetItemState(i, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);

	++i;
	int count = GetItemCount();
	while (count > i)
	{
		const DIFFITEM& di = GetDiffItem(i);
		if (!di.IsAncestor(&dip))
			break;
		DeleteItem(i);
		--count;
	}

	SetRedraw(TRUE);	// Turn updating back on
}

/**
 * @brief Expand subfolder
 * @param [in] sel Folder item index in listview.
 */
void CDirView::ExpandSubdir(int i)
{
	DIFFITEM& dip = GetDiffItemRef(i);
	if (!m_bTreeMode || (dip.customFlags1 & ViewCustomFlags::EXPANDED) || !dip.HasChildren())
		return;

	SetRedraw(FALSE);	// Turn off updating (better performance)

	dip.customFlags1 |= ViewCustomFlags::EXPANDED;
	SetItemState(i, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);

	const CDiffContext *ctxt = m_pFrame->GetDiffContext();
	UINT_PTR diffpos = ctxt->GetFirstChildDiffPosition(GetItemKey(i));
	++i;
	int alldiffs;
	RedisplayChildren(diffpos, dip.GetDepth() + 1, i, alldiffs);

	SortColumnsAppropriately();

	SetRedraw(TRUE);	// Turn updating back on
}

/**
 * @brief Open parent folder if possible.
 */
void CDirView::OpenParentDirectory()
{
	FileLocation filelocLeft, filelocRight;
	switch (m_pFrame->AllowUpwardDirectory(filelocLeft.filepath, filelocRight.filepath))
	{
	case CDirFrame::AllowUpwardDirectory::ParentIsTempPath:
		m_pFrame->m_pTempPathContext = m_pFrame->m_pTempPathContext->DeleteHead();
		// fall through (no break!)
	case CDirFrame::AllowUpwardDirectory::ParentIsRegularPath:
		GetMainFrame()->DoFileOpen(filelocLeft, filelocRight,
			FFILEOPEN_NOMRU, FFILEOPEN_NOMRU, m_pFrame->GetRecursive(), m_pFrame);
		// fall through (no break!)
	case CDirFrame::AllowUpwardDirectory::No:
		break;
	default:
		LanguageSelect.MsgBox(IDS_INVALID_DIRECTORY, MB_ICONSTOP);
		break;
	}
}

/**
 * @brief Get one or two selected items
 *
 * Returns false if 0 or more than 2 items selecte
 */
bool CDirView::GetSelectedItems(int * sel1, int * sel2)
{
	*sel2 = -1;
	*sel1 = GetNextItem(-1, LVNI_SELECTED);
	if (*sel1 == -1)
		return false;
	*sel2 = GetNextItem(*sel1, LVNI_SELECTED);
	if (*sel2 == -1)
		return true;
	int extra = GetNextItem(*sel2, LVNI_SELECTED);
	return (extra == -1);
}

/**
 * @brief Open special items (parent folders etc).
 * @param [in] pos1 First item position.
 * @param [in] pos2 Second item position.
 */
void CDirView::OpenSpecialItems(UINT_PTR pos1, UINT_PTR pos2)
{
	if (!pos2)
	{
		// Browse to parent folder(s) selected
		// SPECIAL_ITEM_POS is position for
		// special items, but there is currenly
		// only one (parent folder)
		OpenParentDirectory();
	}
	else
	{
		// Parent directory & something else selected
		// Not valid action
	}
}

/**
 * @brief Creates a pairing folder for unique folder item.
 * This function creates a pairing folder for unique folder item in
 * folder compare. This way user can browse into unique folder's
 * contents and don't necessarily need to copy whole folder structure.
 * @param [in] di DIFFITEM for folder compare item.
 * @param [in] side1 true if our unique folder item is side1 item.
 * @param [out] newFolder New created folder (full folder path).
 * @return true if user agreed and folder was created.
 */
bool CDirView::CreateFoldersPair(DIFFITEM & di, bool side1, String &newFolder)
{
	String subdir;
	String basedir;
	if (side1)
	{
		// Get left side (side1) folder name (existing) and
		// right side base path (where to create)
		subdir = di.left.filename;
		basedir = GetDocument()->GetRightBasePath();
		basedir = di.GetLeftFilepath(basedir);
	}
	else
	{
		// Get right side (side2) folder name (existing) and
		// left side base path (where to create)
		subdir = di.right.filename;
		basedir = GetDocument()->GetLeftBasePath();
		basedir = di.GetRightFilepath(basedir);
	}
	newFolder = paths_ConcatPath(basedir, subdir);

	int response = LanguageSelect.FormatMessage(
		IDS_CREATE_PAIR_FOLDER, paths_UndoMagic(&String(newFolder).front())
	).MsgBox(MB_YESNO | MB_ICONWARNING);

	return response == IDYES && paths_CreateIfNeeded(newFolder.c_str());
}

/**
 * @brief Open one selected item.
 * @param [in] pos1 Item position.
 * @param [in,out] di1 Pointer to first diffitem.
 * @param [in,out] di2 Pointer to second diffitem.
 * @param [out] path1 First path.
 * @param [out] path2 Second path.
 * @param [out] sel1 Item's selection index in listview.
 * @param [in,out] isDir Is item folder?
 * return false if there was error or item was completely processed.
 */
bool CDirView::OpenOneItem(UINT_PTR pos1, DIFFITEM **di1, DIFFITEM **di2,
		String &path1, String &path2, int & sel1, bool & isDir)
{
	*di1 = &m_pFrame->GetDiffRefByKey(pos1);
	*di2 = *di1;

	GetItemFileNames(sel1, path1, path2);

	if ((*di1)->diffcode.isDirectory())
		isDir = true;

	if (isDir && (*di1)->diffcode.isSideBoth())
	{
		// Check both folders exist. If either folder is missing that means
		// folder has been changed behind our back, so we just tell user to
		// refresh the compare.
		PATH_EXISTENCE path1Exists = paths_DoesPathExist(path1.c_str());
		PATH_EXISTENCE path2Exists = paths_DoesPathExist(path2.c_str());
		if (path1Exists != IS_EXISTING_DIR || path2Exists != IS_EXISTING_DIR)
		{
			String invalid = path1Exists == IS_EXISTING_DIR ? path1 : path2;
			LanguageSelect.MsgBox(IDS_DIRCMP_NOTSYNC, invalid.c_str(), MB_ICONSTOP);
			return false;
		}
	}
	else if ((*di1)->diffcode.isSideLeftOnly())
	{
		// Open left-only item to editor if its not a folder or binary
		if (isDir)
		{
			if (CreateFoldersPair(**di1, true, path2))
			{
				return true;
			}
		}
		else if ((*di1)->diffcode.isBin())
			LanguageSelect.MsgBox(IDS_CANNOT_OPEN_BINARYFILE, MB_ICONSTOP);
		else
			DoOpenWithEditor(SIDE_LEFT);
		return false;
	}
	else if ((*di1)->diffcode.isSideRightOnly())
	{
		// Open right-only item to editor if its not a folder or binary
		if (isDir)
		{
			if (CreateFoldersPair(**di1, false, path1))
			{
				return true;
			}
		}
		else if ((*di1)->diffcode.isBin())
			LanguageSelect.MsgBox(IDS_CANNOT_OPEN_BINARYFILE, MB_ICONSTOP);
		else
			DoOpenWithEditor(SIDE_RIGHT);
		return false;
	}
	// Fall through and compare files (which may be archives)

	return true;
}

/**
 * @brief Open two selected items.
 * @param [in] pos1 First item position.
 * @param [in] pos2 Second item position.
 * @param [in,out] di1 Pointer to first diffitem.
 * @param [in,out] di2 Pointer to second diffitem.
 * @param [out] path1 First path.
 * @param [out] path2 Second path.
 * @param [out] sel1 First item's selection index in listview.
 * @param [out] sel2 Second item's selection index in listview.
 * @param [in,out] isDir Is item folder?
 * return false if there was error or item was completely processed.
 */
bool CDirView::OpenTwoItems(UINT_PTR pos1, UINT_PTR pos2, DIFFITEM **di1, DIFFITEM **di2,
		String &path1, String &path2, int & sel1, int & sel2, bool & isDir)
{
	// Two items selected, get their info
	*di1 = &m_pFrame->GetDiffRefByKey(pos1);
	*di2 = &m_pFrame->GetDiffRefByKey(pos2);

	// Check for binary & side compatibility & file/dir compatibility
	if (!AreItemsOpenable(**di1, **di2))
	{
		return false;
	}
	// Ensure that di1 is on left (swap if needed)
	if ((*di1)->diffcode.isSideRightOnly() || ((*di1)->diffcode.isSideBoth() &&
			(*di2)->diffcode.isSideLeftOnly()))
	{
		DIFFITEM * temp = *di1;
		*di1 = *di2;
		*di2 = temp;
		int num = sel1;
		sel1 = sel2;
		sel2 = num;
	}
	// Fill in pathLeft & pathRight
	String temp;
	GetItemFileNames(sel1, path1, temp);
	GetItemFileNames(sel2, temp, path2);

	if ((*di1)->diffcode.isDirectory())
	{
		isDir = true;
		if (GetPairComparability(path1.c_str(), path2.c_str()) != IS_EXISTING_DIR)
		{
			LanguageSelect.MsgBox(IDS_INVALID_DIRECTORY, MB_ICONSTOP);
			return false;
		}
	}
	return true;
}

/**
 * @brief Open selected files or directories.
 *
 * Opens selected files to file compare. If comparing
 * directories non-recursively, then subfolders and parent
 * folder are opened too.
 *
 * This handles the case that one item is selected
 * and the case that two items are selected (one on each side)
 */
void CDirView::OpenSelection(PackingInfo *infoUnpacker, DWORD commonFlags)
{
	WaitStatusCursor waitstatus(IDS_STATUS_OPENING_SELECTION);
	// First, figure out what was selected (store into pos1 & pos2)
	UINT_PTR pos1 = NULL, pos2 = NULL;
	int sel1 = -1, sel2 = -1;
	if (!GetSelectedItems(&sel1, &sel2))
	{
		// Must have 1 or 2 items selected
		// Not valid action
		return;
	}

	pos1 = GetItemKey(sel1);
	ASSERT(pos1);
	if (sel2 != -1)
		pos2 = GetItemKey(sel2);

	// Now handle the various cases of what was selected

	if (pos1 == SPECIAL_ITEM_POS)
	{
		OpenSpecialItems(pos1, pos2);
		return;
	}

	// Common variables which both code paths below are responsible for setting
	FileLocation filelocLeft, filelocRight;
	DIFFITEM *di1 = NULL, *di2 = NULL; // left & right items (di1==di2 if single selection)
	bool isdir = false; // set if we're comparing directories

	if (pos2)
	{
		bool success = OpenTwoItems(pos1, pos2, &di1, &di2,
			filelocLeft.filepath, filelocRight.filepath, sel1, sel2, isdir);
		if (!success)
			return;
	}
	else
	{
		// Only one item selected, so perform diff on its sides
		bool success = OpenOneItem(pos1, &di1, &di2,
			filelocLeft.filepath, filelocRight.filepath, sel1, isdir);
		if (!success)
			return;
	}

	// Now pathLeft, pathRight, di1, di2, and isdir are all set
	// We have two items to compare, no matter whether same or different underlying DirView item

	if (infoUnpacker == NULL)
	{
		// Open subfolders
		// Don't add folders to MRU
		// Or: Open archives, not adding paths to MRU
		GetMainFrame()->DoFileOpen(
			filelocLeft, filelocRight,
			commonFlags, commonFlags,
			m_pFrame->GetRecursive(), m_pFrame);
	}
	else
	{
		// Regular file case

		// Close open documents first (ask to save unsaved data)
		if (!COptionsMgr::Get(OPT_MULTIDOC_MERGEDOCS))
		{
			if (!m_pFrame->CloseMergeDocs())
				return;
		}

		// Open identical and different files
		BOOL bLeftRO = m_pFrame->GetLeftReadOnly();
		BOOL bRightRO = m_pFrame->GetRightReadOnly();

		filelocLeft.encoding = di1->left.encoding;
		filelocRight.encoding = di2->right.encoding;

		DWORD leftFlags = bLeftRO ? FFILEOPEN_READONLY : 0;
		DWORD rightFlags = bRightRO ? FFILEOPEN_READONLY : 0;

		GetMainFrame()->ShowMergeDoc(m_pFrame, filelocLeft, filelocRight,
			commonFlags | leftFlags, commonFlags | rightFlags, infoUnpacker);
	}
}

void CDirView::OpenSelectionZip()
{
	WaitStatusCursor waitstatus(IDS_STATUS_OPENING_SELECTION);
	// First, figure out what was selected (store into pos1 & pos2)
	UINT_PTR pos1 = NULL, pos2 = NULL;
	int sel1 = -1, sel2 = -1;
	if (!GetSelectedItems(&sel1, &sel2))
	{
		// Must have 1 or 2 items selected
		// Not valid action
		return;
	}

	pos1 = GetItemKey(sel1);
	ASSERT(pos1);
	if (sel2 != -1)
		pos2 = GetItemKey(sel2);

	// Now handle the various cases of what was selected

	if (pos1 == SPECIAL_ITEM_POS)
	{
		ASSERT(FALSE);
		return;
	}

	// Common variables which both code paths below are responsible for setting
	FileLocation filelocLeft, filelocRight;
	DIFFITEM *di1 = NULL, *di2 = NULL; // left & right items (di1==di2 if single selection)
	bool isdir = false; // set if we're comparing directories

	if (pos2)
	{
		bool success = OpenTwoItems(pos1, pos2, &di1, &di2,
			filelocLeft.filepath, filelocRight.filepath, sel1, sel2, isdir);
		if (!success)
			return;
	}
	else
	{
		// Only one item selected, so perform diff on its sides
		bool success = OpenOneItem(pos1, &di1, &di2,
			filelocLeft.filepath, filelocRight.filepath, sel1, isdir);
		if (!success)
			return;
	}

	// Now pathLeft, pathRight, di1, di2, and isdir are all set
	// We have two items to compare, no matter whether same or different underlying DirView item

	// Open subfolders
	// Don't add folders to MRU
	// Or: Open archives, not adding paths to MRU
	GetMainFrame()->DoFileOpen(
		filelocLeft, filelocRight,
		FFILEOPEN_NOMRU | FFILEOPEN_DETECTZIP,
		FFILEOPEN_NOMRU | FFILEOPEN_DETECTZIP,
		m_pFrame->GetRecursive(), m_pFrame);
}

void CDirView::OpenSelectionHex()
{
	WaitStatusCursor waitstatus(IDS_STATUS_OPENING_SELECTION);
	// First, figure out what was selected (store into pos1 & pos2)
	UINT_PTR pos1 = NULL, pos2 = NULL;
	int sel1 = -1, sel2 = -1;
	if (!GetSelectedItems(&sel1, &sel2))
	{
		// Must have 1 or 2 items selected
		// Not valid action
		return;
	}

	pos1 = GetItemKey(sel1);
	ASSERT(pos1);
	if (sel2 != -1)
		pos2 = GetItemKey(sel2);

	// Now handle the various cases of what was selected

	if (pos1 == SPECIAL_ITEM_POS)
	{
		ASSERT(FALSE);
		return;
	}

	// Common variables which both code paths below are responsible for setting
	FileLocation filelocLeft, filelocRight;
	DIFFITEM *di1 = NULL, *di2 = NULL; // left & right items (di1==di2 if single selection)
	bool isdir = false; // set if we're comparing directories
	if (pos2)
	{
		bool success = OpenTwoItems(pos1, pos2, &di1, &di2,
			filelocLeft.filepath, filelocRight.filepath, sel1, sel2, isdir);
		if (!success)
			return;
	}
	else
	{
		// Only one item selected, so perform diff on its sides
		bool success = OpenOneItem(pos1, &di1, &di2,
			filelocLeft.filepath, filelocRight.filepath, sel1, isdir);
		if (!success)
			return;
	}

	// Need to consider only regular file case here

	// Close open documents first (ask to save unsaved data)
	if (!COptionsMgr::Get(OPT_MULTIDOC_MERGEDOCS))
	{
		if (!m_pFrame->CloseMergeDocs())
			return;
	}

	// Open identical and different files
	BOOL bLeftRO = m_pFrame->GetLeftReadOnly();
	BOOL bRightRO = m_pFrame->GetRightReadOnly();

	GetMainFrame()->ShowHexMergeDoc(m_pFrame,
		filelocLeft, filelocRight, bLeftRO, bRightRO);
}

void CDirView::OpenSelectionXML()
{
	PackingInfo packingInfo;
	packingInfo.SetXML();
	OpenSelection(&packingInfo, 0);
}

/**
 * @brief Get keydata associated with item in given index.
 * @param [in] idx Item's index to list in UI.
 * @return Key for item in given index.
 */
UINT_PTR CDirView::GetItemKey(int idx)
{
	return GetItemData(idx);
}

// SetItemKey & GetItemKey encapsulate how the display list items
// are mapped to DiffItems, which in turn are DiffContext keys to the actual DIFFITEM data

/**
 * @brief Get DIFFITEM data for item.
 * This function returns DIFFITEM data for item in given index in GUI.
 * @param [in] sel Item's index in folder compare GUI list.
 * @return DIFFITEM for item.
 */
const DIFFITEM &CDirView::GetDiffItem(int sel) const
{
	CDirView * pDirView = const_cast<CDirView *>(this);
	return pDirView->GetDiffItemRef(sel);
}

/**
 * Given index in list control, get modifiable reference to its DIFFITEM data
 */
DIFFITEM & CDirView::GetDiffItemRef(int sel)
{
	UINT_PTR diffpos = GetItemKey(sel);

	// If it is special item, return empty DIFFITEM
	if (diffpos == SPECIAL_ITEM_POS)
	{
		// TODO: It would be better if there were individual items
		// for whatever these special items are
		// because here we have to hope client does not modify this
		// static (shared) item
		return DIFFITEM::emptyitem;
	}
	return GetDocument()->GetDiffRefByKey(diffpos);
}

/**
 * @brief Given key, get index of item which has it stored.
 * This function searches from list in UI.
 */
int CDirView::GetItemIndex(UINT_PTR key)
{
	LVFINDINFO findInfo;

	findInfo.flags = LVFI_PARAM;  // Search for itemdata
	findInfo.lParam = (LPARAM)key;
	return FindItem(&findInfo);
}

// return selected item index, or -1 if none or multiple
int CDirView::GetSingleSelectedItem()
{
	int sel = -1, sel2 = -1;
	sel = GetNextItem(sel, LVNI_SELECTED);
	if (sel == -1) return -1;
	sel2 = GetNextItem(sel, LVNI_SELECTED);
	if (sel2 != -1) return -1;
	return sel;
}

/**
 * @brief Return index of first selected item in folder compare.
 */
int CDirView::GetFirstSelectedInd()
{
	return GetNextItem(-1, LVNI_SELECTED);
}

/**
 * @brief Get index of next selected item in folder compare.
 * @param [in,out] ind
 * - IN current index, for which next index is searched
 * - OUT new index of found item
 * @return DIFFITEM in found index.
 */
DIFFITEM &CDirView::GetNextSelectedInd(int &ind)
{
	int sel = GetNextItem(ind, LVNI_SELECTED);
	DIFFITEM &di = GetDiffItemRef(ind);
	ind = sel;

	return di;
}

/**
 * @brief Return DIFFITEM from given index.
 * @param [in] ind Index from where DIFFITEM is wanted.
 * @return DIFFITEM in given index.
 */
DIFFITEM &CDirView::GetItemAt(int ind)
{
	ASSERT(ind != -1); // Trap programmer errors in debug
	return GetDiffItemRef(ind);
}

// Go to first diff
// If none or one item selected select found item
// This is used for scrolling to first diff too
void CDirView::OnFirstdiff()
{
	const int count = GetItemCount();
	BOOL found = FALSE;
	int i = 0;
	int currentInd = GetFirstSelectedInd();
	int selCount = GetSelectedCount();

	while (i < count && found == FALSE)
	{
		const DIFFITEM &di = GetItemAt(i);
		if (IsItemNavigableDiff(di))
		{
			MoveFocus(currentInd, i, selCount);
			found = TRUE;
		}
		i++;
	}
}

// Go to last diff
// If none or one item selected select found item
void CDirView::OnLastdiff()
{
	BOOL found = FALSE;
	const int count = GetItemCount();
	int i = count - 1;
	int currentInd = GetFirstSelectedInd();
	int selCount = GetSelectedCount();

	while (i > -1 && found == FALSE)
	{
		const DIFFITEM &di = GetItemAt(i);
		if (IsItemNavigableDiff(di))
		{
			MoveFocus(currentInd, i, selCount);
			found = TRUE;
		}
		i--;
	}
}

// Go to next diff
// If none or one item selected select found item
void CDirView::OnNextdiff()
{
	const int count = GetItemCount();
	BOOL found = FALSE;
	int i = GetFocusedItem();
	int currentInd = 0;
	int selCount = GetSelectedCount();

	currentInd = i;
	i++;

	while (i < count && found == FALSE)
	{
		const DIFFITEM &di = GetItemAt(i);
		if (IsItemNavigableDiff(di))
		{
			MoveFocus(currentInd, i, selCount);
			found = TRUE;
		}
		i++;
	}
}

// Go to prev diff
// If none or one item selected select found item
void CDirView::OnPrevdiff()
{
	BOOL found = FALSE;
	int i = GetFocusedItem();
	int currentInd = 0;
	int selCount = GetSelectedCount();

	currentInd = i;
	if (i > 0)
		i--;

	while (i > -1 && found == FALSE)
	{
		const DIFFITEM &di = GetItemAt(i);
		if (IsItemNavigableDiff(di))
		{
			MoveFocus(currentInd, i, selCount);
			found = TRUE;
		}
		i--;
	}
}

void CDirView::OnCurdiff()
{
	EnsureVisible(GetFocusedItem(), FALSE);
}

int CDirView::GetFocusedItem()
{
	return GetNextItem(-1, LVNI_FOCUSED);
}

int CDirView::GetFirstDifferentItem()
{
	const int count = GetItemCount();
	BOOL found = FALSE;
	int i = 0;
	int foundInd = -1;

	while (i < count && found == FALSE)
	{
		const DIFFITEM &di = GetItemAt(i);
		if (IsItemNavigableDiff(di))
		{
			foundInd = i;
			found = TRUE;
		}
		i++;
	}
	return foundInd;
}

int CDirView::GetLastDifferentItem()
{
	const int count = GetItemCount();
	BOOL found = FALSE;
	int i = count - 1;
	int foundInd = -1;

	while (i > 0 && found == FALSE)
	{
		const DIFFITEM &di = GetItemAt(i);
		if (IsItemNavigableDiff(di))
		{
			foundInd = i;
			found = TRUE;
		}
		i--;
	}
	return foundInd;
}

// When navigating differences, do we stop at this one ?
bool CDirView::IsItemNavigableDiff(const DIFFITEM & di) const
{
	// Not a valid diffitem, one of special items (e.g "..")
	if (di.diffcode.diffcode == 0)
		return false;
	if (di.diffcode.isResultFiltered() || di.diffcode.isResultError())
		return false;
	if (!di.diffcode.isResultDiff() && !di.diffcode.isSideLeftOnly() &&
			!di.diffcode.isSideRightOnly())
		return false;
	return true;
}

/**
 * @brief Move focus to specified item (and selection if multiple items not selected)
 *
 * Moves the focus from item [currentInd] to item [i]
 * Additionally, if there are not multiple items selected,
 *  deselects item [currentInd] and selects item [i]
 */
void CDirView::MoveFocus(int currentInd, int i, int selCount)
{
	if (selCount <= 1)
	{
		// Not multiple items selected, so bring selection with us
		SetItemState(currentInd, 0, LVIS_SELECTED);
		SetItemState(currentInd, 0, LVIS_FOCUSED);
		SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
	}

	// Move focus to specified item
	// (this automatically defocuses old item)
	SetItemState(i, LVIS_FOCUSED, LVIS_FOCUSED);
	EnsureVisible(i, FALSE);
}

CDirFrame *CDirView::GetDocument()
{
	return m_pFrame;
}

LRESULT CDirView::ReflectKeydown(NMLVKEYDOWN *pParam)
{
	// Check if we got 'ESC pressed' -message
	switch (pParam->wVKey)
	{
	case VK_DELETE:
		DoDelAll();
		return 1;
	case VK_BACK:
		if (!GetDocument()->GetRecursive())
			OpenParentDirectory();
		return 1;
	case VK_RETURN:
		OnReturn();
		return 1;
	case VK_LEFT:
		if (!m_bTreeMode)
			break;
		CollapseSubdir(GetFocusedItem());
		return 1;
	case VK_RIGHT:
		if (!m_bTreeMode)
			break;
		ExpandSubdir(GetFocusedItem());
		return 1;
	}
	return 0;
}

/**
 * @brief Called when compare thread asks UI update.
 * @note Currently thread asks update after compare is ready
 * or aborted.
 */
void CDirView::OnUpdateUIMessage()
{
	// Close and destroy the dialog after compare
	m_pCmpProgressDlg->CloseDialog();
	delete m_pCmpProgressDlg;
	m_pCmpProgressDlg = NULL;

	m_pFrame->CompareReady();
	// Don't Redisplay() if triggered by OnMarkedRescan()
	if (GetItemCount() == 0)
	{
		Redisplay();
		if (COptionsMgr::Get(OPT_SCROLL_TO_FIRST))
			OnFirstdiff();
		else
			MoveFocus(0, 0, 0);
	}
	// If compare took more than TimeToSignalCompare seconds, notify user
	clock_t elapsed = clock() - m_compareStart;
	GetMainFrame()->SetStatus(LanguageSelect.Format(IDS_ELAPSED_TIME, elapsed));
	if (elapsed > TimeToSignalCompare * CLOCKS_PER_SEC)
		MessageBeep(IDOK);
	GetMainFrame()->StartFlashing();
}

LRESULT CDirView::OnNotify(LPARAM lParam)
{
	union
	{
		LPARAM lp;
		NMHDR *hdr;
		NMHEADER *header;
	} const nm = { lParam };
	if (nm.hdr->code == HDN_ENDDRAG)
	{
		// User just finished dragging a column header
		int src = nm.header->iItem;
		int dest = nm.header->pitem->iOrder;
		if (src != dest && dest != -1)
			MoveColumn(src, dest);
		return 1;
	}
	if (nm.hdr->code == HDN_BEGINDRAG)
	{
		// User is starting to drag a column header
		// save column widths before user reorders them
		// so we can reload them on the end drag
		SaveColumnWidths();
	}
	return 0;
}

LRESULT CDirView::ReflectNotify(LPARAM lParam)
{
	union
	{
		LPARAM lp;
		NMHDR *hdr;
		NMMOUSE *mouse;
		NMITEMACTIVATE *itemactivate;
		NMLISTVIEW *listview;
		NMLVCUSTOMDRAW *lvcustomdraw;
		NMLVDISPINFO *lvdispinfo;
		NMLVKEYDOWN *lvkeydown;
	} const nm = { lParam };
	if (nm.hdr->hwndFrom == m_pFrame->m_wndStatusBar->m_hWnd)
	{
		if (nm.hdr->code == NM_CLICK)
		{
			switch (nm.mouse->dwItemSpec)
			{
			case 1:
				m_pFrame->m_pMDIFrame->PostMessage(WM_COMMAND, ID_TOOLS_FILTERS);
				break;
			}
		}
		return 0;
	}
	switch (nm.hdr->code)
	{
	case LVN_GETDISPINFO:
		ReflectGetdispinfo(nm.lvdispinfo);
		break;
	case LVN_KEYDOWN:
		return ReflectKeydown(nm.lvkeydown);
	case LVN_BEGINLABELEDIT:
		return ReflectBeginLabelEdit(nm.lvdispinfo);
	case LVN_ENDLABELEDIT:
		return ReflectEndLabelEdit(nm.lvdispinfo);
	case LVN_COLUMNCLICK:
		ReflectColumnClick(nm.listview);
		break;
	case NM_CLICK:
		ReflectClick(nm.itemactivate);
		break;
	case LVN_ITEMACTIVATE:
		ReflectItemActivate(nm.itemactivate);
		break;
	case LVN_BEGINDRAG:
		ReflectBeginDrag();
		break;
	case NM_CUSTOMDRAW:
		return ReflectCustomDraw(nm.lvcustomdraw);
	}
	return 0;
}

/**
 * @brief Remove any windows reordering of columns
 */
void CDirView::FixReordering()
{
	LVCOLUMN lvcol;
	lvcol.mask = LVCF_ORDER;
	lvcol.fmt = 0;
	lvcol.cx = 0;
	lvcol.pszText = 0;
	lvcol.iSubItem = 0;
	for (int i = 0; i < m_numcols; ++i)
	{
		lvcol.iOrder = i;
		SetColumn(i, &lvcol);
	}
}

/** @brief Add columns to display, loading width & order from registry. */
void CDirView::LoadColumnHeaderItems()
{
	bool dummyflag = false;

	HHeaderCtrl *const h = GetHeaderCtrl();
	if (h->GetItemCount())
	{
		dummyflag = true;
		while (h->GetItemCount() > 1)
			DeleteColumn(1);
	}

	for (int i = 0; i < m_dispcols; ++i)
	{
		LVCOLUMN lvc;
		lvc.mask = LVCF_FMT + LVCF_SUBITEM + LVCF_TEXT;
		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = 0;
		lvc.pszText = _T("text");
		lvc.iSubItem = i;
		InsertColumn(i, &lvc);
	}
	if (dummyflag)
		DeleteColumn(1);

}

/// Update all column widths (from registry to screen)
// Necessary when user reorders columns
void CDirView::SetColumnWidths()
{
	for (int i = 0; i < m_numcols; ++i)
	{
		int phy = ColLogToPhys(i);
		if (phy >= 0)
		{
			String sWidthKey = GetColRegValueNameBase(i) + _T("_Width");
			int w = max(10, SettingStore.GetProfileInt(_T("DirView"), sWidthKey.c_str(), DefColumnWidth));
			SetColumnWidth(m_colorder[i], w);
		}
	}
}

/** @brief store current column widths into registry */
void CDirView::SaveColumnWidths()
{
	for (int i = 0; i < m_numcols; i++)
	{
		int phy = ColLogToPhys(i);
		if (phy >= 0)
		{
			String sWidthKey = GetColRegValueNameBase(i) + _T("_Width");
			int w = GetColumnWidth(phy);
			SettingStore.WriteProfileInt(_T("DirView"), sWidthKey.c_str(), w);
		}
	}
}

/** @brief Fire off a resort of the data, to take place when things stabilize. */
void CDirView::InitiateSort()
{
	// Remove the windows reordering, as we're doing it ourselves
	FixReordering();
	// Now redraw screen
	UpdateColumnNames();
	SetColumnWidths();
	Redisplay();
}

/**
 * @brief Open dialog to customize dirview columns
 */
void CDirView::OnCustomizeColumns()
{
	// Located in DirViewColHandler.cpp
	OnEditColumns();
	SaveColumnOrders();
}

/**
 * @brief Fill string list with current dirview column registry key names
 */
void CDirView::GetCurrentColRegKeys(stl::vector<String> &colKeys)
{
	int nphyscols = GetHeaderCtrl()->GetItemCount();
	for (int col = 0; col < nphyscols; ++col)
	{
		int logcol = ColPhysToLog(col);
		colKeys.push_back(GetColRegValueNameBase(logcol));
	}
}

/**
 * @brief Generate report from dir compare results.
 */
void CDirView::OnToolsGenerateReport()
{
	// Make list of registry keys for columns
	// (needed for XML reports)
	stl::vector<String> colKeys;
	GetCurrentColRegKeys(colKeys);

	DirCmpReport report(colKeys);
	report.SetList(static_cast<HListView *>(m_pWnd));

	String left = m_pFrame->GetLeftBasePath().c_str();
	String right = m_pFrame->GetRightBasePath().c_str();

	// If inside archive, convert paths
	if (m_pFrame->IsArchiveFolders())
	{
		m_pFrame->ApplyLeftDisplayRoot(left);
		m_pFrame->ApplyRightDisplayRoot(right);
	}

	report.SetRootPaths(left.c_str(), right.c_str());
	report.SetColumns(m_dispcols);
	String errStr;
	if (report.GenerateReport(errStr))
	{
		if (errStr.empty())
			LanguageSelect.MsgBox(IDS_REPORT_SUCCESS, MB_OK | MB_ICONINFORMATION);
		else
			LanguageSelect.MsgBox(IDS_REPORT_ERROR, errStr.c_str(), MB_OK | MB_ICONSTOP);
	}
}

/**
 * @brief Add special items for non-recursive compare
 * to directory view.
 *
 * Currently only special item is ".." for browsing to
 * parent folders.
 * @return number of items added to view
 */
int CDirView::AddSpecialItems()
{
	int retVal = 0;
	BOOL bEnable = TRUE;
	String leftParent, rightParent;
	switch (m_pFrame->AllowUpwardDirectory(leftParent, rightParent))
	{
	case CDirFrame::AllowUpwardDirectory::No:
		bEnable = FALSE;
		// fall through
	default:
		AddParentFolderItem(bEnable);
		retVal = 1;
		// fall through
	case CDirFrame::AllowUpwardDirectory::Never:
		break;
	}
	return retVal;
}

/**
 * @brief Add "Parent folder" ("..") item to directory view
 */
void CDirView::AddParentFolderItem(BOOL bEnable)
{
	AddNewItem(0, SPECIAL_ITEM_POS, bEnable ? DIFFIMG_DIRUP : DIFFIMG_DIRUP_DISABLE, 0);
}

/**
 * @brief Zip selected files from left side.
 */
void CDirView::OnCtxtDirZipLeft()
{
	if (!HasZipSupport())
	{
		LanguageSelect.MsgBox(IDS_NO_ZIP_SUPPORT, MB_ICONINFORMATION);
		return;
	}

	DirItemEnumerator
	(
		this, LVNI_SELECTED
		|	DirItemEnumerator::Left
	).CompressArchive();
}

/**
 * @brief Zip selected files from right side.
 */
void CDirView::OnCtxtDirZipRight()
{
	if (!HasZipSupport())
	{
		LanguageSelect.MsgBox(IDS_NO_ZIP_SUPPORT, MB_ICONINFORMATION);
		return;
	}

	DirItemEnumerator
	(
		this, LVNI_SELECTED
		|	DirItemEnumerator::Right
	).CompressArchive();
}

/**
 * @brief Zip selected files from both sides, using original/altered format.
 */
void CDirView::OnCtxtDirZipBoth()
{
	if (!HasZipSupport())
	{
		LanguageSelect.MsgBox(IDS_NO_ZIP_SUPPORT, MB_ICONINFORMATION);
		return;
	}

	DirItemEnumerator
	(
		this, LVNI_SELECTED
		|	DirItemEnumerator::Original
		|	DirItemEnumerator::Altered
		|	DirItemEnumerator::BalanceFolders
	).CompressArchive();
}

/**
 * @brief Zip selected diffs from both sides, using original/altered format.
 */
void CDirView::OnCtxtDirZipBothDiffsOnly()
{
	if (!HasZipSupport())
	{
		LanguageSelect.MsgBox(IDS_NO_ZIP_SUPPORT, MB_ICONINFORMATION);
		return;
	}

	DirItemEnumerator
	(
		this, LVNI_SELECTED
		|	DirItemEnumerator::Original
		|	DirItemEnumerator::Altered
		|	DirItemEnumerator::BalanceFolders
		|	DirItemEnumerator::DiffsOnly
	).CompressArchive();
}


/**
 * @brief Select all visible items in dir compare
 */
void CDirView::OnSelectAll()
{
	// While the user is renaming an item, select all the edited text.
	if (HEdit *pEdit = GetEditControl())
	{
		pEdit->SetSel(pEdit->GetWindowTextLength());
	}
	else
	{
		int selCount = GetItemCount();

		for (int i = 0; i < selCount; i++)
		{
			// Don't select special items (SPECIAL_ITEM_POS)
			UINT_PTR diffpos = GetItemKey(i);
			if (diffpos != SPECIAL_ITEM_POS)
				SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
		}
	}
}

/**
 * @brief Resets column widths to defaults.
 */
void CDirView::ResetColumnWidths()
{
	for (int i = 0; i < m_numcols; i++)
	{
		int phy = ColLogToPhys(i);
		if (phy >= 0)
		{
			String sWidthKey = GetColRegValueNameBase(i) + _T("_Width");
			SettingStore.WriteProfileInt(_T("DirView"), sWidthKey.c_str(), DefColumnWidth);
		}
	}
}

/**
 * @brief Copy selected item left side paths (containing filenames) to clipboard.
 */
void CDirView::OnCopyLeftPathnames()
{
	if (!OpenClipboard())
		return;
	UniStdioFile file;
	file.SetUnicoding(UCS2LE);
	if (HGLOBAL hMem = file.CreateStreamOnHGlobal())
	{
		int sel = -1;
		while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
		{
			const DIFFITEM& di = GetDiffItem(sel);
			if (!di.diffcode.isSideRightOnly())
			{
				// Append space between paths. Space is better than
				// EOL since it allows copying to console/command line.
				if (GlobalSize(hMem) != 0)
					file.WriteString(_T(" "), 1);
				String path = di.GetLeftFilepath(GetDocument()->GetLeftBasePath());
				// If item is a folder then subfolder (relative to base folder)
				// is in filename member.
				path = paths_ConcatPath(path, di.left.filename);
				file.WriteString(path);
			}
		}
		file.WriteString(_T(""), 1);
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, hMem);
	}
	CloseClipboard();
}

/**
 * @brief Copy selected item right side paths (containing filenames) to clipboard.
 */
void CDirView::OnCopyRightPathnames()
{
	if (!OpenClipboard())
		return;
	UniStdioFile file;
	file.SetUnicoding(UCS2LE);
	if (HGLOBAL hMem = file.CreateStreamOnHGlobal())
	{
		int sel = -1;
		while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
		{
			const DIFFITEM& di = GetDiffItem(sel);
			if (!di.diffcode.isSideLeftOnly())
			{
				// Append space between paths. Space is better than
				// EOL since it allows copying to console/command line.
				if (GlobalSize(hMem) != 0)
					file.WriteString(_T(" "), 1);
				String path = di.GetRightFilepath(GetDocument()->GetRightBasePath());
				// If item is a folder then subfolder (relative to base folder)
				// is in filename member.
				path = paths_ConcatPath(path, di.right.filename);
				file.WriteString(path);
			}
		}
		file.WriteString(_T(""), 1);
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, hMem);
	}
	CloseClipboard();
}

/**
 * @brief Copy selected item both side paths (containing filenames) to clipboard.
 */
void CDirView::OnCopyBothPathnames()
{
	if (!OpenClipboard())
		return;
	UniStdioFile file;
	file.SetUnicoding(UCS2LE);
	if (HGLOBAL hMem = file.CreateStreamOnHGlobal())
	{
		int sel = -1;
		while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
		{
			const DIFFITEM& di = GetDiffItem(sel);
			if (!di.diffcode.isSideRightOnly())
			{
				// Append space between paths. Space is better than
				// EOL since it allows copying to console/command line.
				if (GlobalSize(hMem) != 0)
					file.WriteString(_T(" "), 1);
				String path = di.GetLeftFilepath(GetDocument()->GetLeftBasePath());
				// If item is a folder then subfolder (relative to base folder)
				// is in filename member.
				path = paths_ConcatPath(path, di.left.filename);
				file.WriteString(path);
			}
			if (!di.diffcode.isSideLeftOnly())
			{
				// Append space between paths. Space is better than
				// EOL since it allows copying to console/command line.
				if (GlobalSize(hMem) != 0)
					file.WriteString(_T(" "), 1);
				String path = di.GetRightFilepath(GetDocument()->GetRightBasePath());
				// If item is a folder then subfolder (relative to base folder)
				// is in filename member.
				path = paths_ConcatPath(path, di.right.filename);
				file.WriteString(path);
			}
		}
		file.WriteString(_T(""), 1);
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, hMem);
	}
	CloseClipboard();
}

/**
 * @brief Copy selected item filenames to clipboard.
 */
void CDirView::OnCopyFilenames()
{
	if (!OpenClipboard())
		return;
	UniStdioFile file;
	file.SetUnicoding(UCS2LE);
	if (HGLOBAL hMem = file.CreateStreamOnHGlobal())
	{
		int sel = -1;
		while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
		{
			const DIFFITEM& di = GetDiffItem(sel);
			if (!di.diffcode.isDirectory())
			{
				// Append space between paths. Space is better than
				// EOL since it allows copying to console/command line.
				if (GlobalSize(hMem) != 0)
					file.WriteString(_T(" "), 1);
				file.WriteString(di.left.filename);
			}
		}
		file.WriteString(_T(""), 1);
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, hMem);
	}
	CloseClipboard();
}

/**
 * @brief Rename a selected item on both sides.
 *
 */
void CDirView::OnItemRename()
{
	ASSERT(1 == GetSelectedCount());
	int nSelItem = GetNextItem(-1, LVNI_SELECTED);
	ASSERT(-1 != nSelItem);
	EditLabel(nSelItem);
}

/**
 * @brief hide selected item filenames (removes them from the ListView)
 */
void CDirView::OnHideFilenames()
{
	int sel = -1;
	SetRedraw(FALSE);	// Turn off updating (better performance)
	while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
	{
		UINT_PTR pos = GetItemKey(sel);
		if (pos == (UINT_PTR) SPECIAL_ITEM_POS)
			continue;
		m_pFrame->SetItemViewFlag(pos, ViewCustomFlags::HIDDEN, ViewCustomFlags::VISIBILITY);
		const DIFFITEM &di = GetDiffItem(sel);
		if (m_bTreeMode && di.diffcode.isDirectory())
		{
			int count = GetItemCount();
			for (int i = sel + 1; i < count; i++)
			{
				const DIFFITEM &dic = GetDiffItem(i);
				if (!dic.IsAncestor(&di))
					break;
				DeleteItem(i--);
				count--;
			}
		}
		DeleteItem(sel--);
		m_nHiddenItems++;
	}
	SetRedraw(TRUE);	// Turn updating back on
}

LRESULT CDirView::ReflectCustomDraw(NMLVCUSTOMDRAW *pNM)
{
	if (pNM->nmcd.dwDrawStage == CDDS_PREPAINT)
	{
		OnUpdateStatusNum();
	}
	return 0;
}

/**
 * @brief Called before user start to item label edit.
 *
 * Disable label edit if initiated from a user double-click.
 */
LRESULT CDirView::ReflectBeginLabelEdit(NMLVDISPINFO *pdi)
{
	if (IsItemSelectedSpecial())
		return 1;
	// If label edit is allowed.
	// Locate the edit box on the right column in case the user changed the
	// column order.
	const int nColPos = ColLogToPhys(0);

	// Get text from the "File Name" column.
	String sText = GetItemText(pdi->item.iItem, nColPos);
	ASSERT(!sText.empty());
	HEdit *pEdit = GetEditControl();
	pEdit->SetWindowText(sText.c_str());

	String::size_type i = sText.find('|');
	if (i != String::npos)
	{
		// Names differ in case. Pre-select right name so user can easily delete
		// it to make both names uniform.
		pEdit->SetSel(i, -1);
	}
	return 0;
}

/**
 * @brief Called when user done with item label edit.
 */
LRESULT CDirView::ReflectEndLabelEdit(NMLVDISPINFO *pdi)
{
	LRESULT lResult = 0;
	if (pdi->item.pszText && pdi->item.pszText[0])
		lResult = DoItemRename(pdi->item.pszText);
	return lResult;
}

/**
 * @brief Called to update the item count in the status bar
 */
void CDirView::OnUpdateStatusNum()
{
	TCHAR num[20], idx[20], cnt[20];

	int items = GetSelectedCount();
	_itot(items, num, 10);
	m_pFrame->SetStatus(LanguageSelect.FormatMessage(
		items == 1 ? IDS_STATUS_SELITEM1 : IDS_STATUS_SELITEMS, num));

	CMainFrame *const pMDIFrame = GetMainFrame();
	if (!pMDIFrame->GetActiveDocFrame()->IsChild(m_pWnd))
		return;

	m_pFrame->UpdateCmdUI<ID_L2R>();
	m_pFrame->UpdateCmdUI<ID_R2L>();
	m_pFrame->UpdateCmdUI<ID_MERGE_DELETE>();

	pMDIFrame->UpdateCmdUI<ID_FILE_ENCODING>(items != 0 ? 0 : MF_GRAYED);
	if (items == 1)
	{
		int i = GetNextItem(-1, LVNI_SELECTED);
		const DIFFITEM& di = GetDiffItem(i);
		if (!IsItemOpenable(di))
			items = 0;
	}

	int sel1, sel2;
	GetSelectedItems(&sel1, &sel2);
	pMDIFrame->UpdateCmdUI<ID_MERGE_COMPARE>
	(
		GetSelectedItems(&sel1, &sel2)
	&&	(
			sel2 == -1 && IsItemOpenable(GetDiffItem(sel1))
		||	AreItemsOpenable(GetDiffItem(sel1), GetDiffItem(sel2))
		) ? MF_ENABLED : MF_GRAYED
	);

	int count = GetItemCount();
	int focusItem = GetFocusedItem();

	pMDIFrame->UpdateCmdUI<ID_CURDIFF>( 
		GetItemState(focusItem, LVIS_SELECTED) ? MF_ENABLED : MF_GRAYED);

	BYTE enable = MF_GRAYED;
	int i = focusItem;
	while (--i >= 0)
	{
		const DIFFITEM& di = GetDiffItem(i);
		if (IsItemNavigableDiff(di))
		{
			enable = MF_ENABLED;
			break;
		}
	}
	pMDIFrame->UpdateCmdUI<ID_PREVDIFF>(enable);

	enable = MF_GRAYED;
	i = focusItem;
	while (++i < count)
	{
		const DIFFITEM& di = GetDiffItem(i);
		if (IsItemNavigableDiff(di))
		{
			enable = MF_ENABLED;
			break;
		}
	}
	pMDIFrame->UpdateCmdUI<ID_NEXTDIFF>(enable);

	String s;
	if (focusItem == -1)
	{
		// No item has focus
		_itot(count, cnt, 10);
		// "Items: %1"
		s = LanguageSelect.FormatMessage(IDS_DIRVIEW_STATUS_FMT_NOFOCUS, cnt);
	}
	else
	{
		// An item has focus
		// Don't show number to special items
		UINT_PTR pos = GetItemKey(focusItem);
		if (pos != SPECIAL_ITEM_POS)
		{
			// If compare is non-recursive reduce special items count
			BOOL bRecursive = GetDocument()->GetRecursive();
			if (!bRecursive)
			{
				--focusItem;
				--count;
			}
			_itot(focusItem + 1, idx, 10);
			_itot(count, cnt, 10);
			// "Item %1 of %2"
			s = LanguageSelect.FormatMessage(IDS_DIRVIEW_STATUS_FMT_FOCUS, idx, cnt);
		}
	}
	pMDIFrame->GetStatusBar()->SetPartText(2, s.c_str());
}

/**
 * @brief Show all hidden items.
 */
void CDirView::OnViewShowHiddenItems()
{
	GetDocument()->SetItemViewFlag(ViewCustomFlags::VISIBLE, ViewCustomFlags::VISIBILITY);
	m_nHiddenItems = 0;
	Redisplay();
}

/**
 * @brief Toggle Tree Mode
 */
void CDirView::OnViewTreeMode()
{
	m_bTreeMode = !m_bTreeMode;
	COptionsMgr::SaveOption(OPT_TREE_MODE, m_bTreeMode); // reverse
	Redisplay();
}

/**
 * @brief Expand all subfolders
 */
void CDirView::OnViewExpandAllSubdirs()
{
	CDiffContext *ctxt = m_pFrame->GetDiffContext();
	UINT_PTR diffpos = ctxt->GetFirstDiffPosition();
	while (diffpos)
	{
		DIFFITEM &di = ctxt->GetNextDiffRefPosition(diffpos);
		di.customFlags1 |= ViewCustomFlags::EXPANDED;
	}
	Redisplay();
}

/**
 * @brief Collapse all subfolders
 */
void CDirView::OnViewCollapseAllSubdirs()
{
	CDiffContext *ctxt = m_pFrame->GetDiffContext();
	UINT_PTR diffpos = ctxt->GetFirstDiffPosition();
	while (diffpos)
	{
		DIFFITEM &di = ctxt->GetNextDiffRefPosition(diffpos);
		di.customFlags1 &= ~ViewCustomFlags::EXPANDED;
	}
	Redisplay();
}

void CDirView::OnViewCompareStatistics()
{
	CompareStatisticsDlg dlg(GetDocument()->GetCompareStats());
	LanguageSelect.DoModal(dlg);
}

/**
 * @brief TRUE if selected item is a "special item".
 */
BOOL CDirView::IsItemSelectedSpecial()
{
	int nSelItem = GetNextItem(-1, LVNI_SELECTED);
	ASSERT(-1 != nSelItem);
	return (SPECIAL_ITEM_POS == GetItemKey(nSelItem));
}

/**
 * @brief Allow edit "Paste" when renaming an item.
 */
void CDirView::OnEditCopy()
{
	if (HEdit *pEdit = GetEditControl())
		pEdit->Copy();
}

/**
 * @brief Allow edit "Cut" when renaming an item.
 */
void CDirView::OnEditCut()
{
	if (HEdit *pEdit = GetEditControl())
		pEdit->Cut();
}

/**
* @brief Allow edit "Paste" when renaming an item.
 */
void CDirView::OnEditPaste()
{
	if (HEdit *pEdit = GetEditControl())
		pEdit->Paste();
}

/**
 * @brief Allow edit "Undo" when renaming an item.
 */
void CDirView::OnEditUndo()
{
	if (HEdit *pEdit = GetEditControl())
		pEdit->Undo();
}

LPCTSTR CDirView::SuggestArchiveExtensions()
{
	// 7z can only write 7z, zip, and tar(.gz|.bz2) archives, so don't suggest
	// other formats here.
	static const TCHAR extensions[]
	(
		_T("7z\0*.7z\0")
		//_T("z\0*.z\0")
		_T("zip\0*.zip\0")
		_T("jar (zip)\0*.jar\0")
		_T("ear (zip)\0*.ear\0")
		_T("war (zip)\0*.war\0")
		_T("xpi (zip)\0*.xpi\0")
		//_T("rar\0*.rar\0")
		_T("tar\0*.tar\0")
		_T("tar.z\0*.tar.z\0")
		_T("tar.gz\0*.tar.gz\0")
		_T("tar.bz2\0*.tar.bz2\0")
		//_T("tz\0*.tz\0")
		_T("tgz\0*.tgz\0")
		_T("tbz2\0*.tbz2\0")
		//_T("lzh\0*.lzh\0")
		//_T("cab\0*.cab\0")
		//_T("arj\0*.arj\0")
		//_T("deb\0*.deb\0")
		//_T("rpm\0*.rpm\0")
		//_T("cpio\0*.cpio\0")
		//_T("\0")
	);
	return extensions;
}

bool CDirView::IsShellMenuCmdID(UINT id)
{
	return (id >= LeftCmdFirst) && (id <= RightCmdLast);
}

void CDirView::HandleMenuSelect(WPARAM wParam, LPARAM lParam)
{
	if (HIWORD(wParam) & MF_POPUP)
	{
		HMENU hMenu = reinterpret_cast<HMENU>(lParam);
		MENUITEMINFO mii;
		mii.cbSize = sizeof mii;
		mii.fMask = MIIM_SUBMENU;
		::GetMenuItemInfo(hMenu, LOWORD(wParam), TRUE, &mii);
		HMENU hSubMenu = mii.hSubMenu;
		if (hSubMenu == m_hShellContextMenuLeft)
		{
			mii.hSubMenu = ListShellContextMenu(SIDE_LEFT);
			m_hShellContextMenuLeft = NULL;
		}
		else if (hSubMenu == m_hShellContextMenuRight)
		{
			mii.hSubMenu = ListShellContextMenu(SIDE_RIGHT);
			m_hShellContextMenuRight = NULL;
		}
		if (hSubMenu != mii.hSubMenu)
		{
			::SetMenuItemInfo(hMenu, LOWORD(wParam), TRUE, &mii);
			::DestroyMenu(hSubMenu);
		}
	}
}

/**
 * @brief Handle messages related to correct menu working.
 *
 * We need to requery shell context menu each time we switch from context menu
 * for one side to context menu for other side. Here we check whether we need to
 * requery and call ShellContextMenuHandleMenuMessage.
 */
LRESULT CDirView::HandleMenuMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT res = 0;
	if (message == WM_INITMENUPOPUP)
	{
		HMenu *const pMenu = reinterpret_cast<HMenu *>(wParam);
		if (pMenu == m_pCompareAsScriptMenu)
			m_pCompareAsScriptMenu = GetMainFrame()->SetScriptMenu(pMenu, "CompareAs.Menu");
	}
	m_pShellContextMenuLeft->HandleMenuMessage(message, wParam, lParam, res);
	m_pShellContextMenuRight->HandleMenuMessage(message, wParam, lParam, res);
	return res;
}

/** 
 * @brief Retrieves file list of all selected files, and store them like 
 * file_path1\nfile_path2\n...file_pathN\n
 *
 * @param [out] pstm Stream where file list will be stored
 */
void CDirView::PrepareDragData(UniStdioFile &file)
{
	const CDiffContext *ctxt = GetDocument()->GetDiffContext();
	int i = -1; 
	while ((i = GetNextItem(i, LVNI_SELECTED)) != -1)
	{ 
		const DIFFITEM &diffitem = GetItemAt(i);
		// check for special items (e.g not "..") 
		if (diffitem.diffcode.diffcode == 0) 
		{ 
			continue; 
		} 

		if (diffitem.diffcode.isSideLeftOnly()) 
		{ 
			String spath = diffitem.GetLeftFilepath(ctxt->GetLeftPath()); 
			spath = paths_ConcatPath(spath, diffitem.left.filename);
			file.WriteString(spath);
		} 
		else if (diffitem.diffcode.isSideRightOnly()) 
		{ 
			String spath = diffitem.GetRightFilepath(ctxt->GetRightPath()); 
			spath = paths_ConcatPath(spath, diffitem.right.filename); 
			file.WriteString(spath);
		} 
		else if (diffitem.diffcode.isSideBoth()) 
		{ 
			// when both files equal, there is no difference between what file to drag 
			// so we put file from the left panel 
			String spath = diffitem.GetLeftFilepath(ctxt->GetLeftPath()); 
			spath = paths_ConcatPath(spath, diffitem.left.filename); 
			file.WriteString(spath);

			// if both files are different then we also put file from the right panel 
			if (diffitem.diffcode.isResultDiff()) 
			{ 
				file.WriteString(_T("\n"), 1); // end of left file path 
				String spath = diffitem.GetRightFilepath(ctxt->GetRightPath()); 
				spath = paths_ConcatPath(spath, diffitem.right.filename); 
				file.WriteString(spath);
			} 
		} 
		file.WriteString(_T("\n"), 1); // end of file path 
	}
	file.WriteString(_T(""), 1); // include terminating zero
} 

/** 
 * @brief Drag files/directories from folder compare listing view. 
 */ 
void CDirView::ReflectBeginDrag()
{
	DWORD de;
	DoDragDrop(this, this, DROPEFFECT_COPY, &de);
}

HRESULT CDirView::QueryInterface(REFIID iid, void **ppv)
{
    static const QITAB rgqit[] = 
    {   
        QITABENT(CDirView, IDropSource),
        QITABENT(CDirView, IDataObject),
        { 0 }
    };
    return QISearch(this, rgqit, iid, ppv);
}

ULONG CDirView::AddRef()
{
	return 1;
}

ULONG CDirView::Release()
{
	return 1;
}

HRESULT CDirView::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
	if (fEscapePressed)
		return DRAGDROP_S_CANCEL;
	if (!(grfKeyState & MK_LBUTTON))
		return DRAGDROP_S_DROP;
	return S_OK;
}

HRESULT CDirView::GiveFeedback(DWORD dwEffect)
{
	return DRAGDROP_S_USEDEFAULTCURSORS;
}

HRESULT CDirView::GetData(LPFORMATETC etc, LPSTGMEDIUM stg)
{
	if (etc->cfFormat != CF_UNICODETEXT)
		return DV_E_FORMATETC;
	stg->tymed = TYMED_HGLOBAL;
	UniStdioFile file;
	file.SetUnicoding(UCS2LE);
	stg->hGlobal = file.CreateStreamOnHGlobal();
	PrepareDragData(file);
	stg->pUnkForRelease = NULL;
	return S_OK;
}

HRESULT CDirView::GetDataHere(LPFORMATETC, LPSTGMEDIUM)
{
	return E_NOTIMPL;
}

HRESULT CDirView::QueryGetData(LPFORMATETC etc)
{
	if (etc->cfFormat != CF_UNICODETEXT)
		return DV_E_FORMATETC;
	return S_OK;
}

HRESULT CDirView::GetCanonicalFormatEtc(LPFORMATETC, LPFORMATETC)
{
	return E_NOTIMPL;
}

HRESULT CDirView::SetData(LPFORMATETC, LPSTGMEDIUM, BOOL)
{
	return E_NOTIMPL;
}

HRESULT CDirView::EnumFormatEtc(DWORD, LPENUMFORMATETC*)
{
	return E_NOTIMPL;
}

HRESULT CDirView::DAdvise(LPFORMATETC, DWORD, LPADVISESINK, LPDWORD)
{
	return E_NOTIMPL;
}

HRESULT CDirView::DUnadvise(DWORD)
{
	return E_NOTIMPL;
}

HRESULT CDirView::EnumDAdvise(LPENUMSTATDATA*)
{
	return E_NOTIMPL;
}
