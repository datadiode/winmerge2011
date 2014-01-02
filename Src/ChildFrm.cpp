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
 * @file  ChildFrm.cpp
 *
 * @brief Implementation file for CChildFrame
 *
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "stdafx.h"
#include "Merge.h"
#include "ChildFrm.h"
#include "DirFrame.h"
#include "MainFrm.h"
#include "LanguageSelect.h"
#include "MergeEditView.h"
#include "MergeDiffDetailView.h"
#include "LocationView.h"
#include "WMGotoDlg.h"
#include "Common/SettingStore.h"
#include "Common/stream_util.h"
#include "Common/coretools.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/** @brief Location for file compare specific help to open. */
static const TCHAR MergeViewHelpLocation[] = _T("::/htmlhelp/Compare_files.html");

static void SaveBuffForDiff(CDiffTextBuffer & buf, CDiffTextBuffer & buf2, DiffFileData &diffdata, int i);

/////////////////////////////////////////////////////////////////////////////
// CChildFrame

HRESULT CChildFrame::FindBehavior(BSTR bstrBehavior, BSTR bstrBehaviorUrl, IElementBehaviorSite *pSite, IElementBehavior **ppBehavior)
{
	if (EatPrefix(bstrBehaviorUrl, L"#WinMerge#"))
	{
		if (StrCmpIW(bstrBehavior, L"MergeDoc") == 0)
		{
			*ppBehavior = this;
			return S_OK;
		}
	}
	return m_pMDIFrame->FindBehavior(bstrBehavior, bstrBehaviorUrl, pSite, ppBehavior);
}

void CChildFrame::OnEditUndo()
{
	CMergeEditView *tgt = *(curUndo - 1);
	if (tgt->QueryEditable())
	{
		WaitStatusCursor waitstatus(IDS_STATUS_UNDO);
		tgt->SetFocus();
		if (tgt->DoEditUndo())
		{
			--curUndo;
			UpdateHeaderPath(tgt->m_nThisPane);
			FlushAndRescan();
			int nAction;
			m_ptBuf[tgt->m_nThisPane]->GetRedoActionCode(nAction);
			if (nAction == CE_ACTION_MERGE)
				// select the diff so we may just merge it again
				tgt->OnCurdiff();
		}
	}
}

void CChildFrame::OnEditRedo()
{
	CMergeEditView *tgt = *(curUndo);
	if (tgt->QueryEditable())
	{
		WaitStatusCursor waitstatus(IDS_STATUS_REDO);
		tgt->SetFocus();
		if (tgt->DoEditRedo())
		{
			++curUndo;
			UpdateHeaderPath(tgt->m_nThisPane);
			FlushAndRescan();
		}
	}
}

/**
 * @brief Show "Go To" dialog and scroll views to line or diff.
 *
 * Before dialog is opened, current line and file is determined
 * and selected.
 * @note Conversions needed between apparent and real lines
 */
void CChildFrame::OnWMGoto()
{
	CMergeEditView *pMergeView = GetActiveMergeView();
	WMGotoDlg dlg(pMergeView);
	// Set active file and current line selected in dialog
	dlg.m_nFile = pMergeView->m_nThisPane;
	POINT pos = pMergeView->GetCursorPos();
	int nParam = m_ptBuf[dlg.m_nFile]->ComputeRealLine(pos.y);
	dlg.m_strParam = NumToStr(nParam + 1, 10);
	if (LanguageSelect.DoModal(dlg) == IDOK)
	{
		// Reassign pMergeView because user may have altered the target pane
		pMergeView = GetView(dlg.m_nFile);
		pMergeView->SetFocus();
		nParam = _ttol(dlg.m_strParam.c_str()) - 1;
		if (nParam < 0)
			nParam = 0;
		if (dlg.m_nGotoWhat == 0)
		{
			CDiffTextBuffer *const pBuf = m_ptBuf[pMergeView->m_nThisPane];
			int nLastLine = pBuf->GetLineCount() - 1;
			nLastLine = pBuf->ComputeRealLine(nLastLine);
			if (nParam > nLastLine)
				nParam = nLastLine;
			nParam = pBuf->ComputeApparentLine(nParam);
			pMergeView->GotoLine(nParam);
		}
		else
		{
			int nLastDiff = m_diffList.GetSize() - 1;
			if (nParam > nLastDiff)
				nParam = nLastDiff;
			pMergeView->SelectDiff(nParam);
		}
	}
}

/**
 * @brief Open active file with associated application.
 *
 * First tries to open file using shell 'Edit' action, since that
 * action open scripts etc. to editor instead of running them. If
 * edit-action is not registered, 'Open' action is used.
 */
void CChildFrame::OnOpenFile()
{
	CMergeEditView *const pMergeView = GetActiveMergeView();
	LPCTSTR file = m_strPath[pMergeView->m_nThisPane].c_str();
	if (file[0] == _T('\0'))
		return;
	int rtn = (int)ShellExecute(NULL, _T("edit"), file, 0, 0, SW_SHOWNORMAL);
	if (rtn == SE_ERR_NOASSOC)
		rtn = (int)ShellExecute(NULL, _T("open"), file, 0, 0, SW_SHOWNORMAL);
	if (rtn == SE_ERR_NOASSOC)
		OnOpenFileWith();
}

/**
 * @brief Open active file with app selection dialog
 */
void CChildFrame::OnOpenFileWith()
{
	CMergeEditView *const pMergeView = GetActiveMergeView();
	LPCTSTR file = m_strPath[pMergeView->m_nThisPane].c_str();
	if (file[0] != _T('\0'))
		m_pMDIFrame->OpenFileWith(file);
}

/**
 * @brief Open active file with external editor
 */
void CChildFrame::OnOpenFileWithEditor()
{
	CMergeEditView *const pMergeView = GetActiveMergeView();
	LPCTSTR file = m_strPath[pMergeView->m_nThisPane].c_str();
	if (file[0] != _T('\0'))
		m_pMDIFrame->OpenFileToExternalEditor(file);
}

