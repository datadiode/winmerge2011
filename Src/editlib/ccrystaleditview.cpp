///////////////////////////////////////////////////////////////////////////
//  File:       ccrystaleditview.cpp
//  Version:    1.2.0.5
//  Created:    29-Dec-1998
//
//  Copyright:  Stcherbatchenko Andrei
//  E-mail:     windfall@gmx.de
//
//  Implementation of the CCrystalEditView class, a part of the Crystal Edit -
//  syntax coloring text editor.
//
//  You are free to use or modify this code to the following restrictions:
//  - Acknowledge me somewhere in your about box, simple "Parts of code by.."
//  will be enough. If you can't (or don't want to), contact me personally.
//  - LEAVE THIS HEADER INTACT
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//  21-Feb-99
//      Paul Selormey, James R. Twine:
//  +   FEATURE: description for Undo/Redo actions
//  +   FEATURE: multiple MSVC-like bookmarks
//  +   FEATURE: 'Disable backspace at beginning of line' option
//  +   FEATURE: 'Disable drag-n-drop editing' option
//
//  +   FEATURE: Auto indent
//  +   FIX: ResetView() was overriden to provide cleanup
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//  19-Jul-99
//      Ferdinand Prantl:
//  +   FEATURE: support for autoindenting brackets and parentheses
//  +   FEATURE: menu options, view and window
//  +   FEATURE: SDI+MDI versions with help
//  +   FEATURE: extended registry support for saving settings
//  +   FEATURE: some other things I've forgotten ...
//  27-Jul-99
//  +   FIX: treating groups in regular expressions corrected
//  +   FIX: autocomplete corrected
//
//  ... it's being edited very rapidly so sorry for non-commented
//        and maybe "ugly" code ...
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//	??-Aug-99
//		Sven Wiegand (search for "//BEGIN SW" to find my changes):
//	+ FEATURE: "Go to last change" (sets the cursor on the position where
//			the user did his last edit actions
//	+ FEATURE: Support for incremental search in CCrystalTextView
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//	24-Oct-99
//		Sven Wiegand
//	+ FEATURE: Supporting [Return] to exit incremental-search-mode
//		     (see OnChar())
////////////////////////////////////////////////////////////////////////////

/**
 * @file  ccrystaleditview.cpp
 *
 * @brief Implementation of the CCrystalEditView class
 */
#include "StdAfx.h"
#include <MyCom.h>
#include "LanguageSelect.h"
#include "editcmd.h"
#include "ccrystaleditview.h"
#include "ccrystaltextbuffer.h"
#include "ceditreplacedlg.h"
#include "SettingStore.h"
#include "string_util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const unsigned int MAX_TAB_LEN = 64;  // Same as in CrystalViewText.cpp

#define DRAG_BORDER_X       5
#define DRAG_BORDER_Y       5

/////////////////////////////////////////////////////////////////////////////
// CCrystalEditView

CCrystalEditView::CCrystalEditView(size_t ZeroInit)
: CCrystalTextView(ZeroInit)
, m_bAutoIndent(true)
{
}

CCrystalEditView::~CCrystalEditView()
{
}

CCrystalTextView::TextDefinition *CCrystalEditView::DoSetTextType(TextDefinition *def)
{
	SetAutoIndent((def->flags & SRCOPT_AUTOINDENT) != 0);
	return CCrystalTextView::DoSetTextType(def);
}

/////////////////////////////////////////////////////////////////////////////
// CCrystalEditView message handlers

void CCrystalEditView::ResetView()
{
	m_bOvrMode = false;
	m_bLastReplace = FALSE;
	CCrystalTextView::ResetView();
}

bool CCrystalEditView::QueryEditable()
{
	return m_pTextBuffer && !m_pTextBuffer->GetReadOnly();
}

void CCrystalEditView::DeleteCurrentSelection()
{
	if (IsSelection())
	{
		POINT ptSelStart, ptSelEnd;
		GetSelection(ptSelStart, ptSelEnd);

		POINT ptCursorPos = ptSelStart;
		ASSERT_VALIDTEXTPOS(ptCursorPos);
		SetAnchor(ptCursorPos);
		SetSelection(ptCursorPos, ptCursorPos);
		SetCursorPos(ptCursorPos);
		EnsureCursorVisible();

		// [JRT]:
		m_pTextBuffer->DeleteText(this, ptSelStart.y, ptSelStart.x, ptSelEnd.y, ptSelEnd.x, CE_ACTION_DELSEL);
	}
}

void CCrystalEditView::OnEditPaste()
{
	if (!QueryEditable())
		return;
	if (!OpenClipboard())
		return;
	HGLOBAL hData = GetClipboardData(CF_UNICODETEXT);
	if (LPCTSTR pszText = reinterpret_cast<LPTSTR>(::GlobalLock(hData)))
	{
		SIZE_T cbData = ::GlobalSize(hData);
		int cchText = static_cast<int>(cbData / sizeof(TCHAR) - 1);

		if (cchText <= 0)
			cchText = 0;
		// If in doubt, assume zero-terminated string
		else if (!IsClipboardFormatAvailable(RegisterClipboardFormat(_T("WinMergeClipboard"))))
			cchText = static_cast<int>(_tcslen(pszText));

		m_pTextBuffer->BeginUndoGroup();

		POINT ptCursorPos;
		if (IsSelection ())
		{
			POINT ptSelStart, ptSelEnd;
			GetSelection(ptSelStart, ptSelEnd);
			ptCursorPos = ptSelStart;
			// [JRT]:
			m_pTextBuffer->DeleteText(this, ptSelStart.y, ptSelStart.x, ptSelEnd.y, ptSelEnd.x, CE_ACTION_PASTE);
		}
		else
		{
			ptCursorPos = GetCursorPos();
		}
		ASSERT_VALIDTEXTPOS(ptCursorPos);

		ptCursorPos = m_pTextBuffer->InsertText(this, ptCursorPos.y, ptCursorPos.x, pszText, cchText, CE_ACTION_PASTE);  //  [JRT]
		ASSERT_VALIDTEXTPOS(ptCursorPos);
		SetAnchor(ptCursorPos);
		SetSelection(ptCursorPos, ptCursorPos);
		SetCursorPos(ptCursorPos);
		EnsureCursorVisible();

		m_pTextBuffer->FlushUndoGroup(this);
		GlobalUnlock(hData);
	}
	CloseClipboard();
	m_pTextBuffer->SetModified(true);
}

void CCrystalEditView::OnEditCut()
{
	if (!QueryEditable())
		return;

	HGLOBAL hData = PrepareDragData();
	if (hData == NULL)
		return;
	PutToClipboard(hData);

	POINT ptSelStart, ptSelEnd;
	GetSelection(ptSelStart, ptSelEnd);

	POINT ptCursorPos = ptSelStart;
	ASSERT_VALIDTEXTPOS(ptCursorPos);
	SetAnchor(ptCursorPos);
	SetSelection(ptCursorPos, ptCursorPos);
	SetCursorPos(ptCursorPos);
	EnsureCursorVisible();

	m_pTextBuffer->DeleteText(this, ptSelStart.y, ptSelStart.x, ptSelEnd.y, ptSelEnd.x, CE_ACTION_CUT);  // [JRT]
	m_pTextBuffer->SetModified(true);
}

