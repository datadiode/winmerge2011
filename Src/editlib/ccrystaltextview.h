////////////////////////////////////////////////////////////////////////////
//  File:       ccrystaltextview.h
//  Version:    1.0.0.0
//  Created:    29-Dec-1998
//
//  Author:     Stcherbatchenko Andrei
//  E-mail:     windfall@gmx.de
//
//  Interface of the CCrystalTextView class, a part of Crystal Edit -
//  syntax coloring text editor.
//
//  You are free to use or modify this code to the following restrictions:
//  - Acknowledge me somewhere in your about box, simple "Parts of code by.."
//  will be enough. If you can't (or don't want to), contact me personally.
//  - LEAVE THIS HEADER INTACT
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//  19-Jul-99
//      Ferdinand Prantl:
//  +   FEATURE: see cpps ...
//
//  ... it's being edited very rapidly so sorry for non-commented
//        and maybe "ugly" code ...
////////////////////////////////////////////////////////////////////////////
/** 
 * @file  ccrystaltextview.h
 *
 * @brief Declaration file for CCrystalTextView
 */
#pragma once

#include "crystalparser.h"

////////////////////////////////////////////////////////////////////////////
// Forward class declarations

class CCrystalTextBuffer;
class CUpdateContext;
struct ViewableWhitespaceChars;
class CSyntaxColors;

////////////////////////////////////////////////////////////////////////////
// CCrystalTextView class declaration

//  CCrystalTextView::FindText() flags
enum
{
	FIND_MATCH_CASE = 0x0001,
	FIND_WHOLE_WORD = 0x0002,
	FIND_REGEXP = 0x0004,
	FIND_DIRECTION_UP = 0x0010,
	REPLACE_SELECTION = 0x0100, 
	FIND_NO_WRAP = 0x0200
};

//  CCrystalTextView::UpdateView() flags
enum
{
	UPDATE_HORZRANGE = 0x0001,  //  update horz scrollbar
	UPDATE_VERTRANGE = 0x0002, //  update vert scrollbar
	UPDATE_SINGLELINE = 0x0100,    //  single line has changed
	UPDATE_FLAGSONLY = 0x0200, //  only line-flags were changed

	UPDATE_RESET = 0x1000       //  document was reloaded, update all!
};

/**
 * @brief Class for text view.
 * This class implements class for text viewing. Class implements all
 * the routines we need for showing, selecting text etc. BUT it does
 * not implement text editing. There are classes inherited from this
 * class which implement text editing.
 */
