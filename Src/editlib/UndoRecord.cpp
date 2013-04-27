/** 
 * @file  UndoRecord.cpp
 *
 * @brief Implementation of UndoRecord struct.
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "stdafx.h"
#include "UndoRecord.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void UndoRecord::Clone(const UndoRecord &src)
{
	m_dwFlags = src.m_dwFlags;
	m_ptStartPos = src.m_ptStartPos;
	m_ptEndPos = src.m_ptEndPos;
	m_nAction = src.m_nAction;
	SetText(src.GetText(), src.GetTextLength());
}

void UndoRecord::SetText(LPCTSTR pszText, int nLength)
{
	FreeText();
	if (nLength)
	{
		if (nLength > 1)
		{
			m_pszText = static_cast<TextBuffer *>(
				malloc(sizeof(TextBuffer) + nLength * sizeof(TCHAR)));
			m_pszText->size = nLength;
			memcpy(m_pszText->data, pszText, nLength * sizeof(TCHAR));
			m_pszText->data[nLength] = _T('?'); // debug sentinel
		}
		else
		{
			m_szText[0] = pszText[0];
		}
	}
}

void UndoRecord::FreeText()
{
	// See the m_szText/m_pszText definition
	// Check if m_pszText is a pointer by removing bits having
	// possible char value
	if (reinterpret_cast<UINT_PTR>(m_pszText) > 0xFFFF)
		free(m_pszText);
	m_pszText = NULL;
}

bool UndoRecord::VerifyText(const String &text) const
{
	// Verify recorded text against passed-in text, regardless of the former's
	// EOL style, but assuming the latter to follow Unix EOL style.
	LPCTSTR p = text.c_str();
	LPCTSTR r = p + text.length();
	LPCTSTR p2 = GetText();
	LPCTSTR r2 = p2 + GetTextLength();
	while (size_t n = r - p)
	{
		LPCTSTR q = wmemchr(p, L'\n', n);
		if (q == NULL)
			q = p + n;
		size_t delta = q - p;
		if (delta > static_cast<size_t>(r2 - p2))
			break;
		if (memcmp(p, p2, delta * sizeof(TCHAR)) != 0)
			break;
		p2 += delta;
		p = q;
		if (p < r)
		{
			if (p2 == r2)
				break;
			TCHAR c = *p2;
			if (c != '\r' && c != '\n')
				break;
			if (++p2 < r2 && c == '\r' && *p2 == '\n')
				++p2;
			++p;
		}
	}
	return p == r;
}