void CCrystalEditView::OnEditDelete()
{
	if (!QueryEditable())
		return;

	POINT ptSelStart, ptSelEnd;
	GetSelection(ptSelStart, ptSelEnd);
	if (ptSelStart == ptSelEnd)
	{
		if (ptSelEnd.x == GetLineLength(ptSelEnd.y))
		{
			if (ptSelEnd.y == GetLineCount() - 1)
				return;
			ptSelEnd.y++;
			ptSelEnd.x = 0;
		}
		else 
		{
			ptSelEnd.x++;
		}
	}

	POINT ptCursorPos = ptSelStart;
	ASSERT_VALIDTEXTPOS(ptCursorPos);
	SetAnchor(ptCursorPos);
	SetSelection(ptCursorPos, ptCursorPos);
	SetCursorPos(ptCursorPos);
	EnsureCursorVisible();

	m_pTextBuffer->DeleteText(this, ptSelStart.y, ptSelStart.x, ptSelEnd.y, ptSelEnd.x, CE_ACTION_DELETE);   // [JRT]
}

void CCrystalEditView::OnChar(WPARAM nChar)
{
	if ((::GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0 ||
		(::GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0)
	{
		return;
	}

	if (nChar == VK_RETURN)
	{
		if (m_bOvrMode)
		{
			POINT ptCursorPos = GetCursorPos();
			ASSERT_VALIDTEXTPOS(ptCursorPos);
			if (ptCursorPos.y < GetLineCount() - 1)
			{
				ptCursorPos.x = 0;
				ptCursorPos.y++;

				ASSERT_VALIDTEXTPOS(ptCursorPos);
				SetSelection(ptCursorPos, ptCursorPos);
				SetAnchor(ptCursorPos);
				SetCursorPos(ptCursorPos);
				EnsureCursorVisible();
				return;
			}
		}

		m_pTextBuffer->BeginUndoGroup(m_bMergeUndo);
		m_bMergeUndo = false;

		if (QueryEditable())
		{
			POINT ptCursorPos;
			if (IsSelection())
			{
				POINT ptSelStart, ptSelEnd;
				GetSelection(ptSelStart, ptSelEnd);
				ptCursorPos = ptSelStart;
				// [JRT]:
				m_pTextBuffer->DeleteText(this, ptSelStart.y, ptSelStart.x, ptSelEnd.y, ptSelEnd.x, CE_ACTION_TYPING);
			}
			else
			{
				ptCursorPos = GetCursorPos();
			}
			ASSERT_VALIDTEXTPOS(ptCursorPos);
			LPCTSTR pszText = m_pTextBuffer->GetDefaultEol();
			int cchText = static_cast<int>(_tcslen(pszText));

			ptCursorPos = m_pTextBuffer->InsertText(this, ptCursorPos.y, ptCursorPos.x, pszText, cchText, CE_ACTION_TYPING);  //  [JRT]
			ASSERT_VALIDTEXTPOS(ptCursorPos);
			SetSelection(ptCursorPos, ptCursorPos);
			SetAnchor(ptCursorPos);
			SetCursorPos(ptCursorPos);
			EnsureCursorVisible();
		}

		m_pTextBuffer->FlushUndoGroup (this);
		return;
	}
	// Accept control characters other than [\t\n\r] through Alt-Numpad
	if (nChar > 31 ||
		GetKeyState(VK_CONTROL) >= 0 &&
		(nChar != 27 || GetKeyState(VK_ESCAPE) >= 0) &&
		nChar != 9 && nChar != 10 && nChar != 13)
	{
		if (QueryEditable())
        {
			m_pTextBuffer->BeginUndoGroup(m_bMergeUndo);

			POINT ptSelStart, ptSelEnd;
			GetSelection(ptSelStart, ptSelEnd);
			POINT ptCursorPos;
			if (ptSelStart != ptSelEnd)
			{
				ptCursorPos = ptSelStart;
				// [JRT]:
				m_pTextBuffer->DeleteText(this, ptSelStart.y, ptSelStart.x, ptSelEnd.y, ptSelEnd.x, CE_ACTION_TYPING);
			}
			else
			{
				ptCursorPos = GetCursorPos();
				if (m_bOvrMode)
				{
					LPCTSTR pszChars = GetLineChars(ptCursorPos.y);
					int nLineLength = GetLineLength(ptCursorPos.y);
					int nEndPos = ptCursorPos.x;
					if (nEndPos < nLineLength)
					{
						do
						{
							++nEndPos;
						} while (nEndPos < nLineLength && GetCharWidthFromChar(pszChars + nEndPos) == 0);
						m_pTextBuffer->DeleteText(this, ptCursorPos.y, ptCursorPos.x, ptCursorPos.y, nEndPos, CE_ACTION_TYPING);     // [JRT]
					}
				}
			}

			ASSERT_VALIDTEXTPOS(ptCursorPos);

			TCHAR pszText[2] = { static_cast<TCHAR>(nChar), _T('\0') };

			ptCursorPos = m_pTextBuffer->InsertText(this, ptCursorPos.y, ptCursorPos.x, pszText, 1, CE_ACTION_TYPING);    // [JRT]
			ASSERT_VALIDTEXTPOS(ptCursorPos);
			SetSelection(ptCursorPos, ptCursorPos);
			SetAnchor(ptCursorPos);
			SetCursorPos(ptCursorPos);
			EnsureCursorVisible();

			m_bMergeUndo = true;
			m_pTextBuffer->FlushUndoGroup(this);
		}
	}
}

void CCrystalEditView::OnEditDeleteBack()
{
	if (!QueryEditable())
		return;

	if (IsSelection())
	{
		OnEditDelete();
		return;
	}
	POINT ptCurrentCursorPos = m_ptCursorPos;
	MoveLeft(FALSE);
	if (ptCurrentCursorPos != m_ptCursorPos)
	{
		m_pTextBuffer->DeleteText(this, m_ptCursorPos.y, m_ptCursorPos.x,
			ptCurrentCursorPos.y, ptCurrentCursorPos.x, CE_ACTION_BACKSPACE);  // [JRT]
	}
}

void CCrystalEditView::OnEditTab()
{
	if (!QueryEditable())
		return;

	POINT ptSelStart, ptSelEnd;
	GetSelection(ptSelStart, ptSelEnd);
	POINT ptCursorPos = GetCursorPos();
	ASSERT_VALIDTEXTPOS(ptCursorPos);
	// If we have more than one line selected, tabify sel lines
	const bool bTabify = ptSelStart.y != ptSelEnd.y;
	const int nTabSize = GetTabSize();
	int nChars = nTabSize;
	if (!bTabify)
		nChars -= CalculateActualOffset(ptCursorPos.y, ptCursorPos.x) % nTabSize;
	ASSERT(nChars > 0 && nChars <= nTabSize);

	TCHAR pszText[MAX_TAB_LEN + 1];
	// If inserting tabs, then initialize the text to a tab.
	if (m_pTextBuffer->GetInsertTabs())
	{
		pszText[0] = '\t';
		pszText[1] = '\0';
	}
	else //...otherwise, built whitespace depending on the location and tab stops
	{
		for (int i = 0; i < nChars; i++)
			pszText[i] = ' ';
		pszText[nChars] = '\0';
	}

	// Indent selected lines (multiple lines selected)
	if (bTabify)
	{
		m_pTextBuffer->BeginUndoGroup();
		int nStartLine = ptSelStart.y;
		int nEndLine = ptSelEnd.y;
		ptSelStart.x = 0;
		if (ptSelEnd.x == 0)
		{
			// Do not indent empty line.
			--nEndLine;
		}
		else if (ptSelEnd.y == GetLineCount() - 1)
		{
			ptSelEnd.x = GetLineLength(ptSelEnd.y);
		}
		else
		{
			ptSelEnd.x = 0;
			++ptSelEnd.y;
		}
		SetSelection(ptSelStart, ptSelEnd);
		SetCursorPos(ptSelEnd);
		EnsureCursorVisible();

		//  Shift selection to right
		for (int i = nStartLine ; i <= nEndLine ; ++i)
		{
			if (GetLineLength(i))
			{
				m_pTextBuffer->InsertText(NULL, i, 0, pszText, static_cast<int>(_tcslen(pszText)), CE_ACTION_INDENT);  //  [JRT]
				m_pTextBuffer->UpdateViews(this, NULL, UPDATE_SINGLELINE, i);
			}
		}
		m_pTextBuffer->UpdateViews(this, NULL, UPDATE_SINGLELINE | UPDATE_HORZRANGE);
		m_pTextBuffer->FlushUndoGroup(this);
		return;
	}
	// Overwrite mode, replace next char with tab/spaces
	if (m_bOvrMode)
    {
		int nLineLength = GetLineLength(ptCursorPos.y);
		LPCTSTR pszLineChars = GetLineChars(ptCursorPos.y);
		// Not end of line
		if (ptCursorPos.x < nLineLength)
		{
			while (nChars > 0)
			{
				if (ptCursorPos.x == nLineLength)
					break;
				if (pszLineChars[ptCursorPos.x] == _T('\t'))
				{
					++ptCursorPos.x;
					break;
				}
				++ptCursorPos.x;
				--nChars;
			}
			ASSERT(ptCursorPos.x <= nLineLength);
			ASSERT_VALIDTEXTPOS(ptCursorPos);
			SetSelection(ptCursorPos, ptCursorPos);
			SetAnchor(ptCursorPos);
			SetCursorPos(ptCursorPos);
			EnsureCursorVisible();
			return;
		}
	}

	m_pTextBuffer->BeginUndoGroup();

	// Text selected, no overwrite mode, replace sel with tab
	if (IsSelection())
	{
		POINT ptSelEnd;
		GetSelection(ptCursorPos, ptSelEnd);
		// [JRT]:
		m_pTextBuffer->DeleteText(this, ptCursorPos.y, ptCursorPos.x, ptSelEnd.y, ptSelEnd.x, CE_ACTION_TYPING);
	}

	// No selection, add tab
	ptCursorPos = m_pTextBuffer->InsertText(this, ptCursorPos.y, ptCursorPos.x, pszText, static_cast<int>(_tcslen(pszText)), CE_ACTION_TYPING);  //  [JRT]
	ASSERT_VALIDTEXTPOS(ptCursorPos);
	SetSelection(ptCursorPos, ptCursorPos);
	SetAnchor(ptCursorPos);
	SetCursorPos(ptCursorPos);
	EnsureCursorVisible();
	m_pTextBuffer->FlushUndoGroup(this);
}

void CCrystalEditView::OnEditUntab()
{
	if (!QueryEditable())
		return;

	POINT ptSelStart, ptSelEnd;
	GetSelection(ptSelStart, ptSelEnd);
	// If we have more than one line selected, tabify sel lines
	const bool bTabify = ptSelStart.y != ptSelEnd.y;

	if (bTabify)
	{
		m_pTextBuffer->BeginUndoGroup();
		int nStartLine = ptSelStart.y;
		int nEndLine = ptSelEnd.y;
		ptSelStart.x = 0;
		if (ptSelEnd.x == 0)
		{
			--nEndLine;
		}
		else if (ptSelEnd.y == GetLineCount() - 1)
		{
			ptSelEnd.x = GetLineLength(ptSelEnd.y);
		}
		else
		{
			ptSelEnd.x = 0;
			++ptSelEnd.y;
		}
		SetSelection(ptSelStart, ptSelEnd);
		SetCursorPos(ptSelEnd);
		EnsureCursorVisible();

		//  Shift selection to left
		for (int i = nStartLine ; i <= nEndLine ; ++i)
        {
			int nLength = GetLineLength(i);
			if (nLength > 0)
			{
				LPCTSTR pszChars = GetLineChars(i);
				int nPos = 0, nOffset = 0;
				while (nPos < nLength)
				{
					if (pszChars[nPos] == _T(' '))
					{
						nPos++;
						if (++nOffset >= GetTabSize())
							break;
					}
					else
					{
						if (pszChars[nPos] == _T('\t'))
							nPos++;
						break;
					}
				}
				if (nPos > 0)
				{
					m_pTextBuffer->DeleteText(NULL, i, 0, i, nPos, CE_ACTION_INDENT);  // [JRT]
					m_pTextBuffer->UpdateViews(this, NULL, UPDATE_SINGLELINE, i);
				}
			}
		}
		m_pTextBuffer->UpdateViews(this, NULL, UPDATE_SINGLELINE | UPDATE_HORZRANGE);
		m_pTextBuffer->FlushUndoGroup(this);
	}
	else
    {
		POINT ptCursorPos = GetCursorPos();
		ASSERT_VALIDTEXTPOS(ptCursorPos);
		if (ptCursorPos.x > 0)
		{
			int nTabSize = GetTabSize();
			int nOffset = CalculateActualOffset(ptCursorPos.y, ptCursorPos.x);
			int nNewOffset = nOffset / nTabSize * nTabSize;
			if (nOffset == nNewOffset && nNewOffset > 0)
				nNewOffset -= nTabSize;
			ASSERT(nNewOffset >= 0);

			LPCTSTR pszChars = GetLineChars(ptCursorPos.y);
			int nCurrentOffset = 0;
			int i = 0;
			while (nCurrentOffset < nNewOffset)
			{
				if (pszChars[i] == _T('\t'))
					nCurrentOffset = nCurrentOffset / nTabSize * nTabSize + nTabSize;
				else
					nCurrentOffset++;
				++i;
			}

			ASSERT(nCurrentOffset == nNewOffset);

			ptCursorPos.x = i;
			ASSERT_VALIDTEXTPOS(ptCursorPos);
			SetSelection(ptCursorPos, ptCursorPos);
			SetAnchor(ptCursorPos);
			SetCursorPos(ptCursorPos);
			EnsureCursorVisible();
		}
	}
}

void CCrystalEditView::OnEditSwitchOvrmode()
{
	m_bOvrMode = !m_bOvrMode;
	UpdateCaret();
}

HRESULT CCrystalEditView::QueryInterface(REFIID iid, void **ppv)
{
    static const QITAB rgqit[] = 
    {   
        QITABENT(CCrystalEditView, IDropSource),
        QITABENT(CCrystalEditView, IDataObject),
        QITABENT(CCrystalEditView, IDropTarget),
        { 0 }
    };
    return QISearch(this, rgqit, iid, ppv);
}

ULONG CCrystalEditView::AddRef()
{
	return 1;
}

ULONG CCrystalEditView::Release()
{
	return 1;
}

HRESULT CCrystalEditView::DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
	HRESULT hr = CMyFormatEtc(CF_UNICODETEXT).QueryGetData(pDataObj);
	if (hr == S_OK)
	{
		POINT ptClient = { pt.x, pt.y };
		ScreenToClient(&ptClient);
		ShowDropIndicator(ptClient);
		if (*pdwEffect != DROPEFFECT_COPY)
			*pdwEffect = grfKeyState & MK_CONTROL ? DROPEFFECT_COPY : DROPEFFECT_MOVE;
	}
	else
	{
		HideDropIndicator();
		*pdwEffect = DROPEFFECT_NONE;
	}
	return hr;
}

HRESULT CCrystalEditView::DragLeave()
{
	HideDropIndicator();
	return S_OK;
}

HRESULT CCrystalEditView::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
	if (QueryEditable() && !GetDisableDragAndDrop())
	{
		POINT ptClient = { pt.x, pt.y };
		ScreenToClient(&ptClient);
		if ((grfKeyState & MK_SHIFT) == 0)
			DoDragScroll(ptClient);
		ShowDropIndicator(ptClient);
		if (*pdwEffect != DROPEFFECT_COPY)
			*pdwEffect = grfKeyState & MK_CONTROL ? DROPEFFECT_COPY : DROPEFFECT_MOVE;
	}
	else
	{
		HideDropIndicator();
		*pdwEffect = DROPEFFECT_NONE;
	}
	return S_OK;
}

