////////////////////////////////////////////////////////////////////////////
//  File:       ccrystaltextview.cpp
//  Version:    1.2.0.5
//  Created:    29-Dec-1998
//
//  Author:     Stcherbatchenko Andrei
//  E-mail:     windfall@gmx.de
//
//  Implementation of the CCrystalTextView class, a part of Crystal Edit -
//  syntax coloring text editor.
//
//  You are free to use or modify this code to the following restrictions:
//  - Acknowledge me somewhere in your about box, simple "Parts of code by.."
//  will be enough. If you can't (or don't want to), contact me personally.
//  - LEAVE THIS HEADER INTACT
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//  17-Feb-99
//  FIX:    missing UpdateCaret() in CCrystalTextView::SetFont
//  FIX:    missing UpdateCaret() in CCrystalTextView::RecalcVertScrollBar
//  FIX:    mistype in CCrystalTextView::RecalcPageLayouts + instead of +=
//  FIX:    removed condition 'm_nLineHeight < 20' in
//      CCrystalTextView::CalcLineCharDim(). This caused painting defects
//      when using very small fonts.
//
//  FEATURE:    Some experiments with smooth scrolling, controlled by
//      m_bSmoothScroll member variable, by default turned off.
//      See ScrollToLine function for implementation details.
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//  21-Feb-99
//      Paul Selormey, James R. Twine
//  +   FEATURE: description for Undo/Redo actions
//  +   FEATURE: multiple MSVC-like bookmarks
//  +   FEATURE: 'Disable backspace at beginning of line' option
//  +   FEATURE: 'Disable drag-n-drop editing' option
//
//  +   FIX:  ResetView() now virtual
//  +   FEATURE: Added OnEditOperation() virtual: base for auto-indent,
//      smart indent etc.
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//  19-Jul-99
//      Ferdinand Prantl:
//  +   FEATURE: regular expressions, go to line and things ...
//  +   FEATURE: plenty of syntax highlighting definitions
//  +   FEATURE: corrected bug in syntax highlighting C comments
//  +   FEATURE: extended registry support for saving settings
//  +   FEATURE: some other things I've forgotten ...
//
//  ... it's being edited very rapidly so sorry for non-commented
//        and maybe "ugly" code ...
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//	01-Jun-99 to 31-Aug-99
//		Sven Wiegand (search for "//BEGIN SW" to find my changes):
//
//	+ FEATURE: support for language switching on the fly with class 
//			CCrystalParser
//	+	FEATURE: word wrapping
//	+ FIX:	Setting m_nIdealCharPos, when choosing cursor position by mouse
//	+ FIX:	Backward search
//	+ FEATURE: incremental search
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//	24-Oct-99
//		Sven Wiegand
//
//	+ FIX: Opening large files won't crash anymore and will go very fast
//	       (removed call to RecalcVertScrollBar() in WrapLineCached())
//	+ FIX: Problems with repainting and cursor-position by resizing window 
//	       fixed by adding call to ScrollToSubLine() in OnSize().
//	+ FEATURE: Supporting [Return] to exit incremental-search-mode
//		     (see OnChar())
///////////////////////////////////////////////////////////////////////////////

/**
 * @file  ccrystaltextview.cpp
 *
 * @brief Implementation of the CCrystalTextView class
 */
#include "StdAfx.h"
#include "resource.h"
#include "LanguageSelect.h"
#include <imm.h> /* IME */
#include "editcmd.h"
#include "ccrystaltextview.h"
#include "ccrystaltextbuffer.h"
#include "cfindtextdlg.h"
#include "coretools.h"
#include "ViewableWhitespace.h"
#include "SyntaxColors.h"
#include "SettingStore.h"
#include "string_util.h"
#include "pcre.h"
#include "wcwidth.h"

using std::vector;

// Escaped character constants in range 0x80-0xFF are interpreted in current codepage
// Using C locale gets us direct mapping to Unicode codepoints
#pragma setlocale("C")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/** @brief Width of revision marks. */
const UINT MARGIN_REV_WIDTH = 3;
/** @brief Width of icons printed in the margin. */
const UINT MARGIN_ICON_WIDTH = 12;
/** @brief Height of icons printed in the margin. */
const UINT MARGIN_ICON_HEIGHT = 12;

/** @brief Color of unsaved line revision mark (dark yellow). */
const COLORREF UNSAVED_REVMARK_CLR = RGB(0xD7, 0xD7, 0x00);
/** @brief Color of saved line revision mark (green). */
const COLORREF SAVED_REVMARK_CLR = RGB(0x00, 0xFF, 0x00);

#define ICON_INDEX_WRAPLINE         15

static void setAtGrow(std::vector<int> &v, stl_size_t index, int value)
{
	if (v.size() <= index)
	{
		v.reserve((index | 0xFFF) + 1);
		v.resize(index + 1);
	}
	v[index] = value;
}

static int upperBound(const std::vector<int> &v)
{
	return v.size() - 1;
}

HImageList *CCrystalTextView::m_pIcons = NULL;
HBrush *CCrystalTextView::m_pHatchBrush = NULL;

void CCrystalTextView::InitSharedResources()
{
	m_pIcons = LanguageSelect.LoadImageList(IDR_MARGIN_ICONS, MARGIN_ICON_WIDTH, 0);
	m_pHatchBrush = HBrush::CreateHatchBrush(HS_DIAGCROSS, 0);
}

void CCrystalTextView::FreeSharedResources()
{
	m_pHatchBrush->DeleteObject();
	m_pIcons->Destroy();
}

int CCrystalTextView::GetScrollPos(UINT nBar, UINT nSBCode)
{
	SCROLLINFO si;
	si.cbSize = sizeof si;
	si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;
	GetScrollInfo(nBar, &si);
	--si.nPage;
	si.nMax -= si.nPage;
	switch (nSBCode)
	{
	case SB_LEFT:             // Scroll to far left.
		si.nPos = si.nMin;
		break;
	case SB_RIGHT:            // Scroll to far right.
		si.nPos = si.nMax;
		break;
	case SB_LINELEFT:         // Scroll left.
		si.nPage = 1;
		// fall through
	case SB_PAGELEFT:         // Scroll one page left.
		si.nPos -= si.nPage;
		if (si.nPos < si.nMin)
			si.nPos = si.nMin;
		break;
	case SB_LINERIGHT:        // Scroll right.
		si.nPage = 1;
		// fall through
	case SB_PAGERIGHT:        // Scroll one page right.
		si.nPos += si.nPage;
		if (si.nPos > si.nMax)
			si.nPos = si.nMax;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		si.nPos = si.nTrackPos;
		break;
	}
	return si.nPos;
}

////////////////////////////////////////////////////////////////////////////
// CCrystalTextView

LRESULT CCrystalTextView::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		OnDestroy();
		break;
	case WM_SIZE:
		OnSize();
		break;
	case WM_SETFOCUS:
		OnSetFocus();
		break;
	case WM_KILLFOCUS:
		OnKillFocus();
		break;
	case WM_SETCURSOR:
		if (OnSetCursor(LOWORD(lParam)))
			return TRUE;
		break;
	case WM_MOUSEMOVE:
		OnMouseMove(lParam);
		break;
	case WM_LBUTTONDOWN:
		OnLButtonDown(lParam);
		break;
	case WM_LBUTTONUP:
		OnLButtonUp(lParam);
		break;
	case WM_LBUTTONDBLCLK:
		OnLButtonDblClk(lParam);
		break;
	case WM_RBUTTONDOWN:
		OnRButtonDown(lParam);
		break;
	case WM_HSCROLL:
		OnHScroll(LOWORD(wParam));
		break;
	case WM_VSCROLL:
		OnVScroll(LOWORD(wParam));
		break;
	case WM_TIMER:
		OnTimer(wParam);
		break;
	case WM_ERASEBKGND:
		return TRUE;
	case WM_PAINT:
		PAINTSTRUCT ps;
		if (HSurface *pdc = BeginPaint(&ps))
		{
			OnDraw(pdc);
			EndPaint(&ps);
		}
		return 0;
	case WM_MOUSEWHEEL:
		if (OnMouseWheel(wParam, lParam))
			return 0;
		break;
	case WM_IME_STARTCOMPOSITION:
		UpdateCompositionWindowFont();
		UpdateCompositionWindowPos();
		break;
	case WM_SYSCOLORCHANGE:
		Invalidate();
		break;
	}
	return OWindow::WindowProc(uMsg, wParam, lParam);
}

// Tabsize is commented out since we have only GUI setting for it now.
// Not removed because we may later want to have per-filetype settings again.
// See ccrystaltextview.h for table declaration.
CCrystalTextView::TextDefinition CCrystalTextView::m_StaticSourceDefs[] =
{
	SRC_PLAIN, _T ("Plain"), _T ("txt;doc;diz"), &ParseLinePlain, SRCOPT_AUTOINDENT, /*4,*/ _T (""), _T (""), _T (""),
	SRC_ASP, _T ("ASP"), _T ("asp"), &ParseLineAsp, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T (""), _T (""), _T ("'"),
	SRC_BASIC, _T ("Basic"), _T ("bas;vb;vbs;frm;dsm;cls;ctl;pag;dsr"), &ParseLineBasic, SRCOPT_AUTOINDENT, /*4,*/ _T (""), _T (""), _T ("\'"),
	SRC_BATCH, _T ("Batch"), _T ("bat;btm;cmd"), &ParseLineBatch, SRCOPT_INSERTTABS|SRCOPT_AUTOINDENT, /*4,*/ _T (""), _T (""), _T ("rem "),
	SRC_C, _T ("C"), _T ("c;cc;cpp;cxx;h;hpp;hxx;hm;inl;rh;tlh;tli;xs"), &ParseLineC, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T ("/*"), _T ("*/"), _T ("//"),
	SRC_CSHARP, _T ("C#"), _T ("cs"), &ParseLineCSharp, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T ("/*"), _T ("*/"), _T ("//"),
	SRC_CSS, _T ("CSS"), _T ("css"), &ParseLineCss, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T ("/*"), _T ("*/"), _T (""),
	SRC_DCL, _T ("DCL"), _T ("dcl;dcc"), &ParseLineDcl, SRCOPT_AUTOINDENT|SRCOPT_BRACEGNU, /*2,*/ _T ("/*"), _T ("*/"), _T ("//"),
	SRC_FORTRAN, _T ("Fortran"), _T ("f;f90;f9p;fpp;for;f77"), &ParseLineFortran, SRCOPT_INSERTTABS|SRCOPT_AUTOINDENT, /*8,*/ _T (""), _T (""), _T ("!"),
	SRC_HTML, _T ("HTML"), _T ("html;htm;shtml;ihtml;ssi;stm;stml;jsp"), &ParseLineHtml, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T ("<!--"), _T ("-->"), _T (""),
	SRC_INI, _T ("INI"), _T ("ini;reg;vbp;isl"), &ParseLineIni, SRCOPT_AUTOINDENT|SRCOPT_BRACEGNU|SRCOPT_EOLNUNIX, /*2,*/ _T (""), _T (""), _T (";"),
	SRC_INNOSETUP, _T ("InnoSetup"), _T ("iss"), &ParseLineInnoSetup, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T ("{"), _T ("}"), _T (";"),
	SRC_INSTALLSHIELD, _T ("InstallShield"), _T ("rul"), &ParseLineIS, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T ("/*"), _T ("*/"), _T ("//"),
	SRC_JAVA, _T ("Java"), _T ("java;jav;js"), &ParseLineJava, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T ("/*"), _T ("*/"), _T ("//"),
	SRC_LISP, _T ("AutoLISP"), _T ("lsp;dsl"), &ParseLineLisp, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T (";|"), _T ("|;"), _T (";"),
	SRC_NSIS, _T ("NSIS"), _T ("nsi;nsh"), &ParseLineNsis, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T ("/*"), _T ("*/"), _T (";"),
	SRC_PASCAL, _T ("Pascal"), _T ("pas"), &ParseLinePascal, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T ("{"), _T ("}"), _T (""),
	SRC_PERL, _T ("Perl"), _T ("pl;pm;plx"), &ParseLinePerl, SRCOPT_AUTOINDENT|SRCOPT_EOLNUNIX, /*4,*/ _T (""), _T (""), _T ("#"),
	SRC_PHP, _T ("PHP"), _T ("php;php3;php4;php5;phtml"), &ParseLinePhp, SRCOPT_AUTOINDENT|SRCOPT_BRACEGNU, /*2,*/ _T ("/*"), _T ("*/"), _T ("//"),
	SRC_PO, _T ("PO"), _T ("po;pot"), &ParseLinePo, SRCOPT_AUTOINDENT|SRCOPT_EOLNUNIX, /*4,*/ _T (""), _T (""), _T ("#"),
	SRC_POWERSHELL, _T ("PowerShell"), _T ("ps1"), &ParseLinePowerShell, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T (""), _T (""), _T ("#"),
	SRC_PYTHON, _T ("Python"), _T ("py"), &ParseLinePython, SRCOPT_AUTOINDENT|SRCOPT_BRACEGNU, /*2,*/ _T ("/*"), _T ("*/"), _T ("//"),
	SRC_REXX, _T ("REXX"), _T ("rex;rexx"), &ParseLineRexx, SRCOPT_AUTOINDENT, /*4,*/ _T ("/*"), _T ("*/"), _T ("//"),
	SRC_RSRC, _T ("Resources"), _T ("rc;dlg;r16;r32;rc2"), &ParseLineRsrc, SRCOPT_AUTOINDENT, /*4,*/ _T ("/*"), _T ("*/"), _T ("//"),
	SRC_RUBY, _T ("Ruby"), _T ("rb;rbw;rake;gemspec"), &ParseLineRuby, SRCOPT_AUTOINDENT|SRCOPT_EOLNUNIX, /*4,*/ _T (""), _T (""), _T ("#"),
	SRC_SGML, _T ("Sgml"), _T ("sgml"), &ParseLineSgml, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T ("<!--"), _T ("-->"), _T (""),
	SRC_SH, _T ("Shell"), _T ("sh;conf"), &ParseLineSh, SRCOPT_INSERTTABS|SRCOPT_AUTOINDENT|SRCOPT_EOLNUNIX, /*4,*/ _T (""), _T (""), _T ("#"),
	SRC_SIOD, _T ("SIOD"), _T ("scm"), &ParseLineSiod, SRCOPT_AUTOINDENT|SRCOPT_BRACEGNU, /*2,*/ _T (";|"), _T ("|;"), _T (";"),
	SRC_SQL, _T ("SQL"), _T ("sql"), &ParseLineSql, SRCOPT_AUTOINDENT, /*4,*/ _T ("/*"), _T ("*/"), _T ("//"),
	SRC_TCL, _T ("TCL"), _T ("tcl"), &ParseLineTcl, SRCOPT_AUTOINDENT|SRCOPT_BRACEGNU|SRCOPT_EOLNUNIX, /*2,*/ _T (""), _T (""), _T ("#"),
	SRC_TEX, _T ("TEX"), _T ("tex;sty;clo;ltx;fd;dtx"), &ParseLineTex, SRCOPT_AUTOINDENT, /*4,*/ _T (""), _T (""), _T ("%"),
	SRC_VERILOG, _T ("Verilog"), _T ("v;vh"), &ParseLineVerilog, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T ("/*"), _T ("*/"), _T ("//"),
	SRC_XML, _T ("XML"), _T ("xml"), &ParseLineXml, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T ("<!--"), _T ("-->"), _T ("")
}, *CCrystalTextView::m_SourceDefs = m_StaticSourceDefs;

/////////////////////////////////////////////////////////////////////////////
// CCrystalTextView construction/destruction

void CCrystalTextView::ScanParserAssociations(LPTSTR p)
{
	struct tagTextDefinition *defs = new tagTextDefinition[_countof(m_StaticSourceDefs)];
	memcpy(defs, m_StaticSourceDefs, sizeof m_StaticSourceDefs);
	m_SourceDefs = defs;
	while (TCHAR *q = _tcschr(p, _T('=')))
	{
		const size_t r = _tcslen(q);
		*q++ = _T('\0');
		size_t i;
		for (i = 0 ; i < _countof(m_StaticSourceDefs) ; ++i)
		{
			tagTextDefinition &def = defs[i];
			if (_tcsicmp(def.name, p) == 0)
			{
				def.exts = _tcsdup(q);
				break;
			}
		}
		ASSERT(i < _countof(m_StaticSourceDefs));
		p = q + r;
	}
}

void CCrystalTextView::DumpParserAssociations(LPTSTR p)
{
	TextDefinition *def = m_SourceDefs;
	do
	{
		p += wsprintf(p, _T("%-16s= %s"), def->name, def->exts) + 1;
	} while (++def < m_SourceDefs + _countof(m_StaticSourceDefs));
	*p = _T('\0');
}

void CCrystalTextView::FreeParserAssociations()
{
	if (m_SourceDefs != m_StaticSourceDefs)
	{
		size_t i;
		for (i = 0 ; i < _countof(m_StaticSourceDefs) ; ++i)
		{
			TextDefinition &def = m_SourceDefs[i];
			if (def.exts != m_StaticSourceDefs[i].exts)
				free(const_cast<LPTSTR>(def.exts));
		}
		delete[] m_SourceDefs;
		m_SourceDefs = m_StaticSourceDefs;
	}
}

CCrystalTextView::TextDefinition *CCrystalTextView::DoSetTextType(TextDefinition *def)
{
	m_CurSourceDef = def;
	SetFlags(def->flags);
	return def;
}

CCrystalTextView::TextDefinition *CCrystalTextView::GetTextType(LPCTSTR pszExt)
{
	TextDefinition *def = m_SourceDefs;
	do if (PathMatchSpec(pszExt, def->exts))
	{
		return def;
	} while (++def < m_SourceDefs + _countof(m_StaticSourceDefs));
	return NULL;
}

CCrystalTextView::TextDefinition *CCrystalTextView::SetTextType(LPCTSTR pszExt)
{
	m_CurSourceDef = m_SourceDefs;
	TextDefinition *def = GetTextType(pszExt);
	return SetTextType(def);
}

CCrystalTextView::TextDefinition *CCrystalTextView::SetTextType(TextType enuType)
{
	m_CurSourceDef = m_SourceDefs;
	TextDefinition *def = m_SourceDefs;
	do if (def->type == enuType)
	{
		return SetTextType(def);
	} while (++def < m_SourceDefs + _countof(m_StaticSourceDefs));
	return NULL;
}

CCrystalTextView::TextDefinition *CCrystalTextView::SetTextType(TextDefinition *def)
{
	if (def != NULL && m_CurSourceDef != def)
		def = DoSetTextType(def);
	return def;
}

CCrystalTextView::CCrystalTextView(size_t ZeroInit)
: H2O::ZeroInit<CCrystalTextView>(ZeroInit)
, m_bSelMargin(true)
, m_nLastLineIndexCalculatedSubLineIndex(-1)
{
	m_bAutoDelete = true;
	ResetView();
	SetTextType(SRC_PLAIN);
	// font
	_tcscpy(m_lfBaseFont.lfFaceName, _T("FixedSys"));
	m_lfBaseFont.lfWeight = FW_NORMAL;
}

CCrystalTextView::~CCrystalTextView()
{
	ASSERT(m_pTextBuffer == NULL);   //  Must be correctly detached
}

HRESULT CCrystalTextView::QueryInterface(REFIID iid, void **ppv)
{
    static const QITAB rgqit[] = 
    {   
        QITABENT(CCrystalTextView, IDropSource),
        QITABENT(CCrystalTextView, IDataObject),
        { 0 }
    };
    return QISearch(this, rgqit, iid, ppv);
}

ULONG CCrystalTextView::AddRef()
{
	return 1;
}

ULONG CCrystalTextView::Release()
{
	return 1;
}

HRESULT CCrystalTextView::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
	if (fEscapePressed)
		return DRAGDROP_S_CANCEL;
	if (!(grfKeyState & MK_LBUTTON))
		return DRAGDROP_S_DROP;
	return S_OK;
}

