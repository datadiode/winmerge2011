/** 
 * @file  LineInfo.cpp
 *
 * @brief Implementation of LineInfo class.
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "stdafx.h"
#include "LineInfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/**
 @brief Constructor.
 */
LineInfo::LineInfo()
: m_pcLine(NULL)
, m_nLength(0)
, m_nMax(0)
, m_nEolChars(0)
, m_dwFlags(0)
, m_dwRevisionNumber(0)
{
};

/**
 * @brief Clear item.
 * Frees buffer, sets members to initial values.
 */
void LineInfo::Clear()
{
	delete[] m_pcLine;
	m_pcLine = NULL;
	m_nLength = 0;
	m_nMax = 0;
	m_nEolChars = 0;
	m_dwFlags = 0;
	m_dwRevisionNumber = 0;
}

/**
 * @brief Create a line.
 * @param [in] pszLine Line data.
 * @param [in] nLength Line length.
 */
void LineInfo::Create(LPCTSTR pszLine, int nLength)
{
	Clear();
	Append(pszLine, nLength);
}

/**
 * @brief Append a text to the line.
 * @param [in] pszChars String to append to the line.
 * @param [in] nLength Length of the string to append.
 */
void LineInfo::Append(LPCTSTR pszChars, int nLength)
{
	int nBufNeeded = m_nLength + nLength + 1;
	if (nBufNeeded > m_nMax)
	{
		m_nMax = ALIGN_BUF_SIZE(nBufNeeded);
		ASSERT(m_nMax >= m_nLength + nLength);
		TCHAR *pcNewBuf = new TCHAR[m_nMax];
		if (FullLength() > 0)
			memcpy(pcNewBuf, m_pcLine, sizeof(TCHAR) * (FullLength() + 1));
		delete[] m_pcLine;
		m_pcLine = pcNewBuf;
	}
	memcpy(m_pcLine + m_nLength, pszChars, sizeof(TCHAR) * nLength);
	m_nLength += nLength;
	m_pcLine[m_nLength] = '\0';
	// Did line gain eol ? (We asserted above that it had none at start)
	if (nLength > 1 && IsDosEol(&m_pcLine[m_nLength - 2]))
	{
		m_nEolChars = 2;
	}
	else if (LineInfo::IsEol(m_pcLine[m_nLength - 1]))
	{
		m_nEolChars = 1;
	}
	m_nLength -= m_nEolChars;
	ASSERT(m_nLength + m_nEolChars <= m_nMax);
}

/**
 * @brief Has the line EOL?
 * @return TRUE if the line has EOL bytes.
 */
bool LineInfo::HasEol() const
{
	return m_nEolChars != 0;
}

/**
 * @brief Get line's EOL bytes.
 * @return EOL bytes, or NULL if no EOL bytes.
 */
LPCTSTR LineInfo::GetEol() const
{
	return m_nEolChars != 0 ? m_pcLine + m_nLength : NULL;
}

/**
 * @brief Change line's EOL.
 * @param [in] lpEOL New EOL bytes.
 * @return TRUE if succeeded, FALSE if failed (nothing to change).
 */
bool LineInfo::ChangeEol(LPCTSTR lpEOL)
{
	const int nNewEolChars = static_cast<int>(_tcslen(lpEOL));

	// Check if we really are changing EOL.
	if (nNewEolChars == m_nEolChars)
		if (_tcscmp(m_pcLine + m_nLength, lpEOL) == 0)
			return false;

	int nBufNeeded = m_nLength + nNewEolChars + 1;
	if (nBufNeeded > m_nMax)
	{
		m_nMax = ALIGN_BUF_SIZE (nBufNeeded);
		ASSERT (m_nMax >= nBufNeeded);
		TCHAR *pcNewBuf = new TCHAR[m_nMax];
		if (FullLength() > 0)
			memcpy(pcNewBuf, m_pcLine, sizeof(TCHAR) * (FullLength() + 1));
		delete[] m_pcLine;
		m_pcLine = pcNewBuf;
	}
  
	// copy also the 0 to zero-terminate the line
	memcpy(m_pcLine + m_nLength, lpEOL, sizeof(TCHAR) * (nNewEolChars + 1));
	m_nEolChars = nNewEolChars;
	return true;
}

/**
 * @brief Delete part of the line.
 * @param [in] nStartChar Start position for removal.
 * @param [in] nEndChar End position for removal.
 */
void LineInfo::Delete(int nStartChar, int nEndChar)
{
	if (nEndChar < Length() || m_nEolChars)
	{
		// preserve characters after deleted range by shifting up
		memcpy(m_pcLine + nStartChar, m_pcLine + nEndChar,
			sizeof(TCHAR) * (FullLength() - nEndChar));
	}
	m_nLength -= (nEndChar - nStartChar);
	m_pcLine[FullLength()] = _T('\0');
}

/**
 * @brief Delete line contents from given index to the end.
 * @param [in] Index of first character to remove.
 */
void LineInfo::DeleteEnd(int nStartChar)
{
	m_nLength = nStartChar;
	RemoveEol();
}

/**
 * @brief Remove EOL from line.
 */
void LineInfo::RemoveEol()
{
	if (m_pcLine)
		m_pcLine[m_nLength] = _T('\0');
	m_nEolChars = 0;
}

/**
 * @brief Get line contents.
 * @param [in] index Index of first character to get.
 * @note Make a copy from returned string, as it can get reallocated.
 */
LPCTSTR LineInfo::GetLine(int index) const
{
	return &m_pcLine[index];
}
