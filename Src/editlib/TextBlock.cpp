/**
 * @file TextBlock.cpp
 */
#pragma once

#include "StdAfx.h"
#include "TextBlock.h"
#include "coretools.h"
#include "VersionData.h"

 // Tabsize is commented out since we have only GUI setting for it now.
 // Not removed because we may later want to have per-filetype settings again.
 // See ccrystaltextview.h for table declaration.
TextDefinition const TextDefinition::m_StaticSourceDefs[] =
{
	SRC_PLAIN, _T("Plain"), _T("txt;doc;diz"), &ParseLinePlain, SRCOPT_AUTOINDENT, /*4,*/ _T(""), _T(""), _T(""),
	SRC_ASP, _T("ASP"), _T("asp;aspx"), &ParseLineAsp, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI | SRCOPT_HTML_LEXIS | SRCOPT_COOKIE(COOKIE_PARSER_BASIC), /*2,*/ _T(""), _T(""), _T("'"),
	SRC_AUTOIT, _T("AutoIt"), _T("au3"), &ParseLineAutoIt, SRCOPT_AUTOINDENT, /*4,*/ _T(""), _T(""), _T(";"),
	SRC_BASIC, _T("Basic"), _T("bas;vb;frm;dsm;cls;ctl;pag;dsr"), &ParseLineBasic, SRCOPT_AUTOINDENT, /*4,*/ _T(""), _T(""), _T("\'"),
	SRC_VBSCRIPT, _T("VBScript"), _T("vbs"), &ParseLineBasic, SRCOPT_AUTOINDENT | SRCOPT_COOKIE(COOKIE_PARSER_VBSCRIPT), /*4,*/ _T(""), _T(""), _T("\'"),
	SRC_BATCH, _T("Batch"), _T("bat;btm;cmd"), &ParseLineBatch, SRCOPT_INSERTTABS | SRCOPT_AUTOINDENT, /*4,*/ _T(""), _T(""), _T("rem "),
	SRC_C, _T("C"), _T("c;cc;cpp;cxx;h;hpp;hxx;hm;inl;rh;tlh;tli;xs"), &ParseLineC, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI, /*2,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_CSHARP, _T("C#"), _T("cs"), &ParseLineCSharp, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI, /*2,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_CSHTML, _T("CSHTML"), _T("cshtml"), &ParseLineRazor, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI | SRCOPT_HTML_LEXIS, /*2,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_CSS, _T("CSS"), _T("css"), &ParseLineCss, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI, /*2,*/ _T("/*"), _T("*/"), _T(""),
	SRC_DCL, _T("DCL"), _T("dcl;dcc"), &ParseLineDcl, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI, /*2,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_FORTRAN, _T("Fortran"), _T("f;f90;f9p;fpp;for;f77"), &ParseLineFortran, SRCOPT_INSERTTABS | SRCOPT_AUTOINDENT, /*8,*/ _T(""), _T(""), _T("!"),
	SRC_GO, _T("Go"), _T("go"), &ParseLineGo, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI, /*2,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_HTML, _T("HTML"), _T("html;htm;shtml;ihtml;ssi;stm;stml"), &ParseLineAsp, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI | SRCOPT_HTML_LEXIS | SRCOPT_COOKIE(COOKIE_PARSER), /*2,*/ _T("<!--"), _T("-->"), _T(""),
	SRC_INI, _T("INI"), _T("ini;reg;vbp;isl"), &ParseLineIni, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI | SRCOPT_EOLNUNIX, /*2,*/ _T(""), _T(""), _T(";"),
	SRC_INNOSETUP, _T("InnoSetup"), _T("iss"), &ParseLineInnoSetup, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI, /*2,*/ _T("{"), _T("}"), _T(";"),
	SRC_INSTALLSHIELD, _T("InstallShield"), _T("rul"), &ParseLineIS, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI, /*2,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_JAVA, _T("Java"), _T("java;jav"), &ParseLineJava, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI | SRCOPT_COOKIE(COOKIE_PARSER_JAVA), /*2,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_JSCRIPT, _T("JavaScript"), _T("js;json"), &ParseLineJava, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI | SRCOPT_COOKIE(COOKIE_PARSER_JSCRIPT), /*2,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_JSP, _T("JSP"), _T("jsp;jspx"), &ParseLineAsp, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI | SRCOPT_HTML_LEXIS | SRCOPT_COOKIE(COOKIE_PARSER_JAVA), /*2,*/ _T("<!--"), _T("-->"), _T(""),
	SRC_LISP, _T("AutoLISP"), _T("lsp;dsl"), &ParseLineLisp, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI, /*2,*/ _T(";|"), _T("|;"), _T(";"),
	SRC_LUA, _T("LUA"), _T("lua"), &ParseLineLua, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI, /*2,*/ _T(";|"), _T("|;"), _T(";"),
	SRC_MARKDOWN, _T("Markdown"), _T("markdown;mdown;mkdn;mdwn;mkd;md;Rmd"), &ParseLineAsp, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI | SRCOPT_HTML_LEXIS | SRCOPT_COOKIE(COOKIE_PARSER_MARKDOWN) | COOKIE_PARSER_MARKDOWN, /*2,*/ _T("<!--"), _T("-->"), _T(""),
	SRC_MWSL, _T("MWSL"), _T("mwsl"), &ParseLineAsp, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI | SRCOPT_HTML_LEXIS | SRCOPT_COOKIE(COOKIE_PARSER_MWSL), /*2,*/ _T("<!--"), _T("-->"), _T(""),
	SRC_NSIS, _T("NSIS"), _T("nsi;nsh"), &ParseLineNsis, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI, /*2,*/ _T("/*"), _T("*/"), _T(";"),
	SRC_PASCAL, _T("Pascal"), _T("pas"), &ParseLinePascal, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI, /*2,*/ _T("{"), _T("}"), _T(""),
	SRC_PERL, _T("Perl"), _T("pl;pm;plx"), &ParseLinePerl, SRCOPT_AUTOINDENT | SRCOPT_EOLNUNIX, /*4,*/ _T(""), _T(""), _T("#"),
	SRC_PHP, _T("PHP"), _T("php;php3;php4;php5;phtml"), &ParseLineAsp, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI | SRCOPT_HTML_LEXIS | SRCOPT_COOKIE(COOKIE_PARSER_PHP), /*2,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_PO, _T("PO"), _T("po;pot"), &ParseLinePo, SRCOPT_AUTOINDENT | SRCOPT_EOLNUNIX, /*4,*/ _T(""), _T(""), _T("#"),
	SRC_POWERSHELL, _T("PowerShell"), _T("ps1"), &ParseLinePowerShell, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI, /*2,*/ _T(""), _T(""), _T("#"),
	SRC_PYTHON, _T("Python"), _T("py"), &ParseLinePython, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI, /*2,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_REXX, _T("REXX"), _T("rex;rexx"), &ParseLineRexx, SRCOPT_AUTOINDENT, /*4,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_RSRC, _T("Resources"), _T("rc;dlg;r16;r32;rc2"), &ParseLineRsrc, SRCOPT_AUTOINDENT, /*4,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_RUBY, _T("Ruby"), _T("rb;rbw;rake;gemspec"), &ParseLineRuby, SRCOPT_AUTOINDENT | SRCOPT_EOLNUNIX, /*4,*/ _T(""), _T(""), _T("#"),
	SRC_RUST, _T("Rust"), _T("rs"), &ParseLineRust, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI, /*2,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_SGML, _T("Sgml"), _T("sgm;sgml"), &ParseLineSgml, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI, /*2,*/ _T("<!--"), _T("-->"), _T(""),
	SRC_SH, _T("Shell"), _T("sh;conf"), &ParseLineSh, SRCOPT_INSERTTABS | SRCOPT_AUTOINDENT | SRCOPT_EOLNUNIX, /*4,*/ _T(""), _T(""), _T("#"),
	SRC_SIOD, _T("SIOD"), _T("scm"), &ParseLineSiod, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI, /*2,*/ _T(";|"), _T("|;"), _T(";"),
	SRC_SQL, _T("SQL"), _T("sql"), &ParseLineSql, SRCOPT_AUTOINDENT, /*4,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_TCL, _T("TCL"), _T("tcl"), &ParseLineTcl, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI | SRCOPT_EOLNUNIX, /*2,*/ _T(""), _T(""), _T("#"),
	SRC_TEX, _T("TEX"), _T("tex;sty;clo;ltx;fd;dtx"), &ParseLineTex, SRCOPT_AUTOINDENT, /*4,*/ _T(""), _T(""), _T("%"),
	SRC_VERILOG, _T("Verilog"), _T("v;vh"), &ParseLineVerilog, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI, /*2,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_VHDL, _T("VHDL"), _T("vhd;vhdl;vho"), &ParseLineVhdl, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI, /*2,*/ _T(""), _T(""), _T("--"),
	SRC_XML, _T("XML"), _T("xml;dtd"), &ParseLineXml, SRCOPT_AUTOINDENT | SRCOPT_BRACEANSI, /*2,*/ _T("<!--"), _T("-->"), _T("")
}, *TextDefinition::m_SourceDefs = m_StaticSourceDefs;

