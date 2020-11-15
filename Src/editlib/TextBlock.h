/**
 * @file TextBlock.h
 *
 * @brief Declaration for TextBlock class.
 *
 */
#pragma once

#include "SyntaxColors.h"

/**
 * @brief Class for recording visual attributes of line fragments in the editor.
 */
class TextBlock
{
private:
	~TextBlock() { } // prevents deletion from outside TextBlock::Array
public:
	int m_nCharPos;
	int m_nColorIndex;
	int m_nBgColorIndex;
	class Array
	{
	private:
		TextBlock *m_pBuf;
	public:
		void const *m_bRecording;
		int m_nBack;
		explicit Array(TextBlock *pBuf)
			: m_pBuf(pBuf), m_bRecording(m_pBuf), m_nBack(0)
		{ }
		~Array() { delete[] m_pBuf; }
		operator TextBlock *() { return m_pBuf; }
		void swap(Array &other)
		{
			eastl::swap(m_pBuf, other.m_pBuf);
			eastl::swap(m_nBack, other.m_nBack);
		}
		__forceinline void DefineBlock(int pos, int colorindex)
		{
			if (m_bRecording)
			{
				if (pos >= m_pBuf[m_nBack].m_nCharPos)
				{
					// If current back has zero length, recycle it
					if (pos > m_pBuf[m_nBack].m_nCharPos)
						m_pBuf[++m_nBack].m_nCharPos = pos;
					m_pBuf[m_nBack].m_nColorIndex = colorindex;
					m_pBuf[m_nBack].m_nBgColorIndex = COLORINDEX_BKGND;
				}
			}
		}
	};

	/**
	  * @brief Parse cookie to seed the parser at the beginning of a text line.
	  */
	class Cookie
	{
	public:
		explicit Cookie(DWORD dwCookie = -1)
			: m_dwCookie(dwCookie), m_dwNesting(0)
		{ }
		void Clear()
		{
			m_dwCookie = -1;
			m_dwNesting = 0;
		}
		BOOL Empty() const
		{
			return m_dwCookie == -1;
		}
		/** Basic parser state information, semantics being up to the parser */
		DWORD m_dwCookie;
		/** Stack of block nesting levels inside JavaScript template strings */
		DWORD m_dwNesting;
	private:
		void operator=(int);
	};
};

#define COMMON_LEXIS 001
#define NATIVE_LEXIS 002
#define SCRIPT_LEXIS 004
#define NATIVE_LEXIS_ONLY _T(STRINGIZE(TOKEN_PASTE2(\, NATIVE_LEXIS)))
#define SCRIPT_LEXIS_ONLY _T(STRINGIZE(TOKEN_PASTE2(\, SCRIPT_LEXIS)))

#define COOKIE_PARSER           0x0F000000UL
#define COOKIE_PARSER_BASIC     0x01000000UL
#define COOKIE_PARSER_CSHARP    0x02000000UL
#define COOKIE_PARSER_JAVA      0x03000000UL
#define COOKIE_PARSER_PERL      0x04000000UL
#define COOKIE_PARSER_PHP       0x05000000UL
#define COOKIE_PARSER_JSCRIPT   0x06000000UL
#define COOKIE_PARSER_VBSCRIPT  0x07000000UL
#define COOKIE_PARSER_CSS       0x08000000UL
#define COOKIE_PARSER_MWSL      0x09000000UL
#define COOKIE_PARSER_GLOBAL    0xF0000000UL

#define SRCOPT_COOKIE(X)        ((X) << 4)
C_ASSERT(COOKIE_PARSER_GLOBAL == COOKIE_PARSER << 4);

// Cookie bits which are shared across parsers
#define COOKIE_TEMPLATE_STRING  0x80

// Tabsize is commented out since we have only GUI setting for it now.
// Not removed because we may later want to have per-filetype settings again.
// See ccrystaltextview.cpp for per filetype table initialization.
struct TextDefinition
{
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

	enum SRCOPTIONS : DWORD
	{
		SRCOPT_INSERTTABS = 0x0001,
		SRCOPT_SHOWTABS = 0x0002,
		//	SRCOPT_BSATBOL			= 0x0004,
		SRCOPT_SELMARGIN = 0x0008,
		SRCOPT_AUTOINDENT = 0x0010,
		SRCOPT_BRACEANSI = 0x0020,
		SRCOPT_BRACEGNU = 0x0040,
		SRCOPT_EOLNDOS = 0x0080,
		SRCOPT_EOLNUNIX = 0x0100,
		SRCOPT_EOLNMAC = 0x0200,
		SRCOPT_FNBRACE = 0x0400,
		//	SRCOPT_WORDWRAP			= 0x0800,
		SRCOPT_HTML_LEXIS = 0x3000,
		SRCOPT_HTML4_LEXIS = 0x1000,
		SRCOPT_HTML5_LEXIS = 0x2000,
	};

	TextType type;
	LPCTSTR name;
	LPCTSTR exts;
	typedef void (TextDefinition::*ParseProc)(TextBlock::Cookie &, LPCTSTR const, int const, int, TextBlock::Array &) const;
	ParseProc ParseLineX;
	DWORD flags;
	// int tabsize;
	LPCTSTR opencomment;
	LPCTSTR closecomment;
	LPCTSTR commentline;

	static TextDefinition const m_StaticSourceDefs[];
	static TextDefinition const *m_SourceDefs;
	static int const m_SourceDefsCount;

	static BOOL ScanParserAssociations(LPTSTR);
	static void DumpParserAssociations(LPTSTR);
	static void FreeParserAssociations();

	static TextDefinition const *GetTextType(LPCTSTR pszExt);
	static TextDefinition const *GetTextType(TextType enuType);
	static TextDefinition const *GetTextType(int index);

private:
	static DWORD ScriptCookie(LPCTSTR lang, DWORD defval = COOKIE_PARSER);
	static TextDefinition::ParseProc ScriptParseProc(DWORD dwCookie);
	void ParseLinePlain(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineUnknown(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineAsp(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineBasic(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineBatch(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineC(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineCSharp(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineRazor(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineCss(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineDcl(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineFortran(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineGo(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineIni(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineInnoSetup(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineIS(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineJava(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineLisp(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineLua(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineNsis(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLinePascal(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLinePerl(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLinePhp(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLinePo(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLinePowerShell(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLinePython(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineRexx(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineRsrc(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineRuby(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineRust(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineSgml(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineSh(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineSiod(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineSql(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineTcl(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineTex(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineVerilog(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineVhdl(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
	void ParseLineXml(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const;
};