void CChildFrame::DoPrint()
{
	CMyVariant arguments = this;
	m_pMDIFrame->ShowHTMLDialog(
		L"print_template:\\PrintTemplates\\MergeDocPrintTemplate.html",
		&arguments,
		L"dialogWidth: 100in; dialogHeight: 100in; status: yes; unadorned: yes;",
		NULL);
}

void CChildFrame::ReloadDocs()
{
	UpdateCmdUI();
	if (!FileLoadResult::IsError(ReloadDoc(0)) &&
		!FileLoadResult::IsError(ReloadDoc(1)))
	{
		m_pView[0]->ReAttachToBuffer(m_ptBuf[0]);
		m_pView[1]->ReAttachToBuffer(m_ptBuf[1]);
		m_pDetailView[0]->ReAttachToBuffer(m_ptBuf[0]);
		m_pDetailView[1]->ReAttachToBuffer(m_ptBuf[1]);
		bool bIdentical = false;
		Rescan2(bIdentical);
	}
	UpdateAllViews();
}

void CChildFrame::AlignScrollPositions()
{
	CMergeEditView *const pActiveView = GetActiveMergeView();
	pActiveView->EnsureCursorVisible();
}

BOOL CChildFrame::PreTranslateMessage(MSG *pMsg)
{
	if (CCrystalTextView *pTextView = GetActiveTextView())
	{
		HACCEL hAccel = m_pHandleSet->m_hAccelShared;
		if (hAccel && ::TranslateAccelerator(m_hWnd, hAccel, pMsg))
		{
			return TRUE;
		}
	}
	return FALSE;
}

