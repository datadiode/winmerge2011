////////////////////////////////////////////////////////////////////////////
//  File:       ccrystaltextbuffer.h
//  Version:    1.0.0.0
//  Created:    29-Dec-1998
//
//  Author:     Stcherbatchenko Andrei
//  E-mail:     windfall@gmx.de
//
//  Interface of the CCrystalTextBuffer class, a part of Crystal Edit -
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
 * @file  ccrystaltextbuffer.h
 *
 * @brief Declaration file for CCrystalTextBuffer.
 */
#pragma once

#include "UndoRecord.h"
#include "ccrystaltextview.h"

enum LINEFLAGS : DWORD
{
	LF_BOOKMARK_FIRST		= 0x00000001UL,
	LF_EXECUTION			= 0x00010000UL,
	LF_BREAKPOINT			= 0x00020000UL,
	LF_COMPILATION_ERROR	= 0x00040000UL,
	LF_BOOKMARKS			= 0x00080000UL,
	LF_INVALID_BREAKPOINT	= 0x00100000UL,
};

#define LF_BOOKMARK(id)     (LF_BOOKMARK_FIRST << id)

enum SRCOPTIONS : DWORD
{
	SRCOPT_INSERTTABS		= 0x0001,
	SRCOPT_SHOWTABS			= 0x0002,
//	SRCOPT_BSATBOL			= 0x0004,
	SRCOPT_SELMARGIN		= 0x0008,
	SRCOPT_AUTOINDENT		= 0x0010,
	SRCOPT_BRACEANSI		= 0x0020,
	SRCOPT_BRACEGNU			= 0x0040,
	SRCOPT_EOLNDOS			= 0x0080,
	SRCOPT_EOLNUNIX			= 0x0100,
	SRCOPT_EOLNMAC			= 0x0200,
	SRCOPT_FNBRACE			= 0x0400,
//	SRCOPT_WORDWRAP			= 0x0800,
	SRCOPT_HTML4_LEXIS		= 0x1000,
	SRCOPT_HTML5_LEXIS		= 0x2000,
};

enum CRLFSTYLE
{
	CRLF_STYLE_AUTOMATIC = -1,
	CRLF_STYLE_DOS = 0,
	CRLF_STYLE_UNIX = 1,
	CRLF_STYLE_MAC = 2,
	CRLF_STYLE_MIXED = 3
};

enum EDITMODE
{
	EDIT_MODE_INSERT = 0,
	EDIT_MODE_OVERWRITE = 1,
	EDIT_MODE_READONLY = 2
};

enum
{
	CE_ACTION_UNKNOWN = 0,
	CE_ACTION_PASTE = 1,
	CE_ACTION_DELSEL = 2,
	CE_ACTION_CUT = 3,
	CE_ACTION_TYPING = 4,
	CE_ACTION_BACKSPACE = 5,
	CE_ACTION_INDENT = 6,
	CE_ACTION_DRAGDROP = 7,
	CE_ACTION_REPLACE = 8,
	CE_ACTION_DELETE = 9,
	CE_ACTION_AUTOINDENT = 10,
	CE_ACTION_AUTOCOMPLETE = 11,
	CE_ACTION_AUTOEXPAND = 12,
	CE_ACTION_LOWERCASE = 13,
	CE_ACTION_UPPERCASE = 14,
	CE_ACTION_SWAPCASE = 15,
	CE_ACTION_CAPITALIZE = 16,
	CE_ACTION_SENTENCIZE = 17,
	CE_ACTION_RECODE = 18,
	CE_ACTION_SPELL = 19
	//  ...
	//  Expandable: user actions allowed
};

/////////////////////////////////////////////////////////////////////////////
// CUpdateContext class

class CUpdateContext
{
public:
	virtual void RecalcPoint(POINT & ptPoint) = 0;
};

/////////////////////////////////////////////////////////////////////////////
// CCrystalTextBuffer command target

class CCrystalTextBuffer
{
public:
	DWORD m_dwCurrentRevisionNumber;
	DWORD m_dwRevisionNumberOnSave;

protected:
#ifdef _DEBUG
	bool m_bInit;
#endif
	bool m_bReadOnly;
	bool m_bModified;
	bool m_bInsertTabs;
	bool m_bSeparateCombinedChars;
	CRLFSTYLE m_nCRLFMode;
	DWORD m_dwFlags;
	int m_nTabSize;
	int m_nMaxLineLength;
	int m_nParseCookieCount;
	int m_nModeLineOverrides;

	enum
	{
		UNDO_INSERT = 0x0001,
		UNDO_BEGINGROUP = 0x0100
	};

	class CInsertContext : public CUpdateContext
	{
	public:
		POINT m_ptStart, m_ptEnd;
		virtual void RecalcPoint(POINT & ptPoint);
	};

