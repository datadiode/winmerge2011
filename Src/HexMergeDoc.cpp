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
 * @file  HexMergeDoc.cpp
 *
 * @brief Implementation file for CHexMergeDoc
 *
 */
#include "stdafx.h"
#include "Merge.h"
#include "LanguageSelect.h"
#include "HexMergeFrm.h"
#include "HexMergeView.h"
#include "MainFrm.h"
#include "Environment.h"
#include "coretools.h"
#include "DirFrame.h"
#include "SaveClosingDlg.h"
#include "paths.h"
#include "FileOrFolderSelect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/**
 * @brief Update diff item
 */
static void UpdateDiffItem(DIFFITEM *di, CDiffContext *pCtxt)
{
	di->diffcode |= DIFFCODE::SIDEFLAGS;
	di->left.ClearPartial();
	di->right.ClearPartial();
	if (!pCtxt->UpdateInfoFromDiskHalf(di, TRUE))
		di->diffcode &= ~DIFFCODE::LEFT;
	if (!pCtxt->UpdateInfoFromDiskHalf(di, FALSE))
		di->diffcode &= ~DIFFCODE::RIGHT;
	// 1. Clear flags
	di->diffcode &= ~(DIFFCODE::TEXTFLAGS | DIFFCODE::COMPAREFLAGS);
	// 2. Process unique files
	// We must compare unique files to itself to detect encoding
	if (di->isSideLeftOnly() || di->isSideRightOnly())
	{
		int compareMethod = pCtxt->m_nCompMethod;
		if (compareMethod != CMP_DATE &&
			compareMethod != CMP_DATE_SIZE &&
			compareMethod != CMP_SIZE)
		{
			di->diffcode |= DIFFCODE::SAME;
			FolderCmp folderCmp(pCtxt);
			int diffCode = folderCmp.prepAndCompareTwoFiles(di);
			// Add possible binary flag for unique items
			if (diffCode & DIFFCODE::BIN)
				di->diffcode |= DIFFCODE::BIN;
		}
	}
	// 3. Compare two files
	else
	{
		// Really compare
		FolderCmp folderCmp(pCtxt);
		di->diffcode |= folderCmp.prepAndCompareTwoFiles(di);
	}
}

/**
 * @brief Issue an error popup if passed in HRESULT is nonzero
 */
static int Try(HRESULT hr, UINT type = MB_OKCANCEL | MB_ICONSTOP)
{
	return hr ? OException(hr).ReportError(theApp.m_pMainWnd->m_hWnd, type) : 0;
}

/////////////////////////////////////////////////////////////////////////////
// CHexMergeDoc

/////////////////////////////////////////////////////////////////////////////
// CHexMergeDoc construction/destruction

CHexMergeView *CHexMergeFrame::GetActiveView() const
{
	return m_pView[::GetFocus() == m_pView[1]->m_hWnd];
}

/**
 * @brief Update associated diff item
 */
void CHexMergeFrame::UpdateDiffItem(CDirFrame *pDirDoc)
{
	// If directory compare has results
	if (pDirDoc)
	{
		const String &pathLeft = m_strPath[0];
		const String &pathRight = m_strPath[1];
		if (DIFFITEM *di = pDirDoc->FindItemFromPaths(pathLeft.c_str(), pathRight.c_str()))
		{
			::UpdateDiffItem(di, pDirDoc->GetDiffContext());
		}
	}
	int lengthLeft = m_pView[0]->GetLength();
	void *bufferLeft = m_pView[0]->GetBuffer(lengthLeft);
	int lengthRight = m_pView[1]->GetLength();
	void *bufferRight = m_pView[1]->GetBuffer(lengthRight);
	SetLastCompareResult(lengthLeft != lengthRight ||
		bufferLeft && bufferRight && memcmp(bufferLeft, bufferRight, lengthLeft));
}

/**
 * @brief Asks and then saves modified files
 */