template<>
LRESULT CChildFrame::OnWndMsg<WM_COMMAND>(WPARAM wParam, LPARAM lParam)
{
	CGhostTextView *const pTextView = GetActiveTextView();
	CMergeEditView *const pActiveView = GetActiveMergeView();
	switch (const UINT id = lParam ? static_cast<UINT>(wParam) : LOWORD(wParam))
	{
	case ID_FILE_SAVE:
		OnFileSave();
		UpdateCmdUI();
		break;
	case ID_FILE_SAVE_LEFT:
		OnFileSaveLeft();
		UpdateCmdUI();
		break;
	case ID_FILE_SAVE_RIGHT:
		OnFileSaveRight();
		UpdateCmdUI();
		break;
	case ID_FILE_SAVEAS_LEFT:
		OnFileSaveAsLeft();
		UpdateCmdUI();
		break;
	case ID_FILE_SAVEAS_RIGHT:
		OnFileSaveAsRight();
		UpdateCmdUI();
		break;
	case ID_FILE_ENCODING:
		OnFileEncoding();
		break;
	case ID_FILE_PRINT_PREVIEW:
		DoPrint();
		break;
	case ID_EOL_TO_DOS:
	case ID_EOL_TO_UNIX:
	case ID_EOL_TO_MAC:
		pActiveView->OnConvertEolTo(id);
		break;
	case ID_TOOLS_COMPARE_SELECTION:
		OnToolsCompareSelection();
		break;
	case ID_TOOLS_GENERATEREPORT:
		OnToolsGenerateReport();
		break;
	case ID_FILE_LEFT_READONLY:
		OnReadOnly(0);
		break;
	case ID_FILE_RIGHT_READONLY:
		OnReadOnly(1);
		break;
	case ID_FILE_MERGINGMODE:
		SetMergingMode(!GetMergingMode());
		UpdateMergeStatusUI();
		break;
	case ID_L2R:
		OnL2r();
		break;
	case ID_R2L:
		OnR2l();
		break;
	case ID_L2RNEXT:
		if (OnL2r())
			pActiveView->OnNextdiff();
		break;
	case ID_R2LNEXT:
		if (OnR2l())
			pActiveView->OnNextdiff();
		break;
	case ID_ALL_LEFT:
		OnAllLeft();
		break;
	case ID_ALL_RIGHT:
		OnAllRight();
		break;
	case ID_REFRESH:
		if (m_idContextLines != ID_VIEW_CONTEXT_UNLIMITED)
		{
			PostMessage(WM_COMMAND, ID_VIEW_CONTEXT_UNLIMITED);
		}
		else
		{
			// Refresh display using text-buffers
			// NB: This DOES NOT reload files!
			FlushAndRescan(true);
		}
		break;
	case ID_VIEW_LINENUMBERS:
		COptionsMgr::SaveOption(OPT_VIEW_LINENUMBERS, !COptionsMgr::Get(OPT_VIEW_LINENUMBERS));
		RefreshOptions();
		AlignScrollPositions();
		break;
	case ID_VIEW_FILEMARGIN:
		COptionsMgr::SaveOption(OPT_VIEW_FILEMARGIN, !COptionsMgr::Get(OPT_VIEW_FILEMARGIN));
		RefreshOptions();
		AlignScrollPositions();
		break;
	case ID_VIEW_LINEDIFFS:
		COptionsMgr::SaveOption(OPT_WORDDIFF_HIGHLIGHT, !COptionsMgr::Get(OPT_WORDDIFF_HIGHLIGHT));
		RefreshOptions();
		FlushAndRescan(true);
		break;
	case ID_VIEW_WORDWRAP:
		COptionsMgr::SaveOption(OPT_WORDWRAP, !COptionsMgr::Get(OPT_WORDWRAP));
		RefreshOptions();
		AlignScrollPositions();
		break;

		for (;;) // Establish a local context for breaking
		{
		case ID_VIEW_CONTEXT_0:
		case ID_VIEW_CONTEXT_1:
		case ID_VIEW_CONTEXT_2:
		case ID_VIEW_CONTEXT_3:
		case ID_VIEW_CONTEXT_4:
		case ID_VIEW_CONTEXT_5:
			// Editing does not work with limited context, so disable it.
			m_ptBuf[0]->SetReadOnly();
			m_ptBuf[1]->SetReadOnly();
			break;
		case ID_VIEW_CONTEXT_UNLIMITED:
			m_ptBuf[0]->SetReadOnly(m_bInitialReadOnly[0]);
			m_ptBuf[1]->SetReadOnly(m_bInitialReadOnly[1]);
			break;
		}
		m_pView[0]->UpdateLineInfoStatus();
		m_pView[1]->UpdateLineInfoStatus();
		// Clear the detail views or else they will go out of sync.
		m_pDetailView[0]->OnDisplayDiff(-1);
		m_pDetailView[1]->OnDisplayDiff(-1);
		m_idContextLines = id;
		ReloadDocs();
		break;
	case ID_SET_SYNCPOINT:
		if (IsCursorAtSyncPoint())
			ClearSyncPoint();
		else
			SetSyncPoint();
		UpdateSyncPointUI();
		break;
	case ID_CLEAR_SYNCPOINTS:
		ClearSyncPoints();
		UpdateSyncPointUI();
		break;
	case ID_EDIT_SELECT_ALL:
		pTextView->SelectAll();
		break;
	case ID_EDIT_COPY:
		pTextView->OnEditCopy();
		break;
	case ID_EDIT_CUT:
		pTextView->OnEditCut();
		break;
	case ID_EDIT_PASTE:
		pTextView->OnEditPaste();
		break;
	case ID_EDIT_TAB:
		pTextView->OnEditTab();
		break;
	case ID_EDIT_UNTAB:
		pTextView->OnEditUntab();
		break;
	case ID_EDIT_COPY_LINENUMBERS:
		pActiveView->OnEditCopyLineNumbers();
		break;
	case ID_EDIT_DELETE:
		pTextView->OnEditDelete();
		break;
	case ID_EDIT_DELETE_BACK:
		pTextView->OnEditDeleteBack();
		break;
	case ID_EDIT_REPLACE:
		pTextView->OnEditReplace();
		break;
	case ID_EDIT_LOWERCASE:
		pTextView->OnEditLowerCase();
		break;
	case ID_EDIT_UPPERCASE:
		pTextView->OnEditUpperCase();
		break;
	case ID_EDIT_SWAPCASE:
		pTextView->OnEditSwapCase();
		break;
	case ID_EDIT_CAPITALIZE:
		pTextView->OnEditCapitalize();
		break;
	case ID_EDIT_SENTENCE:
		pTextView->OnEditSentence();
		break;
	case ID_EDIT_GOTO_LAST_CHANGE:
		pTextView->OnEditGotoLastChange();
		break;
	case ID_EDIT_DELETE_WORD:
		pTextView->OnEditDeleteWord();
		break;
	case ID_EDIT_DELETE_WORD_BACK:
		pTextView->OnEditDeleteWordBack();
		break;
	case ID_EDIT_UNDO:
		OnEditUndo();
		break;
	case ID_EDIT_REDO:
		OnEditRedo();
		break;
	case ID_EDIT_WMGOTO:
		OnWMGoto();
		break;
	case ID_FILE_OPEN_REGISTERED:
		OnOpenFile();
		break;
	case ID_FILE_OPEN_WITHEDITOR:
		OnOpenFileWithEditor();
		break;
	case ID_FILE_OPEN_WITH:
		OnOpenFileWith();
		break;
	case ID_EDIT_RIGHT_TO_LEFT:
		pTextView->OnEditRightToLeft();
		break;
	case ID_EDIT_TOGGLE_BOOKMARK:
		pActiveView->OnToggleBookmark();
		UpdateBookmarkUI();
		break;
	case ID_EDIT_GOTO_NEXT_BOOKMARK:
		pActiveView->OnNextBookmark();
		break;
	case ID_EDIT_GOTO_PREV_BOOKMARK:
		pActiveView->OnPrevBookmark();
		break;
	case ID_EDIT_CLEAR_ALL_BOOKMARKS:
		pActiveView->OnClearAllBookmarks();
		UpdateBookmarkUI();
		break;
	case ID_EDIT_TOGGLE_BOOKMARK0:
	case ID_EDIT_TOGGLE_BOOKMARK1:
	case ID_EDIT_TOGGLE_BOOKMARK2:
	case ID_EDIT_TOGGLE_BOOKMARK3:
	case ID_EDIT_TOGGLE_BOOKMARK4:
	case ID_EDIT_TOGGLE_BOOKMARK5:
	case ID_EDIT_TOGGLE_BOOKMARK6:
	case ID_EDIT_TOGGLE_BOOKMARK7:
	case ID_EDIT_TOGGLE_BOOKMARK8:
	case ID_EDIT_TOGGLE_BOOKMARK9:
		pActiveView->OnToggleBookmark(id);
		break;
	case ID_EDIT_GO_BOOKMARK0:
	case ID_EDIT_GO_BOOKMARK1:
	case ID_EDIT_GO_BOOKMARK2:
	case ID_EDIT_GO_BOOKMARK3:
	case ID_EDIT_GO_BOOKMARK4:
	case ID_EDIT_GO_BOOKMARK5:
	case ID_EDIT_GO_BOOKMARK6:
	case ID_EDIT_GO_BOOKMARK7:
	case ID_EDIT_GO_BOOKMARK8:
	case ID_EDIT_GO_BOOKMARK9:
		pActiveView->OnGoBookmark(id);
		break;
	case ID_EDIT_CLEAR_BOOKMARKS:
		pActiveView->OnClearBookmarks();
		break;
	case ID_EDIT_CHAR_LEFT:
		if (GetMergingMode())
			OnR2l();
		else
			pTextView->MoveLeft(FALSE);
		break;
	case ID_EDIT_EXT_CHAR_LEFT:
		pTextView->MoveLeft(TRUE);
		break;
	case ID_EDIT_CHAR_RIGHT:
		if (GetMergingMode())
			OnL2r();
		else
			pTextView->MoveRight(FALSE);
		break;
	case ID_EDIT_EXT_CHAR_RIGHT:
		pTextView->MoveRight(TRUE);
		break;
	case ID_EDIT_WORD_LEFT:
		pTextView->MoveWordLeft(FALSE);
		break;
	case ID_EDIT_EXT_WORD_LEFT:
		pTextView->MoveWordLeft(TRUE);
		break;
	case ID_EDIT_WORD_RIGHT:
		pTextView->MoveWordRight(FALSE);
		break;
	case ID_EDIT_EXT_WORD_RIGHT:
		pTextView->MoveWordRight(TRUE);
		break;
	case ID_EDIT_LINE_UP:
		if (GetMergingMode())
			pActiveView->OnPrevdiff();
		else
			pTextView->MoveUp(FALSE);
		break;
	case ID_EDIT_EXT_LINE_UP:
		pTextView->MoveUp(TRUE);
		break;
	case ID_EDIT_LINE_DOWN:
		if (GetMergingMode())
			pActiveView->OnNextdiff();
		else
			pTextView->MoveDown(FALSE);
		break;
	case ID_EDIT_EXT_LINE_DOWN:
		pTextView->MoveDown(TRUE);
		break;
	case ID_EDIT_PAGE_UP:
		pTextView->MovePgUp(FALSE);
		break;
	case ID_EDIT_EXT_PAGE_UP:
		pTextView->MovePgUp(TRUE);
		break;
	case ID_EDIT_PAGE_DOWN:
		pTextView->MovePgDn(FALSE);
		break;
	case ID_EDIT_EXT_PAGE_DOWN:
		pTextView->MovePgDn(TRUE);
		break;
	case ID_EDIT_HOME:
		pTextView->MoveHome(FALSE);
		break;
	case ID_EDIT_EXT_HOME:
		pTextView->MoveHome(TRUE);
		break;
	case ID_EDIT_LINE_END:
		pTextView->MoveEnd(FALSE);
		break;
	case ID_EDIT_EXT_LINE_END:
		pTextView->MoveEnd(TRUE);
		break;
	case ID_EDIT_TEXT_BEGIN:
		pTextView->MoveCtrlHome(FALSE);
		break;
	case ID_EDIT_EXT_TEXT_BEGIN:
		pTextView->MoveCtrlHome(TRUE);
		break;
	case ID_EDIT_TEXT_END:
		pTextView->MoveCtrlEnd(FALSE);
		break;
	case ID_EDIT_EXT_TEXT_END:
		pTextView->MoveCtrlEnd(TRUE);
		break;
	case ID_EDIT_MATCHBRACE:
		pTextView->OnMatchBrace(FALSE);
		break;
	case ID_EDIT_EXT_MATCHBRACE:
		pTextView->OnMatchBrace(TRUE);
		break;
	case ID_EDIT_SCROLL_UP:
		pTextView->ScrollUp();
		break;
	case ID_EDIT_SCROLL_DOWN:
		pTextView->ScrollDown();
		break;
	case ID_EDIT_FIND:
		pTextView->OnEditFind();
		break;
	case ID_EDIT_REPEAT:
		pTextView->OnEditRepeat();
		break;
	case ID_EDIT_SWITCH_OVRMODE:
		pActiveView->OnEditSwitchOvrmode();
		break;
	case ID_CURDIFF:
		pActiveView->OnCurdiff();
		break;
	case ID_FIRSTDIFF:
		pActiveView->OnFirstdiff();
		break;
	case ID_LASTDIFF:
		pActiveView->OnLastdiff();
		break;
	case ID_NEXTDIFF:
		pActiveView->OnNextdiff();
		break;
	case ID_PREVDIFF:
		pActiveView->OnPrevdiff();
		break;
	case ID_SELECTLINEDIFF:
		Showlinediff(pTextView, pActiveView,
			COptionsMgr::Get(OPT_BREAK_ON_WORDS) ?
			WORDDIFF : BYTEDIFF);
		break;
	case ID_VIEW_SWAPPANES:
		// Swap the two panes
		SwapFiles();
		break;
	case ID_VIEW_ZOOMIN:
	case ID_VIEW_ZOOMNORMAL:
	case ID_VIEW_ZOOMOUT:
		C_ASSERT(ID_VIEW_ZOOMNORMAL - ID_VIEW_ZOOMIN == 1);
		C_ASSERT(ID_VIEW_ZOOMNORMAL - ID_VIEW_ZOOMOUT == -1);
		pActiveView->ZoomText(ID_VIEW_ZOOMNORMAL - id);
		AlignScrollPositions();
		break;
	case ID_NEXT_PANE:
	case ID_WINDOW_CHANGE_PANE:
		if (HWindow *pWndNext = GetNextDlgTabItem(HWindow::GetFocus()))
		{
			pWndNext->SetFocus();
			CMergeEditView *const pNewActiveView = GetActiveMergeView();
			if (pNewActiveView != pActiveView)
			{
				POINT ptCursor = pActiveView->GetCursorPos();
				ptCursor.x = 0;
				if (ptCursor.y > pNewActiveView->GetLineCount() - 1)
					ptCursor.y = pNewActiveView->GetLineCount() - 1;
				pNewActiveView->SetCursorPos(ptCursor);
				pNewActiveView->SetAnchor(ptCursor);
				pNewActiveView->SetSelection(ptCursor, ptCursor);
			}
		}
		break;
	case IDCANCEL:
		PostMessage(WM_CLOSE);
		break;
	default:
		if (id >= ID_COLORSCHEME_FIRST && id <= ID_COLORSCHEME_LAST)
		{
			CCrystalTextView::TextType enuType = static_cast
				<CCrystalTextView::TextType>(id - ID_COLORSCHEME_FIRST);
			m_pView[0]->SetTextType(enuType);
			m_pView[1]->SetTextType(enuType);
			m_pDetailView[0]->SetTextType(enuType);
			m_pDetailView[1]->SetTextType(enuType);
			UpdateAllViews();
			UpdateSourceTypeUI();
			break;
		}
		return FALSE;
	}
	return TRUE;
}