int const TextDefinition::m_SourceDefsCount = _countof(m_StaticSourceDefs);

BOOL TextDefinition::ScanParserAssociations(LPTSTR p)
{
	p = EatPrefix(p, _T("version="));
	if (p == NULL)
		return FALSE;
	if (_tcsicmp(p, _T("ignore")) != 0)
	{
		ULARGE_INTEGER version = { 0xFFFFFFFF, 0xFFFFFFFF };
		do
		{
			version.QuadPart <<= 16;
			version.QuadPart |= _tcstoul(p, &p, 10);
		} while (*p && *p++ == _T('.'));
		while (HIWORD(version.HighPart) == 0xFFFF)
			version.QuadPart <<= 16;
		if (VS_FIXEDFILEINFO const *const pVffInfo =
			reinterpret_cast<const VS_FIXEDFILEINFO *>(CVersionData::Load()->Data()))
		{
			if (pVffInfo->dwFileVersionLS != version.LowPart ||
				pVffInfo->dwFileVersionMS != version.HighPart)
			{
				return FALSE;
			}
		}
	}
	p += _tcslen(p) + 1;
	TextDefinition *defs = new TextDefinition[_countof(m_StaticSourceDefs)];
	memcpy(defs, m_StaticSourceDefs, sizeof m_StaticSourceDefs);
	m_SourceDefs = defs;
	while (LPTSTR q = _tcschr(p, _T('=')))
	{
		const size_t r = _tcslen(q);
		*q++ = _T('\0');
		size_t i;
		for (i = 0; i < _countof(m_StaticSourceDefs); ++i)
		{
			TextDefinition &def = defs[i];
			if (_tcsicmp(def.name, p) == 0)
			{
				// Look for additional tags at end of entry
				DWORD srcopt_html_lexis = 0;
				while (LPTSTR t = _tcsrchr(q, _T(':')))
				{
					*t++ = '\0';
					if (m_StaticSourceDefs[i].flags & SRCOPT_HTML_LEXIS)
					{
						if (DWORD dwStartCookie = ScriptCookie(t, 0))
						{
							def.flags &= ~SRCOPT_COOKIE(COOKIE_PARSER);
							def.flags |= dwStartCookie << 4;
						}
						else if (PathMatchSpec(t, _T("HTML4")))
							srcopt_html_lexis |= SRCOPT_HTML4_LEXIS;
						else if (PathMatchSpec(t, _T("HTML5")))
							srcopt_html_lexis |= SRCOPT_HTML5_LEXIS;
					}
				}
				if (srcopt_html_lexis != 0)
				{
					def.flags &= ~SRCOPT_HTML_LEXIS;
					def.flags |= srcopt_html_lexis;
				}
				def.exts = _tcsdup(q);
				break;
			}
		}
		ASSERT(i < _countof(m_StaticSourceDefs));
		p = q + r;
	}
	return TRUE;
}