bool CHexMergeFrame::PromptAndSaveIfNeeded(bool bAllowCancel)
{
	const BOOL bLModified = m_pView[0]->GetModified();
	const BOOL bRModified = m_pView[1]->GetModified();

	if (!bLModified && !bRModified) //Both files unmodified
		return true;

	const String &pathLeft = m_strPath[0];
	const String &pathRight = m_strPath[1];

	bool result = true;
	bool bLSaveSuccess = false;
	bool bRSaveSuccess = false;

	SaveClosingDlg dlg;
	dlg.m_bAskForLeft = bLModified;
	dlg.m_bAskForRight = bRModified;
	if (!bAllowCancel)
		dlg.m_bDisableCancel = TRUE;
	if (!pathLeft.empty())
		dlg.m_sLeftFile = pathLeft;
	else
		dlg.m_sLeftFile = m_strDesc[0];
	if (!pathRight.empty())
		dlg.m_sRightFile = pathRight;
	else
		dlg.m_sRightFile = m_strDesc[1];

	if (LanguageSelect.DoModal(dlg) == IDOK)
	{
		if (bLModified)
		{
			if (dlg.m_leftSave == SaveClosingDlg::SAVECLOSING_SAVE)
			{
				switch (Try(m_pView[0]->SaveFile(pathLeft.c_str())))
				{
				case 0:
					bLSaveSuccess = true;
					break;
				case IDCANCEL:
					result = false;
					break;
				}
			}
			else
			{
				m_pView[0]->SetModified(FALSE);
			}
		}
		if (bRModified)
		{
			if (dlg.m_rightSave == SaveClosingDlg::SAVECLOSING_SAVE)
			{
				switch (Try(m_pView[1]->SaveFile(pathRight.c_str())))
				{
				case 0:
					bRSaveSuccess = true;
					break;
				case IDCANCEL:
					result = false;
					break;
				}
			}
			else
			{
				m_pView[1]->SetModified(FALSE);
			}
		}
	}
	else
	{	
		result = false;
	}

	// If file were modified and saving was successfull,
	// update status on dir view
	if (bLSaveSuccess || bRSaveSuccess)
	{
		UpdateDiffItem(m_pDirDoc);
	}

	return result;
}

/**
 * @brief Save modified documents
 */
bool CHexMergeFrame::SaveModified()
{
	return PromptAndSaveIfNeeded(true);
}

/**
 * @brief Saves both files
 */
void CHexMergeFrame::OnFileSave() 
{
	BOOL bUpdate = FALSE;
	if (m_pView[0]->GetModified())
	{
		const String &pathLeft = m_strPath[0];
		if (Try(m_pView[0]->SaveFile(pathLeft.c_str())) == IDCANCEL)
			return;
		UpdateHeaderPath(0);
		bUpdate = TRUE;
	}
	if (m_pView[1]->GetModified())
	{
		const String &pathRight = m_strPath[1];
		if (Try(m_pView[1]->SaveFile(pathRight.c_str())) == IDCANCEL)
			return;
		UpdateHeaderPath(1);
		bUpdate = TRUE;
	}
	if (bUpdate)
		UpdateDiffItem(m_pDirDoc);
}

/**
 * @brief Saves left-side file
 */
void CHexMergeFrame::OnFileSaveLeft()
{
	if (m_pView[0]->GetModified())
	{
		const String &pathLeft = m_strPath[0];
		if (Try(m_pView[0]->SaveFile(pathLeft.c_str())) == IDCANCEL)
			return;
		UpdateDiffItem(m_pDirDoc);
	}
}

/**
 * @brief Saves right-side file
 */
void CHexMergeFrame::OnFileSaveRight()
{
	if (m_pView[1]->GetModified())
	{
		const String &pathRight = m_strPath[1];
		if (Try(m_pView[1]->SaveFile(pathRight.c_str())) == IDCANCEL)
			return;
		UpdateDiffItem(m_pDirDoc);
	}
}