HRESULT CCrystalTextView::GiveFeedback(DWORD dwEffect)
{
	return DRAGDROP_S_USEDEFAULTCURSORS;
}

HRESULT CCrystalTextView::GetData(LPFORMATETC etc, LPSTGMEDIUM stg)
{
	if (etc->cfFormat != CF_UNICODETEXT)
		return DV_E_FORMATETC;
	stg->tymed = TYMED_HGLOBAL;
	stg->hGlobal = PrepareDragData();
	stg->pUnkForRelease = NULL;
	return S_OK;
}

HRESULT CCrystalTextView::GetDataHere(LPFORMATETC, LPSTGMEDIUM)
{
	return E_NOTIMPL;
}

HRESULT CCrystalTextView::QueryGetData(LPFORMATETC etc)
{
	if (etc->cfFormat != CF_UNICODETEXT)
		return DV_E_FORMATETC;
	return S_OK;
}

HRESULT CCrystalTextView::GetCanonicalFormatEtc(LPFORMATETC, LPFORMATETC)
{
	return E_NOTIMPL;
}

HRESULT CCrystalTextView::SetData(LPFORMATETC, LPSTGMEDIUM, BOOL)
{
	return E_NOTIMPL;
}

HRESULT CCrystalTextView::EnumFormatEtc(DWORD, LPENUMFORMATETC*)
{
	return E_NOTIMPL;
}

HRESULT CCrystalTextView::DAdvise(LPFORMATETC, DWORD, LPADVISESINK, LPDWORD)
{
	return E_NOTIMPL;
}

HRESULT CCrystalTextView::DUnadvise(DWORD)
{
	return E_NOTIMPL;
}

HRESULT CCrystalTextView::EnumDAdvise(LPENUMSTATDATA*)
{
	return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////////////////////
// CCrystalTextView drawing

void CCrystalTextView::GetSelection(POINT &ptStart, POINT &ptEnd)
{
	PrepareSelBounds();
	ptStart = m_ptDrawSelStart;
	ptEnd = m_ptDrawSelEnd;
}

void CCrystalTextView::GetFullySelectedLines(int &firstLine, int &lastLine)
{
	POINT ptStart;
	POINT ptEnd;
	GetSelection(ptStart, ptEnd);
	if (ptStart.x == 0)
		firstLine = ptStart.y;
	else
		firstLine = ptStart.y + 1;
	if (ptEnd.x == GetLineLength(ptEnd.y))
		lastLine = ptEnd.y;
	else
		lastLine = ptEnd.y - 1;
}

/**
 * @brief : Get the line length, for cursor movement 
 *
 * @note : there are at least 4 line lengths :
 * - number of characters (memory, no EOL)
 * - number of characters (memory, with EOL)
 * - number of characters for cursor position (tabs are expanded, no EOL)
 * - number of displayed characters (tabs are expanded, with EOL)
 * Corresponding functions :
 * - GetLineLength
 * - GetFullLineLength
 * - GetLineActualLength
 * - ExpandChars (returns the line to be displayed as a String)
 */
int CCrystalTextView::GetLineActualLength(int nLineIndex)
{
	const int nLineCount = GetLineCount();
	ASSERT(nLineCount > 0);
	ASSERT(nLineIndex >= 0 && nLineIndex < nLineCount);
	if (!m_pnActualLineLength.size())
	{
		m_pnActualLineLength.assign(nLineCount, -1);
	}

	if (m_pnActualLineLength[nLineIndex] != - 1)
		return m_pnActualLineLength[nLineIndex];

	//  Actual line length is not determined yet, let's calculate a little
	int nActualLength = 0;
	int nLength = GetLineLength(nLineIndex);
	if (nLength > 0)
    {
		LPCTSTR pszChars = GetLineChars(nLineIndex);
		const int nTabSize = GetTabSize();
		int i;
		for (i = 0; i < nLength; i++)
		{
			TCHAR c = pszChars[i];
			if (c == _T('\t'))
				nActualLength += (nTabSize - nActualLength % nTabSize);
			else if (c >= _T('\x00') && c <= _T('\x1F') && c != _T('\r') && c != _T('\n'))
				nActualLength += 3;
			/*else if (c >= UNI_SUR_MIN && c <= UNI_SUR_MAX)
				nActualLength += 5;*/
			else
				nActualLength++;
		}
	}

	m_pnActualLineLength[nLineIndex] = nActualLength;
	return nActualLength;
}

void CCrystalTextView::ScrollToChar(int nNewOffsetChar)
{
	// no horizontal scrolling, when word wrapping is enabled
	if (m_bWordWrap)
		return;

	if (m_nOffsetChar != nNewOffsetChar)
	{
		int nScrollChars = m_nOffsetChar - nNewOffsetChar;
		m_nOffsetChar = nNewOffsetChar;
		RECT rcScroll;
		GetClientRect(&rcScroll);
		rcScroll.left += GetMarginWidth();
		ScrollWindow(nScrollChars * GetCharWidth(), 0, &rcScroll, &rcScroll);
		UpdateWindow();
		RecalcHorzScrollBar(true);
	}
}

/**
 * @brief Scroll view to given line.
 * Scrolls view so that given line is first line in the view. We limit
 * scrolling so that there is only one empty line visible after the last
 * line at max. So we don't allow user to scroll last line being at top or
 * even at middle of the screen. This is how many editors behave and I
 * (Kimmo) think it is good for us too.
 * @param [in] nNewTopSubLine New top line for view.
 */
void CCrystalTextView::ScrollToSubLine(int nNewTopSubLine)
{
	if (m_nTopSubLine != nNewTopSubLine)
	{
		// Limit scrolling so that we show one empty line at end of file
		const int nMaxTopSubLine = GetSubLineCount() - 1;
		if (nNewTopSubLine > nMaxTopSubLine)
			nNewTopSubLine = nMaxTopSubLine;
		if (nNewTopSubLine < 0)
			nNewTopSubLine = 0;
		if (const int nScrollLines = m_nTopSubLine - nNewTopSubLine)
		{
			m_nTopSubLine = nNewTopSubLine;
			// OnDraw() uses m_nTopLine to determine topline
			int dummy;
			GetLineBySubLine(m_nTopSubLine, m_nTopLine, dummy);
			ScrollWindow(0, nScrollLines * GetLineHeight());
			UpdateWindow();
			RecalcVertScrollBar(true);
			int nDummy;
			GetLineBySubLine(m_nTopSubLine, m_nTopLine, nDummy);
		}
	}
}

void CCrystalTextView::ScrollToLine(int nNewTopLine)
{
	if (m_nTopLine != nNewTopLine)
		ScrollToSubLine(GetSubLineIndex(nNewTopLine));
}

int CCrystalTextView::ExpandChars(LPCTSTR pszChars, int nOffset, int nCount, String &line, int nActualOffset)
{
	line.clear();

	if (nCount <= 0)
	{
		return 0;
	}

	const int nTabSize = GetTabSize();

	pszChars += nOffset;
	int nLength = nCount;

	int i;
	for (i = 0; i < nLength; i++)
	{
		TCHAR c = pszChars[i];
		if (c == _T('\t'))
			nCount += nTabSize - 1;
		else if (c >= _T('\x00') && c <= _T('\x1F') && c != _T('\r') && c != _T('\n'))
			nCount += 2;
		/*else if (c >= UNI_SUR_MIN && c <= UNI_SUR_MAX)
			nCount += 4;*/
	}

	// Preallocate line buffer, to avoid reallocations as we add characters
	line.reserve(nCount + 1); // at least this many characters
	int nCurPos = 0;

	if (nCount > nLength || m_bViewTabs || m_bViewEols)
	{
		for (i = 0; i < nLength; i++)
		{
			TCHAR c = pszChars[i];
			if (c == _T('\t'))
			{
				c = m_bViewTabs ? ViewableWhitespaceChars::c_tab : _T(' ');
				int nSpaces = nTabSize - (nActualOffset + nCurPos) % nTabSize;
				do
				{
					line.push_back(c);
					c = _T(' ');
					nCurPos++;
					nSpaces--;
				} while (nSpaces > 0);
			}
			else if (c == _T(' ') && m_bViewTabs)
			{
				line.push_back(ViewableWhitespaceChars::c_space);
				nCurPos++;
			}
			else if (c == _T('\r') || c == _T('\n'))
			{
				if (m_bViewEols)
				{
					if (m_bDistinguishEols)
					{
						line.push_back(pszChars[i] == _T('\r') ?
							ViewableWhitespaceChars::c_cr : ViewableWhitespaceChars::c_lf);
						nCurPos++;
					}
					else if (i + nOffset == 0 || pszChars[i - 1] != _T('\r'))
					{
						line.push_back(ViewableWhitespaceChars::c_eol);
						nCurPos++;
					}
				}
			}
			else if (c >= _T('\x00') && c <= _T('\x1F'))
			{
				line.append_sprintf(_T("\1%02X"), static_cast<int>(c));
				nCurPos += 3;
			}
			/*else if (c >= UNI_SUR_MIN && c <= UNI_SUR_MAX)
			{
				line.append_sprintf(_T("\2%04X"), static_cast<int>(c));
				nCurPos += 5;
			}*/
			else
			{
				line.push_back(c);
				nCurPos += GetCharWidthFromChar(pszChars + i);
			}
		}
	}
	else
	{
		for (i = 0; i < nLength; ++i)
		{
			TCHAR c = pszChars[i];
			line.push_back(c);
			nCurPos += GetCharWidthFromChar(pszChars + i);
		}
	}
	return nCurPos;
}

/**
 * @brief Return width of specified character
 */
int CCrystalTextView::GetCharWidthFromChar(LPCTSTR pch) const
{
	UINT ch = *pch;
	if (ch >= _T('\x00') && ch <= _T('\x1F') && ch != _T('\t') && ch != _T('\r') && ch != _T('\n'))
		return 3;

	if (ch >= UNI_SUR_LOW_START && ch <= UNI_SUR_LOW_END)
		return 0;

	if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END)
	{
		/* If the 16 bits following the high surrogate are in the source buffer... */
		UINT ch2 = *++pch;
		/* If it's a low surrogate, convert to UTF32. */
		if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END)
		{
			ch = ((ch - UNI_SUR_HIGH_START) << 10) + (ch2 - UNI_SUR_LOW_START) + 0x10000;
		}
		else
		{
			ch = '?';
		}
	}
	/*if (ch >= UNI_SUR_MIN && ch <= UNI_SUR_MAX)
		return 5;*/
	// This assumes a fixed width font
	// But the UNICODE case handles double-wide glyphs (primarily Chinese characters)
	int wcwidth = mk_wcwidth(ch);
	if (wcwidth < (m_bSeparateCombinedChars ? 1 : 0))
		wcwidth = 1; // applies to 8-bit control characters in range 0x7F..0x9F
	return wcwidth;
}

/**
 * @brief Return width of displayable version of character
 *
 * Differs from GetCharWidthFromChar when viewable whitespace is involved
 */
int CCrystalTextView::GetCharWidthFromDisplayableChar(LPCTSTR pch) const
{
	if (*pch == _T(' ') && m_bViewTabs)
		pch = &ViewableWhitespaceChars::c_space;
	return GetCharWidthFromChar(pch);
}

/**
 * @brief Draw a chunk of text (one color, one line, full or part of line)
 *
 * @note In ANSI build, this routine is buggy for multibytes or double-width characters
 */
void CCrystalTextView::DrawLineHelperImpl(
	HSurface *pdc, POINT &ptOrigin, const RECT &rcClip,
	int nColorIndex, int nBgColorIndex, COLORREF crText, COLORREF crBkgnd,
	LPCTSTR pszChars, int nOffset, int nCount, int &nActualOffset,
	int nBgColorIndexZeorWidthBlock, int cxZeroWidthBlock)
{
	ASSERT(nCount > 0);
	String line;
	nActualOffset += ExpandChars(pszChars, nOffset, nCount, line, nActualOffset);
	// When not separating combined characters, assume at most 5 characters per
	// combination, and go with the risk of visual glitches when proved wrong.
	const int nMaxCombinings = m_bSeparateCombinedChars ? 1 : 5;
	const int nMaxEscapement = 2;
	// TODO: When implementing fallback to %04X format, set nMaxEscapement to 4.
	const LPCTSTR szLine = line.c_str();
	const int nLength = line.length();
	const int nCharWidth = GetCharWidth();
	const int nCharWidthNarrowed = nCharWidth / 2;
	const int nCharWidthWidened = nCharWidth * 2 - nCharWidthNarrowed;
	const int nLineHeight = GetLineHeight();

	// i the character index, from 0 to lineLen-1
	int i = 0;

	// Pass if the text begins after the right end of the clipping region
	if (ptOrigin.x < rcClip.right)
	{
		// Because ExtTextOut is buggy when ptOrigin.x < - 4095 * charWidth
		// or when nCount >= 4095
		// and because this is not well documented,
		// we decide to do the left & right clipping here

		// Update the position after the left clipped characters
		// stop for i = first visible character, at least partly
		const int clipLeft = rcClip.left - nCharWidth * nMaxEscapement;
		for ( ; i < nLength; ++i)
		{
			int pnWidthsCurrent = szLine[i] < _T('\t') ?
				nCharWidth : nCharWidth * GetCharWidthFromChar(szLine + i);
			ptOrigin.x += pnWidthsCurrent;
			if (ptOrigin.x >= clipLeft)
			{
				ptOrigin.x -= pnWidthsCurrent;
				break;
			}
		}

		if (i < nLength)
		{
			// We have to draw some characters
			int iBegin = i;
			int nSumWidth = 0;

			// A raw estimate of the number of characters to display
			// For wide characters, nCountFit may be overvalued.
			// The + 2 is supposedly meant to account for partially displayed
			// characters both to the left and right side of the visible area.
			int nWidth = rcClip.right - ptOrigin.x;
			int nCount = nLength - iBegin;
			int nCountFit = nMaxCombinings * (nWidth / nCharWidth + 2);
			if (nCount > nCountFit)
				nCount = nCountFit;

			// Table of charwidths as CCrystalEditor thinks they are
			// Seems that CrystalEditor's and ExtTextOut()'s charwidths aren't
			// same with some fonts and text is drawn only partially
			// if this table is not used.
			int *pnWidths = new int[nCount + nMaxEscapement];
			while (i < iBegin + nCount && nSumWidth <= nWidth)
			{
				TCHAR c = szLine[i];
				if (c == _T('\1')) // %02X escape sequence leadin?
				{
					// Substitute a space narrowed to half the width of a character cell.
					line[i] = _T(' ');
					nSumWidth += pnWidths[i - iBegin] = nCharWidthNarrowed;
					// 1st hex digit has normal width.
					nSumWidth += pnWidths[++i - iBegin] = nCharWidth;
					// 2nd hex digit is padded by half the width of a character cell.
					nSumWidth += pnWidths[++i - iBegin] = nCharWidthWidened;
				}
				/*else if (c == _T('\2')) // %04X escape sequence leadin?
				{
					line[i] = _T(' ');
					// Substitute a space narrowed to half the width of a character cell.
					nSumWidth += pnWidths[i - iBegin] = nCharWidthNarrowed;
					// 1st..3rd hex digit has normal width.
					nSumWidth += pnWidths[++i - iBegin] = nCharWidth;
					nSumWidth += pnWidths[++i - iBegin] = nCharWidth;
					nSumWidth += pnWidths[++i - iBegin] = nCharWidth;
					// 4th hex digit is padded by half the width of a character cell.
					nSumWidth += pnWidths[++i - iBegin] = nCharWidthWidened;
				}*/
				else
				{
					nSumWidth += pnWidths[i - iBegin] = nCharWidth * GetCharWidthFromChar(szLine + i);
				}
				++i;
			}
			nCount = i - iBegin;

			if (ptOrigin.x + nSumWidth > rcClip.left)
			{
				if (crText == CLR_NONE || nColorIndex & COLORINDEX_APPLYFORCE)
					pdc->SetTextColor(GetColor(nColorIndex));
				else
					pdc->SetTextColor(crText);
				if (crBkgnd == CLR_NONE || nBgColorIndex & COLORINDEX_APPLYFORCE)
					pdc->SetBkColor(GetColor(nBgColorIndex));
				else
					pdc->SetBkColor(crBkgnd);

				pdc->SelectObject(GetFont(nColorIndex));
				// we are sure to have less than 4095 characters because all the chars are visible
				VERIFY(pdc->ExtTextOut(ptOrigin.x, ptOrigin.y, ETO_CLIPPED,
					&rcClip, line.c_str() + iBegin, nCount, pnWidths));
				if (cxZeroWidthBlock != 0 && PtInRect(&rcClip, ptOrigin))
				{
					pdc->SetBkColor(crBkgnd == CLR_NONE ||
						nBgColorIndexZeorWidthBlock & COLORINDEX_APPLYFORCE ?
						GetColor(nBgColorIndexZeorWidthBlock) : crBkgnd);
					RECT rcClipZeroWidthBlock =
					{
						ptOrigin.x - 1, rcClip.top,
						ptOrigin.x + 2, rcClip.bottom
					};
					if (rcClipZeroWidthBlock.left < rcClip.left)
						rcClipZeroWidthBlock.left = rcClip.left;
					if (rcClipZeroWidthBlock.right > rcClip.right)
						rcClipZeroWidthBlock.right = rcClip.right;
					VERIFY(pdc->ExtTextOut(ptOrigin.x, ptOrigin.y, ETO_CLIPPED,
						&rcClipZeroWidthBlock, line.c_str() + iBegin, nCount, pnWidths));
				}
				// Draw rounded rectangles around control characters
				pdc->SaveDC();
				pdc->IntersectClipRect(rcClip.left, rcClip.top, rcClip.right, rcClip.bottom);
				HGdiObj *const pBrush = pdc->SelectStockObject(NULL_BRUSH);
				HGdiObj *const pPen = pdc->SelectObject(HPen::Create(PS_SOLID, 1, pdc->GetTextColor()));
				int x = ptOrigin.x;
				for (int j = 0 ; j < nCount ; ++j)
				{
					// Assume narrowed space is converted escape sequence leadin.
					if (line[iBegin + j] == _T(' ') && pnWidths[j] < nCharWidth)
					{
						int n = 1;
						do { } while (pnWidths[j + n++] == nCharWidth);
						pdc->RoundRect(x + 2, ptOrigin.y + 1,
							x + n * nCharWidth - 2, ptOrigin.y + nLineHeight - 1,
							nCharWidth / 2, nLineHeight / 2);
					}
					x += pnWidths[j];
				}
				VERIFY(pdc->SelectObject(pPen)->DeleteObject());
				pdc->SelectObject(pBrush);
				pdc->RestoreDC(-1);
			}
			delete [] pnWidths;
			// Update the final position after the visible characters	              
			ptOrigin.x += nSumWidth;
		}
	}
	// Update the final position after the right clipped characters
	for ( ; i < nLength; ++i)
	{
		ptOrigin.x += szLine[i] < _T('\t') ?
			nCharWidth : nCharWidth * GetCharWidthFromChar(szLine + i);
	}
}