HRESULT CCrystalEditView::Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
	HideDropIndicator();
	CMyStgMedium stgmedium;
	POINT ptClient = { pt.x, pt.y };
	ScreenToClient(&ptClient);
	if (QueryEditable() && !GetDisableDragAndDrop() &&
		CMyFormatEtc(CF_UNICODETEXT).GetData(pDataObj, &stgmedium) == S_OK &&
		DoDropText(stgmedium.hGlobal, ptClient))
	{
		if (*pdwEffect != DROPEFFECT_COPY)
			*pdwEffect = grfKeyState & MK_CONTROL ? DROPEFFECT_COPY : DROPEFFECT_MOVE;
	}
	else
	{
		*pdwEffect = DROPEFFECT_NONE;
	}
	return S_OK;
}

void CCrystalEditView::DoDragScroll(const POINT &point)
{
	RECT rcClientRect;
	GetClientRect(&rcClientRect);
	if (point.y < rcClientRect.top + DRAG_BORDER_Y)
	{
		HideDropIndicator();
		ScrollUp();
		UpdateWindow();
		ShowDropIndicator(point);
	}
	else if (point.y >= rcClientRect.bottom - DRAG_BORDER_Y)
	{
		HideDropIndicator();
		ScrollDown();
		UpdateWindow();
		ShowDropIndicator(point);
	}
	else if (point.x < rcClientRect.left + GetMarginWidth() + DRAG_BORDER_X)
	{
		HideDropIndicator();
		ScrollLeft();
		UpdateWindow();
		ShowDropIndicator(point);
	}
	else if (point.x >= rcClientRect.right - DRAG_BORDER_X)
	{
		HideDropIndicator();
		ScrollRight();
		UpdateWindow();
		ShowDropIndicator(point);
	}
}

