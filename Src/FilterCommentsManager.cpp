/////////////////////////////////////////////////////////////////////////////
//    License (GPLv2+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or (at
//    your option) any later version.
//    
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
/////////////////////////////////////////////////////////////////////////////
/** 
 * @file  FilterCommentsManager.cpp
 *
 * @brief FilterCommentsManager class implementation.
 */

#include "StdAfx.h"
#include "coretools.h"
#include "paths.h"
#include "Environment.h"
#include "FilterCommentsManager.h"
#include "DIFF.H"
#include "DiffList.H"
#include "constants.h"
#include "OptionsMgr.h"

/**
@brief FilterCommentsManager constructor, which reads the INI file data
		and populates the mapped member variable m_FilterCommentsSetByFileType.
@param[in]  Optional full INI file name, to include path.
*/
FilterCommentsManager::FilterCommentsManager()
{
	String sFileName = COptionsMgr::Get(OPT_SUPPLEMENT_FOLDER);
	sFileName = paths_ConcatPath(sFileName, _T("IgnoreSectionMarkers.ini"));
	TCHAR buffer[1024];
	if (!GetPrivateProfileSection(_T("FileTypes"), buffer, _countof(buffer), sFileName.c_str()))
	{
		static const TCHAR Markers1[] =
			_T("StartMarker=/*\0EndMarker=*/\0InlineMarker=//\0");
		static const TCHAR FileTypes[] =
			_T("java=cs=cpp=c=h=cxx=cc=js=jsl=tli=tlh=rc=1\0");
		if (WritePrivateProfileSection(_T("1"), Markers1, sFileName.c_str()) &&
			WritePrivateProfileSection(_T("FileTypes"), FileTypes, sFileName.c_str()))
		{
			memcpy(buffer, FileTypes, sizeof FileTypes);
		}
	}
	String link;
	FilterCommentsSet filtercommentsset;
	LPTSTR p = buffer;
	LPTSTR q;
	while (int r = lstrlen(q = StrRChr(p, NULL, _T('='))))
	{
		if (link != ++q)
		{
			link = q;
			TCHAR marker[40];
			GetPrivateProfileString(q, _T("StartMarker"), NULL, marker, _countof(marker), sFileName.c_str());
			filtercommentsset.StartMarker = T2A(marker);
			GetPrivateProfileString(q, _T("EndMarker"), NULL, marker, _countof(marker), sFileName.c_str());
			filtercommentsset.EndMarker = T2A(marker);
			GetPrivateProfileString(q, _T("InlineMarker"), NULL, marker, _countof(marker), sFileName.c_str());
			filtercommentsset.InlineMarker = T2A(marker);
		}
		while (LPTSTR q = StrChr(p, _T('=')))
		{
			*q = _T('\0');
			m_FilterCommentsSetByFileType[p] = filtercommentsset;
			p = q + 1;
		}
		p = q + r;
	}
}

/**
	@brief Get comment markers that are associated with this file type.
		If there are no comment markers associated with this file type,
		then return an empty set.
	@param[in]  The file name extension. Example:("cpp", "java", "c", "h")
				Must be lower case.
*/
const FilterCommentsSet &FilterCommentsManager::GetSetForFileType(const String& FileTypeName) const
{
	std::map<String, FilterCommentsSet>::const_iterator pSet =
		m_FilterCommentsSetByFileType.find(FileTypeName);
	return pSet != m_FilterCommentsSetByFileType.end() ? pSet->second : m_empty;
}

/**
 * @brief Find comment marker in string
 * @param [in]	target string to search
 * @param [in]	marker marker to search for
 * @param [in]	quote_flag controls inclusion of portions enclosed in
 *				apostrophes or quotation marks
 * @return Points to marker, or to EOL if no marker is present
 */
const char *FilterCommentsSet::FindCommentMarker(const char *target, const std::string &marker, char quote_flag)
{
	C_ASSERT(('"' & 0x3F) == '"');
	C_ASSERT(('\'' & 0x3F) == '\'');
	char quote = '\0';
	const char *marker_ptr = marker.c_str();
	size_t marker_len = marker.length();
	while (const char c = *target)
	{
		if (c == '\r' || c == '\n')
			break;
		if (quote <= quote_flag && memcmp(target, marker_ptr, marker_len) == 0)
			break;
		if (c == '\\')
			quote ^= 0x40;
		else if ((c == '"' || c == '\'') && (quote == '\0' || quote == c))
			quote ^= c;
		else
			quote &= 0x3F;
		++target;
	}
	return target;
}