class CCrystalTextView
	: ZeroInit<CCrystalTextView>
	, public OWindow
	, public IDropSource
	, public IDataObject
{
	friend CCrystalParser;

protected:
	//  Search parameters
	bool m_bLastSearch;
	DWORD m_dwLastSearchFlags;
	String m_strLastFindWhat;
	bool m_bCursorHidden;

private:
	//  Line/character dimensions
	int m_nLineHeight, m_nCharWidth;
	void CalcLineCharDim();

	//  Text attributes
	bool m_bViewTabs;
	bool m_bViewEols;
	bool m_bDistinguishEols;
	bool m_bSelMargin;
	bool m_bViewLineNumbers;
	bool m_bSeparateCombinedChars;
	DWORD m_dwFlags;

	CSyntaxColors *m_pColors;

	//BEGIN SW
	/**
	Contains for each line the number of sublines. If the line is not
	wrapped, the value for this line is 1. The value of a line is invalid,
	if it is -1.

	Must create pointer, because contructor uses AFX_ZERO_INIT_OBJECT to
	initialize the member objects. This would destroy a CArray object.
	*/
	std::vector<int> m_panSubLines;
	std::vector<int> m_panSubLineIndexCache;
	int m_nLastLineIndexCalculatedSubLineIndex;
	//END SW

	int m_nMaxLineLength;
	int m_nIdealCharPos;

	bool m_bFocused;
protected:
	POINT m_ptAnchor;
private:
	LOGFONT m_lfBaseFont;
	HFont *m_apFonts[4];

	//  Parsing stuff

	/**  
	This array must be initialized to (DWORD) - 1, code for invalid values (not yet computed).
	We prefer to limit the recomputing delay to the moment when we need to read
	a parseCookie value for drawing.
	GetParseCookie must always be used to read the m_ParseCookies value of a line.
	If the actual value is invalid code, GetParseCookie computes the value, 
	stores it in m_ParseCookies, and returns the new valid value.
	When we edit the text, the parse cookies value may change for the modified line
	and all the lines below (As m_ParseCookies[line i] depends on m_ParseCookies[line (i-1)])
	It would be a loss of time to recompute all these values after each action.
	So we just set all these values to invalid code (DWORD) - 1.
	*/
	std::vector<DWORD> m_ParseCookies;
	DWORD GetParseCookie(int nLineIndex);

	/**
	Pre-calculated line lengths (in characters)
	This array works as the parse cookie Array
	and must be initialized to - 1, code for invalid values (not yet computed).
	for the same reason.
	*/
	std::vector<int> m_pnActualLineLength;

protected:
	bool m_bPreparingToDrag;
	bool m_bDraggingText;
	bool m_bDragSelection, m_bWordSelection, m_bLineSelection;
	UINT_PTR m_nDragSelTimer;

	POINT m_ptDrawSelStart, m_ptDrawSelEnd;

	POINT m_ptCursorPos, m_ptCursorLast;
	POINT m_ptSelStart, m_ptSelEnd;
	void PrepareSelBounds();

	//  Helper functions
	int ExpandChars(LPCTSTR pszChars, int nOffset, int nCount, String &line, int nActualOffset);

	int ApproxActualOffset(int nLineIndex, int nOffset);
	void AdjustTextPoint(POINT & point);
	void DrawLineHelperImpl(HSurface *pdc, POINT &ptOrigin, const RECT &rcClip,
		int nColorIndex, int nBgColorIndex, COLORREF crText, COLORREF crBkgnd,
		LPCTSTR pszChars, int nOffset, int nCount, int &nActualOffset,
		int nBgColorIndexZeorWidthBlock, int cxZeorWidthBlock);
	bool IsInsideSelBlock(POINT ptTextPos);

	bool m_bBookmarkExist;        // More bookmarks
	void ToggleBookmark(int nLine);

public:
	virtual void ResetView();

	// IUnknown
	STDMETHOD(QueryInterface)(REFIID, void **);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();
	// IDropSource
	STDMETHOD(QueryContinueDrag)(BOOL fEscapePressed, DWORD grfKeyState);
	STDMETHOD(GiveFeedback)(DWORD dwEffect);
	// IDataObject
	STDMETHOD(GetData)(LPFORMATETC, LPSTGMEDIUM);
	STDMETHOD(GetDataHere)(LPFORMATETC, LPSTGMEDIUM);
	STDMETHOD(QueryGetData)(LPFORMATETC);
	STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC, LPFORMATETC);
	STDMETHOD(SetData)(LPFORMATETC, LPSTGMEDIUM, BOOL);
	STDMETHOD(EnumFormatEtc)(DWORD, LPENUMFORMATETC *);
	STDMETHOD(DAdvise)(LPFORMATETC, DWORD, LPADVISESINK, LPDWORD);
	STDMETHOD(DUnadvise)(DWORD);
	STDMETHOD(EnumDAdvise)(LPENUMSTATDATA *);
	
	int GetLineCount();
	virtual int ComputeRealLine(int nApparentLine) const;
	virtual void OnUpdateCaret(bool bShowHide);
	bool IsTextBufferInitialized() const;
	LPCTSTR GetTextBufferEol(int nLine) const;

	CSyntaxColors *GetSyntaxColors() { return m_pColors; }
	void SetColorContext(CSyntaxColors *pColors) { m_pColors = pColors; }
	virtual void SetSelection(const POINT &ptStart, const POINT &ptEnd);

	bool IsSelection();
	bool HasFocus() const { return m_bFocused; }
	virtual bool QueryEditable();

	bool HasBookmarks() const { return m_bBookmarkExist; }

	void SelectAll();
	void UpdateCaret(bool bShowHide = false);
	void SetAnchor(const POINT &);

	static void InitSharedResources();
	static void FreeSharedResources();