bool CCrystalEditView::DoDropText(HGLOBAL hData, const POINT &ptClient)
{
	if (hData == NULL)
		return false;

	POINT ptDropPos = ClientToText(ptClient);
	if (m_bDraggingText && IsInsideSelection(ptDropPos))
	{
		SetAnchor(ptDropPos);
		SetSelection(ptDropPos, ptDropPos);
		SetCursorPos(ptDropPos);
		EnsureCursorVisible();
		return false;
	}

	SIZE_T cbData = ::GlobalSize(hData);
	int cchText = static_cast<int>(cbData / sizeof(TCHAR) - 1);
	if (cchText < 0)
		return false;
	LPTSTR pszText = reinterpret_cast<LPTSTR>(::GlobalLock(hData));
	if (pszText == NULL)
		return false;

	// Open the undo group
	// When we drag from the same panel, it is already open, so do nothing
	// (we could test m_pTextBuffer->m_bUndoGroup if it were not a protected member)
	bool bGroupFlag = false;
	if (!m_bDraggingText)
	{
		m_pTextBuffer->BeginUndoGroup();
		bGroupFlag = true;
	} 

	POINT ptCurPos = m_pTextBuffer->InsertText(this, ptDropPos.y, ptDropPos.x, pszText, cchText, CE_ACTION_DRAGDROP);  //   [JRT]
	ASSERT_VALIDTEXTPOS(ptCurPos);
	SetAnchor(ptDropPos);
	SetSelection(ptDropPos, ptCurPos);
	SetCursorPos(ptCurPos);
	EnsureCursorVisible();

	if (bGroupFlag)
		m_pTextBuffer->FlushUndoGroup(this);

	::GlobalUnlock(hData);
	return true;
}

void CCrystalEditView::ShowDropIndicator(const POINT & point)
{
	if (!m_bDropPosVisible)
	{
		HideCursor();
		m_ptSavedCaretPos = GetCursorPos();
		m_bDropPosVisible = true;
		CreateCaret((HBITMAP)1, 2, GetLineHeight());
	}
	m_ptDropPos = ClientToText(point);
	// NB: m_ptDropPos.x is index into char array, which is uncomparable to m_nOffsetChar.
	POINT ptCaretPos = TextToClient(m_ptDropPos);
	if (ptCaretPos.x >= GetMarginWidth())
	{
		SetCaretPos(ptCaretPos.x, ptCaretPos.y);
		ShowCaret();
	}
	else
	{
		HideCaret();
	}
}

void CCrystalEditView::HideDropIndicator()
{
	if (m_bDropPosVisible)
	{
		SetCursorPos(m_ptSavedCaretPos);
		ShowCursor();
		m_bDropPosVisible = false;
	}
}

DWORD CCrystalEditView::GetDropEffect()
{
	return DROPEFFECT_COPY | DROPEFFECT_MOVE;
}

void CCrystalEditView::OnDropSource(DWORD de)
{
	if (!m_bDraggingText)
		return;

	ASSERT_VALIDTEXTPOS(m_ptDraggedTextBegin);
	ASSERT_VALIDTEXTPOS(m_ptDraggedTextEnd);

	if (de == DROPEFFECT_MOVE)
	{
		m_pTextBuffer->DeleteText(this,
			m_ptDraggedTextBegin.y, m_ptDraggedTextBegin.x,
			m_ptDraggedTextEnd.y, m_ptDraggedTextEnd.x, CE_ACTION_DRAGDROP);
	}
}

void CCrystalEditView::UpdateView(CCrystalTextView *pSource, CUpdateContext *pContext, DWORD dwFlags, int nLineIndex)
{
	CCrystalTextView::UpdateView(pSource, pContext, dwFlags, nLineIndex);

	if (m_bSelectionPushed && pContext != NULL)
	{
		pContext->RecalcPoint(m_ptSavedSelStart);
		pContext->RecalcPoint(m_ptSavedSelEnd);
		ASSERT_VALIDTEXTPOS(m_ptSavedSelStart);
		ASSERT_VALIDTEXTPOS(m_ptSavedSelEnd);
	}
	if (m_bDropPosVisible)
	{
		pContext->RecalcPoint(m_ptSavedCaretPos);
		ASSERT_VALIDTEXTPOS(m_ptSavedCaretPos);
	}
}