void CCrystalTextView::DrawLineHelper(
	HSurface *pdc, POINT &ptOrigin, const RECT &rcClip,
	int nColorIndex, int nBgColorIndex, COLORREF crText, COLORREF crBkgnd,
	LPCTSTR pszChars, int nOffset, int nCount, int &nActualOffset, POINT ptTextPos,
	int nBgColorIndexZeorWidthBlock, int cxZeroWidthBlock)
{
	ASSERT(nCount > 0);
	if (m_bFocused || m_bShowInactiveSelection)
	{
		int nSelBegin = 0, nSelEnd = 0;
		if (m_ptDrawSelStart.y > ptTextPos.y)
		{
			nSelBegin = nCount;
		}
		else if (m_ptDrawSelStart.y == ptTextPos.y)
		{
			nSelBegin = m_ptDrawSelStart.x - ptTextPos.x;
			if (nSelBegin < 0)
				nSelBegin = 0;
			if (nSelBegin > nCount)
				nSelBegin = nCount;
		}
		if (m_ptDrawSelEnd.y > ptTextPos.y)
		{
			nSelEnd = nCount;
		}
		else if (m_ptDrawSelEnd.y == ptTextPos.y)
		{
			nSelEnd = m_ptDrawSelEnd.x - ptTextPos.x;
			if (nSelEnd < 0)
				nSelEnd = 0;
			if (nSelEnd > nCount)
				nSelEnd = nCount;
		}

		ASSERT(nSelBegin >= 0 && nSelBegin <= nCount);
		ASSERT(nSelEnd >= 0 && nSelEnd <= nCount);
		ASSERT(nSelBegin <= nSelEnd);

		//  Draw part of the text before selection
		if (nSelBegin > 0)
		{
			DrawLineHelperImpl(pdc, ptOrigin, rcClip,
				nColorIndex, nBgColorIndex, crText, crBkgnd,
				pszChars, nOffset, nSelBegin, nActualOffset,
				nBgColorIndexZeorWidthBlock, cxZeroWidthBlock);
			cxZeroWidthBlock = 0;
		}
		if (nSelBegin < nSelEnd)
		{
			DrawLineHelperImpl(pdc, ptOrigin, rcClip,
				nColorIndex & ~COLORINDEX_APPLYFORCE,
				nBgColorIndex & ~COLORINDEX_APPLYFORCE, 
				GetColor(COLORINDEX_SELTEXT),
				GetColor(COLORINDEX_SELBKGND),
				pszChars, nOffset + nSelBegin, nSelEnd - nSelBegin, nActualOffset,
				nBgColorIndexZeorWidthBlock, 0);
			cxZeroWidthBlock = 0;
		}
		if (nSelEnd < nCount)
		{
			DrawLineHelperImpl(pdc, ptOrigin, rcClip,
				nColorIndex, nBgColorIndex, crText, crBkgnd,
				pszChars, nOffset + nSelEnd, nCount - nSelEnd, nActualOffset,
				nBgColorIndexZeorWidthBlock, cxZeroWidthBlock);
		}
	}
	else
	{
		DrawLineHelperImpl(pdc, ptOrigin, rcClip,
			nColorIndex, nBgColorIndex, crText, crBkgnd,
			pszChars, nOffset, nCount, nActualOffset,
			nBgColorIndexZeorWidthBlock, cxZeroWidthBlock);
	}
}

/*void CCrystalTextView::GetLineColors(int nLineIndex, COLORREF &crBkgnd, COLORREF &crText)
{
	DWORD dwLineFlags = GetLineFlags(nLineIndex);
	crText = RGB(255, 255, 255);
	if (dwLineFlags & LF_EXECUTION)
	{
		crBkgnd = RGB(0, 128, 0);
		return;
	}
	if (dwLineFlags & LF_BREAKPOINT)
	{
		crBkgnd = RGB(255, 0, 0);
		return;
	}
	if (dwLineFlags & LF_INVALID_BREAKPOINT)
	{
		crBkgnd = RGB(128, 128, 0);
		return;
	}
	crBkgnd = CLR_NONE;
	crText = CLR_NONE;
}*/

DWORD CCrystalTextView::GetParseCookie(int nLineIndex)
{
	const int nLineCount = GetLineCount ();
	if (m_ParseCookies.size() == 0)
	{
		// must be initialized to invalid value (DWORD) -1
		m_ParseCookies.assign(nLineCount, -1);
	}

	if (nLineIndex < 0)
		return 0;
	if (m_ParseCookies[nLineIndex] != - 1)
		return m_ParseCookies[nLineIndex];

	int L = nLineIndex;
	while (L >= 0 && m_ParseCookies[L] == - 1)
		L--;
	L++;

	int nBlocks;
	while (L <= nLineIndex)
	{
		DWORD dwCookie = 0;
		if (L > 0)
			dwCookie = m_ParseCookies[L - 1];
		ASSERT(dwCookie != - 1);
		m_ParseCookies[L] = ParseLine(dwCookie, L, NULL, nBlocks);
		ASSERT(m_ParseCookies[L] != - 1);
		L++;
	}

	return m_ParseCookies[nLineIndex];
}

int CCrystalTextView::GetAdditionalTextBlocks(int nLineIndex, TEXTBLOCK *&pBuf)
{
	return 0;
}

//BEGIN SW
void CCrystalTextView::WrapLine(int nLineIndex, int *anBreaks, int &nBreaks)
{
	// There must be a parser attached to this view
	if (m_pParser)
	{
		const int nMaxLineWidth = GetScreenChars();
		m_pParser->WrapLine(nLineIndex, nMaxLineWidth, anBreaks, nBreaks);
	}
}


void CCrystalTextView::WrapLineCached(int nLineIndex, int *anBreaks, int &nBreaks)
{
	// If the word wrap is not active, there is no breaks in the line
	if (!m_bWordWrap)
	{
		nBreaks = 0;
		return;
	}
	// word wrap is active
	if (nLineIndex <= upperBound(m_panSubLines) && !anBreaks && m_panSubLines[nLineIndex] > -1)
	{
		// return cached data
		nBreaks = m_panSubLines[nLineIndex] - 1;
	}
	else
	{
		// recompute line wrap
		nBreaks = 0;
		WrapLine(nLineIndex, anBreaks, nBreaks);
		// cache data
		ASSERT( nBreaks > -1 );
		setAtGrow(m_panSubLines, nLineIndex, nBreaks + 1);
	}
}

void CCrystalTextView::InvalidateLineCache(int nLineIndex1, int nLineIndex2)
{
	// invalidate cached sub line index
	InvalidateSubLineIndexCache(nLineIndex1);
	// invalidate cached sub line count
	if (nLineIndex2 == -1)
		nLineIndex2 = upperBound(m_panSubLines);
	else if (nLineIndex1 > nLineIndex2)
		std::swap(nLineIndex1, nLineIndex2);
    if (nLineIndex2 > upperBound(m_panSubLines))
		nLineIndex2 = upperBound(m_panSubLines);
	for (int i = nLineIndex1; i <= nLineIndex2; i++)
		m_panSubLines[i] = -1;
}

/**
 * @brief Invalidate sub line index cache from the specified index to the end of file.
 * @param [in] nLineIndex Index of the first line to invalidate 
 */
void CCrystalTextView::InvalidateSubLineIndexCache(int nLineIndex)
{
	if (m_nLastLineIndexCalculatedSubLineIndex >= nLineIndex)
		m_nLastLineIndexCalculatedSubLineIndex = nLineIndex - 1;
}

/**
 * @brief Invalidate items related screen size.
 */
void CCrystalTextView::InvalidateScreenRect()
{
	CalcLineCharDim();
	RECT rect;
	GetClientRect(&rect);
	m_nScreenChars = (rect.right - rect.left - GetMarginWidth()) / GetCharWidth();
	m_nScreenLines = (rect.bottom - rect.top) / GetLineHeight();
	InvalidateLineCache(0, -1);
}

void CCrystalTextView::DrawScreenLine(
	HSurface *pdc, POINT &ptOrigin, const RECT &rcClip,
	TEXTBLOCK *pBuf, int nBlocks, int &nActualItem, 
	COLORREF crText, COLORREF crBkgnd,
	LPCTSTR pszChars, int nOffset, int nCount, int &nActualOffset, int nLineIndex)
{
	POINT ptTextPos = { nOffset, nLineIndex };
	POINT originalOrigin = ptOrigin;
	RECT frect = rcClip;
	const int nLineLength = GetViewableLineLength(ptTextPos.y);
	const int nLineHeight = GetLineHeight();
	int nBgColorIndexZeorWidthBlock = COLORINDEX_NONE;
	int cxZeroWidthBlock = 0;
	static const int ZEROWIDTHBLOCK_WIDTH = 2;

	frect.top = ptOrigin.y;
	frect.bottom = frect.top + nLineHeight;

	ASSERT(nActualItem < nBlocks);

	if (nBlocks > 0 && nActualItem < nBlocks - 1 &&
		pBuf[nActualItem + 1].m_nCharPos >= nOffset && 
		pBuf[nActualItem + 1].m_nCharPos <= nOffset + nCount)
	{
		ASSERT(pBuf[nActualItem].m_nCharPos >= 0 &&
			pBuf[nActualItem].m_nCharPos <= nLineLength);
		while (nActualItem < nBlocks - 1 &&
			pBuf[nActualItem + 1].m_nCharPos <= nOffset + nCount)
		{
			ASSERT(pBuf[nActualItem].m_nCharPos >= 0 && pBuf[nActualItem].m_nCharPos <= nLineLength);
			ptTextPos.x = nOffset > pBuf[nActualItem].m_nCharPos ? nOffset : pBuf[nActualItem].m_nCharPos;
			if (int nCount = pBuf[nActualItem + 1].m_nCharPos - ptTextPos.x)
			{
				DrawLineHelper(pdc, ptOrigin, rcClip,
					pBuf[nActualItem].m_nColorIndex, pBuf[nActualItem].m_nBgColorIndex,
					crText, crBkgnd, pszChars, ptTextPos.x, 
					nCount, nActualOffset, ptTextPos,
					nBgColorIndexZeorWidthBlock, cxZeroWidthBlock);
				nBgColorIndexZeorWidthBlock = 0;
				cxZeroWidthBlock = 0;
			}
			else
			{
				if ((nBgColorIndexZeorWidthBlock & COLORINDEX_APPLYFORCE) == 0)
					nBgColorIndexZeorWidthBlock = pBuf[nActualItem].m_nBgColorIndex;
				cxZeroWidthBlock = nBgColorIndexZeorWidthBlock & COLORINDEX_APPLYFORCE ? ZEROWIDTHBLOCK_WIDTH : 0;
			}
			if (ptOrigin.x > rcClip.right)
				break;
			++nActualItem;
		}
		ASSERT(pBuf[nActualItem].m_nCharPos >= 0 &&
			pBuf[nActualItem].m_nCharPos <= nLineLength);
		ptTextPos.x = pBuf[nActualItem].m_nCharPos;
	}
	if (int nRemainingCount = nOffset + nCount - ptTextPos.x)
	{
		DrawLineHelper(pdc, ptOrigin, rcClip,
			pBuf[nActualItem].m_nColorIndex, pBuf[nActualItem].m_nBgColorIndex,
			crText, crBkgnd, pszChars, ptTextPos.x,
			nRemainingCount, nActualOffset, ptTextPos,
			nBgColorIndexZeorWidthBlock, cxZeroWidthBlock);
		cxZeroWidthBlock = 0;
	}
	// Draw space on the right of the text

	frect.left = ptOrigin.x;

	ptTextPos.x = nLineLength;
	if ((m_bFocused || m_bShowInactiveSelection) &&
		IsInsideSelBlock(ptTextPos) &&
		nOffset + nCount == nLineLength)
	{
		if (frect.left >= rcClip.left)
		{
			const int nCharWidth = GetCharWidth();
			pdc->SetBkColor(GetColor(COLORINDEX_SELBKGND));
			RECT rc = { frect.left, frect.top, frect.left + nCharWidth, frect.bottom };
			pdc->ExtTextOut(0, 0, ETO_OPAQUE, &rc, NULL, 0);
			frect.left += nCharWidth;
			cxZeroWidthBlock = 0;
		}
	}

	if (cxZeroWidthBlock != 0)
	{
		pdc->SetBkColor(crBkgnd == CLR_NONE ||
			nBgColorIndexZeorWidthBlock & COLORINDEX_APPLYFORCE ?
			GetColor(nBgColorIndexZeorWidthBlock) : crBkgnd);
		++cxZeroWidthBlock; // Align with ExtTextOut clipping behavior
		RECT rcClipZeroWidthBlock =
		{
			ptOrigin.x, ptOrigin.y,
			ptOrigin.x + cxZeroWidthBlock, ptOrigin.y + nLineHeight
		};
		VERIFY(pdc->ExtTextOut(0, 0, ETO_OPAQUE, &rcClipZeroWidthBlock, NULL, 0, NULL));
		frect.left += cxZeroWidthBlock;
	}

	if (frect.left < rcClip.left)
		frect.left = rcClip.left;

	if (frect.right > frect.left)
	{
		pdc->SetBkColor(crBkgnd == CLR_NONE ? GetColor(COLORINDEX_WHITESPACE) : crBkgnd);
		pdc->ExtTextOut(0, 0, ETO_OPAQUE, &frect, NULL, 0);
	}
	// set origin to beginning of next screen line
	ptOrigin.x = originalOrigin.x;
	ptOrigin.y += nLineHeight;
}

int CCrystalTextView::MergeTextBlocks(
	TEXTBLOCK *pBuf1, int nBlocks1,
	TEXTBLOCK *pBuf2, int nBlocks2,
	TEXTBLOCK *&pMergedBuf)
{
	pMergedBuf = new TEXTBLOCK[nBlocks1 + nBlocks2];
	int i = 0, j = 0, k = 0;
	while (i < nBlocks1 || j < nBlocks2)
	{
		if (i < nBlocks1 && j < nBlocks2 &&
			pBuf1[i].m_nCharPos == pBuf2[j].m_nCharPos)
        {
			pMergedBuf[k].m_nCharPos = pBuf2[j].m_nCharPos;
			if (pBuf2[j].m_nColorIndex == COLORINDEX_NONE)
				pMergedBuf[k].m_nColorIndex = pBuf1[i].m_nColorIndex;
			else
				pMergedBuf[k].m_nColorIndex = pBuf2[j].m_nColorIndex;
			if (pBuf2[j].m_nBgColorIndex == COLORINDEX_NONE)
				pMergedBuf[k].m_nBgColorIndex = pBuf1[i].m_nBgColorIndex;
			else
				pMergedBuf[k].m_nBgColorIndex = pBuf2[j].m_nBgColorIndex;
			i++;
			j++;
		}
		else if (j >= nBlocks2 ||
			(i < nBlocks1 && pBuf1[i].m_nCharPos < pBuf2[j].m_nCharPos))
		{
			pMergedBuf[k].m_nCharPos = pBuf1[i].m_nCharPos;
			if (nBlocks2 == 0 || pBuf2[j - 1].m_nColorIndex == COLORINDEX_NONE)
				pMergedBuf[k].m_nColorIndex = pBuf1[i].m_nColorIndex;
			else
				pMergedBuf[k].m_nColorIndex = pBuf2[j - 1].m_nColorIndex;
			if (nBlocks2 == 0 || pBuf2[j - 1].m_nBgColorIndex == COLORINDEX_NONE)
				pMergedBuf[k].m_nBgColorIndex = pBuf1[i].m_nBgColorIndex;
			else
				pMergedBuf[k].m_nBgColorIndex = pBuf2[j - 1].m_nBgColorIndex;
			i++;
		}
		else if (i >= nBlocks1 ||
			(j < nBlocks2 && pBuf1[i].m_nCharPos > pBuf2[j].m_nCharPos))
		{
			pMergedBuf[k].m_nCharPos = pBuf2[j].m_nCharPos;
			if (i > 0 && pBuf2[j].m_nColorIndex == COLORINDEX_NONE)
				pMergedBuf[k].m_nColorIndex = pBuf1[i - 1].m_nColorIndex;
			else
				pMergedBuf[k].m_nColorIndex = pBuf2[j].m_nColorIndex;
			if (i > 0 && pBuf2[j].m_nBgColorIndex == COLORINDEX_NONE)
				pMergedBuf[k].m_nBgColorIndex = pBuf1[i - 1].m_nBgColorIndex;
			else
				pMergedBuf[k].m_nBgColorIndex = pBuf2[j].m_nBgColorIndex;
			j++;
		}
		k++;
	}
	return k;
}

void CCrystalTextView::DrawSingleLine(HSurface *pdc, const RECT &rc, int nLineIndex)
{
	ASSERT(nLineIndex >= 0 && nLineIndex < GetLineCount());

	//  Acquire the background color for the current line
	COLORREF crBkgnd, crText;
	GetLineColors(nLineIndex, crBkgnd, crText);
	if (crBkgnd != CLR_NONE)
		pdc->SetBkColor(crBkgnd);
	if (crText != CLR_NONE)
		pdc->SetTextColor(crText);

	int nEmptySubLines = 1;

	if (LPCTSTR pszChars = GetLineChars(nLineIndex))
	{
		int nLength = GetViewableLineLength(nLineIndex);

		//  Parse the line
		DWORD dwCookie = GetParseCookie (nLineIndex - 1);
		TEXTBLOCK *pBuf = new TEXTBLOCK[(nLength+1) * 3]; // be aware of nLength == 0

		int nBlocks = 0;
		// insert at least one textblock of normal color at the beginning
		pBuf[0].m_nCharPos = 0;
		pBuf[0].m_nColorIndex = COLORINDEX_NORMALTEXT;
		pBuf[0].m_nBgColorIndex = COLORINDEX_BKGND;
		nBlocks++;

		m_ParseCookies[nLineIndex] = ParseLine(dwCookie, nLineIndex, pBuf, nBlocks);
		ASSERT(m_ParseCookies[nLineIndex] != - 1);

		TEXTBLOCK *pAddedBuf = NULL;
		int nAddedBlocks = GetAdditionalTextBlocks(nLineIndex, pAddedBuf);

		TEXTBLOCK *pMergedBuf;
		int nMergedBlocks = MergeTextBlocks(pBuf, nBlocks, pAddedBuf, nAddedBlocks, pMergedBuf);

		delete[] pBuf;
		delete[] pAddedBuf;

		pBuf = pMergedBuf;
		nBlocks = nMergedBlocks;

		int nActualItem = 0;
		int nActualOffset = 0;

		// Wrap the line
		std::vector<int> anBreaks;
		anBreaks.resize(nLength + 2);
		int nBreaks = 0;
		anBreaks[0] = 0;
		WrapLineCached(nLineIndex, &anBreaks.front() + 1, nBreaks);
		anBreaks[++nBreaks] = nLength;

		//  Draw the line text
		const int nCharWidth = GetCharWidth();
		POINT origin = { rc.left - m_nOffsetChar * nCharWidth, rc.top };

		// Draw all the screen lines of the wrapped line
		for (int i = 0 ; i < nBreaks ; ++i)
		{
			ASSERT(anBreaks[i] >= 0 && anBreaks[i] <= nLength);
			DrawScreenLine(
				pdc, origin, rc,
				pBuf, nBlocks, nActualItem,
				crText, crBkgnd,
				pszChars, anBreaks[i], anBreaks[i + 1] - anBreaks[i],
				nActualOffset, nLineIndex);
		}

		delete[] pBuf;
		nEmptySubLines = GetEmptySubLines(nLineIndex);
	}
	// Draw empty sublines
	if (nEmptySubLines > 0)
	{
		RECT frect = rc;
		frect.top = frect.bottom - nEmptySubLines * GetLineHeight();
		pdc->SetBkColor(crBkgnd == CLR_NONE ? GetColor(COLORINDEX_WHITESPACE) : crBkgnd);
		pdc->ExtTextOut(0, 0, ETO_OPAQUE, &frect, NULL, 0);
	}
}

/**
 * @brief Escape special characters
 * @param [in]      strText The text to escape
 * @param [in, out] nNonbreakChars The number of non-break characters in the text
 * @param [in]      nScreenChars   The maximum number of characters to display per line
 * @return The escaped text
 */
static String EscapeHTML(LPCTSTR pch, int nScreenChars)
{
	String strHTML;
	while (TCHAR ch = *pch++)
	{
		switch (ch)
		{
		case _T('&'):
			strHTML += _T("&amp;");
			break;
		case _T('<'):
			strHTML += _T("&lt;");
			break;
		case _T('>'):
			strHTML += _T("&gt;");
			break;
		case _T(' '):
			ch = _T('\xA0');
			// fall through
		default:
			strHTML += ch;
			break;
		}
	}
	return strHTML;
}