/**
	@brief Performs post-filtering, by setting comment blocks to trivial
	@param [in]  StartPos			- First line in diff block.
	@param [in]  QtyLinesInBlock	- Number of lines in diff block.
	@param [in]  inf				- DiffUtils file_data structure.
	@return		Whether the diff is a candidate for trivialization.
*/
OP_TYPE FilterCommentsSet::PostFilter(int StartPos, int QtyLinesInBlock, const file_data *inf) const
{
	const char **const linebuf = inf->linbuf;
	const int linbuf_base = inf->linbuf_base;
	const int valid_lines = inf->valid_lines;
	int EndPos = StartPos + QtyLinesInBlock;
	// First look within the bounds of the diff block whether there is any
	// content outside inline comments.
	int lowerOutsideInlineCommentLine = StartPos;
	int upperOutsideInlineCommentLine = EndPos;
	const char *lowerOutsideInlineComment = linebuf[lowerOutsideInlineCommentLine];
	const char *upperOutsideInlineComment = linebuf[upperOutsideInlineCommentLine];
	if (!InlineMarker.empty())
	{
		std::swap(lowerOutsideInlineComment, upperOutsideInlineComment);
		std::swap(lowerOutsideInlineCommentLine, upperOutsideInlineCommentLine);
		for (int i = StartPos ; i < EndPos ; ++i)
		{
			const char *lower = linebuf[i];
			const char *upper = FindCommentMarker(lower, InlineMarker);
			if (upper > lower)
			{
				if (lowerOutsideInlineComment > lower)
				{
					lowerOutsideInlineComment = lower;
					lowerOutsideInlineCommentLine = i;
				}
				if (upperOutsideInlineComment < upper)
				{
					upperOutsideInlineComment = upper;
					upperOutsideInlineCommentLine = i;
				}
			}
			// Never trivialize when inline and block markers might interfere.
			char c = *upper;
			if (c != '\0' && c != '\r' && c != '\n')
			{
				// NB: A StartMarker appearing to the right of an inline marker doesn't count.
				if (FindCommentMarker(lower, StartMarker, quote_flag_ignore) < upper)
					return OP_NONE;
				c = *FindCommentMarker(lower, EndMarker, quote_flag_ignore);
				if (c != '\0' && c != '\r' && c != '\n')
					return OP_NONE;
			}
		}
	}
	// If no content outside inline comments was found, trivialize the diff.
	if (lowerOutsideInlineComment >= upperOutsideInlineComment)
	{
		return OP_TRIVIAL;
	}
	// If any content outside inline comments was found, travel forth and back
	// to check for block comment markers. A StartMarker right at the beginning
	// of the diff is a special case that puts an early end to the journey.
	int i = lowerOutsideInlineCommentLine;
	const char *p, *q;
	while (i < valid_lines)
	{
		p = linebuf[i++];
		q = FindCommentMarker(p, EndMarker);
		char c = *q;
		if (c != '\0' && c != '\r' && c != '\n')
		{
			q += EndMarker.length();
			break;
		}
	}
	if (q < upperOutsideInlineComment)
	{
		// Even though we return OP_NONE here, CDiffWrapper::PostFilter()
		// may still trivialize the diff depending on executed code path.
		return OP_NONE;
	}
	q = linebuf[StartPos];
	if (FindCommentMarker(q, StartMarker) == q)
	{
		char c = *q;
		if (c != '\0' && c != '\r' && c != '\n')
			return OP_TRIVIAL;
	}
	i = StartPos;
	while (i > linbuf_base)
	{
		p = linebuf[--i];
		const char *const r = FindCommentMarker(p, InlineMarker);
		do
		{
			bool endMarkerFound = false;
			q = FindCommentMarker(p, EndMarker, quote_flag_ignore);
			char c = *q;
			if (c != '\0' && c != '\r' && c != '\n')
			{
				endMarkerFound = true;
				q += EndMarker.length();
			}
			else
			{
				q = p;
			}
			p = FindCommentMarker(q, StartMarker);
			// NB: A StartMarker appearing to the right of an inline marker doesn't count.
			if (p < r)
			{
				return OP_TRIVIAL;
			}
			if (endMarkerFound)
			{
				return OP_NONE;
			}
		} while (p != q);
	}
	return OP_NONE;
}
