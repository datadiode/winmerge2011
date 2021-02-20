///////////////////////////////////////////////////////////////////////////
//  File:    md.cpp
//  Version: 1.1.0.4
//  Updated: 19-Jul-1998
//
//  Copyright:  Ferdinand Prantl, portions by Stcherbatchenko Andrei
//  E-mail:     prantl@ff.cuni.cz
//
//  Markdown syntax highlighing definition (very limited)
//
//  You are free to use or modify this code to the following restrictions:
//  - Acknowledge me somewhere in your about box, simple "Parts of code by.."
//  will be enough. If you can't (or don't want to), contact me personally.
//  - LEAVE THIS HEADER INTACT
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ccrystaltextbuffer.h"

#define DEFINE_BLOCK pBuf.DefineBlock

void TextDefinition::ParseLineMarkdown(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const
{
	if (pszChars[0] == '#')
	{
		DEFINE_BLOCK(0, COLORINDEX_KEYWORD);
	}
}