/**
 * @brief Return all the styles necessary to render this view as HTML code
 * @return The HTML styles
 */
void CCrystalTextView::GetHTMLStyles(String &strStyles)
{
	static const int arColorIndices[] = {
		COLORINDEX_NORMALTEXT,
		COLORINDEX_SELTEXT,
		COLORINDEX_KEYWORD,
		COLORINDEX_FUNCNAME,
		COLORINDEX_COMMENT,
		COLORINDEX_NUMBER,
		COLORINDEX_OPERATOR,
		COLORINDEX_STRING,
		COLORINDEX_PREPROCESSOR,
		COLORINDEX_HIGHLIGHTTEXT1,
		COLORINDEX_HIGHLIGHTTEXT2,
		COLORINDEX_USER1,
		COLORINDEX_USER2,
	};
	static const int arBgColorIndices[] = {
		COLORINDEX_BKGND,
		COLORINDEX_SELBKGND,
		COLORINDEX_HIGHLIGHTBKGND1,
		COLORINDEX_HIGHLIGHTBKGND2,
		COLORINDEX_HIGHLIGHTBKGND3,
		COLORINDEX_HIGHLIGHTBKGND4,
	};
	int f = 0;
	while (f < _countof(arColorIndices))
	{
		int nColorIndex = arColorIndices[f++];
		COLORREF crText = GetColor(nColorIndex);
		int b = 0;
		while (b < _countof(arBgColorIndices))
		{
			int nBgColorIndex = arBgColorIndices[b++];
			COLORREF crBkgnd = GetColor(nBgColorIndex);
			strStyles.append_sprintf(
				_T(".sf%db%d { color: #%02x%02x%02x; background-color: #%02x%02x%02x; %s%s}\n"),
				nColorIndex, nBgColorIndex,
				GetRValue(crText), GetGValue(crText), GetBValue(crText),
				GetRValue(crBkgnd), GetGValue(crBkgnd), GetBValue(crBkgnd),
				&_T("\0font-weight: bold; ")[GetBold(nColorIndex) != FALSE],
				&_T("\0font-style: italic; ")[GetItalic(nColorIndex) != FALSE]);
		}
	}
}

/**
 * @brief Return the HTML attribute associated with the specified colors
 * @param [in] nColorIndex   Index of text color
 * @param [in] nBgColorIndex Index of background color
 * @param [in] crText        Base text color
 * @param [in] crBkgnd       Base background color
 * @return The HTML attribute
 */
void CCrystalTextView::GetHTMLAttribute(
	int nColorIndex, int nBgColorIndex, COLORREF crText, COLORREF crBkgnd, String &strAttr)
{
	if ((crText == CLR_NONE || (nColorIndex & COLORINDEX_APPLYFORCE)) && 
		(crBkgnd == CLR_NONE || (nBgColorIndex & COLORINDEX_APPLYFORCE)))
	{
		strAttr.append_sprintf(_T("class='sf%db%d'"),
			nColorIndex & ~COLORINDEX_APPLYFORCE, nBgColorIndex & ~COLORINDEX_APPLYFORCE);
		return;
	}
	if (crText == CLR_NONE || (nColorIndex & COLORINDEX_APPLYFORCE))
		crText = GetColor(nColorIndex);
	if (crBkgnd == CLR_NONE || (nBgColorIndex & COLORINDEX_APPLYFORCE))
		crBkgnd = GetColor(nBgColorIndex);
	strAttr.append_sprintf(
		_T("style='color: #%02x%02x%02x; background-color: #%02x%02x%02x; %s%s'"),
		GetRValue(crText), GetGValue(crText), GetBValue(crText),
		GetRValue(crBkgnd), GetGValue(crBkgnd), GetBValue(crBkgnd),
		&_T("\0font-weight: bold; ")[GetBold(nColorIndex) != FALSE],
		&_T("\0font-style: italic; ")[GetItalic(nColorIndex) != FALSE]);
}

/**
 * @brief Retrieve the html version of the line
 * @param [in] nLineIndex  Index of line in view
 * @param [in] pszTag      The HTML tag to enclose the line
 * @return The html version of the line
 */
void CCrystalTextView::GetHTMLLine(int nLineIndex, String &strHTML)
{
	ASSERT(nLineIndex >= -1 && nLineIndex < GetLineCount ());

	int nLength = GetViewableLineLength(nLineIndex);
	LPCTSTR pszChars = GetLineChars(nLineIndex);

	// Acquire the background color for the current line
	COLORREF crBkgnd, crText;
	GetLineColors(nLineIndex, crBkgnd, crText);

	// Parse the line
	DWORD dwCookie = GetParseCookie(nLineIndex - 1);
	TEXTBLOCK *pBuf = new TEXTBLOCK[(nLength + 1) * 3]; // be aware of nLength == 0
	int nBlocks = 0;
	// insert at least one textblock of normal color at the beginning
	pBuf[0].m_nCharPos = 0;
	pBuf[0].m_nColorIndex = COLORINDEX_NORMALTEXT;
	pBuf[0].m_nBgColorIndex = COLORINDEX_BKGND;
	nBlocks++;
	m_ParseCookies[nLineIndex] = ParseLine(dwCookie, nLineIndex, pBuf, nBlocks);
	ASSERT(m_ParseCookies[nLineIndex] != - 1);

	TEXTBLOCK *pAddedBuf = NULL;
	int nAddedBlocks = GetAdditionalTextBlocks(nLineIndex, pAddedBuf);

	TEXTBLOCK *pMergedBuf;
	int nMergedBlocks = MergeTextBlocks(pBuf, nBlocks, pAddedBuf, nAddedBlocks, pMergedBuf);

	delete[] pBuf;
	delete[] pAddedBuf;

	pBuf = pMergedBuf;
	nBlocks = nMergedBlocks;
	///////

	String strExpanded;
	const int nScreenChars = GetScreenChars();

	int nActualOffset = 0;

	strHTML += _T("<code>");

	int i = 0;
	while (i < nBlocks)
	{
		int j = i + 1;
		int nBlockLength = (j < nBlocks ? pBuf[j].m_nCharPos : nLength) - pBuf[i].m_nCharPos;
		nActualOffset += ExpandChars(pszChars, pBuf[i].m_nCharPos, nBlockLength, strExpanded, nActualOffset);
		if (!strExpanded.empty())
		{
			strHTML += _T("<span ");
			GetHTMLAttribute(pBuf[i].m_nColorIndex, pBuf[i].m_nBgColorIndex, crText, crBkgnd, strHTML);
			strHTML += _T(">");
			strHTML += EscapeHTML(strExpanded.c_str(), nScreenChars);
			strHTML += _T("</span>");
		}
		i = j;
	}

	strHTML += _T("</code>");

	delete[] pBuf;
}

COLORREF CCrystalTextView::GetColor(int nColorIndex)
{
	return m_pColors ? m_pColors->GetColor(nColorIndex & ~COLORINDEX_APPLYFORCE) : RGB(0, 0, 0);
}

DWORD CCrystalTextView::GetLineFlags(int nLineIndex) const
{
	return m_pTextBuffer ? m_pTextBuffer->GetLineFlags(nLineIndex) : 0;
}

/**
 * @brief Draw selection margin.
 * @param [in] pdc         Pointer to draw context.
 * @param [in] rect        The rectangle to draw.
 * @param [in] nLineIndex  Index of line in view.
 * @param [in] nLineNumber Line number to display. if -1, it's not displayed.
 */
void CCrystalTextView::DrawMargin(HSurface * pdc, const RECT & rect, int nLineIndex, int nLineNumber)
{
	if (!m_bSelMargin && !m_bViewLineNumbers)
		pdc->SetBkColor(GetColor(COLORINDEX_BKGND));
	else
		pdc->SetBkColor(GetColor(COLORINDEX_SELMARGIN));
	pdc->ExtTextOut(0, 0, ETO_OPAQUE, &rect, NULL, 0);
	if (m_bViewLineNumbers && nLineNumber > 0)
	{
		TCHAR szNumbers[32];
		wsprintf(szNumbers, _T("%d"), nLineNumber);
		HGdiObj *pOldFont = pdc->SelectObject(GetFont());
		COLORREF clrOldColor = pdc->SetTextColor(GetColor(COLORINDEX_NORMALTEXT));
		UINT uiOldAlign = pdc->SetTextAlign(TA_RIGHT);
		pdc->TextOut(rect.right - 4, rect.top, szNumbers, lstrlen(szNumbers));
		pdc->SetTextAlign(uiOldAlign);
		pdc->SelectObject(pOldFont);
		pdc->SetTextColor(clrOldColor);
	}

	// Draw line revision mark (or background) whenever we have valid lineindex
	COLORREF clrRevisionMark = GetColor(COLORINDEX_WHITESPACE);
	if (nLineIndex >= 0 && m_pTextBuffer)
	{
		const LineInfo &li = m_pTextBuffer->GetLineInfo(nLineIndex);
		// get line revision marks color
		if (nLineNumber != -1 && li.m_dwRevisionNumber > 0)
		{
			if (m_pTextBuffer->m_dwRevisionNumberOnSave < li.m_dwRevisionNumber)
				clrRevisionMark = UNSAVED_REVMARK_CLR;
			else
				clrRevisionMark = SAVED_REVMARK_CLR;
		}
	}

	// draw line revision marks
	RECT rc = { rect.right - MARGIN_REV_WIDTH, rect.top, rect.right, rect.bottom };
	pdc->SetBkColor(clrRevisionMark);
	pdc->ExtTextOut(0, 0, ETO_OPAQUE, &rc, NULL, 0);

	if (!m_bSelMargin)
		return;

	int nImageIndex = -1;
	if (nLineIndex >= 0)
	{
		DWORD dwLineFlags = GetLineFlags(nLineIndex);
		static const DWORD adwFlags[] =
		{
			LF_EXECUTION,
			LF_BREAKPOINT,
			LF_COMPILATION_ERROR,
			LF_BOOKMARK(1),
			LF_BOOKMARK(2),
			LF_BOOKMARK(3),
			LF_BOOKMARK(4),
			LF_BOOKMARK(5),
			LF_BOOKMARK(6),
			LF_BOOKMARK(7),
			LF_BOOKMARK(8),
			LF_BOOKMARK(9),
			LF_BOOKMARK(0),
			LF_BOOKMARKS,
			LF_INVALID_BREAKPOINT
		};
		for (int i = 0; i < _countof(adwFlags); ++i)
		{
			if ((dwLineFlags & adwFlags[i]) != 0)
			{
				nImageIndex = i;
				break;
			}
		}
	}
	if (nImageIndex >= 0)
	{
		m_pIcons->Draw(nImageIndex, pdc->m_hDC,
			rect.left + 2, rect.top + (GetLineHeight() - MARGIN_ICON_HEIGHT) / 2,
			ILD_TRANSPARENT);
	}

	// draw wrapped-line-icon
	if (nLineNumber > 0)
	{
		int nBreaks = 0;
		WrapLineCached(nLineIndex, NULL, nBreaks);
		for (int i = 0; i < nBreaks; i++)
		{
			m_pIcons->Draw(ICON_INDEX_WRAPLINE, pdc->m_hDC,
				rect.right - MARGIN_ICON_WIDTH,
				rect.top + (GetLineHeight() - MARGIN_ICON_WIDTH) / 2 + (i + 1) * GetLineHeight(),
				ILD_TRANSPARENT);
		}
	}
}

bool CCrystalTextView::IsInsideSelBlock(POINT ptTextPos)
{
	PrepareSelBounds();
	ASSERT_VALIDTEXTPOS(ptTextPos);
	if (ptTextPos.y < m_ptDrawSelStart.y)
		return false;
	if (ptTextPos.y > m_ptDrawSelEnd.y)
		return false;
	if (ptTextPos.y < m_ptDrawSelEnd.y && ptTextPos.y > m_ptDrawSelStart.y)
		return true;
	if (m_ptDrawSelStart.y < m_ptDrawSelEnd.y)
	{
		if (ptTextPos.y == m_ptDrawSelEnd.y)
			return ptTextPos.x < m_ptDrawSelEnd.x;
		ASSERT(ptTextPos.y == m_ptDrawSelStart.y);
		return ptTextPos.x >= m_ptDrawSelStart.x;
	}
	ASSERT(m_ptDrawSelStart.y == m_ptDrawSelEnd.y);
	return ptTextPos.x >= m_ptDrawSelStart.x && ptTextPos.x < m_ptDrawSelEnd.x;
}

bool CCrystalTextView::IsInsideSelection(const POINT &ptTextPos)
{
	PrepareSelBounds();
	return IsInsideSelBlock(ptTextPos);
}

/**
 * @brief : class the selection extremities in ascending order
 *
 * @note : Updates m_ptDrawSelStart and m_ptDrawSelEnd
 * This function must be called before reading these values
 */
void CCrystalTextView::PrepareSelBounds()
{
	if (m_ptSelStart.y < m_ptSelEnd.y ||
		(m_ptSelStart.y == m_ptSelEnd.y && m_ptSelStart.x < m_ptSelEnd.x))
	{
		m_ptDrawSelStart = m_ptSelStart;
		m_ptDrawSelEnd = m_ptSelEnd;
	}
	else
	{
		m_ptDrawSelStart = m_ptSelEnd;
		m_ptDrawSelEnd = m_ptSelStart;
	}
}

void CCrystalTextView::OnDraw(HSurface *pdc)
{
	RECT rcClient;
	GetClientRect(&rcClient);

	const int nLineCount = GetLineCount();
	const int nLineHeight = GetLineHeight();
	PrepareSelBounds();

	// if the private arrays (m_ParseCookies and m_pnActualLineLength) 
	// are defined, check they are in phase with the text buffer
	ASSERT(m_ParseCookies.size() == 0 || m_ParseCookies.size() == nLineCount);
	ASSERT(m_pnActualLineLength.size() == 0 || m_pnActualLineLength.size() == nLineCount);

	RECT rcLine;
	rcLine.top = rcClient.top;
	int nSubLineOffset = GetSubLineIndex(m_nTopLine) - m_nTopSubLine;
	if (nSubLineOffset < 0)
		rcLine.top = nSubLineOffset * nLineHeight;

	int nCurrentLine = m_nTopLine;
	while (rcLine.top < rcClient.bottom)
	{
		rcLine.left = rcClient.left;
		rcLine.right = rcClient.right;
		if (nCurrentLine < nLineCount)
		{
			rcLine.bottom = rcLine.top + GetSubLines(nCurrentLine) * nLineHeight;
			if (pdc->RectVisible(&rcLine))
			{
				rcLine.right = GetMarginWidth();
				DrawMargin(pdc, rcLine, nCurrentLine, nCurrentLine + 1);
				rcLine.left = rcLine.right;
				rcLine.right = rcClient.right;
				DrawSingleLine(pdc, rcLine, nCurrentLine);
			}
			++nCurrentLine;
		}
		else
		{
			// this is the last iteration
			// Draw area beyond the text
			rcLine.bottom = rcClient.bottom;
			rcLine.right = GetMarginWidth();
			DrawMargin(pdc, rcLine, -1, -1);
			rcLine.left = rcLine.right;
			rcLine.right = rcClient.right;
			pdc->SetBkColor(GetColor(COLORINDEX_WHITESPACE));
			pdc->ExtTextOut(0, 0, ETO_OPAQUE, &rcLine, NULL, 0);
		}
		rcLine.top = rcLine.bottom;
	}
}

void CCrystalTextView::ResetView()
{
	// m_bWordWrap = FALSE;
	m_nTopLine = 0;
	m_nTopSubLine = 0;
	m_nOffsetChar = 0;
	m_nLineHeight = -1;
	m_nCharWidth = -1;
	m_nMaxLineLength = -1;
	m_nScreenLines = INT_MIN;
	m_nScreenChars = INT_MIN;
	m_nIdealCharPos = -1;
	m_ptAnchor.x = 0;
	m_ptAnchor.y = 0;
	for (int I = 0; I < 4; I++)
	{
		if (m_apFonts[I] != NULL)
		{
			m_apFonts[I]->DeleteObject();
			m_apFonts[I] = NULL;
		}
	}
	m_ParseCookies.clear();
	m_pnActualLineLength.clear();
	m_ptCursorPos.x = 0;
	m_ptCursorPos.y = 0;
	m_ptSelStart = m_ptSelEnd = m_ptCursorPos;
	if (m_bDragSelection)
	{
		ReleaseCapture();
		KillTimer(m_nDragSelTimer);
	}
	m_bDragSelection = false;
	if (m_hWnd)
	{
		InvalidateScreenRect();
		UpdateCaret();
	}
	m_bShowInactiveSelection = true; // FP: reverted because I like it
}

void CCrystalTextView::UpdateCaret(bool bShowHide)
{
	ASSERT_VALIDTEXTPOS(m_ptCursorPos);
	if (m_bFocused && !m_bCursorHidden)
	{
		int nActualOffset = CalculateActualOffset(m_ptCursorPos.y, m_ptCursorPos.x);
		if (nActualOffset >= m_nOffsetChar)
		{
			int nWidth = 2;
			if (m_bOvrMode && !IsSelection() &&
				m_ptCursorPos.x < GetLineLength(m_ptCursorPos.y))
			{
				if (LPCTSTR pch = GetLineChars(m_ptCursorPos.y))
				{
					pch += m_ptCursorPos.x;
					nWidth = GetCharWidth() * GetCharWidthFromChar(pch);
					if (*pch == _T('\t'))
					{
						int nTabSize = GetTabSize();
						nWidth *= nTabSize - nActualOffset % nTabSize;
					}
				}
			}
			CreateCaret(NULL, nWidth, GetLineHeight());
			POINT ptCaretPos = TextToClient(m_ptCursorPos);
			SetCaretPos(ptCaretPos.x, ptCaretPos.y);
			ShowCaret();
			UpdateCompositionWindowPos(); /* IME */
			OnUpdateCaret(bShowHide);
			return;
		}
	}
	HideCaret();
}

void CCrystalTextView::OnUpdateCaret(bool)
{
}

int CCrystalTextView::GetTabSize() const
{
	return m_pTextBuffer ? m_pTextBuffer->GetTabSize() : 4;
}

bool CCrystalTextView::GetSeparateCombinedChars() const
{
	return m_bSeparateCombinedChars;
}

void CCrystalTextView::SetTabSize(int nTabSize, bool bSeparateCombinedChars)
{
	ASSERT(nTabSize >= 0 && nTabSize <= 64);
	if (m_pTextBuffer == NULL)
		return;
	if (m_pTextBuffer->GetTabSize() != nTabSize ||
		m_bSeparateCombinedChars != bSeparateCombinedChars)
	{
		m_pTextBuffer->SetTabSize(nTabSize);
		m_bSeparateCombinedChars = bSeparateCombinedChars;
		m_pnActualLineLength.clear();
		m_nMaxLineLength = -1;
		RecalcHorzScrollBar();
		Invalidate();
		UpdateCaret();
	}
}

HFont *CCrystalTextView::GetFont(int nColorIndex)
{
	int nIndex = 0;
	m_lfBaseFont.lfWeight = FW_NORMAL;
	m_lfBaseFont.lfItalic = 0;
	if (nColorIndex != COLORINDEX_NONE)
	{
		if (GetItalic(nColorIndex))
		{
			nIndex |= 2;
			m_lfBaseFont.lfItalic = 1;
		}
		else if (GetBold(nColorIndex))
		{
			nIndex |= 1;
			m_lfBaseFont.lfWeight = FW_BOLD;
		}
	}
	if (m_apFonts[nIndex] == NULL)
	{
		if (!m_lfBaseFont.lfHeight)
		{
			HWindow *pDesktopWindow = HWindow::GetDesktopWindow();
			if (HSurface *pdc = pDesktopWindow->GetDC())
			{
				m_lfBaseFont.lfHeight = -MulDiv(11, pdc->GetDeviceCaps(LOGPIXELSY), 72);
				pDesktopWindow->ReleaseDC(pdc);
			}
		}
		m_apFonts[nIndex] = HFont::CreateIndirect(&m_lfBaseFont);
		if (!m_apFonts[nIndex])
		{
			return OWindow::GetFont();
		}
	}
	return m_apFonts[nIndex];
}