void CCrystalEditView::OnEditReplace()
{
	if (!QueryEditable())
		return;

	CEditReplaceDlg dlg(this);
	DWORD dwFlags = m_bLastReplace ? m_dwLastReplaceFlags :
		SettingStore.GetProfileInt(_T("Editor"), _T("ReplaceFlags"), 0);

	dlg.m_bMatchCase = (dwFlags & FIND_MATCH_CASE) != 0;
	dlg.m_bWholeWord = (dwFlags & FIND_WHOLE_WORD) != 0;
	dlg.m_bRegExp = (dwFlags & FIND_REGEXP) != 0;
	dlg.m_bNoWrap = (dwFlags & FIND_NO_WRAP) != 0;
	dlg.m_sText = m_strLastFindWhat;

	if (IsSelection())
	{
		GetSelection(m_ptSavedSelStart, m_ptSavedSelEnd);
		m_bSelectionPushed = TRUE;

		dlg.SetScope(TRUE);       //  Replace in current selection
		dlg.m_ptCurrentPos = m_ptSavedSelStart;
		dlg.m_bEnableScopeSelection = TRUE;
		dlg.m_ptBlockBegin = m_ptSavedSelStart;
		dlg.m_ptBlockEnd = m_ptSavedSelEnd;

		// If the selection is in one line, copy text to dialog
		if (m_ptSavedSelStart.y == m_ptSavedSelEnd.y)
			GetText(m_ptSavedSelStart, m_ptSavedSelEnd, dlg.m_sText);
	}
	else
	{
		dlg.SetScope(FALSE);      // Set scope when no selection
		dlg.m_ptCurrentPos = GetCursorPos();
		dlg.m_bEnableScopeSelection = FALSE;

		POINT ptCursorPos = GetCursorPos();
		POINT ptStart = WordToLeft(ptCursorPos);
		POINT ptEnd = WordToRight(ptCursorPos);
		if (IsValidTextPos(ptStart) && IsValidTextPos(ptEnd) && ptStart != ptEnd)
			GetText(ptStart, ptEnd, dlg.m_sText);
	}

	//  Execute Replace dialog
	LanguageSelect.DoModal(dlg);

	if (dlg.m_bConfirmed)
	{
		//  Save Replace parameters for 'F3' command
		m_bLastReplace = TRUE;
		m_strLastFindWhat = dlg.m_sText;
		m_dwLastReplaceFlags = 0;
		if (dlg.m_bMatchCase)
			m_dwLastReplaceFlags |= FIND_MATCH_CASE;
		if (dlg.m_bWholeWord)
			m_dwLastReplaceFlags |= FIND_WHOLE_WORD;
		if (dlg.m_bRegExp)
			m_dwLastReplaceFlags |= FIND_REGEXP;
		if (dlg.m_bNoWrap)
			m_dwLastReplaceFlags |= FIND_NO_WRAP;
		//  Save search parameters to registry
		SettingStore.WriteProfileInt(_T("Editor"), _T("ReplaceFlags"), m_dwLastReplaceFlags);
	}
	//  Restore selection
	if (m_bSelectionPushed)
	{
		SetSelection (m_ptSavedSelStart, m_ptSavedSelEnd);
		m_bSelectionPushed = FALSE;
	}
}

/**
 * @brief Replace selected text.
 * This function replaces selected text in the editor pane with given text.
 * @param [in] pszNewText The text replacing selected text.
 * @param [in] cchNewText Length of the replacing text.
 * @param [in] dwFlags Additional modifier flags:
 * - FIND_REGEXP: use the regular expression.
 * @return TRUE if succeeded.
 */
void CCrystalEditView::ReplaceSelection(LPCTSTR pszNewText, int cchNewText, DWORD dwFlags)
{
	if (!cchNewText)
	{
		DeleteCurrentSelection();
		return;
	}

	ASSERT(pszNewText);

	m_pTextBuffer->BeginUndoGroup();

	POINT ptCursorPos, ptEndOfBlock;
	GetSelection(ptCursorPos, ptEndOfBlock);

	// Avoid bogus EOL insertion when selection ends on a ghost line
	int nLastLine = m_pTextBuffer->GetLineCount() - 1;
	while (ptEndOfBlock.y < nLastLine &&
		m_pTextBuffer->GetFullLineLength(ptEndOfBlock.y) == 0)
	{
		++ptEndOfBlock.y;
	}

	if (IsSelection())
		m_pTextBuffer->DeleteText(this, ptCursorPos.y, ptCursorPos.x, ptEndOfBlock.y, ptEndOfBlock.x, CE_ACTION_REPLACE);

	ASSERT_VALIDTEXTPOS(ptCursorPos);

	ptEndOfBlock = m_pTextBuffer->InsertText(this, ptCursorPos.y, ptCursorPos.x, pszNewText, cchNewText, CE_ACTION_REPLACE);  //  [JRT]
	m_nLastReplaceLen = cchNewText;

	ASSERT_VALIDTEXTPOS(ptCursorPos);
	ASSERT_VALIDTEXTPOS(ptEndOfBlock);
	SetAnchor(ptEndOfBlock);
	SetSelection(ptCursorPos, ptEndOfBlock);
	SetCursorPos(ptEndOfBlock);

	m_pTextBuffer->FlushUndoGroup(this);
}

bool CCrystalEditView::DoEditUndo()
{
	if (m_pTextBuffer != NULL && m_pTextBuffer->CanUndo())
	{
		POINT ptCursorPos;
		if (m_pTextBuffer->Undo(this, ptCursorPos))
		{
			ASSERT_VALIDTEXTPOS(ptCursorPos);
			SetAnchor(ptCursorPos);
			SetSelection(ptCursorPos, ptCursorPos);
			SetCursorPos(ptCursorPos);
			EnsureCursorVisible();
			return true;
		}
	}
	return false;
}

bool CCrystalEditView::DoEditRedo()
{
	if (m_pTextBuffer != NULL && m_pTextBuffer->CanRedo())
	{
		POINT ptCursorPos;
		if (m_pTextBuffer->Redo(this, ptCursorPos))
		{
			ASSERT_VALIDTEXTPOS(ptCursorPos);
			SetAnchor(ptCursorPos);
			SetSelection(ptCursorPos, ptCursorPos);
			SetCursorPos(ptCursorPos);
			EnsureCursorVisible();
			return true;
		}
	}
	return false;
}

static bool isopenbrace(TCHAR c)
{
	return c == _T('{') || c == _T('(') || c == _T('[') || c == _T('<');
}

static bool isclosebrace(TCHAR c)
{
	return c == _T('}') || c == _T(')') || c == _T(']') || c == _T('>');
}

static bool isopenbrace(LPCTSTR s)
{
	return s[1] == _T('\0') && isopenbrace(*s);
}

static bool isclosebrace(LPCTSTR s)
{
	return s[1] == _T('\0') && isclosebrace(*s);
}

