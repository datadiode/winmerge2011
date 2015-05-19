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
 * @file  DirFrame.cpp
 *
 * @brief Implementation file for CDirFrame
 *
 */
#include "stdafx.h"
#include "Merge.h"
#include "MainFrm.h"
#include "HexMergeFrm.h"
#include "7zCommon.h"
#include "LanguageSelect.h"
#include "DirFrame.h"
#include "DirView.h"
#include "Common/coretools.h"
#include "paths.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/**
 * @brief Location for folder compare specific help to open.
 */
static const TCHAR DirViewHelpLocation[] = _T("::/htmlhelp/Compare_dirs.html");

/**
 * @brief Statusbar pane indexes
 */
enum
{
	PANE_FILTER = 1,
	PANE_LEFT_RO,
	PANE_RIGHT_RO,
};

/**
 * @brief Width of filter name pane in statusbar
 */
const int FILTER_PANEL_WIDTH = 180;

/**
 * @brief RO status panel width
 */
static UINT RO_PANEL_WIDTH = 40;

/////////////////////////////////////////////////////////////////////////////
// CDirFrame

const LONG CDirFrame::FloatScript[] =
{
	IDC_STATIC_TITLE_LEFT, BY<500>::X2R,
	IDC_STATIC_TITLE_RIGHT, BY<500>::X2L | BY<1000>::X2R,
	0x1000, BY<1000>::X2R | BY<1000>::Y2B,
	0x6000, BY<1000>::Y2T | BY<1000>::Y2B | BY<1000>::X2R,
	0
};

CDirFrame::CDirFrame(CMainFrame *pMDIFrame)
: CDocFrame(pMDIFrame, GetHandleSet<IDR_DIRDOCTYPE>(), FloatScript)
, m_wndStatusBar(NULL)
, m_bROLeft(FALSE)
, m_bRORight(FALSE)
#pragma warning(disable:warning_this_used_in_base_member_initializer_list)
, m_pDirView(new CDirView(this))
#pragma warning(default:warning_this_used_in_base_member_initializer_list)
, m_pCtxt(NULL)
, m_pCompareStats(new CompareStats)
, m_nRecursive(0)
, m_pTempPathContext(NULL)
{
	SubclassWindow(pMDIFrame->CreateChildHandle());
	LoadIcon(CompareStats::DIFFIMG_DIR);
	CreateClient();
	RecalcLayout();
	m_bAutoDelete = true;
}

CDirFrame::~CDirFrame()
{
	DeleteContext();
	delete m_pCompareStats;
	// Inform all of our merge docs that we're closing
	MergeDocPtrList::iterator ppMergeDoc = m_MergeDocs.begin();
	while (ppMergeDoc != m_MergeDocs.end())
	{
		CChildFrame *pMergeDoc = *ppMergeDoc++;
		pMergeDoc->DirDocClosing(this);
	}
	HexMergeDocPtrList::iterator ppHexMergeDoc = m_HexMergeDocs.begin();
	while (ppHexMergeDoc != m_HexMergeDocs.end())
	{
		CHexMergeFrame *pHexMergeDoc = *ppHexMergeDoc++;
		pHexMergeDoc->DirDocClosing(this);
	}
	// Delete all temporary folders belonging to this document
	while (m_pTempPathContext)
	{
		WaitStatusCursor waitstatus(IDS_STATUS_DELETEFILES);
		m_pTempPathContext = m_pTempPathContext->DeleteHead();
	}
	delete m_pDirView;
}

void CDirFrame::DeleteContext()
{
	while ((m_pCtxt = static_cast<CDiffContext *>(m_root.IsSibling(m_root.Flink))) != NULL)
	{
		m_pCtxt->RemoveSelf();
		delete m_pCtxt;
	}
}