void CCrystalTextView::CalcLineCharDim()
{
	HSurface *pdc = GetDC();
	HGdiObj *pOldFont = pdc->SelectObject(GetFont());
	SIZE szCharExt;
	pdc->GetTextExtent(_T("X"), 1, &szCharExt);
	m_nLineHeight = szCharExt.cy;
	if (m_nLineHeight < 1)
		m_nLineHeight = 1;
	m_nCharWidth = szCharExt.cx;
	/*
	TEXTMETRIC tm;
	if (pdc->GetTextMetrics(&tm))
	m_nCharWidth -= tm.tmOverhang;
	*/
	pdc->SelectObject(pOldFont);
	ReleaseDC(pdc);
}

int CCrystalTextView::GetLineHeight()
{
	ASSERT(m_nLineHeight != -1);
	return m_nLineHeight;
}

int CCrystalTextView::GetSubLines( int nLineIndex )
{
	if (!m_bWordWrap)
		return 1;
	// get a number of lines this wrapped lines contains
	int nBreaks = 0;
	WrapLineCached(nLineIndex, NULL, nBreaks);
	return GetEmptySubLines(nLineIndex) + nBreaks + 1;
}

int CCrystalTextView::GetEmptySubLines(int nLineIndex)
{
	return 0;
}

bool CCrystalTextView::IsEmptySubLineIndex(int nSubLineIndex)
{
	int nLineIndex;
	int dummy;
	GetLineBySubLine(nSubLineIndex, nLineIndex, dummy);
	int nSubLineIndexNextLine = GetSubLineIndex(nLineIndex) + GetSubLines(nLineIndex);
	if (nSubLineIndexNextLine - GetEmptySubLines(nLineIndex) <= nSubLineIndex && nSubLineIndex < nSubLineIndexNextLine)
		return true;
	else
		return false;
}

int CCrystalTextView::CharPosToPoint( int nLineIndex, int nCharPos, POINT &charPoint )
{
	// if we do not wrap lines, y is allways 0 and x is equl to nCharPos
	if (!m_bWordWrap)
	{
		charPoint.x = nCharPos;
		charPoint.y = 0;
	}

	// line is wrapped
	int *anBreaks = new int[GetLineLength (nLineIndex)];
	int nBreaks = 0;

	WrapLineCached(nLineIndex, anBreaks, nBreaks);

	int i = nBreaks;
	do
	{
		--i;
	} while (i >= 0 && nCharPos < anBreaks[i]);

	charPoint.x = i >= 0 ? nCharPos - anBreaks[i] : nCharPos;
	charPoint.y = i + 1;

	int nReturnVal = i >= 0 ? anBreaks[i] : 0;
	delete[] anBreaks;
	return nReturnVal;
}

int CCrystalTextView::CursorPointToCharPos(int nLineIndex, const POINT &curPoint)
{
	// calculate char pos out of point
	const int nLength = GetLineLength(nLineIndex);
	LPCTSTR szLine = GetLineChars(nLineIndex);

	// wrap line
	int *anBreaks = new int[nLength];
	int nBreaks = 0;

	WrapLineCached(nLineIndex, anBreaks, nBreaks);

	// find char pos that matches cursor position
	int nXPos = 0;
	int nYPos = 0;
	int nCurPos = 0;
	const int nTabSize = GetTabSize();

	int nIndex = 0;
	for (nIndex = 0; nIndex < nLength; nIndex++)
    {
		if( nBreaks && nIndex == anBreaks[nYPos] )
		{
			nXPos = 0;
			nYPos++;
		}

		const int nOffset = szLine[nIndex] == _T('\t') ?
			nTabSize - nCurPos % nTabSize :
			GetCharWidthFromDisplayableChar(szLine + nIndex);
		nXPos += nOffset;
		nCurPos += nOffset;

		if (nXPos > curPoint.x && nYPos == curPoint.y)
		{
			break;
		}
		else if (nYPos > curPoint.y)
		{
			nIndex--;
			break;
		}
	}
	delete[] anBreaks;
	return nIndex;	
}

void CCrystalTextView::SubLineCursorPosToTextPos(int x, int y, POINT &textPos)
{
	// Get line breaks
	int nSubLineOffset, nLine;
	GetLineBySubLine(y, nLine, nSubLineOffset);
	// compute cursor-position
	POINT curPoint = { x, nSubLineOffset };
	textPos.x = CursorPointToCharPos(nLine, curPoint);
	textPos.y = nLine;
}

/**
 * @brief Calculate last character position in (sub)line.
 * @param [in] nLineIndex Linenumber to check.
 * @param [in] nSublineOffset Subline index in wrapped line.
 * @return Position of the last character.
 */
int CCrystalTextView::SubLineEndToCharPos(int nLineIndex, int nSubLineOffset)
{
	const int nLength = GetLineLength(nLineIndex);

	// if word wrapping is disabled, the end is equal to the length of the line -1
	if (!m_bWordWrap)
		return nLength;

	// wrap line
	int *anBreaks = new int[nLength];
	int nBreaks = 0;

	WrapLineCached(nLineIndex, anBreaks, nBreaks);

	// if there is no break inside the line or the given subline is the last
	// one in this line...
	if (nBreaks == 0 || nSubLineOffset == nBreaks)
	{
		delete [] anBreaks;
		return nLength;
	}

	// compute character position for end of subline
	ASSERT(nSubLineOffset >= 0 && nSubLineOffset <= nBreaks);

	int nReturnVal = anBreaks[nSubLineOffset] - 1;
	delete [] anBreaks;

	return nReturnVal;
}

/** 
 * @brief Calculate first character position in (sub)line.
 * @param [in] nLineIndex Linenumber to check.
 * @param [in] nSublineOffset Subline index in wrapped line.
 * @return Position of the first character.
 */
int CCrystalTextView::SubLineHomeToCharPos(int nLineIndex, int nSubLineOffset)
{
	// if word wrapping is disabled, the start is 0
	if (!m_bWordWrap || nSubLineOffset == 0)
		return 0;

	// wrap line
	int nLength = GetLineLength(nLineIndex);
	int *anBreaks = new int[nLength];
	int nBreaks = 0;

	WrapLineCached(nLineIndex, anBreaks, nBreaks);

	// if there is no break inside the line...
	if (nBreaks == 0)
	{
		delete [] anBreaks;
		return 0;
	}

	// compute character position for end of subline
	ASSERT(nSubLineOffset > 0 && nSubLineOffset <= nBreaks);

	int nReturnVal = anBreaks[nSubLineOffset - 1];
	delete [] anBreaks;

	return nReturnVal;
}
//END SW

int CCrystalTextView::GetCharWidth()
{
	if (m_nCharWidth == -1)
		CalcLineCharDim();
	return m_nCharWidth;
}

int CCrystalTextView::GetMaxLineLength()
{
	if (m_nMaxLineLength == -1)
	{
		m_nMaxLineLength = 0;
		const int nLineCount = GetLineCount ();
		for (int i = 0 ; i < nLineCount ; ++i)
		{
			int nActualLength = GetLineActualLength(i);
			if (m_nMaxLineLength < nActualLength)
				m_nMaxLineLength = nActualLength;
		}
	}
	return m_nMaxLineLength;
}

void CCrystalTextView::GoToLine(int nLine, bool bRelative)
{
	int nLines = m_pTextBuffer->GetLineCount() - 1;
	POINT ptCursorPos = GetCursorPos();
	if (bRelative)
	{
		nLine += ptCursorPos.y;
	}
	if (nLine)
	{
		nLine--;
	}
	if (nLine > nLines)
	{
		nLine = nLines;
	}
	if (nLine >= 0)
	{
		int nChars = m_pTextBuffer->GetLineLength(nLine);
		if (nChars)
		{
			nChars--;
		}
		if (ptCursorPos.x > nChars)
		{
			ptCursorPos.x = nChars;
		}
		if (ptCursorPos.x >= 0)
		{
			ptCursorPos.y = nLine;
			ASSERT_VALIDTEXTPOS(ptCursorPos);
			SetAnchor(ptCursorPos);
			SetSelection(ptCursorPos, ptCursorPos);
			SetCursorPos(ptCursorPos);
			EnsureCursorVisible();
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CCrystalTextView message handlers

int CCrystalTextView::ComputeRealLine(int nApparentLine) const
{
	return nApparentLine;
}

int CCrystalTextView::GetLineCount()
{
	if (m_pTextBuffer == NULL)
		return 1;                   //  Single empty line

	int nLineCount = m_pTextBuffer->GetLineCount();
	ASSERT(nLineCount > 0);
	return nLineCount;
}

//BEGIN SW
int CCrystalTextView::GetSubLineCount()
{
	const int nLineCount = GetLineCount();
	// if we do not wrap words, number of sub lines is
	// equal to number of lines
	if (!m_bWordWrap)
		return nLineCount;
	// calculate number of sub lines
	if (nLineCount <= 0)
		return 0;
	return GetSubLineIndex(nLineCount - 1) + GetSubLines(nLineCount - 1);
}

int CCrystalTextView::GetSubLineIndex(int nLineIndex)
{
	// if we do not wrap words, subline index of this line is equal to its index
	if (!m_bWordWrap)
		return nLineIndex;

	// calculate subline index of the line
	int	nSubLineCount = 0;
	int nLineCount = GetLineCount();

	if (nLineIndex >= nLineCount)
		nLineIndex = nLineCount - 1;

	// return cached subline index of the line if it is already cached.
	if (nLineIndex <= m_nLastLineIndexCalculatedSubLineIndex)
		return m_panSubLineIndexCache[nLineIndex];

	// calculate subline index of the line and cache it.
	if (m_nLastLineIndexCalculatedSubLineIndex >= 0)
	{
		nSubLineCount = m_panSubLineIndexCache[m_nLastLineIndexCalculatedSubLineIndex];
	}
	else
	{
		m_nLastLineIndexCalculatedSubLineIndex = 0;
		setAtGrow(m_panSubLineIndexCache, 0, 0);
	}

	for (int i = m_nLastLineIndexCalculatedSubLineIndex ; i < nLineIndex ; i++)
	{
		setAtGrow(m_panSubLineIndexCache, i, nSubLineCount);
		nSubLineCount+= GetSubLines( i );
	}
	setAtGrow(m_panSubLineIndexCache, nLineIndex, nSubLineCount);
	m_nLastLineIndexCalculatedSubLineIndex = nLineIndex;

	return nSubLineCount;
}

// See comment in the header file
void CCrystalTextView::GetLineBySubLine(int nSubLineIndex, int &nLine, int &nSubLine)
{
	ASSERT( nSubLineIndex < GetSubLineCount() );

	// if we do not wrap words, nLine is equal to nSubLineIndex and nSubLine is allways 0
	if (!m_bWordWrap)
	{
		nLine = nSubLineIndex;
		nSubLine = 0;
		return;
	}

	// compute result
	int nSubLineCount = 0;
	int nLastSubLines = 0;
	const int nLineCount = GetLineCount();

	int i = 0;
	for (i = 0; i < nLineCount; i++)
	{
		nLastSubLines = GetSubLines(i);
		nSubLineCount += nLastSubLines;
		if (nSubLineCount > nSubLineIndex)
			break;
	}

	ASSERT(i < nLineCount);
	nLine = i;
	nSubLine = nSubLineIndex - nSubLineCount + nLastSubLines;
}

int CCrystalTextView::GetLineLength(int nLineIndex) const
{
	return m_pTextBuffer ? m_pTextBuffer->GetLineLength(nLineIndex) : 0;
}

int CCrystalTextView::GetFullLineLength(int nLineIndex) const
{
	return m_pTextBuffer ? m_pTextBuffer->GetFullLineLength(nLineIndex) : 0;
}

// How many bytes of line are displayed on-screen?
int CCrystalTextView::GetViewableLineLength(int nLineIndex) const
{
	if (m_bViewEols)
		return GetFullLineLength(nLineIndex);
	else
		return GetLineLength(nLineIndex);
}

LPCTSTR CCrystalTextView::GetLineChars(int nLineIndex) const
{
	return m_pTextBuffer ? m_pTextBuffer->GetLineChars(nLineIndex) : NULL;
}

/** 
 * @brief Reattach buffer after deleting/inserting ghost lines :
 * 
 * @note no need to reinitialize the horizontal scrollbar
 * no need to reset the editor options (m_bOvrMode, m_bLastReplace)
 */
void CCrystalTextView::ReAttachToBuffer(CCrystalTextBuffer *pBuf)
{
	if (m_pTextBuffer)
		m_pTextBuffer->RemoveView(this);
	ASSERT(pBuf != NULL);
	m_pTextBuffer = pBuf;
	if (m_pTextBuffer)
		m_pTextBuffer->AddView(this);
	// don't reset CCrystalEditView options
	CCrystalTextView::ResetView();
	//  Update vertical scrollbar only
	RecalcVertScrollBar();
}

/** 
 * @brief Attach buffer (maybe for the first time)
 * initialize the view and initialize both scrollbars
 */
void CCrystalTextView::AttachToBuffer(CCrystalTextBuffer *pBuf)
{
	if (m_pTextBuffer)
		m_pTextBuffer->RemoveView(this);
	ASSERT(pBuf != NULL);
	m_pTextBuffer = pBuf;
	if (m_pTextBuffer)
		m_pTextBuffer->AddView (this);
	ResetView();
	//  Update scrollbars
	RecalcVertScrollBar();
	RecalcHorzScrollBar();
}

void CCrystalTextView::DetachFromBuffer()
{
	if (m_pTextBuffer)
	{
		m_pTextBuffer->RemoveView(this);
		m_pTextBuffer = NULL;
		// don't reset CCrystalEditView options
		CCrystalTextView::ResetView();
	}
}

int CCrystalTextView::GetScreenLines()
{
	ASSERT(m_nScreenLines != INT_MIN);
	return m_nScreenLines;
}

bool CCrystalTextView::GetItalic(int nColorIndex)
{
	// WINMERGE - since italic text has problems,
	// lets disable it. E.g. "_" chars disappear and last
	// char may be cropped.
	return nColorIndex == COLORINDEX_LAST;
}

bool CCrystalTextView::GetBold(int nColorIndex)
{
	return m_pColors && m_pColors->GetBold(nColorIndex & ~COLORINDEX_APPLYFORCE);
}

int CCrystalTextView::GetScreenChars()
{
	ASSERT(m_nScreenChars != INT_MIN);
	return m_nScreenChars;
}

void CCrystalTextView::OnDestroy()
{
	GetFont()->GetLogFont(&m_lfBaseFont);
	DetachFromBuffer();
	for (int I = 0; I < 4; I++)
	{
		if (m_apFonts[I] != NULL)
		{
			m_apFonts[I]->DeleteObject();
			m_apFonts[I] = NULL;
		}
	}
}

void CCrystalTextView::OnSize()
{
	// we have to recompute the line wrapping
	InvalidateScreenRect();
	// recalculate m_nTopSubLine
	m_nTopSubLine = GetSubLineIndex(m_nTopLine);
	// set caret to right position
	RecalcVertScrollBar();
	RecalcHorzScrollBar();
	UpdateSiblingScrollPos();
}

void CCrystalTextView::OnUpdateSibling(const CCrystalTextView *pUpdateSource)
{
	ASSERT(pUpdateSource != NULL);
	bool bUpdateCaret = pUpdateSource == this;
	if (pUpdateSource->m_nTopSubLine != m_nTopSubLine)
	{
		ScrollToSubLine(pUpdateSource->m_nTopSubLine);
		bUpdateCaret = true;
	}
	if (pUpdateSource->m_nOffsetChar != m_nOffsetChar)
	{
		ScrollToChar(pUpdateSource->m_nOffsetChar);
		bUpdateCaret = true;
	}
	if (bUpdateCaret)
		UpdateCaret(true);
}

int CCrystalTextView::RecalcVertScrollBar(bool bPositionOnly)
{
	SCROLLINFO si;
	si.cbSize = sizeof si;
	si.fMask = SIF_POS;
	si.nPage = GetScreenLines();
	si.nMin = 0;
	si.nPos = 0;
	si.nMax = 0;
	LPCTSTR pcwAtom = MAKEINTATOM(GetClassAtom());
	HWindow *pParent = GetParent();
	HWindow *pChild = NULL;
	while ((pChild = pParent->FindWindowEx(pChild, pcwAtom)) != NULL)
	{
		CCrystalTextView *pSiblingView = static_cast<CCrystalTextView *>(FromHandle(pChild));
		int nPos = pSiblingView->m_nTopSubLine;
		if (si.nPos < nPos)
			si.nPos = nPos;
		if (!bPositionOnly)
		{
			si.fMask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_POS | SIF_RANGE;
			int nMax = pSiblingView->GetSubLineCount() - 1;
			if (si.nMax < nMax)
				si.nMax = nMax;
		}
	}
	if (!bPositionOnly)
	{
		int nMaxTopSubLine = GetSubLineCount() - GetScreenLines();
		if (m_nTopSubLine > nMaxTopSubLine)
			ScrollToSubLine(nMaxTopSubLine);
	}

	while ((pChild = pParent->FindWindowEx(pChild, pcwAtom)) != NULL)
	{
		CCrystalTextView *pSiblingView = static_cast<CCrystalTextView *>(FromHandle(pChild));
		if (pSiblingView->GetStyle() & WS_VSCROLL)
			pSiblingView->SetScrollInfo(SB_VERT, &si);
	}
	return si.nPos;
}

void CCrystalTextView::OnVScroll(UINT nSBCode)
{
	ASSERT(GetStyle() & WS_VSCROLL);
	int nPos = GetScrollPos(SB_VERT, nSBCode);
	LPCTSTR pcwAtom = MAKEINTATOM(GetClassAtom());
	HWindow *pParent = GetParent();
	HWindow *pChild = NULL;
	CCrystalTextView *pThis = this;
	int nMaxSubLineCount = 0;
	while ((pChild = pParent->FindWindowEx(pChild, pcwAtom)) != NULL)
	{
		CCrystalTextView *pSiblingView = static_cast<CCrystalTextView *>(FromHandle(pChild));
		int nSubLineCount = pSiblingView->GetSubLineCount();
		if (nMaxSubLineCount < nSubLineCount)
		{
			nMaxSubLineCount = nSubLineCount;
			pThis = pSiblingView;
		}
	}
	pThis->ScrollToSubLine(nPos);
	pThis->UpdateSiblingScrollPos();
}

int CCrystalTextView::RecalcHorzScrollBar(bool bPositionOnly)
{
	SCROLLINFO si;
	si.cbSize = sizeof si;
	LPCTSTR pcwAtom = MAKEINTATOM(GetClassAtom());
	HWindow *pParent = GetParent();
	HWindow *pChild = NULL;
	while ((pChild = pParent->FindWindowEx(pChild, pcwAtom)) != NULL)
	{
		CCrystalTextView *pSiblingView = static_cast<CCrystalTextView *>(FromHandle(pChild));
		if (pSiblingView->GetStyle() & WS_HSCROLL)
		{
			if (pSiblingView->m_bWordWrap)
			{
				if (pSiblingView->m_nOffsetChar > 0)
				{
					pSiblingView->m_nOffsetChar = 0;
					pSiblingView->UpdateCaret();
				}
				// Disable horizontal scroll bar
				si.fMask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_POS | SIF_RANGE;
				si.nPage = 0;
				si.nMin = 0;
				si.nMax = 0;
			}
			else if (bPositionOnly)
			{
				si.fMask = SIF_POS;
			}
			else
			{
				si.fMask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_POS | SIF_RANGE;
				si.nPage = pSiblingView->GetScreenChars();
				si.nMin = 0;
				// Horiz scroll limit to longest line + one screenwidth 
				si.nMax = pSiblingView->GetMaxLineLength() + si.nPage;
			}
			si.nPos = pSiblingView->m_nOffsetChar;
			pSiblingView->SetScrollInfo(SB_HORZ, &si);
		}
	}
	return si.nPos;
}

void CCrystalTextView::OnHScroll(UINT nSBCode)
{
	int nPos = GetScrollPos(SB_HORZ, nSBCode);
	ScrollToChar(nPos);
	ASSERT(GetStyle() & WS_HSCROLL);
	UpdateSiblingScrollPos();
}

BOOL CCrystalTextView::OnSetCursor(UINT nHitTest)
{
	if (nHitTest == HTCLIENT)
	{
		POINT pt;
		::GetCursorPos(&pt);
		ScreenToClient(&pt);
		if (pt.x < GetMarginWidth())
		{
			::SetCursor(::LoadCursor(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MARGIN_CURSOR)));
		}
		else
		{
			POINT ptText = ClientToText(pt);
			PrepareSelBounds();
			if (!IsInsideSelBlock(ptText))
			{
				::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_IBEAM)));
			}
			else if (!m_bDisableDragAndDrop)   // If Drag And Drop Not Disabled
			{
				::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)));
			}
			// So else we return TRUE but don't set a cursor? Looks like a bug...
		}
		return TRUE;
	}
	return FALSE;
}