	class CDeleteContext : public CUpdateContext
	{
	public:
		POINT m_ptStart, m_ptEnd;
		virtual void RecalcPoint(POINT & ptPoint);
	};

	std::vector<LineInfo> m_aLines; /**< Text lines. */

	// Undo
	virtual const UndoRecord &GetUndoRecord(stl_size_t i) const = 0;
	virtual stl_size_t GetUndoRecordCount() const = 0;

	stl_size_t m_nUndoPosition;
	stl_size_t m_nSyncPosition;
	BOOL m_bUndoGroup, m_bUndoBeginGroup;

	//BEGIN SW
	/** Position where the last change was made. */
	POINT m_ptLastChange;
	//END SW

	// Connected views
	std::list<CCrystalTextView *> m_lpViews;

	// Helper methods
	void InsertLine(LPCTSTR pszLine, int nLength, int nPosition = -1);
	void AppendLine(int nLineIndex, LPCTSTR pszChars, int nLength);
	void MoveLine(int line1, int line2, int newline1);

	// Implementation
	POINT InternalInsertText(CCrystalTextView *, int nLine, int nPos, LPCTSTR pszText, int cchText);
	void InternalDeleteText(CCrystalTextView *, int nStartLine, int nStartPos, int nEndLine, int nEndPos);
	String StripTail(int i, int bytes);

	// Operations
public:
	// Construction/destruction code
	CCrystalTextBuffer();
	~CCrystalTextBuffer();

	// Basic functions
	BOOL InitNew(CRLFSTYLE nCrlfStyle = CRLF_STYLE_DOS);
	void FreeAll();

	// 'Dirty' flag
	void SetModified(bool bModified = true) { m_bModified = bModified; }
	bool IsModified() const { return m_bModified; }

	// 'Unsaved' indicator
	void SetUnsaved() { m_nSyncPosition = std::vector<UndoRecord>::npos; }
	bool IsUnsaved() const { return m_nSyncPosition != m_nUndoPosition; }

	// Connect/disconnect views
	void AddView(CCrystalTextView *);
	void RemoveView(CCrystalTextView *);

	// Text access functions
	int GetLineCount() const
	{
		return static_cast<int>(m_aLines.size());
	}
	LPCTSTR GetLineEol(int nLine) const;
	bool ChangeLineEol(int nLine, LPCTSTR lpEOL);
	const LineInfo &GetLineInfo(int nLine) const;
	// number of characters in line (excluding any trailing eol characters)
	int GetLineLength(int nLine) const
	{
		return GetLineInfo(nLine).Length();
	}
	// number of characters in line (including any trailing eol characters)
	int GetFullLineLength(int nLine) const // including EOLs
	{
		return GetLineInfo(nLine).FullLength();
	}
	LPCTSTR GetLineChars(int nLine) const
	{
		return GetLineInfo(nLine).GetLine();
	}
	DWORD GetLineFlags(int nLine) const
	{
		return GetLineInfo(nLine).m_dwFlags;
	}
	int FindLineWithFlag(DWORD dwFlag, int nLine = -1) const;
	void SetLineFlags(int nLine, DWORD dwFlags);
	void GetText(int nStartLine, int nStartChar, int nEndLine, int nEndChar,
			String &text, LPCTSTR pszCRLF = NULL) const;

	// Attributes
	CRLFSTYLE GetCRLFMode() const;
	void SetCRLFMode(CRLFSTYLE nCRLFMode);
	/// Adjust all the lines in the buffer to the buffer default EOL Mode
	bool applyEOLMode();
	LPCTSTR GetDefaultEol() const;
	static LPCTSTR GetStringEol(CRLFSTYLE nCRLFMode);
	bool GetReadOnly() const;
	void SetReadOnly(bool bReadOnly = true);

	// Text modification functions
	virtual POINT InsertText(CCrystalTextView *pSource, int nLine, int nPos, LPCTSTR pszText, int cchText, int nAction = CE_ACTION_UNKNOWN, BOOL bHistory = TRUE) = 0;
	virtual void DeleteText(CCrystalTextView *pSource, int nStartLine, int nStartPos, int nEndLine, int nEndPos, int nAction = CE_ACTION_UNKNOWN, BOOL bHistory = TRUE) = 0;

	// Undo/Redo
	bool CanUndo() const;
	bool CanRedo() const;
	virtual bool Undo(POINT &ptCursorPos) = 0;
	virtual bool Redo(POINT &ptCursorPos) = 0;

	//  Undo grouping
	void BeginUndoGroup(BOOL bMergeWithPrevious = FALSE);
	void FlushUndoGroup(CCrystalTextView *pSource);

	//BEGIN SW
	/**
	Returns the position where the last changes where made.
	*/
	POINT GetLastChangePos() const;
	//END SW
	void RestoreLastChangePos(POINT pt);