void TextDefinition::DumpParserAssociations(LPTSTR p)
{
	if (LPCWSTR version = CVersionData::Load()->
		Find(L"StringFileInfo")->First()->Find(L"FileVersion")->Data())
	{
		p += wsprintf(p, _T("version=%ls"), version) + 1;
	}
	for (int i = 0; i < _countof(m_StaticSourceDefs); ++i)
	{
		TextDefinition const &def = m_SourceDefs[i];
		p += wsprintf(p, _T("%-16s= %s"), def.name, def.exts);
		if (m_StaticSourceDefs[i].flags & SRCOPT_HTML_LEXIS)
		{
			switch (def.flags & SRCOPT_HTML_LEXIS)
			{
			case SRCOPT_HTML4_LEXIS:
				p += wsprintf(p, _T(":HTML4"));
				break;
			case SRCOPT_HTML5_LEXIS:
				p += wsprintf(p, _T(":HTML5"));
				break;
			}
			if ((def.flags & COOKIE_PARSER_GLOBAL) != (m_StaticSourceDefs[i].flags & COOKIE_PARSER_GLOBAL))
			{
				switch (def.flags & COOKIE_PARSER_GLOBAL)
				{
				case SRCOPT_COOKIE(COOKIE_PARSER_MWSL):
					p += wsprintf(p, _T(":MWSL"));
					break;
				}
			}
		}
		++p;
	}
	*p = _T('\0');
}