/** 
 * @brief Converts client area point to text position.
 * @param [in] point Client area point.
 * @return Text position (line index, char index in line).
 * @note For gray selection area char index is 0.
 */
POINT CCrystalTextView::ClientToText(const POINT &point)
{
	//BEGIN SW
	const int nSubLineCount = GetSubLineCount();
	const int nLineCount = GetLineCount();

	POINT pt;
	pt.y = m_nTopSubLine + point.y / GetLineHeight();
	if (pt.y >= nSubLineCount)
		pt.y = nSubLineCount - 1;
	if (pt.y < 0)
		pt.y = 0;

	int nLine;
	int nSubLineOffset;
	int nOffsetChar = m_nOffsetChar;

	GetLineBySubLine( pt.y, nLine, nSubLineOffset );
	pt.y = nLine;

	LPCTSTR pszLine = NULL;
	int nLength = 0;
	int *anBreaks = NULL;
	int nBreaks = 0;

	if (pt.y >= 0 && pt.y < nLineCount)
    {
		nLength = GetLineLength( pt.y );
		anBreaks = new int[nLength];
		pszLine = GetLineChars(pt.y);
		WrapLineCached(pt.y, anBreaks, nBreaks);

		if (nSubLineOffset > 0)
			nOffsetChar = anBreaks[nSubLineOffset - 1];
		if (nBreaks > nSubLineOffset)
			nLength = anBreaks[nSubLineOffset] - 1;
	}

	int nPos = 0;
	// Char index for margin area is 0
	if (point.x > GetMarginWidth())
		nPos = nOffsetChar + (point.x - GetMarginWidth()) / GetCharWidth();
	if (nPos < 0)
		nPos = 0;

	int nIndex = 0;
	int nCurPos = 0;
	int n = 0;
	int i = 0;
	const int nTabSize = GetTabSize();

	while (nIndex < nLength)
	{
		if (nBreaks && nIndex == anBreaks[i])
		{
			n = nIndex;
			++i;
		}
		const int nOffset = pszLine[nIndex] == _T('\t')
			? nTabSize - nCurPos % nTabSize
			: GetCharWidthFromDisplayableChar(pszLine + nIndex);
		n += nOffset;
		nCurPos += nOffset;
		if (n > nPos && i == nSubLineOffset)
		{
			break;
		}
		++nIndex;
	}

	delete[] anBreaks;
	ASSERT(nIndex >= 0 && nIndex <= nLength);
	pt.x = nIndex;
	return pt;
}

#ifdef _DEBUG
void CCrystalTextView::AssertValidTextPos(const POINT & point)
{
	if (GetLineCount() > 0)
	{
		ASSERT(m_nTopLine >= 0 && m_nOffsetChar >= 0);
		ASSERT(point.y >= 0 && point.y < GetLineCount());
		ASSERT(point.x >= 0 && point.x <= GetViewableLineLength(point.y));
	}
}
#endif

bool CCrystalTextView::IsValidTextPos(const POINT &point)
{
	return GetLineCount() > 0 && m_nTopLine >= 0 && m_nOffsetChar >= 0 &&
		point.y >= 0 && point.y < GetLineCount() && point.x >= 0 && point.x <= GetLineLength(point.y);
}

bool CCrystalTextView::IsValidTextPosX(const POINT &point)
{
	return GetLineCount() > 0 && m_nTopLine >= 0 && m_nOffsetChar >= 0 &&
		point.y >= 0 && point.y < GetLineCount() && point.x >= 0 && point.x <= GetLineLength(point.y);
}

bool CCrystalTextView::IsValidTextPosY(int y)
{
	return GetLineCount() > 0 && m_nTopLine >= 0 && m_nOffsetChar >= 0 &&
		y >= 0 && y < GetLineCount();
}

POINT CCrystalTextView::TextToClient(const POINT &point)
{
	ASSERT_VALIDTEXTPOS(point);
	LPCTSTR pszLine = GetLineChars(point.y);
	//BEGIN SW
	POINT charPoint;
	int nSubLineStart = CharPosToPoint(point.y, point.x, charPoint);
	charPoint.y += GetSubLineIndex(point.y);
	// compute y-position
	POINT pt = { 0, (charPoint.y - m_nTopSubLine) * GetLineHeight() };
	if (charPoint.x != 0)
	{
		// we have to calculate x-position
		int	nPreOffset = 0;
		/*ORIGINAL
		pt.y = (point.y - m_nTopLine) * GetLineHeight();
		*/
		//END SW
		int nTabSize = GetTabSize();
		for (int nIndex = 0; nIndex < point.x; nIndex++)
		{
			//BEGIN SW
			if (nIndex == nSubLineStart)
				nPreOffset = pt.x;
			//END SW
			pt.x += pszLine[nIndex] == _T('\t') ?
				nTabSize - pt.x % nTabSize :
				GetCharWidthFromDisplayableChar(pszLine + nIndex);
		}
		//BEGIN SW
		pt.x -= nPreOffset;
		//END SW
	}
	pt.x = (pt.x - m_nOffsetChar) * GetCharWidth() + GetMarginWidth();
	return pt;
}

void CCrystalTextView::InvalidateLines(int nLine1, int nLine2)
{
	RECT rcInvalid;
	GetClientRect(&rcInvalid);
	const int nLineHeight = GetLineHeight();
	if (nLine2 != -1)
	{
		if (nLine2 < nLine1)
		{
			int nTemp = nLine1;
			nLine1 = nLine2;
			nLine2 = nTemp;
		}
		//BEGIN SW
		rcInvalid.bottom = (GetSubLineIndex(nLine2) - m_nTopSubLine + GetSubLines(nLine2)) * nLineHeight;
		/*ORIGINAL
		rcInvalid.top = (nLine1 - m_nTopLine) * GetLineHeight();
		rcInvalid.bottom = (nLine2 - m_nTopLine + 1) * GetLineHeight();
		*/
		//END SW
	}
	rcInvalid.top = (GetSubLineIndex(nLine1) - m_nTopSubLine) * nLineHeight;
	InvalidateRect(&rcInvalid, FALSE);
}

void CCrystalTextView::SetSelection(const POINT &ptStart, const POINT &ptEnd)
{
	ASSERT_VALIDTEXTPOS(ptStart);
	ASSERT_VALIDTEXTPOS(ptEnd);
	if (m_ptSelStart == ptStart)
	{
		if (m_ptSelEnd != ptEnd)
			InvalidateLines(ptEnd.y, m_ptSelEnd.y);
	}
	else
	{
		InvalidateLines(ptStart.y, ptEnd.y);
		InvalidateLines(m_ptSelStart.y, m_ptSelEnd.y);
	}
	m_ptSelStart = ptStart;
	m_ptSelEnd = ptEnd;
}

void CCrystalTextView::AdjustTextPoint(POINT &point)
{
	point.x += GetCharWidth() / 2;   //todo
}

void CCrystalTextView::OnSetFocus()
{
	m_bFocused = true;
	if (m_ptSelStart != m_ptSelEnd)
		InvalidateLines(m_ptSelStart.y, m_ptSelEnd.y);
	UpdateCaret();
}

DWORD CCrystalTextView::ParseLinePlain(DWORD dwCookie, int nLineIndex, TEXTBLOCK * pBuf, int &nActualItems)
{
	return 0;
}

DWORD CCrystalTextView::ParseLine(DWORD dwCookie, int nLineIndex, TEXTBLOCK * pBuf, int &nActualItems)
{
	return (this->*(m_CurSourceDef->ParseLineX))(dwCookie, nLineIndex, pBuf, nActualItems);
}

int CCrystalTextView::CalculateActualOffset(int nLineIndex, int nCharIndex, BOOL bAccumulate)
{
	const int nLength = GetLineLength(nLineIndex);
	ASSERT(nCharIndex >= 0 && nCharIndex <= nLength);
	LPCTSTR pszChars = GetLineChars(nLineIndex);
	int nOffset = 0;
	const int nTabSize = GetTabSize();
	//BEGIN SW
	int *anBreaks = new int[nLength];
	int nBreaks = 0;

	WrapLineCached(nLineIndex, anBreaks, nBreaks);

	int nPreOffset = 0;
	int nPreBreak = 0;

	if (nBreaks)
	{
		int j = nBreaks;
		do
		{
			--j;
		} while (j >= 0 && nCharIndex < anBreaks[j]);
		nPreBreak = anBreaks[j];
	}
	delete[] anBreaks;
	//END SW
	int i;
	for (i = 0 ; i < nCharIndex ; ++i)
	{
		//BEGIN SW
		if (nPreBreak == i && nBreaks)
			nPreOffset = nOffset;
		//END SW
		nOffset += pszChars[i] == _T('\t') ?
			nTabSize - nOffset % nTabSize :
			GetCharWidthFromDisplayableChar(pszChars + i);
	}
	if (bAccumulate)
		return nOffset;
	//BEGIN SW
	if (nPreBreak == i && nBreaks)
		return 0;
	else
		return nOffset - nPreOffset;
	/*ORIGINAL
	return nOffset;
	*///END SW
}

int CCrystalTextView::ApproxActualOffset(int nLineIndex, int nOffset)
{
	if (nOffset == 0)
		return 0;
	int nLength = GetLineLength(nLineIndex);
	LPCTSTR pszChars = GetLineChars(nLineIndex);
	int nCurrentOffset = 0;
	int nTabSize = GetTabSize();
	for (int i = 0 ; i < nLength ; i++)
	{
		if (pszChars[i] == _T ('\t'))
			nCurrentOffset += (nTabSize - nCurrentOffset % nTabSize);
		else
			nCurrentOffset++;
		if (nCurrentOffset >= nOffset)
		{
			if (nOffset <= nCurrentOffset - nTabSize / 2)
				return i;
			return i + 1;
		}
	}
	return nLength;
}

void CCrystalTextView::EnsureCursorVisible()
{
	//  Scroll vertically
	int nSubLineCount = GetSubLineCount();
	int nNewTopSubLine = m_nTopSubLine;
	POINT subLinePos;

	CharPosToPoint(m_ptCursorPos.y, m_ptCursorPos.x, subLinePos);
	subLinePos.y += GetSubLineIndex(m_ptCursorPos.y);

	if (nNewTopSubLine <= subLinePos.y - GetScreenLines())
		nNewTopSubLine = subLinePos.y - GetScreenLines() + 1;
	if (nNewTopSubLine > subLinePos.y)
		nNewTopSubLine = subLinePos.y;

	if (nNewTopSubLine < 0)
		nNewTopSubLine = 0;
	if (nNewTopSubLine >= nSubLineCount)
		nNewTopSubLine = nSubLineCount - 1;

	if (!m_bWordWrap)
	{
		// WINMERGE: This line fixes (cursor) slowdown after merges!
		// I don't know exactly why, but propably we are setting
		// m_nTopLine to zero in ResetView() and are not setting to
		// valid value again. Maybe this is a good place to set it?
		m_nTopLine = nNewTopSubLine;
	}
	else
	{
		int dummy;
		GetLineBySubLine(nNewTopSubLine, m_nTopLine, dummy);
	}

	ScrollToSubLine(nNewTopSubLine);

	//  Scroll horizontally
	// we do not need horizontally scrolling, if we wrap the words
	if (!m_bWordWrap)
	{
		int nActualPos = CalculateActualOffset(m_ptCursorPos.y, m_ptCursorPos.x);
		int nNewOffset = m_nOffsetChar;
		const int nScreenChars = GetScreenChars();
	  
		// Keep 5 chars visible right to cursor
		if (nActualPos > nNewOffset + nScreenChars - 5)
		{
			// Add 10 chars width space after line
			nNewOffset = nActualPos - nScreenChars + 10;
		}
		// Keep 5 chars visible left to cursor
		if (nActualPos < nNewOffset + 5)
		{
			// Jump by 10 char steps, so user sees previous letters too
			nNewOffset = nActualPos - 10;
		}

		// Horiz scroll limit to longest line + one screenwidth
		const int nMaxLineLen = GetMaxLineLength();
		if (nNewOffset >= nMaxLineLen + nScreenChars)
			nNewOffset = nMaxLineLen + nScreenChars - 1;
		if (nNewOffset < 0)
			nNewOffset = 0;

		ScrollToChar(nNewOffset);
	}
	UpdateSiblingScrollPos();
}

void CCrystalTextView::UpdateSiblingScrollPos()
{
	LPCTSTR pcwAtom = MAKEINTATOM(GetClassAtom());
	HWindow *pParent = GetParent();
	HWindow *pChild = NULL;
	while ((pChild = pParent->FindWindowEx(pChild, pcwAtom)) != NULL)
	{
		CCrystalTextView *pSiblingView = static_cast<CCrystalTextView *>(FromHandle(pChild));
		pSiblingView->OnUpdateSibling(this);
	}
}

void CCrystalTextView::OnKillFocus()
{
	m_bFocused = false;
	UpdateCaret();
	if (m_ptSelStart != m_ptSelEnd)
		InvalidateLines(m_ptSelStart.y, m_ptSelEnd.y);
	if (m_bDragSelection)
	{
		ReleaseCapture();
		KillTimer(m_nDragSelTimer);
		m_bDragSelection = false;
	}
}

void CCrystalTextView::GetText(int nStartLine, int nStartChar, int nEndLine, int nEndChar, String &text)
{
	if (m_pTextBuffer != NULL)
		m_pTextBuffer->GetText(nStartLine, nStartChar, nEndLine, nEndChar, text);
	else
		text.clear();
}

void CCrystalTextView::UpdateView(CCrystalTextView *pSource, CUpdateContext *pContext, DWORD dwFlags, int nLineIndex)
{
	if (dwFlags & UPDATE_RESET)
	{
		ResetView();
		RecalcVertScrollBar();
		RecalcHorzScrollBar();
		return;
	}

	int nLineCount = GetLineCount();
	ASSERT(nLineCount > 0);
	ASSERT(nLineIndex >= -1 && nLineIndex < nLineCount);
	if ((dwFlags & UPDATE_SINGLELINE) != 0)
	{
		if (nLineIndex != -1)
		{
			//  All text below this line should be reparsed
			const int cookiesSize = m_ParseCookies.size();
			if (cookiesSize > 0)
			{
				ASSERT(cookiesSize == nLineCount);
				// must be reinitialized to invalid value (DWORD) - 1
				for (int i = nLineIndex; i < cookiesSize; ++i)
					m_ParseCookies[i] = -1;
			}
			//  This line'th actual length must be recalculated
			if (m_pnActualLineLength.size())
			{
				ASSERT(m_pnActualLineLength.size() == nLineCount);
				// must be initialized to invalid code -1
				m_pnActualLineLength[nLineIndex] = -1;
				//BEGIN SW
				InvalidateLineCache(nLineIndex, nLineIndex);
				//END SW
			}
			//  Repaint the lines
			InvalidateLines(nLineIndex, -1);
		}
	}
	else
	{
		if (m_bViewLineNumbers)
			// if enabling linenumber, we must invalidate all line-cache in visible area because selection margin width changes dynamically.
			nLineIndex = m_nTopLine < nLineIndex ? m_nTopLine : nLineIndex;

		if (nLineIndex == -1)
			nLineIndex = 0;         //  Refresh all text

		//  All text below this line should be reparsed
		if (m_ParseCookies.size())
		{
			stl_size_t arrSize = m_ParseCookies.size();
			if (arrSize != nLineCount)
			{
				stl_size_t oldsize = arrSize; 
				m_ParseCookies.resize(nLineCount);
				arrSize = nLineCount;
				// must be initialized to invalid value (DWORD) - 1
				for (stl_size_t i = oldsize; i < arrSize; ++i)
				m_ParseCookies[i] = -1;
			}
			for (stl_size_t i = nLineIndex; i < arrSize; ++i)
				m_ParseCookies[i] = -1;
		}

		//  Recalculate actual length for all lines below this
		if (m_pnActualLineLength.size())
		{
			stl_size_t arrsize = m_pnActualLineLength.size();
			if (arrsize != nLineCount)
			{
				//  Reallocate actual length array
				stl_size_t oldsize = arrsize; 
				m_pnActualLineLength.resize(nLineCount);
				arrsize = nLineCount;
				// must be initialized to invalid code -1
				for (stl_size_t i = oldsize; i < arrsize; ++i)
				m_pnActualLineLength[i] = -1;
			}
			for (stl_size_t i = nLineIndex; i < arrsize; ++i)
				m_pnActualLineLength[i] = -1;
		}
		//BEGIN SW
		InvalidateLineCache(nLineIndex, -1);
		//END SW
		//  Repaint the lines
		InvalidateLines(nLineIndex, -1);
	}

	//  All those points must be recalculated and validated
	if (pContext != NULL)
	{
		pContext->RecalcPoint(m_ptCursorPos);
		pContext->RecalcPoint(m_ptSelStart);
		pContext->RecalcPoint(m_ptSelEnd);
		pContext->RecalcPoint(m_ptAnchor);
		ASSERT_VALIDTEXTPOS(m_ptCursorPos);
		ASSERT_VALIDTEXTPOS(m_ptSelStart);
		ASSERT_VALIDTEXTPOS(m_ptSelEnd);
		ASSERT_VALIDTEXTPOS(m_ptAnchor);
		if (m_bDraggingText)
		{
			pContext->RecalcPoint(m_ptDraggedTextBegin);
			pContext->RecalcPoint(m_ptDraggedTextEnd);
			ASSERT_VALIDTEXTPOS(m_ptDraggedTextBegin);
			ASSERT_VALIDTEXTPOS(m_ptDraggedTextEnd);
		}
		POINT ptTopLine = { 0, m_nTopLine };
		pContext->RecalcPoint(ptTopLine);
		ASSERT_VALIDTEXTPOS(ptTopLine);
		m_nTopLine = ptTopLine.y;
		UpdateCaret();
	}

	//  Recalculate vertical scrollbar, if needed
	if ((dwFlags & UPDATE_VERTRANGE) != 0)
	{
		RecalcVertScrollBar();
	}

	//  Recalculate horizontal scrollbar, if needed
	if ((dwFlags & UPDATE_HORZRANGE) != 0)
	{
		m_nMaxLineLength = -1;
		RecalcHorzScrollBar();
	}
}

void CCrystalTextView::SetAnchor(const POINT & ptNewAnchor)
{
	ASSERT_VALIDTEXTPOS(ptNewAnchor);
	m_ptAnchor = ptNewAnchor;
}