protected:
	POINT WordToRight(POINT pt);
	POINT WordToLeft(POINT pt);
	bool m_bOvrMode;

	static HImageList *m_pIcons;
	static HBrush *m_pHatchBrush;
	CCrystalTextBuffer *m_pTextBuffer;
	POINT m_ptDraggedTextBegin, m_ptDraggedTextEnd;
	int GetMarginWidth();
	bool IsValidTextPos(const POINT &);
	bool IsValidTextPosX(const POINT &);
	bool IsValidTextPosY(int);

	bool m_bShowInactiveSelection;
	//  [JRT]
	bool m_bDisableDragAndDrop;

	//BEGIN SW
	bool m_bWordWrap;
	CCrystalParser *m_pParser;
	//END SW

	POINT ClientToText(const POINT &);
	POINT TextToClient(const POINT &);
	void InvalidateLines(int nLine1, int nLine2);
	int CalculateActualOffset(int nLineIndex, int nCharIndex, BOOL bAccumulate = FALSE);

	bool IsInsideSelection(const POINT &ptTextPos);
	void GetSelection(POINT &ptStart, POINT &ptEnd);
	void GetFullySelectedLines(int &firstLine, int &lastLine);

	//  Amount of lines/characters that completely fits the client area
	int m_nScreenLines, m_nScreenChars;

	int m_nTopLine, m_nOffsetChar;
	//BEGIN SW
	/**
	The index of the subline that is the first visible line on the screen.
	*/
	int m_nTopSubLine;
	//END SW

	int GetLineHeight();
	//BEGIN SW
	/**
	Returns the number of sublines the given line contains of.
	Allway "1", if word wrapping is disabled.

	@param nLineIndex Index of the line to get the subline count of.

	@return Number of sublines the given line contains of
	*/
	int GetSubLines(int nLineIndex);

	virtual int GetEmptySubLines(int nLineIndex);
	bool IsEmptySubLineIndex(int nSubLineIndex);

	/**
	Converts the given character position for the given line into a point.

	After the call the x-member of the returned point contains the
	character position relative to the beginning of the subline. The y-member
	contains the zero based index of the subline relative to the line, the
	character position was given for.

	@param nLineIndex Zero based index of the line, nCharPos refers to.
	@param nCharPos The character position, the point shoult be calculated for.
	@param charPoint Reference to a point, which should receive the result.

	@return The character position of the beginning of the subline charPoint.y.
	*/
	int CharPosToPoint(int nLineIndex, int nCharPos, POINT &charPoint);

	/**
	Converts the given cursor point for the given line to the character position
	for the given line.

	The y-member of the cursor position specifies the subline inside the given
	line, the cursor is placed on and the x-member specifies the cursor position
	(in character widths) relative to the beginning of that subline.

	@param nLineIndex Zero based index of the line the cursor position refers to.
	@param curPoint Position of the cursor relative to the line in sub lines and
		char widths.

	@return The character position the best matches the cursor position.
	*/
	int CursorPointToCharPos(int nLineIndex, const POINT &curPoint);

	/**
	Converts the given cursor position to a text position.

	The x-member of the subLinePos parameter describes the cursor position in
	char widths relative to the beginning of the subline described by the
	y-member. The subline is the zero based number of the subline relative to
	the beginning of the text buffer.

	<p>
	The returned point contains a valid text position, where the y-member is
	the zero based index of the textline and the x-member is the character
	position inside this line.

	@param subLinePos The sublinebased cursor position
		(see text above for detailed description).
	@param textPos The calculated line and character position that best matches
		the cursor position (see text above for detailed descritpion).
	*/
	void SubLineCursorPosToTextPos(int x, int y, POINT &textPos);

	/**
	Returns the character position relative to the given line, that matches
	the end of the given sub line.

	@param nLineIndex Zero based index of the line to work on.
	@param nSubLineOffset Zero based index of the subline inside the given line.

	@return Character position that matches the end of the given subline, relative
		to the given line.
	*/
	int SubLineEndToCharPos( int nLineIndex, int nSubLineOffset );

	/**
	Returns the character position relative to the given line, that matches
	the start of the given sub line.

	@param nLineIndex Zero based index of the line to work on.
	@param nSubLineOffset Zero based index of the subline inside the given line.

	@return Character position that matches the start of the given subline, relative
		to the given line.
	*/
	int SubLineHomeToCharPos( int nLineIndex, int nSubLineOffset );
	//END SW
	int GetCharWidth();
	int GetMaxLineLength();
	int GetScreenLines();
	int GetScreenChars();
	HFont *GetFont(int nColorIndex = 0);

	virtual int RecalcVertScrollBar(bool bPositionOnly = false);
	virtual int RecalcHorzScrollBar(bool bPositionOnly = false);

	//  Scrolling helpers
	void ScrollToChar(int nNewOffsetChar);
	void ScrollToLine(int nNewTopLine);

	//BEGIN SW
	/**
	Scrolls to the given sub line, so that the sub line is the first visible
	line on the screen.

	@param nNewTopSubLine Index of the sub line to scroll to.
	@param bNoSmoothScroll TRUE to disable smooth scrolling, else FALSE.
	@param bTrackScrollBar TRUE to recalculate the scroll bar after scrolling,
		else FALSE.
	*/
	virtual void ScrollToSubLine(int nNewTopSubLine);
	//END SW

	//  Splitter support
	void UpdateSiblingScrollPos();

	void OnUpdateSibling(const CCrystalTextView *pUpdateSource);

	//BEGIN SW
	/**
	Returns the number of sublines in the whole text buffer.

	The number of sublines is the sum of all sublines of all lines.

	@return Number of sublines in the whole text buffer.
	*/
	int GetSubLineCount();

	/**
	Returns the zero-based subline index of the given line.

	@param nLineIndex Index of the line to calculate the subline index of

	@return The zero-based subline index of the given line.
	*/
	int GetSubLineIndex(int nLineIndex);

	/**
	 * @brief Splits the given subline index into line and sub line of this line.
	 * @param [in] nSubLineIndex The zero based index of the subline to get info about
	 * @param [out] nLine Gets the line number the give subline is included in
	 * @param [out] nSubLine Get the subline of the given subline relative to nLine
	 */
	void GetLineBySubLine(int nSubLineIndex, int &nLine, int &nSubLine);