void CDirFrame::DeleteTempPathContext()
{
	// Delete the diff contexts belonging to the temp path context
	LIST_ENTRY *p = &m_root;
	while ((m_pCtxt = static_cast<CDiffContext *>(m_root.IsSibling(p = p->Flink))) != NULL)
	{
		if (m_pCtxt->GetLeftPath().find(m_pTempPathContext->m_strLeftRoot) == 0 &&
			m_pCtxt->GetRightPath().find(m_pTempPathContext->m_strRightRoot) == 0)
		{
			p = p->Blink;
			m_pCtxt->RemoveSelf();
			delete m_pCtxt;
		}
	}
	// Delete the temp path context
	m_pTempPathContext = m_pTempPathContext->DeleteHead();
}

/**
 * @brief Update left-readonly menu item
 */
template<>
void CDirFrame::UpdateCmdUI<ID_FILE_LEFT_READONLY>()
{
	m_pMDIFrame->UpdateCmdUI<ID_FILE_LEFT_READONLY>(m_bROLeft ? MF_CHECKED : 0);
}

/**
 * @brief Update right-side readonly menuitem
 */
template<>
void CDirFrame::UpdateCmdUI<ID_FILE_RIGHT_READONLY>()
{
	m_pMDIFrame->UpdateCmdUI<ID_FILE_RIGHT_READONLY>(m_bRORight ? MF_CHECKED : 0);
}

/**
 * @brief Update collect mode menuitem and statusbar indicator
 */
template<>
void CDirFrame::UpdateCmdUI<ID_FILE_COLLECTMODE>()
{
	m_pMDIFrame->UpdateCmdUI<ID_FILE_COLLECTMODE>(
		m_pMDIFrame->m_pCollectingDirFrame == this ?
		MF_CHECKED : MF_UNCHECKED);
	m_pMDIFrame->GetStatusBar()->SetPartText(1,
		m_pMDIFrame->m_pCollectingDirFrame == this ?
		LanguageSelect.LoadString(IDS_DIRVIEW_STATUS_COLLECTMODE).c_str() : NULL);
}

/**
 * @brief Check/Uncheck 'Tree Mode' menuitem.
 */
template<>
void CDirFrame::UpdateCmdUI<ID_VIEW_TREEMODE>()
{
	m_pMDIFrame->UpdateCmdUI<ID_VIEW_TREEMODE>(
		(m_pDirView->m_bTreeMode ? MF_CHECKED : MF_UNCHECKED) |
		(GetRecursive() == 1 ? MF_ENABLED : MF_GRAYED));
	m_pMDIFrame->UpdateCmdUI<ID_VIEW_EXPAND_ALLSUBDIRS>(
		m_pDirView->m_bTreeMode && GetRecursive() == 1 ? MF_ENABLED : MF_GRAYED);
}

template<>
void CDirFrame::UpdateCmdUI<ID_VIEW_SHOWHIDDENITEMS>()
{
	BYTE enable = m_pDirView->m_nHiddenItems ? MF_ENABLED : MF_GRAYED;
	m_pMDIFrame->UpdateCmdUI<ID_VIEW_SHOWHIDDENITEMS>(enable);
}

template<>
void CDirFrame::UpdateCmdUI<ID_REFRESH>()
{
	m_pMDIFrame->UpdateCmdUI<ID_REFRESH>(m_nRecursive != 3 &&
		waitStatusCursor.GetMsgId() != IDS_STATUS_RESCANNING ? MF_ENABLED : MF_GRAYED);
}

template<>
void CDirFrame::UpdateCmdUI<ID_L2R>()
{
	BYTE enable = MF_GRAYED;
	if (!m_bRORight)
	{
		int i = -1;
		while ((i = m_pDirView->GetNextItem(i, LVNI_SELECTED)) != -1)
		{
			const DIFFITEM *di = m_pDirView->GetDiffItem(i);
			if (di == NULL)
				break;
			if (di->isSideLeftOrBoth())
			{
				enable = MF_ENABLED;
				break;
			}
		}
	}
	m_pMDIFrame->UpdateCmdUI<ID_L2R>(enable);
}