void CCrystalTextView::OnEditOperation(int nAction, LPCTSTR pszText)
{
}

const POINT &CCrystalTextView::GetCursorPos() const
{
	return m_ptCursorPos;
}

void CCrystalTextView::SetCursorPos(const POINT &ptCursorPos)
{
	ASSERT_VALIDTEXTPOS(ptCursorPos);
	m_ptCursorPos = ptCursorPos;
	m_nIdealCharPos = CalculateActualOffset(m_ptCursorPos.y, m_ptCursorPos.x);
	UpdateCaret();
}

void CCrystalTextView::UpdateCompositionWindowPos() /* IME */
{
	HIMC hIMC = ImmGetContext(m_hWnd);
	COMPOSITIONFORM compform;
	compform.dwStyle = CFS_FORCE_POSITION;
	GetCaretPos(&compform.ptCurrentPos);
	ImmSetCompositionWindow(hIMC, &compform);
	ImmReleaseContext(m_hWnd, hIMC);
}

void CCrystalTextView::UpdateCompositionWindowFont() /* IME */
{
	HIMC hIMC = ImmGetContext(m_hWnd);
	LOGFONT logfont;
	GetFont()->GetLogFont(&logfont);
	ImmSetCompositionFont(hIMC, &logfont);
	ImmReleaseContext(m_hWnd, hIMC);
}

void CCrystalTextView::SetSelectionMargin(bool bSelMargin)
{
	if (m_bSelMargin != bSelMargin)
	{
		m_bSelMargin = bSelMargin;
		if (m_hWnd)
		{
			OnSize();
		}
	}
}

void CCrystalTextView::SetViewLineNumbers(bool bViewLineNumbers)
{
	if (m_bViewLineNumbers != bViewLineNumbers)
	{
		m_bViewLineNumbers = bViewLineNumbers;
		if (m_hWnd)
		{
			OnSize();
		}
	}
}

void CCrystalTextView::GetFont(LOGFONT &lf) const
{
	lf = m_lfBaseFont;
}

void CCrystalTextView::SetFont(const LOGFONT &lf)
{
	m_lfBaseFont = lf;
	m_nCharWidth = -1;
	m_nLineHeight = -1;
	for (int i = 0; i < 4; ++i)
	{
		if (m_apFonts[i] != NULL)
		{
			m_apFonts[i]->DeleteObject();
			m_apFonts[i] = NULL;
		}
	}
	OnSize();
	Invalidate();
}

void CCrystalTextView::OnToggleBookmark(UINT nCmdID)
{
	int nBookmarkID = nCmdID - ID_EDIT_TOGGLE_BOOKMARK0;
	ASSERT(nBookmarkID >= 0 && nBookmarkID <= 9);
	DWORD dwBit = LF_BOOKMARK(nBookmarkID);
	DWORD dwFlags = m_pTextBuffer->GetLineFlags(m_ptCursorPos.y);
	if ((dwFlags & dwBit) == 0)
	{
		int nLine = m_pTextBuffer->FindLineWithFlag(dwBit);
		if (nLine != -1)
		{
			DWORD dwFlags = m_pTextBuffer->GetLineFlags(nLine);
			m_pTextBuffer->SetLineFlags(nLine, dwFlags ^ dwBit);
		}
	}
	m_pTextBuffer->SetLineFlags(m_ptCursorPos.y, dwFlags ^ dwBit);
}

void CCrystalTextView::OnGoBookmark(UINT nCmdID)
{
	int nBookmarkID = nCmdID - ID_EDIT_GO_BOOKMARK0;
	ASSERT(nBookmarkID >= 0 && nBookmarkID <= 9);
	int nLine = m_pTextBuffer->FindLineWithFlag(LF_BOOKMARK(nBookmarkID));
	if (nLine >= 0)
	{
		POINT pt = { 0, nLine };
		ASSERT_VALIDTEXTPOS(pt);
		SetSelection(pt, pt);
		SetCursorPos(pt);
		SetAnchor(pt);
		EnsureCursorVisible();
	}
}

void CCrystalTextView::OnClearBookmarks()
{
	for (DWORD dwBit = LF_BOOKMARK(0); dwBit <= LF_BOOKMARK(9); dwBit <<= 1)
	{
		int nLine = m_pTextBuffer->FindLineWithFlag(dwBit);
		if (nLine >= 0)
		{
			DWORD dwFlags = m_pTextBuffer->GetLineFlags(nLine);
			m_pTextBuffer->SetLineFlags(nLine, dwFlags & ~dwBit);
		}
	}
}

void CCrystalTextView::ShowCursor()
{
	m_bCursorHidden = false;
	UpdateCaret(true);
}

void CCrystalTextView::HideCursor()
{
	m_bCursorHidden = true;
	UpdateCaret(true);
}

DWORD CCrystalTextView::GetDropEffect()
{
	return DROPEFFECT_COPY;
}

void CCrystalTextView::OnDropSource(DWORD de)
{
	ASSERT(de == DROPEFFECT_COPY);
}

HGLOBAL CCrystalTextView::PrepareDragData()
{
	PrepareSelBounds();
	String text;
	GetText(m_ptDrawSelStart, m_ptDrawSelEnd, text);
	int cchText = text.size();
	if (cchText == 0)
		return NULL;
	SIZE_T cbData = (cchText + 1) * sizeof(TCHAR);
	HGLOBAL hData = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, cbData);
	if (hData == NULL)
		return NULL;
	::GlobalReAlloc(hData, cbData, 0);
	ASSERT(::GlobalSize(hData) == cbData);

	LPTSTR pszData = reinterpret_cast<LPTSTR>(::GlobalLock(hData));
	memcpy(pszData, text.c_str(), cbData);
	::GlobalUnlock(hData);

	m_ptDraggedTextBegin = m_ptDrawSelStart;
	m_ptDraggedTextEnd = m_ptDrawSelEnd;
	return hData;
}

static const TCHAR *MemSearch(const TCHAR *p, size_t pLen, const TCHAR *q, size_t qLen)
{
	if (qLen > pLen)
		return NULL;
	const TCHAR *pEnd = p + pLen - qLen;
	const TCHAR *qEnd = q + qLen;
	do
	{
		const TCHAR *u = p;
		const TCHAR *v = q;
		do
		{
			if (v == qEnd)
				return p;
		} while (*u++ == *v++);
	} while (++p <= pEnd);
	return NULL;
}

int CCrystalTextView::FindStringHelper(
	LPCTSTR pchFindWhere, int cchFindWhere,
	LPCTSTR pchFindWhat, DWORD dwFlags,
	Captures &ovector)
{
	if (dwFlags & FIND_REGEXP)
	{
		const char *errormsg = NULL;
		int erroroffset = 0;
		const OString regexString = HString::Uni(pchFindWhat)->Oct(CP_UTF8);
		pcre *regexp = pcre_compile(regexString.A,
			dwFlags & FIND_MATCH_CASE ?
			PCRE_UTF8 | PCRE_BSR_ANYCRLF :
			PCRE_UTF8 | PCRE_BSR_ANYCRLF | PCRE_CASELESS,
			&errormsg,
			&erroroffset, NULL);
		pcre_extra *pe = NULL;
		if (regexp)
		{
			errormsg = NULL;
			pe = pcre_study(regexp, 0, &errormsg);
		}

		const OString compString = HString::Uni(pchFindWhere, cchFindWhere)->Oct(CP_UTF8);
		int result = pcre_exec(
			regexp, pe, compString.A, compString.ByteLen(), 0, 0, ovector, _countof(ovector));

		if (result >= 0)
		{
			// Convert UTF-8 offsets to WCHAR offsets
			int i = 2 * std::max(result, 1);
			do
			{
				--i;
				ovector[i] = MultiByteToWideChar(CP_UTF8, 0, compString.A, ovector[i], 0, 0);
			} while(i != 0);
		}

		pcre_free(regexp);
		pcre_free(pe);
		return result;
	}
	else
	{
		ASSERT(pchFindWhere != NULL);
		ASSERT(pchFindWhat != NULL);
		const int cchFindWhat = static_cast<int>(_tcslen(pchFindWhat));
		ovector[0] = 0;
		ovector[1] = cchFindWhat;
		while (LPCTSTR pchPos = MemSearch(pchFindWhere, cchFindWhere, pchFindWhat, cchFindWhat))
		{
			int nLen = static_cast<int>(pchPos - pchFindWhere);
			ovector[0] += nLen;
			ovector[1] += nLen;
			if ((dwFlags & FIND_WHOLE_WORD) == 0)
				return 0;
			if (!(nLen > 0 && xisalnum(pchPos[-1]) || xisalnum(pchPos[cchFindWhat])))
				return 0;
			++ovector[0];
			++ovector[1];
			pchFindWhere = pchPos + 1;
		}
		return -1;
	}
	ASSERT(FALSE); // Unreachable
}

/** 
 * @brief Select text in editor.
 * @param [in] ptStartPos Star position for highlight.
 * @param [in] nLength Count of characters to highlight.
 * @param [in] bCursorToLeft If TRUE cursor is positioned to Left-end of text
 *  selection, if FALSE cursor is positioned to right-end.
 */
BOOL CCrystalTextView::HighlightText(
	const POINT &ptStartPos, int nLength, BOOL bCursorToLeft)
{
	ASSERT_VALIDTEXTPOS(ptStartPos);
	POINT ptEndPos = ptStartPos;
	int nCount = GetLineLength(ptEndPos.y) - ptEndPos.x;
	if (nLength <= nCount)
	{
		ptEndPos.x += nLength;
	}
	else
	{
		while (nLength > nCount)
		{
			nLength -= nCount + 1;
			nCount = GetLineLength(++ptEndPos.y);
		}
		ptEndPos.x = nLength;
	}
	ASSERT_VALIDTEXTPOS(m_ptCursorPos);  //  Probably 'nLength' is bigger than expected...

	m_ptCursorPos = bCursorToLeft ? ptStartPos : ptEndPos;
	m_ptAnchor = m_ptCursorPos;
	SetSelection(ptStartPos, ptEndPos);
	UpdateCaret();

	// Scrolls found text to middle of screen if out-of-screen
	int nScreenLines = GetScreenLines();
	if (ptStartPos.y < m_nTopLine || ptEndPos.y > m_nTopLine + nScreenLines)
	{
		if (ptStartPos.y > nScreenLines / 2)
			ScrollToLine(ptStartPos.y - nScreenLines / 2);
		else
			ScrollToLine(ptStartPos.y);
	}
	EnsureSelectionVisible();
	return TRUE;
}

int CCrystalTextView::FindText(
	LPCTSTR pszText, const POINT & ptStartPos,
	DWORD dwFlags, BOOL bWrapSearch, POINT &ptFoundPos,
	Captures &captures)
{
	int nLineCount = GetLineCount();
	const POINT ptBlockBegin = { 0, 0 };
	const POINT ptBlockEnd = { GetLineLength(nLineCount - 1), nLineCount - 1 };
	return FindTextInBlock(pszText, ptStartPos, ptBlockBegin, ptBlockEnd,
		dwFlags, bWrapSearch, ptFoundPos, captures);
}

int HowManyStr(LPCTSTR s, LPCTSTR m)
{
	LPCTSTR p = s;
	int n = 0, l = static_cast<int>(_tcslen(m));
	while ((p = _tcsstr(p, m)) != NULL)
	{
		n++;
		p += l;
	}
	return n;
}

int HowManyStr(LPCTSTR s, TCHAR c)
{
	LPCTSTR p = s;
	int n = 0;
	while ((p = _tcschr(p, c)) != NULL)
	{
		n++;
		p++;
	}
	return n;
}

int CCrystalTextView::FindTextInBlock(
	LPCTSTR pszText, const POINT &ptStartPosition,
	const POINT &ptBlockBegin, const POINT &ptBlockEnd,
	DWORD dwFlags, BOOL bWrapSearch, POINT &ptFoundPos,
	Captures &captures)
{
	POINT ptCurrentPos = ptStartPosition;

	ASSERT(pszText != NULL && *pszText != _T('\0'));
	ASSERT_VALIDTEXTPOS(ptCurrentPos);
	ASSERT_VALIDTEXTPOS(ptBlockBegin);
	ASSERT_VALIDTEXTPOS(ptBlockEnd);
	ASSERT(ptBlockBegin.y < ptBlockEnd.y || ptBlockBegin.y == ptBlockEnd.y &&
			ptBlockBegin.x <= ptBlockEnd.x);
	if (ptBlockBegin == ptBlockEnd)
		return -1;
	WaitStatusCursor waitCursor;
	if (ptCurrentPos.y < ptBlockBegin.y || ptCurrentPos.y == ptBlockBegin.y &&
			ptCurrentPos.x < ptBlockBegin.x)
		ptCurrentPos = ptBlockBegin;

	String what = pszText;
	int nEolns;
	if (dwFlags & FIND_REGEXP)
	{
		nEolns = HowManyStr(what.c_str(), _T("\\n"));
	}
	else
	{
		nEolns = 0;
		if ((dwFlags & FIND_MATCH_CASE) == 0)
			what.make_upper();
	}
	if (dwFlags & FIND_DIRECTION_UP)
	{
		// Let's check if we deal with whole text.
		// At this point, we cannot search *up* in selection
		ASSERT(ptBlockBegin.x == 0 && ptBlockBegin.y == 0);
		ASSERT(ptBlockEnd.x == GetLineLength (GetLineCount() - 1) &&
			ptBlockEnd.y == GetLineCount() - 1);

		// Proceed as if we have whole text search.
		for (;;)
		{
			while (ptCurrentPos.y >= 0)
			{
				int nLineLength;
				String line;
				if (dwFlags & FIND_REGEXP)
				{
					for (int i = 0; i <= nEolns && ptCurrentPos.y >= i; i++)
					{
						LPCTSTR pszChars = GetLineChars(ptCurrentPos.y - i);
						if (i)
						{
							nLineLength = GetLineLength(ptCurrentPos.y - i);
							ptCurrentPos.x = 0;
							line = _T('\n') + line;
						}
						else
						{
							nLineLength = ptCurrentPos.x != -1 ? ptCurrentPos.x : GetLineLength(ptCurrentPos.y - i);
						}
						if (nLineLength > 0)
							line.insert(0, pszChars, nLineLength);
					}
					nLineLength = line.length();
					if (ptCurrentPos.x == -1)
						ptCurrentPos.x = 0;
				}
				else
				{
					nLineLength = GetLineLength(ptCurrentPos.y);
					if (ptCurrentPos.x == -1)
						ptCurrentPos.x = nLineLength;
					else if (ptCurrentPos.x >= nLineLength)
						ptCurrentPos.x = nLineLength - 1;

					LPCTSTR pszChars = GetLineChars(ptCurrentPos.y);
					line.assign(pszChars, ptCurrentPos.x + 1);
					if ((dwFlags & FIND_MATCH_CASE) == 0)
						line.make_upper();
				}

				int nFoundPos = -1;
				int nMatchLen = what.length();
				int nCaptures, nTmp;
				while ((nTmp = FindStringHelper(line.c_str(), line.length(), what.c_str(), dwFlags, captures)) >= 0)
				{
					nCaptures = nTmp;
					int nPos = captures[0];
					m_nLastFindWhatLen = captures[1] - captures[0];
					nFoundPos = nFoundPos == -1 ? nPos : nFoundPos + nPos;
					nFoundPos += nMatchLen;
					line.erase(0, nMatchLen + nPos);
				}

				if (nFoundPos >= 0)	// Found text!
				{
					ptCurrentPos.x = nFoundPos - nMatchLen;
					ptFoundPos = ptCurrentPos;
					return nCaptures;
				}

				ptCurrentPos.y--;
				if (ptCurrentPos.y >= 0)
					ptCurrentPos.x = GetLineLength(ptCurrentPos.y);
			}

			// Beginning of text reached
			if (!bWrapSearch)
				return -1;

			// Start again from the end of text
			bWrapSearch = FALSE;
			ptCurrentPos.y = GetLineCount() - 1;
			ptCurrentPos.x = ptCurrentPos.y >= 0 ? GetLineLength(ptCurrentPos.y) : 0;
		}
	}
	else
	{
		for (;;)
		{
			while (ptCurrentPos.y <= ptBlockEnd.y)
			{
				String line;
				if (dwFlags & FIND_REGEXP)
				{
					const int nLines = m_pTextBuffer->GetLineCount();
					for (int i = 0; i <= nEolns && ptCurrentPos.y + i < nLines; i++)
					{
						LPCTSTR pszChars = GetLineChars(ptCurrentPos.y + i);
						int nLineLength = GetLineLength(ptCurrentPos.y + i);
						if (i)
						{
							line += _T('\n');
						}
						else
						{
							pszChars += ptCurrentPos.x;
							nLineLength -= ptCurrentPos.x;
						}
						if (nLineLength > 0)
							line.append(pszChars, nLineLength);
					}
				}
				else
				{
					int nLineLength = GetLineLength(ptCurrentPos.y) - ptCurrentPos.x;
					if (nLineLength <= 0)
					{
						ptCurrentPos.x = 0;
						ptCurrentPos.y++;
						continue;
					}
					LPCTSTR pszChars = GetLineChars(ptCurrentPos.y);
					pszChars += ptCurrentPos.x;
					// Prepare necessary part of line
					line.assign(pszChars, nLineLength);
					if ((dwFlags & FIND_MATCH_CASE) == 0)
						line.make_upper();
				}

				// Perform search in the line
				int nCaptures = FindStringHelper(line.c_str(), line.length(), what.c_str(), dwFlags, captures);
				if (nCaptures >= 0)
				{
					int nPos = captures[0];
					m_nLastFindWhatLen = captures[1] - captures[0];
					if (nEolns)
					{
						String item(line.c_str(), nPos);
						LPCTSTR current = _tcsrchr(item.c_str(), _T('\n'));
						if (current)
							current++;
						else
							current = item.c_str();
						nEolns = HowManyStr(item.c_str(), _T('\n'));
						if (nEolns)
						{
							ptCurrentPos.y += nEolns;
							ptCurrentPos.x = nPos - static_cast<int>(current - item.c_str());
						}
						else
						{
							ptCurrentPos.x += nPos - static_cast<int>(current - item.c_str());
						}
						if (ptCurrentPos.x < 0)
							ptCurrentPos.x = 0;
					}
					else
					{
						ptCurrentPos.x += nPos;
					}
					// Check of the text found is outside the block.
					if (ptCurrentPos.y == ptBlockEnd.y && ptCurrentPos.x >= ptBlockEnd.x)
						break;

					ptFoundPos = ptCurrentPos;
					return nCaptures;
				}
				//  Go further, text was not found
				ptCurrentPos.x = 0;
				ptCurrentPos.y++;
			}

			//  End of text reached
			if (!bWrapSearch)
				return -1;

			//  Start from the beginning
			bWrapSearch = FALSE;
			ptCurrentPos = ptBlockBegin;
		}
	}

	ASSERT(FALSE); // Unreachable

	return -1;
}