public:
	int GetLineLength(int nLineIndex) const;
	int GetFullLineLength(int nLineIndex) const;
	int GetViewableLineLength(int nLineIndex) const;
	int GetLineActualLength(int nLineIndex);
	LPCTSTR GetLineChars(int nLineIndex) const;
	DWORD GetLineFlags(int nLineIndex) const;
	void GetText(int nStartLine, int nStartChar, int nEndLine, int nEndChar, String &text);

protected:
	void GetText(const POINT &ptStart, const POINT &ptEnd, String &text)
	{
		GetText(ptStart.y, ptStart.x, ptEnd.y, ptEnd.x, text);
	}

	// Clipboard
	void PutToClipboard(HGLOBAL);

	// Drag-n-drop
	HGLOBAL PrepareDragData();
	virtual DWORD GetDropEffect();
	virtual void OnDropSource(DWORD de);

	virtual COLORREF GetColor(int nColorIndex);
	virtual void GetLineColors(int nLineIndex, COLORREF &crBkgnd, COLORREF &crText) = 0;
	virtual bool GetItalic(int nColorIndex);
	virtual bool GetBold(int nColorIndex);

	void DrawLineHelper(HSurface *, POINT &ptOrigin, const RECT &rcClip,
		int nColorIndex, int nBgColorIndex, COLORREF crText, COLORREF crBkgnd,
		LPCTSTR pszChars, int nOffset, int nCount, int &nActualOffset, POINT ptTextPos,
		int nBgColorIndexZeorWidthBlock, int cxZeorWidthBlock);
	virtual void DrawSingleLine(HSurface *, const RECT &, int nLineIndex);
	virtual void DrawMargin(HSurface *, const RECT &, int nLineIndex, int nLineNumber);

	int GetCharWidthFromChar(LPCTSTR);
	int GetCharWidthFromDisplayableChar(LPCTSTR);

	void AdjustCursorAfterMoveLeft();
	void AdjustCursorAfterMoveRight();

	//BEGIN SW
	// word wrapping

	/**
	Called to wrap the line with the given index into sublines.

	The standard implementation wraps the line at the first non-whitespace after
	an whitespace that exceeds the visible line width (nMaxLineWidth). Override
	this function to provide your own word wrapping.

	<b>Attention:</b> Never call this function directly,
	call WrapLineCached() instead, which calls this method.

	@param nLineIndex The index of the line to wrap

	@param nMaxLineWidth The number of characters a subline of this line should
	not exceed (except whitespaces)

	@param anBreaks An array of integers. Put the positions where to wrap the line
	in that array (its allready allocated). If this pointer is NULL, the function
	has only to compute the number of breaks (the parameter nBreaks).

	@param nBreaks The number of breaks this line has (number of sublines - 1). When
	the function is called, this variable is 0. If the line is not wrapped, this value
	should be 0 after the call.

	@see WrapLineCached()
	*/
	void WrapLine(int nLineIndex, int *anBreaks, int &nBreaks);

	/**
	Called to wrap the line with the given index into sublines.

	Call this method instead of WrapLine() (which is called internal by this
	method). This function uses an internal cache which contains the number
	of sublines for each line, so it has only to call WrapLine(), if the
	cache for the given line is invalid or if the caller wants to get the
	wrap postions (anBreaks != NULL).

	This functions also tests m_bWordWrap -- you can call it even if
	word wrapping is disabled and you will retrieve a valid value.

	@param nLineIndex The index of the line to wrap

	@param nMaxLineWidth The number of characters a subline of this line should
	not exceed (except whitespaces)

	@param anBreaks An array of integers. Put the positions where to wrap the line
	in that array (its allready allocated). If this pointer is NULL, the function
	has only to compute the number of breaks (the parameter nBreaks).

	@param nBreaks The number of breaks this line has (number of sublines - 1). When
	the function is called, this variable is 0. If the line is not wrapped, this value
	should be 0 after the call.

	@see WrapLine()
	@see m_anSubLines
	*/
	void WrapLineCached(int nLineIndex, int *anBreaks, int &nBreaks);

	/**
	Invalidates the cached data for the given lines.

	<b>Remarks:</b> Override this method, if your derived class caches other
	view specific line info, which becomes invalid, when this line changes.
	Call this standard implementation in your overriding.

	@param nLineIndex1 The index of the first line to invalidate.

	@param nLineIndex2 The index of the last line to invalidate. If this value is
	-1 (default) all lines from nLineIndex1 to the end are invalidated.
	*/
	virtual void InvalidateLineCache(int nLineIndex1, int nLineIndex2);
	virtual void InvalidateSubLineIndexCache(int nLineIndex1);
	void InvalidateScreenRect();
	//END SW

	//  Syntax coloring overrides
	struct TEXTBLOCK
	  {
		int m_nCharPos;
		int m_nColorIndex;
		int m_nBgColorIndex;
	  };

	//BEGIN SW
	// function to draw a single screen line
	// (a wrapped line can consist of many screen lines
	void DrawScreenLine(HSurface *pdc, POINT &ptOrigin, const RECT &rcClip,
		TEXTBLOCK *pBuf, int nBlocks, int &nActualItem,
		COLORREF crText, COLORREF crBkgnd,
		LPCTSTR pszChars,
		int nOffset, int nCount, int &nActualOffset, int nLineIndex);
	//END SW

	int MergeTextBlocks(TEXTBLOCK *pBuf1, int nBlocks1, TEXTBLOCK *pBuf2, int nBlocks2, TEXTBLOCK *&pBufMerged);
	virtual int GetAdditionalTextBlocks(int nLineIndex, TEXTBLOCK *&pBuf);