void TextDefinition::FreeParserAssociations()
{
	if (m_SourceDefs != m_StaticSourceDefs)
	{
		size_t i;
		for (i = 0; i < _countof(m_StaticSourceDefs); ++i)
		{
			TextDefinition const &def = m_SourceDefs[i];
			if (def.exts != m_StaticSourceDefs[i].exts)
				free(const_cast<LPTSTR>(def.exts));
		}
		delete[] m_SourceDefs;
		m_SourceDefs = m_StaticSourceDefs;
	}
}

TextDefinition const *TextDefinition::GetTextType(int index)
{
	if (index >= 0 && index < _countof(m_StaticSourceDefs))
		return m_SourceDefs + index;
	return NULL;
}

TextDefinition const *TextDefinition::GetTextType(TextType enuType)
{
	TextDefinition const *def = m_SourceDefs;
	do if (def->type == enuType)
	{
		return def;
	} while (++def < m_SourceDefs + _countof(m_StaticSourceDefs));
	return NULL;
}

TextDefinition const *TextDefinition::GetTextType(LPCTSTR pszExt)
{
	TextDefinition const *def = m_SourceDefs;
	do if (PathMatchSpec(pszExt, def->exts))
	{
		return def;
	} while (++def < m_SourceDefs + _countof(m_StaticSourceDefs));
	return NULL;
}

DWORD TextDefinition::ScriptCookie(LPCTSTR lang, DWORD defval)
{
	if (PathMatchSpec(lang, _T("VB")))
		return COOKIE_PARSER_BASIC;
	if (PathMatchSpec(lang, _T("C#")))
		return COOKIE_PARSER_CSHARP;
	if (PathMatchSpec(lang, _T("JAVA")))
		return COOKIE_PARSER_JAVA;
	if (PathMatchSpec(lang, _T("PERL")))
		return COOKIE_PARSER_PERL;
	if (PathMatchSpec(lang, _T("PHP")))
		return COOKIE_PARSER_PHP;
	if (PathMatchSpec(lang, _T("JS*;JAVAS*")))
		return COOKIE_PARSER_JSCRIPT;
	if (PathMatchSpec(lang, _T("VBS*")))
		return COOKIE_PARSER_VBSCRIPT;
	if (PathMatchSpec(lang, _T("CSS")))
		return COOKIE_PARSER_CSS;
	if (PathMatchSpec(lang, _T("MWSL")))
		return COOKIE_PARSER_MWSL;
	if (PathMatchSpec(lang, _T("x-jquery-tmpl")))
		return 0;
	return defval;
}

TextDefinition::ParseProc TextDefinition::ScriptParseProc(DWORD dwCookie)
{
	switch (dwCookie & COOKIE_PARSER)
	{
	case COOKIE_PARSER_BASIC:
	case COOKIE_PARSER_VBSCRIPT:
		return &ParseLineBasic;
	case COOKIE_PARSER_CSHARP:
		return &ParseLineCSharp;
	case COOKIE_PARSER_JAVA:
	case COOKIE_PARSER_JSCRIPT:
	case COOKIE_PARSER_MWSL:
		return &ParseLineJava;
	case COOKIE_PARSER_PERL:
		return &ParseLinePerl;
	case COOKIE_PARSER_PHP:
		return &ParseLinePhp;
	case COOKIE_PARSER_CSS:
		return &ParseLineCss;
	}
	return &ParseLineUnknown;
}

void TextDefinition::ParseLinePlain(TextBlock::Cookie &dwCookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const
{
}

void TextDefinition::ParseLineUnknown(TextBlock::Cookie &dwCookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const
{
	pBuf.DefineBlock(I + 1, COLORINDEX_FUNCNAME);
}