LRESULT CChildFrame::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		if (LRESULT lResult = OnWndMsg<WM_COMMAND>(wParam, lParam))
			return lResult;
		break;
	case WM_HELP:
		m_pMDIFrame->ShowHelp(MergeViewHelpLocation);
		return 0;
	case WM_NCACTIVATE:
		if (wParam)
		{
			m_pMDIFrame->InitCmdUI();
			UpdateCmdUI();
			UpdateMergeStatusUI();
			OnUpdateStatusNum();
			m_pMDIFrame->UpdateCmdUI<ID_REFRESH>(MF_ENABLED);
			m_pMDIFrame->UpdateCmdUI<ID_EDIT_TOGGLE_BOOKMARK>(MF_ENABLED);
			m_pMDIFrame->UpdateCmdUI<ID_TOOLS_GENERATEREPORT>(MF_ENABLED);
		}
		break;
	case WM_CLOSE:
		if (!SaveModified())
			return 0;
		if (m_pOpener)
			EnableModeless(TRUE);
		break;
	}
	return CDocFrame::WindowProc(uMsg, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////
// CChildFrame construction/destruction

const LONG CChildFrame::FloatScript[] =
{
	IDC_STATIC_TITLE_LEFT, BY<500>::X2R,
	IDC_STATIC_TITLE_RIGHT, BY<500>::X2L | BY<1000>::X2R,
	0x1000, BY<500>::X2R | BY<1000>::Y2B,
	0x1001, BY<500>::X2L | BY<1000>::X2R | BY<1000>::Y2B,
	0x6000, BY<1000>::Y2T | BY<1000>::Y2B | BY<500>::X2R,
	0x6001, BY<500>::X2L | BY<1000>::X2R | BY<1000>::Y2T | BY<1000>::Y2B,
	0
};

const LONG CChildFrame::SplitScript[] =
{
	0x6001, IDC_STATIC_TITLE_LEFT, ~20,
	0
};

const LONG CChildFrame::FloatScriptLocationBar[] =
{
	152, BY<1000>::X2R | BY<1000>::Y2B,
	0
};

const LONG CChildFrame::FloatScriptDiffViewBar[] =
{
	0x1000, BY<1000>::X2R | BY<500>::Y2B,
	0x1001, BY<1000>::X2R | BY<500>::Y2T | BY<1000>::Y2B,
	0
};

/**
 * @brief Constructor.
 */
CChildFrame::CChildFrame(CMainFrame *pMDIFrame, CDirFrame *pDirDoc, CChildFrame *pOpener)
: CEditorFrame(pMDIFrame, pDirDoc, GetHandleSet<IDR_MERGEDOCTYPE>(), FloatScript, SplitScript)
, m_pOpener(pOpener)
#pragma warning(disable:warning_this_used_in_base_member_initializer_list)
, m_wndLocationView(this)
, m_wndLocationBar(this, FloatScriptLocationBar, HTRIGHT)
, m_wndDiffViewBar(this, FloatScriptDiffViewBar, HTTOP)
#pragma warning(default:warning_this_used_in_base_member_initializer_list)
, m_bEnableRescan(true)
, m_nCurDiff(-1)
, m_idContextLines(ID_VIEW_CONTEXT_UNLIMITED)
, m_pInfoUnpacker(new PackingInfo)
, m_diffWrapper(&m_diffList)
, m_pSaveFileInfo(2)
, m_pRescanFileInfo(2)
, m_pFileTextStats(2)
{
	curUndo = undoTgt.begin();
	ASSERT(m_bMixedEol == false);
	ASSERT(m_pView[0] == NULL);
	ASSERT(m_pView[1] == NULL);
	ASSERT(m_pDetailView[0] == NULL);
	ASSERT(m_pDetailView[1] == NULL);
	m_bMergingMode = COptionsMgr::Get(OPT_MERGE_MODE);
	ASSERT(m_bEditAfterRescan[0] == false);
	ASSERT(m_bEditAfterRescan[1] == false);
	ASSERT(m_bInitialReadOnly[0] == false);
	ASSERT(m_bInitialReadOnly[1] == false);
	m_ptBuf[0] = new CDiffTextBuffer(this, 0);
	m_ptBuf[1] = new CDiffTextBuffer(this, 1);
	m_diffWrapper.RefreshOptions();
	SubclassWindow(pMDIFrame->CreateChildHandle());
	CreateClient();
	RecalcLayout();
	if (m_pOpener)
		EnableModeless(FALSE);
	m_bAutoDelete = true;
}

/**
 * Destructor.
 */
CChildFrame::~CChildFrame()
{
	if (m_pDirDoc)
		m_pDirDoc->MergeDocClosing(this);
	delete m_ptBuf[0];
	delete m_ptBuf[1];
}

CMergeEditView *CChildFrame::CreatePane(int iPane)
{
	RECT rect;
	HEdit *pEdit = m_wndFilePathBar.GetControlRect(iPane, &rect);
	const int xFilePathEdit = rect.left;
	const int cyFilePathEdit = rect.bottom;

	LONG additionalStyles = 0;
	if (iPane == 1)
	{
		additionalStyles = WS_VSCROLL | SBS_SIZEGRIP;
		rect.right += GetSystemMetrics(SM_CXVSCROLL);
	}

	HWindow *pWndView = HWindow::CreateEx(
		WS_EX_CLIENTEDGE, WinMergeWindowClass, NULL,
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_HSCROLL | additionalStyles & WS_VSCROLL,
		rect.left, rect.bottom, rect.right - rect.left, 0,
		m_wndFilePathBar.m_pWnd, 0x1000 + iPane);

	CMergeEditView *pView = new CMergeEditView(pWndView, this, iPane);

	HWindow *pParent = wine_version ? m_wndFilePathBar.m_pWnd : pView->m_pWnd;
	const int cxStatusBar = rect.right - rect.left;
	HStatusBar *pBar = HStatusBar::Create(
		WS_CHILD | WS_VISIBLE | CCS_NOPARENTALIGN | additionalStyles & SBS_SIZEGRIP,
		rect.left, rect.bottom, cxStatusBar, rect.bottom,
		pParent, 0x6000 + iPane);
	pBar->GetWindowRect(&rect);
	pParent->ScreenToClient(&rect);
	pBar->SetParent(m_wndFilePathBar.m_pWnd);
	const int cyStatusBar = rect.bottom - rect.top;
	pBar->SetStyle(pBar->GetStyle() | CCS_NORESIZE);

	pBar->SetWindowPos(reinterpret_cast<HWindow *>(HWND_BOTTOM),
		xFilePathEdit, rect.top, cxStatusBar, cyStatusBar,
		SWP_NOACTIVATE);

	pView->SetWindowPos(NULL,
		xFilePathEdit, cyStatusBar,
		cxStatusBar, cyFilePathEdit - cyStatusBar,
		SWP_NOZORDER | SWP_NOACTIVATE);

	pEdit->SetWindowPos(NULL, 0, 0,
		cxStatusBar, cyStatusBar,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

	pView->m_pStatusBar = pBar;
	
	return pView;
}

/**
* @brief Create the child frame, the splitter, the filename bar, the status bar,
* the diff dockable bar, and the four views
*
* @note  the panels layout is 
* <ul>
*  <li>   -----------
*  <li>		!  header !
*  <li>		!.........!
*  <li>		!.   .   .!
*  <li>		!. 1 . 2 .!
*  <li>		!.   .   .!
*  <li>		!.........!
*  <li>		!.........!
*  <li>		!  status !
*  <li>		-----------
*  <li>		!.........!
*  <li>		!.   3   .!
*  <li>		!.dockable!
*  <li>		! splitbar!
*  <li>		!.   4   .!
*  <li>		!.........!
*  <li>		-----------
* </ul>
*/
void CChildFrame::CreateClient()
{
	m_wndLocationBar.SubclassWindow(HWindow::CreateEx(
		0, WinMergeWindowClass, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		0, 0, 24, 10, m_pWnd, ID_VIEW_LOCATION_BAR));
	m_wndLocationView.SubclassWindow(HWindow::CreateEx(
		WS_EX_CLIENTEDGE, WinMergeWindowClass, NULL, WS_CHILD | WS_VISIBLE,
		0, 0, 20, 10, m_wndLocationBar.m_pWnd, 152, NULL));

	m_wndDiffViewBar.SubclassWindow(HWindow::CreateEx(
		0, WinMergeWindowClass, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		0, 0, 100, 48, m_pWnd, ID_VIEW_DETAIL_BAR));

	const int cyScroll = GetSystemMetrics(SM_CYHSCROLL);
	const int cyClient = (40 - cyScroll) / 2;

	CMergeDiffDetailView *pLeftDetail = new CMergeDiffDetailView(this, 0);
	pLeftDetail->SubclassWindow(HWindow::CreateEx(
		WS_EX_CLIENTEDGE, WinMergeWindowClass, NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL,
		0, 4, 100, cyClient, m_wndDiffViewBar.m_pWnd, 0x1000));
	CMergeDiffDetailView *pRightDetail = new CMergeDiffDetailView(this, 1);
	pRightDetail->SubclassWindow(HWindow::CreateEx(
		WS_EX_CLIENTEDGE, WinMergeWindowClass, NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL,
		0, cyClient + 8, 100, cyClient + cyScroll, m_wndDiffViewBar.m_pWnd, 0x1001));

	// Merge frame has a header bar at top
	m_wndFilePathBar.Create(m_hWnd);

	CMergeEditView *pLeft = CreatePane(0);
	CMergeEditView *pRight = CreatePane(1);
	// tell merge doc about these views
	SetMergeViews(pLeft, pRight);

	// tell merge doc about these views
	SetMergeDetailViews(pLeftDetail, pRightDetail);

	if (!COptionsMgr::Get(OPT_SHOW_LOCATIONBAR))
		m_wndLocationBar.ShowWindow(SW_HIDE);

	if (!COptionsMgr::Get(OPT_SHOW_DIFFVIEWBAR))
		m_wndDiffViewBar.ShowWindow(SW_HIDE);
}

/////////////////////////////////////////////////////////////////////////////
// CChildFrame message handlers

void CChildFrame::ActivateFrame() 
{
	TCHAR entry[8];
	GetAtomName(static_cast<ATOM>(m_pHandleSet->m_id), entry, _countof(entry));
	DWORD dim = SettingStore.GetProfileInt(_T("ScreenLayout"), entry, 0);
	int cx = SHORT LOWORD(dim);
	int cy = SHORT HIWORD(dim);
	m_wndLocationBar.SetWindowPos(NULL,
		0, 0, cx, cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	m_wndDiffViewBar.SetWindowPos(NULL,
		0, 0, cx, cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	RecalcLayout();
	CMDIChildWnd::ActivateFrame();
}

/**
 * @brief Save coordinates of the frame, splitters, and bars
 *
 * @note Do not save the maximized/restored state here. We are interested
 * in the state of the active frame, and maybe this frame is not active
 */
void CChildFrame::SavePosition()
{
	RECT rectLocationBar;
	m_wndLocationBar.GetWindowRect(&rectLocationBar);
	RECT rectDiffViewBar;
	m_wndDiffViewBar.GetWindowRect(&rectDiffViewBar);
	DWORD dim = MAKELONG(
		rectLocationBar.right - rectLocationBar.left,
		rectDiffViewBar.bottom - rectDiffViewBar.top);
	TCHAR entry[8];
	GetAtomName(static_cast<ATOM>(m_pHandleSet->m_id), entry, _countof(entry));
	SettingStore.WriteProfileInt(_T("ScreenLayout"), entry, dim);
}

/**
* @brief Reflect comparison result in window's icon.
* @param nResult [in] Last comparison result which the application returns.
*/
void CChildFrame::SetLastCompareResult(int nResult)
{
	HICON hCurrent = GetIcon(FALSE);
	HICON hReplace = LanguageSelect.LoadIcon(
		nResult == 0 ? IDI_EQUALTEXTFILE : IDI_NOTEQUALTEXTFILE);

	if (hCurrent != hReplace)
	{
		SetIcon(hReplace, TRUE);
		BOOL bMaximized;
		m_pMDIFrame->GetActiveDocFrame(&bMaximized);
		// When MDI maximized the window icon is drawn on the menu bar, so we
		// need to notify it that our icon has changed.
		if (bMaximized)
		{
			m_pMDIFrame->DrawMenuBar();
		}
	}
}

void CChildFrame::UpdateEditCmdUI()
{
	const bool bROLeft = m_ptBuf[0]->GetReadOnly();
	const bool bRORight = m_ptBuf[1]->GetReadOnly();
	const BYTE enableSaveLeft =
		!m_pInfoUnpacker->saveAsPath.empty() && bRORight > bROLeft ||
		m_ptBuf[0]->IsSaveable() ? MF_ENABLED : MF_GRAYED;
	const BYTE enableSaveRight =
		!m_pInfoUnpacker->saveAsPath.empty() && bROLeft > bRORight ||
		m_ptBuf[1]->IsSaveable() ? MF_ENABLED : MF_GRAYED;
	m_pMDIFrame->UpdateCmdUI<ID_FILE_SAVE_LEFT>(enableSaveLeft);
	m_pMDIFrame->UpdateCmdUI<ID_FILE_SAVE_RIGHT>(enableSaveRight);
	m_pMDIFrame->UpdateCmdUI<ID_FILE_SAVE>(enableSaveLeft & enableSaveRight);
	m_pMDIFrame->UpdateCmdUI<ID_EDIT_UNDO>(
		curUndo != undoTgt.begin() && (*(curUndo - 1))->QueryEditable() ?
		MF_ENABLED : MF_GRAYED);
	m_pMDIFrame->UpdateCmdUI<ID_EDIT_REDO>(
		curUndo != undoTgt.end() && (*curUndo)->QueryEditable() ?
		MF_ENABLED : MF_GRAYED);
}

void CChildFrame::UpdateGeneralCmdUI()
{
	int firstDiff, lastDiff;
	int currDiff = GetContextDiff(firstDiff, lastDiff);
	BYTE enable = MF_GRAYED;
	if (firstDiff != -1 && lastDiff != -1 && (lastDiff >= firstDiff) ||
		currDiff != -1 && m_diffList.IsDiffSignificant(currDiff))
	{
		enable = MF_ENABLED;
	}

	// Merging commands which are independent of caret position
	m_pMDIFrame->UpdateCmdUI<ID_ALL_LEFT>(
		m_diffList.FirstSignificantDiff() != -1 &&
		!m_ptBuf[0]->GetReadOnly() ? MF_ENABLED : MF_GRAYED);
	m_pMDIFrame->UpdateCmdUI<ID_ALL_RIGHT>(
		m_diffList.FirstSignificantDiff() != -1 &&
		!m_ptBuf[1]->GetReadOnly() ? MF_ENABLED : MF_GRAYED);

	// Merging commands which are dependent of caret position
	m_pMDIFrame->UpdateCmdUI<ID_L2R>(
		!m_ptBuf[1]->GetReadOnly() ? enable : MF_GRAYED);
	m_pMDIFrame->UpdateCmdUI<ID_R2L>(
		!m_ptBuf[0]->GetReadOnly() ? enable : MF_GRAYED);

	// Strictly disallow editing when operating in limited context mode, or on
	// preprocessed content, so as to reduce risk of messing up original files.
	enable = m_idContextLines != ID_VIEW_CONTEXT_UNLIMITED ||
		m_pInfoUnpacker.get()->readOnly ? MF_GRAYED : 0;

	m_pMDIFrame->UpdateCmdUI<ID_FILE_LEFT_READONLY>(
		enable | (m_ptBuf[0]->GetReadOnly() ? MF_CHECKED : 0));

	m_pMDIFrame->UpdateCmdUI<ID_FILE_RIGHT_READONLY>(
		enable | (m_ptBuf[1]->GetReadOnly() ? MF_CHECKED : 0));

	// Navigation
	CMergeEditView *const pMergeView = GetActiveMergeView();

	const POINT pos = pMergeView->GetCursorPos();

	if (currDiff == -1)
		currDiff = m_diffList.LineToDiff(pos.y);
	m_pMDIFrame->UpdateCmdUI<ID_CURDIFF>(
		currDiff != -1 && m_diffList.IsDiffSignificant(currDiff) ?
		MF_ENABLED : MF_GRAYED);

	currDiff = m_diffList.FirstSignificantDiff();
	m_pMDIFrame->UpdateCmdUI<ID_PREVDIFF>(currDiff != -1 &&
		pos.y > static_cast<long>(m_diffList.DiffRangeAt(currDiff)->dend0) ?
		MF_ENABLED : MF_GRAYED);
	currDiff = m_diffList.LastSignificantDiff();
	m_pMDIFrame->UpdateCmdUI<ID_NEXTDIFF>(currDiff != -1 &&
		pos.y < static_cast<long>(m_diffList.DiffRangeAt(currDiff)->dbegin0) ?
		MF_ENABLED : MF_GRAYED);

	// Enable select difference menuitem if current line is inside difference.
	m_pMDIFrame->UpdateCmdUI<ID_SELECTLINEDIFF>(
		pMergeView->GetLineFlags(pos.y) & LF_DIFF ? MF_ENABLED : MF_GRAYED);

	// Clipboard
	CCrystalTextView *const pTextView = GetActiveTextView();
	m_pMDIFrame->UpdateCmdUI<ID_EDIT_CUT>(
		pTextView && pTextView->QueryEditable() && pTextView->IsSelection() ? MF_ENABLED : MF_GRAYED);
	m_pMDIFrame->UpdateCmdUI<ID_EDIT_COPY>(
		pTextView && pTextView->IsSelection() ? MF_ENABLED : MF_GRAYED);
	m_pMDIFrame->UpdateCmdUI<ID_EDIT_PASTE>(
		pTextView && pTextView->QueryEditable() && IsClipboardFormatAvailable(CF_UNICODETEXT) ? MF_ENABLED : MF_GRAYED);
	m_pMDIFrame->UpdateCmdUI<ID_TOOLS_COMPARE_SELECTION>(
		m_pOpener == NULL && m_pView[0]->IsSelection() && m_pView[1]->IsSelection() ? MF_ENABLED : MF_GRAYED);

	// General editing
	enable = pMergeView->QueryEditable() ? MF_ENABLED : MF_GRAYED;
	m_pMDIFrame->UpdateCmdUI<ID_EDIT_REPLACE>(enable);

	// EOL style
	CDiffTextBuffer *const pTextBuffer = m_ptBuf[pMergeView->m_nThisPane];
	int nStyle = pTextBuffer->GetCRLFMode();
	if (COptionsMgr::Get(OPT_ALLOW_MIXED_EOL) ||
		IsMixedEOL(pMergeView->m_nThisPane))
	{
		nStyle = CRLF_STYLE_AUTOMATIC;
	}
	m_pMDIFrame->UpdateCmdUI<ID_EOL_TO_DOS>(
		enable | (nStyle == CRLF_STYLE_DOS ? MF_CHECKED : 0));
	m_pMDIFrame->UpdateCmdUI<ID_EOL_TO_UNIX>(
		enable | (nStyle == CRLF_STYLE_UNIX ? MF_CHECKED : 0));
	m_pMDIFrame->UpdateCmdUI<ID_EOL_TO_MAC>(
		enable | (nStyle == CRLF_STYLE_MAC ? MF_CHECKED : 0));

	UpdateSyncPointUI();
}

void CChildFrame::UpdateCmdUI()
{
	if (m_pMDIFrame->GetActiveDocFrame() != this)
		return;

	// Commands which are applicable only after editing
	UpdateEditCmdUI();

	// General
	UpdateGeneralCmdUI();

	// Bookmarks
	UpdateBookmarkUI();
	UpdateSourceTypeUI();
}

void CChildFrame::UpdateBookmarkUI()
{
	if (CCrystalTextView *pTextView = GetActiveTextView())
	{
		m_pMDIFrame->UpdateCmdUI<ID_EDIT_GOTO_NEXT_BOOKMARK>(
			pTextView->HasBookmarks() ? MF_ENABLED : MF_GRAYED);
	}
}

void CChildFrame::UpdateSyncPointUI()
{
	BYTE enable = !HasSyncPoints() ? MF_GRAYED :
		IsCursorAtSyncPoint() ? MF_CHECKED : MF_UNCHECKED;
	m_pMDIFrame->UpdateCmdUI<ID_CLEAR_SYNCPOINTS>(enable & MF_GRAYED);
	m_pMDIFrame->UpdateCmdUI<ID_SET_SYNCPOINT>(enable & MF_CHECKED);
}

void CChildFrame::UpdateSourceTypeUI()
{
	if (CCrystalTextView *pTextView = GetActiveTextView())
	{
		m_pMDIFrame->UpdateSourceTypeUI(pTextView->m_CurSourceDef->type);
	}
}

/**
 * @brief Update MergingMode UI in statusbar
 */
void CChildFrame::UpdateMergeStatusUI()
{
	const bool bMergingMode = GetMergingMode();
	m_pMDIFrame->GetStatusBar()->SetPartText(1, bMergingMode ?
		LanguageSelect.LoadString(IDS_MERGEMODE_MERGING).c_str() : NULL);
}

/**
 * @brief Enable/disable buffer read-only
 */
void CChildFrame::OnReadOnly(int nSide)
{
	bool bReadOnly = !m_ptBuf[nSide]->GetReadOnly();
	m_bInitialReadOnly[nSide] = bReadOnly;
	m_ptBuf[nSide]->SetReadOnly(bReadOnly);
	m_pView[nSide]->UpdateLineInfoStatus();
}

/**
 * @brief Update any resources necessary after a GUI language change
 */
void CChildFrame::UpdateResources()
{
	if (m_strPath[0].empty() &&
		m_strPath[1].empty())
	{
		m_strDesc[0] = LanguageSelect.LoadString(IDS_EMPTY_LEFT_FILE);
		m_strDesc[1] = LanguageSelect.LoadString(IDS_EMPTY_RIGHT_FILE);
	}
	UpdateHeaderPath(0);
	UpdateHeaderPath(1);
	GetLeftView()->UpdateResources();
	GetRightView()->UpdateResources();
}

/**
 * @brief Save pane sizes and positions when one of panes requests it.
 */
void CChildFrame::RecalcLayout()
{
	RECT rectClient;
	GetClientRect(&rectClient);
	if (m_wndDiffViewBar.m_hWnd && (m_wndDiffViewBar.GetStyle() & WS_VISIBLE))
	{
		RECT rect;
		m_wndDiffViewBar.GetWindowRect(&rect);
		ScreenToClient(&rect);
		rectClient.bottom -= rect.bottom -= rect.top;
		m_wndDiffViewBar.MoveWindow(rect.left, rectClient.bottom,
			rectClient.right - rectClient.left, rect.bottom);
	}
	if (m_wndLocationBar.m_hWnd && (m_wndLocationBar.GetStyle() & WS_VISIBLE))
	{
		RECT rect;
		m_wndLocationBar.GetWindowRect(&rect);
		ScreenToClient(&rect);
		rectClient.left += rect.right -= rect.left;
		m_wndLocationBar.MoveWindow(rect.left, rect.top,
			rect.right, rectClient.bottom - rectClient.top);
	}
	if (m_wndFilePathBar.m_hWnd)
	{
		m_wndFilePathBar.MoveWindow(
			rectClient.left, rectClient.top,
			rectClient.right - rectClient.left,
			rectClient.bottom - rectClient.top);
	}
}