template<>
void CDirFrame::UpdateCmdUI<ID_R2L>()
{
	BYTE enable = MF_GRAYED;
	if (!m_bROLeft)
	{
		int i = -1;
		while ((i = m_pDirView->GetNextItem(i, LVNI_SELECTED)) != -1)
		{
			const DIFFITEM *di = m_pDirView->GetDiffItem(i);
			if (di == NULL)
				break;
			if (di->isSideRightOrBoth())
			{
				enable = MF_ENABLED;
				break;
			}
		}
	}
	m_pMDIFrame->UpdateCmdUI<ID_R2L>(enable);
}

template<>
void CDirFrame::UpdateCmdUI<ID_MERGE_DELETE>()
{
	BYTE enable = MF_GRAYED;
	// If both sides are read-only, then there is nothing to delete
	if (!m_bROLeft || !m_bRORight)
	{
		// Enable if one deletable item is found
		int i = -1;
		while ((i = m_pDirView->GetNextItem(i, LVNI_SELECTED)) != -1)
		{
			const DIFFITEM *di = m_pDirView->GetDiffItem(i);
			if (di == NULL)
				break;
			if (di->isDeletableOnLeft() && !m_bROLeft ||
				di->isDeletableOnRight() && !m_bRORight)
			{
				enable = MF_ENABLED;
				break;
			}
		}
	}
	m_pMDIFrame->UpdateCmdUI<ID_MERGE_DELETE>(enable);
}