void CCrystalTextView::OnEditFind()
{
	CFindTextDlg dlg(this);
	DWORD dwFlags = m_bLastSearch ? m_dwLastSearchFlags :
		SettingStore.GetProfileInt(_T("Editor"), _T("FindFlags"), 0);

	dlg.m_bMatchCase = (dwFlags & FIND_MATCH_CASE) != 0;
	dlg.m_bWholeWord = (dwFlags & FIND_WHOLE_WORD) != 0;
	dlg.m_bRegExp = (dwFlags & FIND_REGEXP) != 0;
	dlg.m_nDirection = (dwFlags & FIND_DIRECTION_UP) == 0;
	dlg.m_bNoWrap = (dwFlags & FIND_NO_WRAP) != 0;
	dlg.m_sText = m_strLastFindWhat;

	//  Take the current selection, if any
	if (IsSelection())
	{
		POINT ptSelStart, ptSelEnd;
		GetSelection(ptSelStart, ptSelEnd);
		if (ptSelStart.y == ptSelEnd.y)
		{
			LPCTSTR pszChars = GetLineChars(ptSelStart.y);
			int nChars = ptSelEnd.x - ptSelStart.x;
			dlg.m_sText.assign(pszChars + ptSelStart.x, nChars);
		}
	}
	else
	{
		POINT ptCursorPos = GetCursorPos();
		POINT ptStart = WordToLeft(ptCursorPos);
		POINT ptEnd = WordToRight(ptCursorPos);
		if (IsValidTextPos(ptStart) && IsValidTextPos(ptEnd) && ptStart != ptEnd)
			GetText(ptStart, ptEnd, dlg.m_sText);
	}

	// Execute Find dialog
	dlg.m_ptCurrentPos = m_ptCursorPos;   //  Search from cursor position

	// m_bShowInactiveSelection = TRUE; // FP: removed because I like it
	LanguageSelect.DoModal(dlg);
	// m_bShowInactiveSelection = FALSE; // FP: removed because I like it
	if (dlg.m_bConfirmed)
	{
		//  Save search parameters for 'F3' command
		m_bLastSearch = true;
		m_strLastFindWhat = dlg.m_sText;
		m_dwLastSearchFlags = 0;
		if (dlg.m_bMatchCase)
			m_dwLastSearchFlags |= FIND_MATCH_CASE;
		if (dlg.m_bWholeWord)
			m_dwLastSearchFlags |= FIND_WHOLE_WORD;
		if (dlg.m_bRegExp)
			m_dwLastSearchFlags |= FIND_REGEXP;
		if (dlg.m_nDirection == 0)
			m_dwLastSearchFlags |= FIND_DIRECTION_UP;
		if (dlg.m_bNoWrap)
			m_dwLastSearchFlags |= FIND_NO_WRAP;
		// Save search parameters to registry
		SettingStore.WriteProfileInt(_T("Editor"), _T("FindFlags"), m_dwLastSearchFlags);
	}
}

void CCrystalTextView::OnEditRepeat()
{
	String sText;

	// CTRL-F3 will find selected text
	if (::GetAsyncKeyState(VK_CONTROL) < 0)
	{
		POINT ptSelStart, ptSelEnd;
		GetSelection(ptSelStart, ptSelEnd);
		GetText(ptSelStart, ptSelEnd, sText);
	}
	else if (m_bLastSearch)
	{
		sText = m_strLastFindWhat;
	}

	// SHIFT-F3 will take the opposite direction
	if (::GetAsyncKeyState(VK_SHIFT) < 0)
		m_dwLastSearchFlags |= FIND_DIRECTION_UP;
	else
		m_dwLastSearchFlags &= ~FIND_DIRECTION_UP;

	// Find the text as figured out above, or else open the Find dialog
	if (!sText.empty())
	{
		POINT ptFoundPos, ptSearchPos;

		if (m_dwLastSearchFlags & FIND_DIRECTION_UP)
			GetSelection(ptSearchPos, ptFoundPos);
		else
			GetSelection(ptFoundPos, ptSearchPos);

		Captures captures;
		if (FindText(sText.c_str(), ptSearchPos, m_dwLastSearchFlags,
			(m_dwLastSearchFlags & FIND_NO_WRAP) == 0, ptFoundPos, captures) < 0)
		{
			LanguageSelect.Format(IDS_EDIT_TEXT_NOT_FOUND, sText.c_str()).MsgBox();
			return;
		}
		HighlightText(ptFoundPos, m_nLastFindWhatLen, (m_dwLastSearchFlags & FIND_DIRECTION_UP) != 0);
	}
	else
	{
		OnEditFind(); // No previous find, open Find-dialog
	}
}

/** 
 * @brief Adds/removes bookmark on given line.
 * This functions adds bookmark or removes bookmark on given line.
 * @param [in] Index (0-based) of line to add/remove bookmark.
 */
void CCrystalTextView::ToggleBookmark(int nLine)
{
	ASSERT(nLine < GetSubLineCount());
	DWORD dwFlags = m_pTextBuffer->GetLineFlags(nLine);
	m_pTextBuffer->SetLineFlags(nLine, dwFlags ^ LF_BOOKMARKS);
	const int nBookmarkLine = m_pTextBuffer->FindLineWithFlag(LF_BOOKMARKS);
	m_bBookmarkExist = nBookmarkLine >= 0;
}

/** 
 * @brief Called when Toggle Bookmark is selected from the GUI.
 */
void CCrystalTextView::OnToggleBookmark()
{
	ToggleBookmark(m_ptCursorPos.y);
}

void CCrystalTextView::OnNextBookmark()
{
	int nLine = m_pTextBuffer->FindNextBookmarkLine(m_ptCursorPos.y, +1);
	if (nLine != m_ptCursorPos.y)
	{
		POINT pt = { 0, nLine };
		ASSERT_VALIDTEXTPOS(pt);
		SetSelection(pt, pt);
		SetCursorPos(pt);
		SetAnchor(pt);
		EnsureCursorVisible();
	}
}

void CCrystalTextView::OnPrevBookmark()
{
	int nLine = m_pTextBuffer->FindNextBookmarkLine(m_ptCursorPos.y, -1);
	if (nLine != m_ptCursorPos.y)
	{
		POINT pt = { 0, nLine };
		ASSERT_VALIDTEXTPOS(pt);
		SetSelection(pt, pt);
		SetCursorPos(pt);
		SetAnchor(pt);
		EnsureCursorVisible();
	}
}

void CCrystalTextView::OnClearAllBookmarks()
{
	int nLineCount = GetLineCount();
	for (int nLine = 0; nLine < nLineCount; ++nLine)
	{
		DWORD dwFlags = m_pTextBuffer->GetLineFlags(nLine);
		m_pTextBuffer->SetLineFlags(nLine, dwFlags & ~LF_BOOKMARKS);
	}
	m_bBookmarkExist = false;
}

bool CCrystalTextView::GetViewTabs() const
{
	return m_bViewTabs;
}

void CCrystalTextView::SetViewTabs(bool bViewTabs)
{
	if (bViewTabs != m_bViewTabs)
	{
		m_bViewTabs = bViewTabs;
		if (m_hWnd)
			Invalidate();
	}
}

void CCrystalTextView::SetViewEols(bool bViewEols, bool bDistinguishEols)
{
	if (bViewEols != m_bViewEols || bDistinguishEols != m_bDistinguishEols)
	{
		m_bViewEols = bViewEols;
		m_bDistinguishEols = bDistinguishEols;
		if (m_hWnd)
			Invalidate();
	}
}

DWORD CCrystalTextView::GetFlags()
{
	return m_dwFlags;
}

void CCrystalTextView::SetFlags(DWORD dwFlags)
{
	if (m_dwFlags != dwFlags)
	{
		m_dwFlags = dwFlags;
		if (m_hWnd)
			Invalidate();
	}
}

bool CCrystalTextView::GetSelectionMargin() const
{
	return m_bSelMargin;
}

bool CCrystalTextView::GetViewLineNumbers() const
{
	return m_bViewLineNumbers;
}

/**
 * @brief Calculate margin area width.
 * This function calculates needed margin width. Needed width is (approx.)
 * one char-width for bookmark etc markers and rest to linenumbers (if
 * visible). If we have linenumbers visible we need to adjust width so that
 * biggest number fits.
 * @return Margin area width in pixels.
 */
int CCrystalTextView::GetMarginWidth()
{
	int nMarginWidth = 0;
	if (m_bViewLineNumbers)
	{
		int nLines = GetLineCount();
		if (nLines != 0)
			nLines = ComputeRealLine(nLines - 1) + 1;
		int nNumbers = 0;
		int n;
		for (n = 1; n <= nLines; n *= 10)
			++nNumbers;
		nMarginWidth += GetCharWidth() * nNumbers;
		if (!m_bSelMargin)
			nMarginWidth += 2; // Small gap when symbol part disabled
	}
	if (m_bSelMargin)
		nMarginWidth += MARGIN_ICON_WIDTH  + 7; // Width for icon markers and some margin
	else
		nMarginWidth += MARGIN_REV_WIDTH; // Space for revision marks
	return nMarginWidth;
}

//  [JRT]
bool CCrystalTextView::GetDisableDragAndDrop() const
{
	return m_bDisableDragAndDrop;
}

//  [JRT]
void CCrystalTextView::SetDisableDragAndDrop(bool bDisableDragAndDrop)
{
	m_bDisableDragAndDrop = bDisableDragAndDrop;
}

// Mouse wheel event. zDelta is in multiples of 120.
// Divide by 40 so each click is 3 lines. I know some
// drivers let you set the amount of scroll, but I
// don't know how to retrieve this or if they just
// adjust the zDelta you get here.
BOOL CCrystalTextView::OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
	POINT pt;
	POINTSTOPOINT(pt, lParam);
	short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	int nNewTopSubLine = m_nTopSubLine - zDelta / 40;
	int nMaxTopSubLine = GetSubLineCount() - GetScreenLines();
	if (nNewTopSubLine > nMaxTopSubLine)
		nNewTopSubLine = nMaxTopSubLine;
	if (nNewTopSubLine < 0)
		nNewTopSubLine = 0;
	ScrollToSubLine(nNewTopSubLine);
	UpdateSiblingScrollPos();
	return TRUE;
}

int bracetype(TCHAR);

void CCrystalTextView::OnMatchBrace(BOOL bSelect)
{
	POINT ptCursorPos = GetCursorPos();
	POINT ptAnchor = ptCursorPos;
	int nLength = m_pTextBuffer->GetLineLength(ptCursorPos.y);
	LPCTSTR pszText = m_pTextBuffer->GetLineChars(ptCursorPos.y);
	LPCTSTR pszEnd = pszText + ptCursorPos.x;
	bool bAfter = false;
	int nType = 0;
	if (ptCursorPos.x < nLength)
	{
		nType = bracetype(*pszEnd);
		if (nType)
		{
			bAfter = false;
		}
		else if (!nType && ptCursorPos.x > 0)
		{
			nType = bracetype(pszEnd[-1]);
			bAfter = true;
		}
	}
	else if (ptCursorPos.x > 0)
	{
		nType = bracetype(pszEnd[-1]);
		bAfter = true;
	}
	if (nType)
	{
		int nOther = ((nType - 1) ^ 1) + 1;
		if (bAfter)
		{
			if (nOther & 1)
				--pszEnd;
		}
		else
		{
			if (!(nOther & 1))
				++pszEnd;
		}
		int nCount = 0;
		int nComment = 0;
		const LPCTSTR pszOpenComment = m_CurSourceDef->opencomment;
		const LPCTSTR pszCloseComment = m_CurSourceDef->closecomment;
		const LPCTSTR pszCommentLine = m_CurSourceDef->commentline;
		const int nOpenComment = static_cast<int>(_tcslen(pszOpenComment));
		const int nCloseComment = static_cast<int>(_tcslen(pszCloseComment));
		const int nCommentLine = static_cast<int>(_tcslen(pszCommentLine));
		if (nOther & 1)
		{
			for (;;)
			{
				while (pszEnd > pszText)
				{
					LPCTSTR pszTest = pszEnd - nOpenComment;
					--pszEnd;
					if (pszTest >= pszText && !_tcsnicmp(pszTest, pszOpenComment, nOpenComment))
					{
						--nComment;
						pszEnd = pszTest;
						if (--pszEnd < pszText)
							break;
					}
					pszTest = pszEnd - nCloseComment + 1;
					if (pszTest >= pszText && !_tcsnicmp(pszTest, pszCloseComment, nCloseComment))
					{
						++nComment;
						pszEnd = pszTest;
						if (--pszEnd < pszText)
							break;
					}
					if (!nComment)
					{
						pszTest = pszEnd - nCommentLine + 1;
						if (pszTest >= pszText && !_tcsnicmp(pszTest, pszCommentLine, nCommentLine))
							break;
						if (bracetype(*pszEnd) == nType)
						{
							++nCount;
						}
						else if (bracetype(*pszEnd) == nOther)
						{
							if (!nCount)
							{
								ptCursorPos.x = static_cast<int>(pszEnd - pszText);
								if (!bAfter)
									++ptCursorPos.x;
								if (!bSelect)
									ptAnchor = ptCursorPos;
								SetSelection(ptAnchor, ptCursorPos);
								SetCursorPos(ptCursorPos);
								SetAnchor(ptAnchor);
								EnsureCursorVisible();
								return;
							}
							--nCount;
						}
					}
				}
				if (ptCursorPos.y == 0)
					break;
				ptCursorPos.x = m_pTextBuffer->GetLineLength(--ptCursorPos.y);
				pszText = m_pTextBuffer->GetLineChars(ptCursorPos.y);
				pszEnd = pszText + ptCursorPos.x;
			}
		}
		else
		{
			LPCTSTR pszBegin = pszText;
			pszText = pszEnd;
			pszEnd = pszBegin + nLength;
			int nLines = m_pTextBuffer->GetLineCount();
			for (;;)
			{
				while (pszText < pszEnd)
				{
					LPCTSTR pszTest = pszText + nCloseComment;
					if (pszTest <= pszEnd && !_tcsnicmp(pszText, pszCloseComment, nCloseComment))
					{
						--nComment;
						pszText = pszTest;
						if (pszText > pszEnd)
							break;
					}
					pszTest = pszText + nOpenComment;
					if (pszTest <= pszEnd && !_tcsnicmp(pszText, pszOpenComment, nOpenComment))
					{
						++nComment;
						pszText = pszTest;
						if (pszText > pszEnd)
							break;
					}
					if (!nComment)
					{
						pszTest = pszText + nCommentLine;
						if (pszTest <= pszEnd && !_tcsnicmp(pszText, pszCommentLine, nCommentLine))
							break;
						if (bracetype(*pszText) == nType)
						{
							++nCount;
						}
						else if (bracetype(*pszText) == nOther)
						{
							if (!nCount)
							{
								ptCursorPos.x = static_cast<int>(pszText - pszBegin);
								if (!bAfter)
									++ptCursorPos.x;
								if (!bSelect)
									ptAnchor = ptCursorPos;
								SetSelection(ptAnchor, ptCursorPos);
								SetCursorPos(ptCursorPos);
								SetAnchor(ptAnchor);
								EnsureCursorVisible();
								return;
							}
							--nCount;
						}
					}
					++pszText;
				}
				if (ptCursorPos.y >= nLines)
					break;
				ptCursorPos.x = 0;
				nLength = m_pTextBuffer->GetLineLength(++ptCursorPos.y);
				pszBegin = pszText = m_pTextBuffer->GetLineChars(ptCursorPos.y);
				pszEnd = pszBegin + nLength;
			}
		}
	}
}

BOOL CCrystalTextView::CanMatchBrace()
{
	int nLength = m_pTextBuffer->GetLineLength(m_ptCursorPos.y);
	LPCTSTR pszText = m_pTextBuffer->GetLineChars(m_ptCursorPos.y) + m_ptCursorPos.x;
	return m_ptCursorPos.x < nLength && bracetype(*pszText)
		|| m_ptCursorPos.x > 0 && bracetype(pszText[-1]);
}

//BEGIN SW
bool CCrystalTextView::GetWordWrapping() const
{
	return m_bWordWrap;
}

void CCrystalTextView::SetWordWrapping(bool bWordWrap)
{
	m_bWordWrap = bWordWrap;
	if (m_hWnd)
	{
		OnSize();
	}
}

CCrystalParser *CCrystalTextView::SetParser(CCrystalParser *pParser)
{
	CCrystalParser *pOldParser = m_pParser;
	m_pParser = pParser;
	if (pParser)
		pParser->m_pTextView = this;
	return pOldParser;
}

bool CCrystalTextView::IsTextBufferInitialized() const
{
	return m_pTextBuffer && m_pTextBuffer->IsTextBufferInitialized(); 
}

LPCTSTR CCrystalTextView::GetTextBufferEol(int nLine) const
{
	return m_pTextBuffer->GetLineEol(nLine); 
}

// This function assumes selection is in one line
void CCrystalTextView::EnsureSelectionVisible()
{
	//  Scroll vertically
	//BEGIN SW
	int nSubLineCount = GetSubLineCount();
	int nNewTopSubLine = m_nTopSubLine;
	POINT subLinePos;

	CharPosToPoint(m_ptSelStart.y, m_ptSelStart.x, subLinePos);
	subLinePos.y += GetSubLineIndex(m_ptSelStart.y);

	if (nNewTopSubLine <= subLinePos.y - GetScreenLines())
		nNewTopSubLine = subLinePos.y - GetScreenLines() + 1;
	if (nNewTopSubLine > subLinePos.y)
		nNewTopSubLine = subLinePos.y;

	if (nNewTopSubLine < 0)
		nNewTopSubLine = 0;
	if (nNewTopSubLine >= nSubLineCount)
		nNewTopSubLine = nSubLineCount - 1;

	if (!m_bWordWrap)
	{
		// WINMERGE: This line fixes (cursor) slowdown after merges!
		// I don't know exactly why, but propably we are setting
		// m_nTopLine to zero in ResetView() and are not setting to
		// valid value again. Maybe this is a good place to set it?
		m_nTopLine = nNewTopSubLine;
	}
	else
	{
		int dummy;
		GetLineBySubLine(nNewTopSubLine, m_nTopLine, dummy);
	}

	ScrollToSubLine(nNewTopSubLine);

	//  Scroll horizontally
	//BEGIN SW
	// we do not need horizontally scrolling, if we wrap the words
	if (m_bWordWrap)
		return;
	//END SW
	int nActualPos = CalculateActualOffset(m_ptSelStart.y, m_ptSelStart.x);
	int nActualEndPos = CalculateActualOffset(m_ptSelEnd.y, m_ptSelEnd.x);
	int nNewOffset = m_nOffsetChar;
	const int nScreenChars = GetScreenChars();
	const int nBeginOffset = nActualPos - m_nOffsetChar;
	const int nEndOffset = nActualEndPos - m_nOffsetChar;
	const int nSelLen = nActualEndPos - nActualPos;

	// Selection fits to screen, scroll whole selection visible
	if (nSelLen < nScreenChars)
	{
		// Begin of selection not visible 
		if (nBeginOffset > nScreenChars)
		{
			// Scroll so that there is max 5 chars margin at end
			if (nScreenChars - nSelLen > 5)
				nNewOffset = nActualPos + 5 - nScreenChars + nSelLen;
			else
				nNewOffset = nActualPos - 5;
		}
		else if (nBeginOffset < 0)
		{
			// Scroll so that there is max 5 chars margin at begin
			if (nScreenChars - nSelLen >= 5)
				nNewOffset = nActualPos - 5;
			else
				nNewOffset = nActualPos - 5 - nScreenChars + nSelLen;
		}
		// End of selection not visible
		else if (nEndOffset > nScreenChars || nEndOffset < 0)
		{
			nNewOffset = nActualPos - 5;
		}
	}
	else // Selection does not fit screen so scroll to begin of selection
	{
		nNewOffset = nActualPos - 5;
	}

	// Horiz scroll limit to longest line + one screenwidth
	const int nMaxLineLen = GetMaxLineLength ();
	if (nNewOffset >= nMaxLineLen + nScreenChars)
		nNewOffset = nMaxLineLen + nScreenChars - 1;
	if (nNewOffset < 0)
		nNewOffset = 0;

	ScrollToChar(nNewOffset);

	UpdateSiblingScrollPos();
}

// Analyze the first line of file to detect its type
// Mainly it works for xml files
CCrystalTextView::TextDefinition *CCrystalTextView::SetTextTypeByContent(LPCTSTR pszContent)
{
	Captures captures;
	if (FindStringHelper(pszContent, _tcslen(pszContent),
		_T("^\\s*\\<\\?xml\\s+.+?\\?\\>\\s*$"), FIND_REGEXP, captures) >= 0)
	{
		return SetTextType(CCrystalTextView::SRC_XML);
	}
	return NULL;
}