int bracetype(TCHAR c)
{
	static const TCHAR braces[] = _T("{}()[]<>");
	LPCTSTR pos = _tcschr(braces, c);
	return pos ? static_cast<int>(pos - braces) + 1 : 0;
}

static int bracetype(LPCTSTR s)
{
	if (s[1])
		return 0;
	return bracetype(*s);
}

void CCrystalEditView::OnEditOperation(int nAction, LPCTSTR pszText)
{
	if (m_bAutoIndent)
	{
		//  Analyse last action...
		if (nAction == CE_ACTION_TYPING && _tcscmp(pszText, _T("\r\n")) == 0 && !m_bOvrMode)
		{
			//  Enter stroke!
			POINT ptCursorPos = GetCursorPos();
			ASSERT(ptCursorPos.y > 0);

			//  Take indentation from the previos line
			int nLength = m_pTextBuffer->GetLineLength(ptCursorPos.y - 1);
			LPCTSTR pszLineChars = m_pTextBuffer->GetLineChars(ptCursorPos.y - 1);
			int nPos = 0;
			while (nPos < nLength && xisspace(pszLineChars[nPos]))
				nPos++;

			if (nPos > 0)
			{
				if ((GetFlags() & SRCOPT_BRACEGNU) && isclosebrace(pszLineChars[nLength - 1]) && ptCursorPos.y > 0 && nPos && nPos == nLength - 1)
				{
					if (pszLineChars[nPos - 1] == _T('\t'))
					{
						nPos--;
					}
					else
					{
						int nTabSize = GetTabSize (),
						nDelta = nTabSize - nPos % nTabSize;
						if (!nDelta)
						{
							nDelta = nTabSize;
						}
						nPos -= nDelta;
						if (nPos < 0)
						{
							nPos = 0;
						}
					}
				}
				//  Insert part of the previos line
				TCHAR *pszInsertStr;
				if ((GetFlags() & (SRCOPT_BRACEGNU|SRCOPT_BRACEANSI)) && isopenbrace(pszLineChars[nLength - 1]))
				{
					if (m_pTextBuffer->GetInsertTabs())
					{
						pszInsertStr = (TCHAR *) _alloca (sizeof (TCHAR) * (nPos + 2));
						_tcsncpy (pszInsertStr, pszLineChars, nPos);
						pszInsertStr[nPos++] = _T('\t');
					}
					else
					{
						int nTabSize = GetTabSize ();
						int nChars = nTabSize - nPos % nTabSize;
						pszInsertStr = (TCHAR *) _alloca (sizeof (TCHAR) * (nPos + nChars + 1));
						_tcsncpy (pszInsertStr, pszLineChars, nPos);
						while (nChars--)
						{
							pszInsertStr[nPos++] = _T(' ');
						}
					}
				}
				else
				{
					pszInsertStr = (TCHAR *) _alloca (sizeof (TCHAR) * (nPos + 1));
					_tcsncpy (pszInsertStr, pszLineChars, nPos);
				}
				pszInsertStr[nPos] = 0;

				// m_pTextBuffer->BeginUndoGroup ();
				POINT pt = m_pTextBuffer->InsertText(NULL, ptCursorPos.y, ptCursorPos.x,
						pszInsertStr, nPos, CE_ACTION_AUTOINDENT);
				SetCursorPos(pt);
				SetSelection(pt, pt);
				SetAnchor(pt);
				EnsureCursorVisible();
				// m_pTextBuffer->FlushUndoGroup (this);
            }
			else
			{
				//  Insert part of the previos line
				TCHAR *pszInsertStr;
				if ((GetFlags() & (SRCOPT_BRACEGNU|SRCOPT_BRACEANSI)) && isopenbrace(pszLineChars[nLength - 1]))
				{
					if (m_pTextBuffer->GetInsertTabs())
					{
						pszInsertStr = (TCHAR *) _alloca (sizeof (TCHAR) * 2);
						pszInsertStr[nPos++] = _T('\t');
					}
					else
					{
						int nTabSize = GetTabSize ();
						int nChars = nTabSize - nPos % nTabSize;
						pszInsertStr = (TCHAR *) _alloca (sizeof (TCHAR) * (nChars + 1));
						while (nChars--)
						{
							pszInsertStr[nPos++] = _T(' ');
						}
					}
					pszInsertStr[nPos] = 0;

					// m_pTextBuffer->BeginUndoGroup ();
					POINT pt = m_pTextBuffer->InsertText(NULL, ptCursorPos.y, ptCursorPos.x,
						pszInsertStr, nPos, CE_ACTION_AUTOINDENT);
					SetCursorPos(pt);
					SetSelection(pt, pt);
					SetAnchor(pt);
					EnsureCursorVisible();
					// m_pTextBuffer->FlushUndoGroup (this);
				}
			}
		}
		else if (nAction == CE_ACTION_TYPING && (GetFlags() & SRCOPT_FNBRACE) && bracetype (pszText) == 3)
		{
			//  Enter stroke!
			POINT ptCursorPos = GetCursorPos();
			LPCTSTR pszChars = m_pTextBuffer->GetLineChars(ptCursorPos.y);
			if (ptCursorPos.x > 1 && xisalnum(pszChars[ptCursorPos.x - 2]))
			{
				LPTSTR pszInsertStr = (TCHAR *) _alloca (sizeof (TCHAR) * 2);
				*pszInsertStr = _T(' ');
				pszInsertStr[1] = _T('\0');
				// m_pTextBuffer->BeginUndoGroup ();
				ptCursorPos = m_pTextBuffer->InsertText(NULL, ptCursorPos.y, ptCursorPos.x - 1,
					pszInsertStr, 1, CE_ACTION_AUTOINDENT);
				++ptCursorPos.x;
				SetCursorPos(ptCursorPos);
				SetSelection(ptCursorPos, ptCursorPos);
				SetAnchor(ptCursorPos);
				EnsureCursorVisible();
				// m_pTextBuffer->FlushUndoGroup (this);
			}
		}
		else if (nAction == CE_ACTION_TYPING && (GetFlags() & SRCOPT_BRACEGNU) && isopenbrace(pszText))
		{
			//  Enter stroke!
			POINT ptCursorPos = GetCursorPos ();

			//  Take indentation from the previos line
			int nLength = m_pTextBuffer->GetLineLength (ptCursorPos.y);
			LPCTSTR pszLineChars = m_pTextBuffer->GetLineChars (ptCursorPos.y );
			int nPos = 0;
			while (nPos < nLength && xisspace (pszLineChars[nPos]))
				nPos++;
			if (nPos == nLength - 1)
			{
				TCHAR *pszInsertStr;
				if (m_pTextBuffer->GetInsertTabs())
				{
				  pszInsertStr = (TCHAR *) _alloca (sizeof (TCHAR) * 2);
				  *pszInsertStr = _T('\t');
				  nPos = 1;
				}
				else
				{
				  int nTabSize = GetTabSize ();
				  int nChars = nTabSize - nPos % nTabSize;
				  pszInsertStr = (TCHAR *) _alloca (sizeof (TCHAR) * (nChars + 1));
				  nPos = 0;
				  while (nChars--)
					{
					  pszInsertStr[nPos++] = _T(' ');
					}
				}
				pszInsertStr[nPos] = 0;

				// m_pTextBuffer->BeginUndoGroup ();
				POINT pt = m_pTextBuffer->InsertText(NULL, ptCursorPos.y, ptCursorPos.x - 1,
										 pszInsertStr, nPos, CE_ACTION_AUTOINDENT);
				++pt.x;
				SetCursorPos(pt);
				SetSelection(pt, pt);
				SetAnchor(pt);
				EnsureCursorVisible();
				// m_pTextBuffer->FlushUndoGroup (this);
			}
		}
		else if (nAction == CE_ACTION_TYPING && (GetFlags() & (SRCOPT_BRACEGNU|SRCOPT_BRACEANSI)) && isclosebrace(pszText))
		{
			//  Enter stroke!
			POINT ptCursorPos = GetCursorPos();

			//  Take indentation from the previos line
			int nLength = m_pTextBuffer->GetLineLength (ptCursorPos.y);
			LPCTSTR pszLineChars = m_pTextBuffer->GetLineChars (ptCursorPos.y );
			int nPos = 0;
			while (nPos < nLength && xisspace (pszLineChars[nPos]))
			nPos++;
			if (ptCursorPos.y > 0 && nPos && nPos == nLength - 1)
			{
				if (pszLineChars[nPos - 1] == _T('\t'))
				{
					nPos = 1;
				}
				else
				{
					int nTabSize = GetTabSize ();
					nPos = nTabSize - (ptCursorPos.x - 1) % nTabSize;
					if (!nPos)
					{
						nPos = nTabSize;
					}
					if (nPos > nLength - 1)
					{
						nPos = nLength - 1;
					}
				}
				// m_pTextBuffer->BeginUndoGroup ();
				m_pTextBuffer->DeleteText(NULL, ptCursorPos.y, ptCursorPos.x - nPos - 1,
				ptCursorPos.y, ptCursorPos.x - 1, CE_ACTION_AUTOINDENT);
				ptCursorPos.x -= nPos;
				SetCursorPos(ptCursorPos);
				SetSelection(ptCursorPos, ptCursorPos);
				SetAnchor(ptCursorPos);
				EnsureCursorVisible();
				// m_pTextBuffer->FlushUndoGroup (this);
			}
		}
	}
}