template<>
LRESULT CDirFrame::OnWndMsg<WM_COMMAND>(WPARAM wParam, LPARAM lParam)
{
	switch (UINT id = lParam ? static_cast<UINT>(wParam) : LOWORD(wParam))
	{
	case ID_FILE_LEFT_READONLY:
		// Change left-side readonly-status
		SetLeftReadOnly(!m_bROLeft);
		UpdateCmdUI<ID_FILE_LEFT_READONLY>();
		UpdateCmdUI<ID_R2L>();
		UpdateCmdUI<ID_MERGE_DELETE>();
		break;
	case ID_FILE_RIGHT_READONLY:
		// Change right-side readonly-status
		SetRightReadOnly(!m_bRORight);
		UpdateCmdUI<ID_FILE_RIGHT_READONLY>();
		UpdateCmdUI<ID_L2R>();
		UpdateCmdUI<ID_MERGE_DELETE>();
		break;
	case ID_DIR_RESCAN:
		if (int nCompareSelected = m_pDirView->MarkSelectedForRescan())
			Rescan(nCompareSelected);
		break;
	case ID_REFRESH:
		Rescan();
		break;
	case ID_FIRSTDIFF:
		m_pDirView->OnFirstdiff();
		break;
	case ID_LASTDIFF:
		m_pDirView->OnLastdiff();
		break;
	case ID_NEXTDIFF:
		m_pDirView->OnNextdiff();
		break;
	case ID_PREVDIFF:
		m_pDirView->OnPrevdiff();
		break;
	case ID_CURDIFF:
		m_pDirView->OnCurdiff();
		break;
	case ID_EDIT_COPY:
		m_pDirView->OnEditCopy();
		break;
	case ID_EDIT_CUT:
		m_pDirView->OnEditCut();
		break;
	case ID_EDIT_PASTE:
		m_pDirView->OnEditPaste();
		break;
	case ID_EDIT_UNDO:
		m_pDirView->OnEditUndo();
		break;
	case ID_DIR_COPY_PATHNAMES_LEFT:
		m_pDirView->OnCopyLeftPathnames();
		break;
	case ID_DIR_COPY_PATHNAMES_RIGHT:
		m_pDirView->OnCopyRightPathnames();
		break;
	case ID_DIR_COPY_PATHNAMES_BOTH:
		m_pDirView->OnCopyBothPathnames();
		break;
	case ID_DIR_DEL_LEFT:
		m_pDirView->DoDelLeft();
		break;
	case ID_DIR_DEL_RIGHT:
		m_pDirView->DoDelRight();
		break;
	case ID_DIR_DEL_BOTH:
		m_pDirView->DoDelBoth();
		break;
	case ID_MERGE_DELETE:
		m_pDirView->DoDelAll();
		break;
	case ID_DIR_COPY_FILENAMES:
		m_pDirView->OnCopyFilenames();
		break;
	case ID_DIR_OPEN_LEFT:
		m_pDirView->DoOpen(CDirView::SIDE_LEFT);
		break;
	case ID_DIR_OPEN_RIGHT:
		m_pDirView->DoOpen(CDirView::SIDE_RIGHT);
		break;
	case ID_DIR_OPEN_LEFT_WITH:
		m_pDirView->DoOpenWith(CDirView::SIDE_LEFT);
		break;
	case ID_DIR_OPEN_RIGHT_WITH:
		m_pDirView->DoOpenWith(CDirView::SIDE_RIGHT);
		break;
	case ID_DIR_OPEN_LEFT_WITHEDITOR:
		m_pDirView->DoOpenWithEditor(CDirView::SIDE_LEFT);
		break;
	case ID_DIR_OPEN_RIGHT_WITHEDITOR:
		m_pDirView->DoOpenWithEditor(CDirView::SIDE_RIGHT);
		break;
	case ID_DIR_OPEN_LEFT_FOLDER:
		m_pDirView->DoOpenFolder(CDirView::SIDE_LEFT);
		break;
	case ID_DIR_OPEN_RIGHT_FOLDER:
		m_pDirView->DoOpenFolder(CDirView::SIDE_RIGHT);
		break;
	case ID_L2R:
	case ID_DIR_COPY_LEFT_TO_RIGHT:
		m_pDirView->DoCopyLeftToRight();
		break;
	case ID_R2L:
	case ID_DIR_COPY_RIGHT_TO_LEFT:
		m_pDirView->DoCopyRightToLeft();
		break;
	case ID_DIR_COPY_LEFT_TO_BROWSE:
		m_pDirView->DoCopyLeftTo();
		break;
	case ID_DIR_COPY_RIGHT_TO_BROWSE:
		m_pDirView->DoCopyRightTo();
		break;
	case ID_DIR_MOVE_LEFT_TO_BROWSE:
		m_pDirView->DoMoveLeftTo();
		break;
	case ID_DIR_MOVE_RIGHT_TO_BROWSE:
		m_pDirView->DoMoveRightTo();
		break;
	case ID_FILE_MERGINGMODE: // This is to catch the VK_F9 accelerator
	case ID_FILE_COLLECTMODE:
		m_pMDIFrame->m_pCollectingDirFrame =
			m_pMDIFrame->m_pCollectingDirFrame != this ? this : NULL;
		UpdateCmdUI<ID_FILE_COLLECTMODE>();
		break;
	case ID_FILE_ENCODING:
		m_pDirView->DoFileEncodingDialog();
		break;
	case ID_EDIT_SELECT_ALL:
		m_pDirView->OnSelectAll();
		break;
	case ID_MERGE_COMPARE:
	case ID_MERGE_COMPARE_TEXT:
	case ID_MERGE_COMPARE_ZIP:
	case ID_MERGE_COMPARE_HEX:
	case ID_MERGE_COMPARE_XML:
		m_pDirView->OpenSelection(NULL, id);
		break;
	case ID_VIEW_TREEMODE:
		m_pDirView->OnViewTreeMode();
		UpdateCmdUI<ID_VIEW_TREEMODE>();
		break;
	case ID_DIR_HIDE_FILENAMES:
		m_pDirView->OnHideFilenames();
		UpdateCmdUI<ID_VIEW_SHOWHIDDENITEMS>();
		break;
	case ID_VIEW_SHOWHIDDENITEMS:
		m_pDirView->OnViewShowHiddenItems();
		UpdateCmdUI<ID_VIEW_SHOWHIDDENITEMS>();
		break;
	case ID_VIEW_EXPAND_ALLSUBDIRS:
		m_pDirView->OnViewExpandAllSubdirs();
		break;
	case ID_VIEW_COLLAPSE_ALLSUBDIRS:
		m_pDirView->OnViewCollapseAllSubdirs();
		break;
	case ID_VIEW_DIR_STATISTICS:
		m_pDirView->OnViewCompareStatistics();
		break;
	case ID_TOOLS_CUSTOMIZECOLUMNS:
		m_pDirView->OnCustomizeColumns();
		break;
	case ID_TOOLS_GENERATEREPORT:
		m_pDirView->OnToolsGenerateReport();
		break;
	case ID_TOOLS_SAVE_TO_XLS_ALL:
		m_pDirView->OnToolsSaveToXLS(NULL);
		break;
	case ID_TOOLS_SAVE_TO_XLS_SELECTED:
		m_pDirView->OnToolsSaveToXLS(NULL, LVNI_SELECTED);
		break;
	case ID_TOOLS_VIEW_AS_XLS_ALL:
		m_pDirView->OnToolsSaveToXLS(_T("open"));
		break;
	case ID_TOOLS_VIEW_AS_XLS_SELECTED:
		m_pDirView->OnToolsSaveToXLS(_T("open"), LVNI_SELECTED);
		break;
	case ID_TOOLS_PRINT_AS_XLS_ALL:
		m_pDirView->OnToolsSaveToXLS(_T("print"));
		break;
	case ID_TOOLS_PRINT_AS_XLS_SELECTED:
		m_pDirView->OnToolsSaveToXLS(_T("print"), LVNI_SELECTED);
		break;
	case ID_DIR_ZIP_LEFT:
		m_pDirView->OnCtxtDirZipLeft();
		break;
	case ID_DIR_ZIP_RIGHT:
		m_pDirView->OnCtxtDirZipRight();
		break;
	case ID_DIR_ZIP_BOTH:
		m_pDirView->OnCtxtDirZipBoth();
		break;
	case ID_DIR_ZIP_BOTH_DIFFS_ONLY:
		m_pDirView->OnCtxtDirZipBothDiffsOnly();
		break;
	case ID_DIR_ITEM_RENAME:
		m_pDirView->OnItemRename();
		break;
	case IDC_STATIC_TITLE_LEFT:
	case IDC_STATIC_TITLE_RIGHT:
		InitMrgmanCompare();
		break;
	case ID_NEXT_PANE:
	case ID_WINDOW_CHANGE_PANE:
		if (HWindow *pWndNext = GetNextDlgTabItem(HWindow::GetFocus()))
		{
			pWndNext->SetFocus();
		}
		break;
	case IDCANCEL:
		PostMessage(WM_CLOSE);
		break;
	}
	return 0;
}