/**
 * @brief Saves left-side file with name asked
 */
void CHexMergeFrame::OnFileSaveAsLeft()
{
	String strPath = m_strPath[0];
	if (SelectFile(m_pMDIFrame->m_hWnd, strPath, IDS_SAVE_LEFT_AS, NULL, FALSE))
	{
		if (Try(m_pView[0]->SaveFile(strPath.c_str())) == IDCANCEL)
			return;
		m_strPath[0] = strPath;
		UpdateDiffItem(m_pDirDoc);
	}
}

/**
 * @brief Saves right-side file with name asked
 */
void CHexMergeFrame::OnFileSaveAsRight()
{
	String strPath = m_strPath[1];
	if (SelectFile(m_pMDIFrame->m_hWnd, strPath, IDS_SAVE_LEFT_AS, NULL, FALSE))
	{
		if (Try(m_pView[1]->SaveFile(strPath.c_str())) == IDCANCEL)
			return;
		m_strPath[1] = strPath;
		UpdateDiffItem(m_pDirDoc);
	}
}

/**
* @brief Load one file
*/
HRESULT CHexMergeFrame::LoadOneFile(int index, const FileLocation &fileinfo, BOOL readOnly)
{
	if (Try(m_pView[index]->LoadFile(fileinfo.filepath.c_str()), MB_ICONSTOP) != 0)
		return E_FAIL;
	m_pView[index]->SetReadOnly(readOnly);
	m_strPath[index] = fileinfo.filepath;
	ASSERT(m_nBufferType[index] == BUFFER_NORMAL); // should have been initialized to BUFFER_NORMAL in constructor
	if (!fileinfo.description.empty())
	{
		m_strDesc[index] = fileinfo.description;
		m_nBufferType[index] = BUFFER_NORMAL_NAMED;
	}
	UpdateHeaderPath(index);
	m_pView[index]->ResizeWindow();
	return S_OK;
}

/**
 * @brief Load files and initialize frame's compare result icon
 */
HRESULT CHexMergeFrame::OpenDocs(
	const FileLocation &filelocLeft,
	const FileLocation &filelocRight,
	BOOL bROLeft, BOOL bRORight)
{
	HRESULT hr;
	if (SUCCEEDED(hr = LoadOneFile(0, filelocLeft, bROLeft)) &&
		SUCCEEDED(hr = LoadOneFile(1, filelocRight, bRORight)))
	{
		UpdateDiffItem(0);
		// An extra ResizeWindow() on the left view aligns scroll ranges, and
		// also triggers initial diff coloring by invalidating the client area.
		m_pView[0]->ResizeWindow();
		ActivateFrame();
		UpdateWindow();
		if (COptionsMgr::Get(OPT_SCROLL_TO_FIRST))
			SendMessage(WM_COMMAND, ID_FIRSTDIFF);
	}
	else
	{
		DestroyFrame();
	}
	return hr;
}

/**
 * @brief Write path and filename to headerbar
 * @note SetText() does not repaint unchanged text
 */
void CHexMergeFrame::UpdateHeaderPath(int pane)
{
	String sText;

	if (m_nBufferType[pane] == BUFFER_UNNAMED ||
		m_nBufferType[pane] == BUFFER_NORMAL_NAMED)
	{
		sText = m_strDesc[pane];
	}
	else
	{
		sText = m_strPath[pane];
		if (m_pDirDoc)
		{
			if (pane == 0)
				m_pDirDoc->ApplyLeftDisplayRoot(sText);
			else
				m_pDirDoc->ApplyRightDisplayRoot(sText);
		}
	}
	m_wndFilePathBar.SetText(pane, sText.c_str(), m_pView[pane]->GetModified());
	SetTitle();
}

/**
 * @brief Update document filenames to title
 */