void CCrystalEditView::OnEditLowerCase()
{
	if (!QueryEditable())
		return;

	if (IsSelection())
	{
		POINT ptSelStart, ptSelEnd;
		GetSelection(ptSelStart, ptSelEnd);
		String text;
		GetText(ptSelStart, ptSelEnd, text);
		CharLower(&text.front());
		m_pTextBuffer->BeginUndoGroup();
		// [JRT]:
		m_pTextBuffer->DeleteText(this, ptSelStart.y, ptSelStart.x, ptSelEnd.y, ptSelEnd.x, CE_ACTION_LOWERCASE);
		m_pTextBuffer->InsertText(this, ptSelStart.y, ptSelStart.x, text.c_str(), text.size(), CE_ACTION_LOWERCASE);
		ASSERT_VALIDTEXTPOS(ptSelStart);
		SetAnchor(ptSelStart);
		SetSelection(ptSelStart, ptSelEnd);
		SetCursorPos(ptSelStart);
		EnsureCursorVisible();
		m_pTextBuffer->FlushUndoGroup(this);
	}
}

void CCrystalEditView::OnEditUpperCase()
{
	if (!QueryEditable())
		return;

	if (IsSelection())
	{
		POINT ptSelStart, ptSelEnd;
		GetSelection(ptSelStart, ptSelEnd);
		String text;
		GetText(ptSelStart, ptSelEnd, text);
		CharUpper(&text.front());
		m_pTextBuffer->BeginUndoGroup();
		// [JRT]:
		m_pTextBuffer->DeleteText(this, ptSelStart.y, ptSelStart.x, ptSelEnd.y, ptSelEnd.x, CE_ACTION_UPPERCASE);
		m_pTextBuffer->InsertText(this, ptSelStart.y, ptSelStart.x, text.c_str(), text.size(), CE_ACTION_UPPERCASE);
		ASSERT_VALIDTEXTPOS(ptSelStart);
		SetAnchor(ptSelStart);
		SetSelection(ptSelStart, ptSelEnd);
		SetCursorPos(ptSelStart);
		EnsureCursorVisible();
		m_pTextBuffer->FlushUndoGroup(this);
	}
}

void CCrystalEditView::OnEditSwapCase()
{
	if (!QueryEditable())
		return;

	if (IsSelection())
	{
		POINT ptSelStart, ptSelEnd;
		GetSelection(ptSelStart, ptSelEnd);
		String text;
		GetText(ptSelStart, ptSelEnd, text);
		LPTSTR pszText = &text.front();
		while (*pszText)
			*pszText++ ^= static_cast<TCHAR>(_totupper(*pszText)) ^ static_cast<TCHAR>(_totlower(*pszText));
		m_pTextBuffer->BeginUndoGroup();
		// [JRT]:
		m_pTextBuffer->DeleteText(this, ptSelStart.y, ptSelStart.x, ptSelEnd.y, ptSelEnd.x, CE_ACTION_SWAPCASE);
		m_pTextBuffer->InsertText(this, ptSelStart.y, ptSelStart.x, text.c_str(), text.size(), CE_ACTION_SWAPCASE);
		ASSERT_VALIDTEXTPOS(ptSelStart);
		SetAnchor(ptSelStart);
		SetSelection(ptSelStart, ptSelEnd);
		SetCursorPos(ptSelStart);
		EnsureCursorVisible();
		m_pTextBuffer->FlushUndoGroup(this);
	}
}

void CCrystalEditView::OnEditCapitalize()
{
	if (!QueryEditable())
		return;

	if (IsSelection())
	{
		POINT ptSelStart, ptSelEnd;
		GetSelection(ptSelStart, ptSelEnd);
		String text;
		GetText(ptSelStart, ptSelEnd, text);
		LPTSTR pszText = &text.front();
		bool bCapitalize = true;
		while (*pszText)
		{
			if (xisspace(*pszText))
				bCapitalize = true;
			else if (_istalpha(*pszText))
				if (bCapitalize)
				{
					*pszText = (TCHAR)_totupper(*pszText);
					bCapitalize = false;
				}
				else
					*pszText = (TCHAR)_totlower(*pszText);
			pszText++;
		}

		m_pTextBuffer->BeginUndoGroup();
		// [JRT]:
		m_pTextBuffer->DeleteText(this, ptSelStart.y, ptSelStart.x, ptSelEnd.y, ptSelEnd.x, CE_ACTION_CAPITALIZE);
		m_pTextBuffer->InsertText(this, ptSelStart.y, ptSelStart.x, text.c_str(), text.size(), CE_ACTION_CAPITALIZE);
		ASSERT_VALIDTEXTPOS(ptSelStart);
		SetAnchor(ptSelStart);
		SetSelection(ptSelStart, ptSelEnd);
		SetCursorPos(ptSelStart);
		EnsureCursorVisible();
		m_pTextBuffer->FlushUndoGroup(this);
	}
}