public:
	void GetHTMLLine(int nLineIndex, String &);
	void GetHTMLStyles(String &);
	void GetHTMLAttribute(int nColorIndex, int nBgColorIndex, COLORREF crText, COLORREF crBkgnd, String &);

	void GoToLine(int nLine, bool bRelative);
	DWORD ParseLine(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLinePlain(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLineAsp(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLineBasic(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLineBatch(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLineC(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLineCSharp(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLineCss(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLineDcl(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLineFortran(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLineHtml(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLineIni(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLineInnoSetup(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLineIS(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLineJava(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLineLisp(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLineNsis(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLinePascal(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLinePerl(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLinePhp(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLinePo(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLinePowerShell(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLinePython(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLineRexx(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLineRsrc(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLineRuby(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLineSgml(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLineSh(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLineSiod(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLineSql(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLineTcl(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLineTex(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLineVerilog(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
	DWORD ParseLineXml(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);

	// Attributes
public:
	bool GetViewTabs() const;
	void SetViewTabs(bool);
	void SetViewEols(bool bViewEols, bool bDistinguishEols);
	int GetTabSize() const;
	void SetTabSize(int nTabSize, bool bSeparateCombinedChars);
	bool GetSelectionMargin() const;
	void SetSelectionMargin(bool);
	bool GetViewLineNumbers() const;
	void SetViewLineNumbers(bool);
	void GetFont(LOGFONT &) const;
	void SetFont(const LOGFONT &);
	DWORD GetFlags();
	void SetFlags(DWORD dwFlags);
	//  [JRT]:
	bool GetDisableDragAndDrop() const;
	void SetDisableDragAndDrop(bool);

	//BEGIN SW
	bool GetWordWrapping() const;
	void SetWordWrapping(bool);

	/**
	Sets the Parser to use to parse the file.

	@param pParser Pointer to parser to use. Set to NULL to use no parser.

	@return Pointer to parser used before or NULL, if no parser has been used before.
	*/
	CCrystalParser *SetParser( CCrystalParser *pParser );
	//END SW

	int m_nLastFindWhatLen;
	LPTSTR m_pszMatched;

	enum TextType
	{
		SRC_PLAIN,
		SRC_ASP,
		SRC_BASIC,
		SRC_BATCH,
		SRC_C,
		SRC_CSHARP,
		SRC_CSS,
		SRC_DCL,
		SRC_FORTRAN,
		SRC_HTML,
		SRC_INI,
		SRC_INNOSETUP,
		SRC_INSTALLSHIELD,
		SRC_JAVA,
		SRC_LISP,
		SRC_NSIS,
		SRC_PASCAL,
		SRC_PERL,
		SRC_PHP,
		SRC_PO,
		SRC_POWERSHELL,
		SRC_PYTHON,
		SRC_REXX,
		SRC_RSRC,
		SRC_RUBY,
		SRC_SGML,
		SRC_SH,
		SRC_SIOD,
		SRC_SQL,
		SRC_TCL,
		SRC_TEX,
		SRC_VERILOG,
		SRC_XML
	};

// Tabsize is commented out since we have only GUI setting for it now.
// Not removed because we may later want to have per-filetype settings again.
// See ccrystaltextview.cpp for per filetype table initialization.
	typedef const struct tagTextDefinition
	{
		TextType type;
		LPCTSTR name;
		LPCTSTR exts;
		DWORD (CCrystalTextView::*ParseLineX)(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
		DWORD flags;
		LPCTSTR opencomment;
		LPCTSTR closecomment;
		LPCTSTR commentline;
	} TextDefinition;

	static const DWORD SRCOPT_INSERTTABS = 1;
	static const DWORD SRCOPT_SHOWTABS = 2;
	//static const DWORD SRCOPT_BSATBOL = 4;
	static const DWORD SRCOPT_SELMARGIN = 8;
	static const DWORD SRCOPT_AUTOINDENT = 16;
	static const DWORD SRCOPT_BRACEANSI = 32;
	static const DWORD SRCOPT_BRACEGNU = 64;
	static const DWORD SRCOPT_EOLNDOS = 128;
	static const DWORD SRCOPT_EOLNUNIX = 256;
	static const DWORD SRCOPT_EOLNMAC = 512;
	static const DWORD SRCOPT_FNBRACE = 1024;
	//static const DWORD SRCOPT_WORDWRAP = 2048;

	//  Source type
	TextDefinition *m_CurSourceDef;
	static TextDefinition m_StaticSourceDefs[];
	static TextDefinition *m_SourceDefs;
	virtual TextDefinition *DoSetTextType(TextDefinition *);
	static TextDefinition *GetTextType(LPCTSTR pszExt);
	static void ScanParserAssociations(LPTSTR);
	static void DumpParserAssociations(LPTSTR);
	static void FreeParserAssociations();
	TextDefinition *SetTextType(LPCTSTR pszExt);
	TextDefinition *SetTextType(TextType enuType);
	TextDefinition *SetTextType(TextDefinition *);
	TextDefinition *SetTextTypeByContent(LPCTSTR pszContent);

	// Operations
public:
	void ReAttachToBuffer(CCrystalTextBuffer *);
	void AttachToBuffer(CCrystalTextBuffer *);
	void DetachFromBuffer();

	//  Buffer-view interaction, multiple views
	CCrystalTextBuffer *GetTextBuffer() { return m_pTextBuffer; }
	virtual void UpdateView(CCrystalTextView *pSource, CUpdateContext *pContext, DWORD dwFlags, int nLineIndex = -1);

	//  Attributes
	const POINT &GetCursorPos() const;
	void SetCursorPos(const POINT &ptCursorPos);
	void ShowCursor();
	void HideCursor();

	//  Operations
	void EnsureCursorVisible();
	void EnsureSelectionVisible();

	//  Text search helpers
	BOOL FindText(LPCTSTR pszText, const POINT &ptStartPos,
		DWORD dwFlags, BOOL bWrapSearch, POINT &ptFoundPos);
	BOOL FindTextInBlock(LPCTSTR pszText, const POINT &ptStartPos,
		const POINT &ptBlockBegin, const POINT &ptBlockEnd,
		DWORD dwFlags, BOOL bWrapSearch, POINT &ptFoundPos);
	BOOL HighlightText(const POINT & ptStartPos, int nLength, BOOL bCursorToLeft = FALSE);

	// IME (input method editor)
	void UpdateCompositionWindowPos();
	void UpdateCompositionWindowFont();

	//  Overridable: an opportunity for Auto-Indent, Smart-Indent etc.
	virtual void OnEditOperation(int nAction, LPCTSTR pszText);

public:
	CCrystalTextView(size_t);
	~CCrystalTextView();
#ifdef _DEBUG
	void AssertValidTextPos(const POINT & pt);
#endif
	int GetScrollPos(UINT nBar, UINT nSBCode);

	void OnDraw(HSurface *);

	//  Keyboard handlers
	void MoveLeft(BOOL bSelect);
	void MoveRight(BOOL bSelect);
	void MoveWordLeft(BOOL bSelect);
	void MoveWordRight(BOOL bSelect);
	void MoveUp(BOOL bSelect);
	void MoveDown(BOOL bSelect);
	void MoveHome(BOOL bSelect);
	void MoveEnd(BOOL bSelect);
	void MovePgUp(BOOL bSelect);
	void MovePgDn(BOOL bSelect);
	void MoveCtrlHome(BOOL bSelect);
	void MoveCtrlEnd(BOOL bSelect);
	void OnSize();
	void OnVScroll(UINT nSBCode);
	BOOL OnSetCursor(UINT nHitTest);
	void OnLButtonDown(LPARAM);
	void OnSetFocus();
	void OnHScroll(UINT nSBCode);
	void OnLButtonUp(LPARAM);
	void OnMouseMove(LPARAM);
	void OnTimer(UINT_PTR nIDEvent);
	void OnKillFocus();
	void OnLButtonDblClk(LPARAM);
	void OnRButtonDown(LPARAM);
	void OnEditCopy();
	void OnEditFind();
	void OnEditRepeat();
	BOOL OnMouseWheel(WPARAM, LPARAM);
	void OnPageUp();
	void OnExtPageUp();
	void OnPageDown();
	void OnExtPageDown();
	void OnToggleBookmark(UINT nCmdID);
	void OnGoBookmark(UINT nCmdID);
	void OnClearBookmarks();

	void OnToggleBookmark(); // More bookmarks

	void OnClearAllBookmarks();
	void OnNextBookmark();
	void OnPrevBookmark();

	void ScrollUp();
	void ScrollDown();
	void ScrollLeft();
	void ScrollRight();

	void OnMatchBrace(BOOL bSelect);
	BOOL CanMatchBrace();

	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	void OnDestroy();
};

#ifdef _DEBUG
#define ASSERT_VALIDTEXTPOS(pt) AssertValidTextPos(pt);
#else
#define ASSERT_VALIDTEXTPOS(pt)
#endif