template<>
LRESULT CDirFrame::OnWndMsg<WM_NCACTIVATE>(WPARAM wParam, LPARAM)
{
	if (wParam)
	{
		m_pMDIFrame->InitCmdUI();
		UpdateCmdUI<ID_FILE_LEFT_READONLY>();
		UpdateCmdUI<ID_FILE_RIGHT_READONLY>();
		UpdateCmdUI<ID_FILE_COLLECTMODE>();
		UpdateCmdUI<ID_VIEW_TREEMODE>();
		UpdateCmdUI<ID_VIEW_SHOWHIDDENITEMS>();
		UpdateCmdUI<ID_REFRESH>();
		m_pMDIFrame->UpdateCmdUI<ID_TOOLS_GENERATEREPORT>(MF_ENABLED);
	}
	return 0;
}

template<>
LRESULT CDirFrame::OnWndMsg<WM_CONTEXTMENU>(WPARAM, LPARAM lParam)
{
	POINT point;
	POINTSTOPOINT(point, lParam);
	if (lParam == MAKELPARAM(-1, -1))
	{
		point.x = point.y = 5;
		m_pDirView->ClientToScreen(&point);
		m_pDirView->ListContextMenu(point);
	}
	else if (m_pDirView->GetHeaderCtrl()->SendMessage(WM_NCHITTEST, 0, lParam) == HTCLIENT)
	{
		m_pDirView->HeaderContextMenu(point);
	}
	else if (m_pDirView->SendMessage(WM_NCHITTEST, 0, lParam) == HTCLIENT)
	{
		m_pDirView->ListContextMenu(point);
	}
	return 0;
}