void CHexMergeFrame::SetTitle()
{
	string_format sTitle(_T("%s%s - %s%s"),
		&_T("\0* ")[m_pView[0]->GetModified() != FALSE],
		!m_strDesc[0].empty() ? m_strDesc[0].c_str() : PathFindFileName(m_strPath[0].c_str()),
		&_T("\0* ")[m_pView[1]->GetModified() != FALSE],
		!m_strDesc[1].empty() ? m_strDesc[1].c_str() : PathFindFileName(m_strPath[1].c_str()));
	SetWindowText(sTitle.c_str());
}

/**
 * @brief We have two child views (left & right), so we keep pointers directly
 * at them (the MFC view list doesn't have them both)
 */
void CHexMergeFrame::SetMergeViews(CHexMergeView * pLeft, CHexMergeView * pRight)
{
	ASSERT(pLeft && !m_pView[0]);
	m_pView[0] = pLeft;
	ASSERT(pRight && !m_pView[1]);
	m_pView[1] = pRight;
}

/**
 * @brief Copy selected bytes from source view to destination view
 * @note Grows destination buffer as appropriate
 */
void CHexMergeFrame::CopySel(CHexMergeView *pViewSrc, CHexMergeView *pViewDst)
{
	const IHexEditorWindow::Status *pStatSrc = pViewSrc->GetStatus();
	int i = min(pStatSrc->iStartOfSelection, pStatSrc->iEndOfSelection);
	int j = max(pStatSrc->iStartOfSelection, pStatSrc->iEndOfSelection);
	int u = pViewSrc->GetLength();
	int v = pViewDst->GetLength();
	if (pStatSrc->bSelected && i <= v)
	{
		if (v <= j)
			v = j + 1;
		BYTE *p = pViewSrc->GetBuffer(u);
		BYTE *q = pViewDst->GetBuffer(v);
		memcpy(q + i, p + i, j - i + 1);
		HWND hwndFocus = ::GetFocus();
		if (hwndFocus != pViewSrc->m_hWnd)
			pViewDst->RepaintRange(i, j);
		if (hwndFocus != pViewDst->m_hWnd)
			pViewSrc->RepaintRange(i, j);
		pViewDst->SetModified(TRUE);
	}
}

/**
 * @brief Copy all bytes from source view to destination view
 * @note Grows destination buffer as appropriate
 */
void CHexMergeFrame::CopyAll(CHexMergeView *pViewSrc, CHexMergeView *pViewDst)
{
	if (int i = pViewSrc->GetLength())
	{
		int j = pViewDst->GetLength();
		BYTE *p = pViewSrc->GetBuffer(i);
		BYTE *q = pViewDst->GetBuffer(max(i, j));
		if (q == 0)
			OException::Throw(ERROR_OUTOFMEMORY);
		memcpy(q, p, i);
		HWND hwndFocus = ::GetFocus();
		if (hwndFocus != pViewSrc->m_hWnd)
			pViewDst->RepaintRange(0, i);
		if (hwndFocus != pViewDst->m_hWnd)
			pViewSrc->RepaintRange(0, i);
		pViewDst->SetModified(TRUE);
	}
}

/**
 * @brief Copy selected bytes from left to right
 */
void CHexMergeFrame::OnL2r()
{
	CopySel(m_pView[0], m_pView[1]);
}

/**
 * @brief Copy selected bytes from right to left
 */
void CHexMergeFrame::OnR2l()
{
	CopySel(m_pView[1], m_pView[0]);
}

/**
 * @brief Copy all bytes from left to right
 */
void CHexMergeFrame::OnAllRight()
{
	CopyAll(m_pView[0], m_pView[1]);
}

/**
 * @brief Copy all bytes from right to left
 */
void CHexMergeFrame::OnAllLeft()
{
	CopyAll(m_pView[1], m_pView[0]);
}

/**
 * @brief Called when user selects View/Zoom In|Out|Normal from menu.
 */
void CHexMergeFrame::OnViewZoom(int direction)
{
	m_pView[0]->ZoomText(direction);
	m_pView[1]->ZoomText(direction);
	RecalcBytesPerLine();
}