	// Browse undo sequence
	stl_size_t GetUndoActionCode(int &nAction, stl_size_t pos = 0) const;
	stl_size_t GetRedoActionCode(int &nAction, stl_size_t pos = 0) const;

	int GetParseCookieCount() const;
	void SetParseCookieCount(int);

	// Notify all connected views about changes in text
	void UpdateViews(CCrystalTextView *pSource, CUpdateContext *pContext,
		DWORD dwUpdateFlags, int nLineIndex = -1);

	// Tabs/space inserting
	bool GetInsertTabs() const;
	void SetInsertTabs(bool bInsertTabs);

	DWORD GetFlags() const;
	void SetFlags(DWORD dwFlags);

	// Tabbing
	int GetTabSize() const;
	bool GetSeparateCombinedChars() const;
	void SetTabSize(int nTabSize, bool bSeparateCombinedChars);

	void ParseModeLine();
	void ParseEditorConfig(LPCTSTR path);

	int GetCharWidthFromChar(LPCTSTR pch) const;
	int GetLineActualLength(int nLineIndex);
	int GetMaxLineLength(bool bRecalc);

	// More bookmarks
	int FindNextBookmarkLine(int nCurrentLine, int nDirection) const;

	static int FindStringHelper(
		LPCTSTR pchFindWhere, UINT cchFindWhere,
		LPCTSTR pchFindWhat, DWORD dwFlags,
		Captures &ovector);

	enum TextType
	{
		SRC_PLAIN,
		SRC_ASP,
		SRC_BASIC,
		SRC_VBSCRIPT,
		SRC_BATCH,
		SRC_C,
		SRC_CSHARP,
		SRC_CSHTML,
		SRC_CSS,
		SRC_DCL,
		SRC_FORTRAN,
		SRC_GO,
		SRC_HTML,
		SRC_INI,
		SRC_INNOSETUP,
		SRC_INSTALLSHIELD,
		SRC_JAVA,
		SRC_JSCRIPT,
		SRC_JSP,
		SRC_LISP,
		SRC_LUA,
		SRC_MWSL,
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
		SRC_RUST,
		SRC_SGML,
		SRC_SH,
		SRC_SIOD,
		SRC_SQL,
		SRC_TCL,
		SRC_TEX,
		SRC_VERILOG,
		SRC_VHDL,
		SRC_XML
	};

// Tabsize is commented out since we have only GUI setting for it now.
// Not removed because we may later want to have per-filetype settings again.
// See ccrystaltextview.cpp for per filetype table initialization.
	typedef struct tagTextDefinition
	{
		TextType type;
		LPCTSTR name;
		LPCTSTR exts;
		TextBlock::ParseProc ParseLineX;
		DWORD flags;
		LPCTSTR opencomment;
		LPCTSTR closecomment;
		LPCTSTR commentline;
	} const TextDefinition;

	//  Source type
	TextDefinition *m_CurSourceDef;
	static TextDefinition m_StaticSourceDefs[];
	static TextDefinition *m_SourceDefs;
	static TextDefinition *GetTextType(LPCTSTR pszExt);
	static TextDefinition *GetTextType(int index);
	static BOOL ScanParserAssociations(LPTSTR);
	static void DumpParserAssociations(LPTSTR);
	static void FreeParserAssociations();
	TextDefinition *SetTextType(LPCTSTR pszExt);
	TextDefinition *SetTextType(TextType enuType);
	TextDefinition *SetTextType(TextDefinition *);
	TextDefinition *SetTextTypeByContent(LPCTSTR pszContent);

	void InitParseCookie();

	/**
	 * @brief Compute the parse cookie value of a line.
	 *
	 * @note Once computed, the value is cached in LineInfo::m_cookie.
	 */
	TextBlock::Cookie GetParseCookie(int nLineIndex);

	void ParseLine(TextBlock::Cookie &cookie, int nLineIndex, TextBlock::Array &pBuf);

private:
	static DWORD ScriptCookie(LPCTSTR lang, DWORD defval = COOKIE_PARSER);
	static TextBlock::ParseProc ScriptParseProc(DWORD dwCookie);
	void ParseLinePlain(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineUnknown(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineAsp(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineBasic(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineBatch(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineC(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineCSharp(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineRazor(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineCss(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineDcl(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineFortran(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineGo(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineIni(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineInnoSetup(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineIS(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineJava(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineLisp(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineLua(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineNsis(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLinePascal(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLinePerl(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLinePhp(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLinePo(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLinePowerShell(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLinePython(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineRexx(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineRsrc(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineRuby(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineRust(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineSgml(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineSh(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineSiod(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineSql(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineTcl(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineTex(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineVerilog(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineVhdl(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineXml(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
};