LRESULT CDirFrame::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
		if (PreventFromClosing())
			return 0;
		if (m_pMDIFrame->m_pCollectingDirFrame == this)
			m_pMDIFrame->m_pCollectingDirFrame = NULL;
		break;
	case WM_COMMAND:
		if (LRESULT lResult = OnWndMsg<WM_COMMAND>(wParam, lParam))
			return lResult;
		break;
	case WM_HELP:
		m_pMDIFrame->ShowHelp(DirViewHelpLocation);
		return 0;
	case WM_NOTIFY:
		if (LRESULT lResult = m_pDirView->ReflectNotify(reinterpret_cast<UNotify *>(lParam)))
			return lResult;
		break;
	case WM_NCACTIVATE:
		OnWndMsg<WM_NCACTIVATE>(wParam, lParam);
		break;
	case WM_CONTEXTMENU:
		OnWndMsg<WM_CONTEXTMENU>(wParam, lParam);
		break;
	}
	return CDocFrame::WindowProc(uMsg, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////
// CDirFrame message handlers

void CDirFrame::CreateClient()
{
	// Dir frame has a header bar at top
	m_wndFilePathBar.Create(m_hWnd);
	RECT rect;
	m_wndFilePathBar.GetClientRect(&rect);
	// Directory frame has a status bar
	HWindow *pParent = wine_version ? m_wndFilePathBar.m_pWnd : m_pWnd;
	m_wndStatusBar = HStatusBar::Create(
		WS_CHILD | WS_VISIBLE | CCS_NOPARENTALIGN,
		rect.left, rect.top, rect.right - rect.left, rect.bottom,
		pParent, 0x6000);
	m_wndStatusBar->GetWindowRect(&rect);
	pParent->ScreenToClient(&rect);
	m_wndStatusBar->SetParent(m_wndFilePathBar.m_pWnd);
	rect.bottom -= rect.top;
	m_wndStatusBar->SetStyle(m_wndStatusBar->GetStyle() | CCS_NORESIZE);

	// Show selection all the time, so user can see current item even when
	// focus is elsewhere (ie, on file edit window)
	HListView *pLv = HListView::Create(
		WS_CHILD | WS_VISIBLE | WS_TABSTOP |
		LVS_REPORT | LVS_SHOWSELALWAYS | LVS_EDITLABELS | LVS_SHAREIMAGELISTS,
		0, rect.bottom, rect.right, rect.top - rect.bottom, m_pWnd, 0x1000);
	pLv->SetParent(m_wndFilePathBar.m_pWnd);
	m_pDirView->SubclassWindow(pLv);
	m_pDirView->OnInitialUpdate();
	// Align appearance of file path edit controls with statusbar.
	HFont *pFont = m_wndStatusBar->GetFont();
	int cyBar = rect.bottom;
	HEdit *pEdit = m_wndFilePathBar.GetControlRect(0, &rect);
	pEdit->SetWindowPos(NULL, 0, 0, rect.right - rect.left, cyBar, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	pEdit->SetFont(pFont);
	pEdit = m_wndFilePathBar.GetControlRect(1, &rect);
	pEdit->SetWindowPos(NULL, 0, 0, rect.right - rect.left, cyBar, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	pEdit->SetFont(pFont);
	// draw the headers as active ones
	m_wndFilePathBar.SetActive(0, true);
	m_wndFilePathBar.SetActive(1, true);
}

/**
 * @brief Set statusbar text
 */
void CDirFrame::SetStatus(LPCTSTR szStatus)
{
	m_wndStatusBar->SetPartText(0, szStatus);
}

/**
 * @brief Set active filter name to statusbar
 * @param [in] szFilter Filtername to show
 */
void CDirFrame::SetFilterStatusDisplay(LPCTSTR szFilter)
{
	m_wndStatusBar->SetPartText(PANE_FILTER, szFilter);
}

/**
 * @brief Update any resources necessary after a GUI language change
 */
void CDirFrame::UpdateResources()
{
	m_pDirView->UpdateResources();
	Redisplay();
}

bool CDirFrame::PreventFromClosing()
{
	if (m_pCtxt && m_pCtxt->IsBusy())
		return true;
	if (!CanFrameClose())
	{
		SendMessage(WM_SYSCOMMAND, SC_NEXTWINDOW);
		return true;
	}
	return false;
}

void CDirFrame::AlignStatusBar(int cx)
{
	if (m_wndStatusBar)
	{
		int parts[4];
		parts[3] = cx;
		cx -= RO_PANEL_WIDTH;
		parts[2] = cx;
		cx -= RO_PANEL_WIDTH;
		parts[1] = cx;
		cx -= FILTER_PANEL_WIDTH;
		parts[0] = cx;
		m_wndStatusBar->SetParts(4, parts);
	}
}

/**
 * @brief Set left side readonly-status
 * @param bReadOnly New status
 */
void CDirFrame::SetLeftReadOnly(BOOL bReadOnly)
{
	m_bROLeft = bReadOnly;
	m_wndStatusBar->SetPartText(PANE_LEFT_RO, m_bROLeft ?
		LanguageSelect.LoadString(IDS_STATUSBAR_READONLY).c_str() : NULL);
}

/**
 * @brief Set right side readonly-status
 * @param bReadOnly New status
 */
void CDirFrame::SetRightReadOnly(BOOL bReadOnly)
{
	m_bRORight = bReadOnly;
	m_wndStatusBar->SetPartText(PANE_RIGHT_RO, m_bRORight ?
		LanguageSelect.LoadString(IDS_STATUSBAR_READONLY).c_str() : NULL);
}

bool CDirFrame::AddToCollection(FileLocation &filelocLeft, FileLocation &filelocRight)
{
	const String &lpath = m_pCtxt->GetLeftPath();
	const String &rpath = m_pCtxt->GetRightPath();
	LPCTSTR rname = EatPrefix(filelocRight.filepath.c_str(), rpath.c_str());
	if (rname == NULL)
	{
		std::swap(filelocLeft, filelocRight);
		rname = EatPrefix(filelocRight.filepath.c_str(), rpath.c_str());
		if (rname == NULL)
		{
			return false;
		}
	}
	LPCTSTR lname = EatPrefix(filelocLeft.filepath.c_str(), lpath.c_str());
	if (lname == NULL)
	{
		// Don't add this item to the collection if the prefix is ambiguous.
		lname = EatPrefix(filelocLeft.filepath.c_str(), rpath.c_str());
		if (lname != NULL)
			return false;
		// Don't add this item to the collection if no temp folder is involved.
		if (m_pTempPathContext == NULL)
			return false;
		// Copy the left file to the temp folder, assigning it the same name as
		// given on the right side. Also mimic the respective folder structure.
		lname = rname;
		String filepath = paths_ConcatPath(m_pTempPathContext->m_strLeftRoot, lname);
		paths_CreateIfNeeded(filepath.c_str(), true);
		CopyFile(filelocLeft.filepath.c_str(), filepath.c_str(), TRUE);
		filelocLeft.filepath = filepath;
	}
	int i = -1;
	if (DIFFITEM *di = FindItemFromPaths(filelocLeft.filepath.c_str(), filelocRight.filepath.c_str()))
	{
		i = m_pDirView->GetItemIndex(di);
	}
	else
	{
		di = m_pCtxt->AddDiff(NULL);
		di->diffcode = DIFFCODE::NEEDSCAN;
		di->left.path = paths_GetParentPath(lname);
		di->left.filename = PathFindFileName(lname);
		di->right.path = paths_GetParentPath(rname);
		di->right.filename = PathFindFileName(rname);
		i = m_pDirView->AddNewItem(m_pDirView->GetItemCount(), di, I_IMAGECALLBACK, 0);
		Rescan(1);
	}
	if (i != -1)
	{
		m_pDirView->SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
		m_pDirView->EnsureVisible(i, FALSE);
	}
	return true;
}

BOOL CDirFrame::PreTranslateMessage(MSG *pMsg)
{
	// Check if we got 'ESC pressed' -message
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_ESCAPE:
			if (m_pDirView->GetEditControl())
			{
				DispatchMessage(pMsg);
				return TRUE;
			}
			break;
		}
	}
	return FALSE;
}