void CCrystalEditView::OnEditSentence()
{
	if (!QueryEditable())
		return;

	if (IsSelection())
	{
		POINT ptSelStart, ptSelEnd;
		GetSelection(ptSelStart, ptSelEnd);
		String text;
		GetText(ptSelStart, ptSelEnd, text);
		LPTSTR pszText = &text.front();
		bool bCapitalize = true;
		while (*pszText)
		{
			if (!xisspace(*pszText))
				if (*pszText == _T('.'))
				{
					if (pszText[1] && !_istdigit(pszText[1]))
						bCapitalize = true;
				}
				else if (_istalpha(*pszText))
					if (bCapitalize)
					{
						*pszText = (TCHAR)_totupper(*pszText);
						bCapitalize = false;
					}
					else
						*pszText = (TCHAR)_totlower(*pszText);
			pszText++;
		}

		m_pTextBuffer->BeginUndoGroup();

		// [JRT]:
		m_pTextBuffer->DeleteText(this, ptSelStart.y, ptSelStart.x, ptSelEnd.y, ptSelEnd.x, CE_ACTION_SENTENCIZE);
		m_pTextBuffer->InsertText(this, ptSelStart.y, ptSelStart.x, text.c_str(), text.size(), CE_ACTION_SENTENCIZE);
		ASSERT_VALIDTEXTPOS(ptSelStart);
		SetAnchor(ptSelStart);
		SetSelection(ptSelStart, ptSelEnd);
		SetCursorPos(ptSelStart);
		EnsureCursorVisible();
		m_pTextBuffer->FlushUndoGroup(this);
	}
}

//BEGIN SW
void CCrystalEditView::OnEditGotoLastChange()
{
	if (!QueryEditable())
		return;

	POINT ptLastChange = m_pTextBuffer->GetLastChangePos();
	if( ptLastChange.x < 0 || ptLastChange.y < 0 )
		return;

	// goto last change
	SetCursorPos(ptLastChange);
	SetSelection(ptLastChange, ptLastChange);
	EnsureCursorVisible();
}
//END SW

void CCrystalEditView::OnEditDeleteWord()
{
	if (!QueryEditable())
		return;

	if (IsSelection())
	{
		POINT ptSelStart, ptSelEnd;
		GetSelection(ptSelStart, ptSelEnd);
		m_pTextBuffer->BeginUndoGroup();
		m_pTextBuffer->DeleteText(this, ptSelStart.y, ptSelStart.x, ptSelEnd.y, ptSelEnd.x, CE_ACTION_DELETE);
		ASSERT_VALIDTEXTPOS(ptSelStart);
		SetAnchor(ptSelStart);
		SetSelection(ptSelStart, ptSelStart);
		SetCursorPos(ptSelStart);
		EnsureCursorVisible();
		m_pTextBuffer->FlushUndoGroup(this);
	}
	else
	{
		MoveWordRight(TRUE);
	}
}

void CCrystalEditView::OnEditDeleteWordBack()
{
	if (!QueryEditable())
		return;

	if (IsSelection())
	{
		POINT ptSelStart, ptSelEnd;
		GetSelection(ptSelStart, ptSelEnd);
		m_pTextBuffer->BeginUndoGroup();
		m_pTextBuffer->DeleteText(this, ptSelStart.y, ptSelStart.x, ptSelEnd.y, ptSelEnd.x, CE_ACTION_DELETE);
		ASSERT_VALIDTEXTPOS(ptSelStart);
		SetAnchor(ptSelStart);
		SetSelection(ptSelStart, ptSelStart);
		SetCursorPos(ptSelStart);
		EnsureCursorVisible();
		m_pTextBuffer->FlushUndoGroup(this);
	}
	else
	{
		MoveWordLeft(TRUE);
	}
}

void CCrystalEditView::OnEditRightToLeft()
{
	POINT ptStart, ptEnd;
	GetSelection(ptStart, ptEnd);
	// The selection must not include line endings
	if (ptStart.y != ptEnd.y)
		return;
	String text;
	GetText(ptStart, ptEnd, text);
	ptStart = TextToClient(ptStart);
	ClientToScreen(&ptStart);
	HEdit *const pEdit = HEdit::Create(WS_POPUP | WS_BORDER | ES_AUTOHSCROLL,
		0, 0, 16000, 16000, NULL, 0, text.c_str(), WS_EX_TOOLWINDOW);
	if (!QueryEditable())
		pEdit->SetReadOnly(TRUE);
	HFont *const pFont = GetFont();
	pEdit->SetFont(pFont);
	RECT rcInner;
	pEdit->GetRect(&rcInner);
	ptStart.x -= rcInner.left;
	ptStart.y -= rcInner.top;
	if (ptStart.x < 0)
		ptStart.x = 0;
	pEdit->SetFocus();
	pEdit->SetSel(0, text.length());
	RECT rcDesktop;
	H2O::GetDesktopWorkArea(m_hWnd, &rcDesktop);
	BOOL bEffective = FALSE;
	BOOL bModified = TRUE;
	do
	{
		if (bModified)
		{
			pEdit->UpdateWindow();
			pEdit->GetWindowText(text);
			if (HSurface *const pDC = pEdit->GetDC())
			{
				pDC->SelectObject(pFont);
				DWORD dim = pDC->GetTabbedTextExtent(text.c_str(), text.length());
				SIZE size = { LOWORD(dim) + 2 * rcInner.left, GetLineHeight() + 2 * rcInner.top };
				int limit = rcDesktop.right - rcDesktop.left;
				if (size.cx > limit)
					size.cx = limit;
				int left = ptStart.x;
				if (left < rcDesktop.left)
					left = rcDesktop.left;
				limit = rcDesktop.right - size.cx;
				if (left > limit)
					left = limit;
				pEdit->SetWindowPos(NULL, left, ptStart.y, size.cx, size.cy, SWP_SHOWWINDOW);
				// Dynamic resizing interferes with the edit control's auto-scroll facility.
				// Trick it into scrolling appropriately by moving the caret back and forth.
				int nStart, nEnd;
				pEdit->GetSel(nStart, nEnd);
				if (nStart == nEnd)
				{
					nEnd = pEdit->GetWindowTextLength();
					pEdit->SetRedraw(FALSE);
					pEdit->SetSel(0, 0);
					pEdit->SetSel(nEnd, nEnd);
					pEdit->SetSel(nStart, nStart);
					pEdit->SetRedraw(TRUE);
					pEdit->Invalidate();
				}
				pEdit->ReleaseDC(pDC);
			}
			pEdit->SetModify(FALSE);
		}
		MSG msg;
		if (!GetMessage(&msg, NULL, 0, 0))
			break;
		if (msg.message == WM_KEYDOWN)
		{
			if (msg.wParam == VK_ESCAPE)
				break;
			if (msg.wParam == VK_RETURN)
			{
				if (bEffective)
					ReplaceSelection(text.c_str(), text.length(), 0);
				break;
			}
		}
		TranslateMessage(&msg);
		if (msg.message == WM_CHAR && msg.wParam == VK_TAB)
		{
			pEdit->ReplaceSel(reinterpret_cast<LPCTSTR>(&msg.wParam), TRUE);
		}
		else
		{
			DispatchMessage(&msg);
		}
		bModified = pEdit->GetModify();
		if (bModified)
			bEffective = TRUE;
	} while (HWindow::GetFocus() == pEdit);
	pEdit->DestroyWindow();
}
