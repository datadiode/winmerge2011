/**
 * @file LineInfo.h
 *
 * @brief Declaration for LineInfo class.
 *
 */
#pragma once

#include "SyntaxColors.h"

class CCrystalTextView;

/**
 * @brief Line information.
 * This class presents one line in the editor.
 */
class LineInfo
{
public:
	DWORD m_dwFlags; /**< Line flags. */
	union
	{
		DWORD m_dwRevisionNumber; /**< Edit revision (for edit tracking). */
		int m_nSkippedLines; /**< Number of skipped lines (if LF_SKIPPED). */
	};
	LineInfo();
	void Clear();
	void Create(LPCTSTR pszLine, int nLength);
	void Append(LPCTSTR pszChars, int nLength);
	void Delete(int nStartChar, int nEndChar);
	void DeleteEnd(int nStartChar);
	bool HasEol() const;
	LPCTSTR GetEol() const;
	bool ChangeEol(LPCTSTR lpEOL);
	void RemoveEol();
	LPCTSTR GetLine(int index = 0) const;

	/** @brief Return full line length (including EOL bytes). */
	int FullLength() const { return m_nLength + m_nEolChars; }
	/** @brief Return line length. */
	int Length() const { return m_nLength; }

	/** @brief Is the char an EOL char? */
	static bool IsEol(TCHAR ch)
	{
	  return ch == '\r' || ch == '\n';
	};

	/** @brief Are the characters DOS EOL bytes? */
	static bool IsDosEol(LPCTSTR sz)
	{
		return sz[0] == '\r' && sz[1] == '\n';
	};

	// Parsing stuff
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
			int m_nActualItems;
			Array(TextBlock *pBuf)
				: m_pBuf(pBuf), m_bRecording(m_pBuf), m_nActualItems(0)
			{ }
			~Array() { delete[] m_pBuf; }
			operator TextBlock *() { return m_pBuf; }
			void swap(Array &other)
			{
				std::swap(m_pBuf, other.m_pBuf);
				std::swap(m_nActualItems, other.m_nActualItems);
			}
			__forceinline void DefineBlock(int pos, int colorindex)
			{
				if (m_bRecording)
				{
					if (m_nActualItems == 0 || m_pBuf[m_nActualItems - 1].m_nCharPos <= pos)
					{
						m_pBuf[m_nActualItems].m_nCharPos = pos;
						m_pBuf[m_nActualItems].m_nColorIndex = colorindex;
						m_pBuf[m_nActualItems].m_nBgColorIndex = COLORINDEX_BKGND;
						++m_nActualItems;
					}
				}
			}
		};
		class Cookie
		{
		public:
			Cookie(DWORD dwCookie = -1)
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
			DWORD m_dwCookie;
			DWORD m_dwNesting;
		private:
			void operator=(int);
		};
		typedef void (CCrystalTextView::*ParseProc)(Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, Array &pBuf);
	};

	TextBlock::Cookie m_cookie;

private:
	TCHAR *m_pcLine; /**< Line data. */
	int m_nMax; /**< Allocated space for line data. */
	int m_nLength; /**< Line length (without EOL bytes). */
	int m_nEolChars; /**< # of EOL bytes. */
};
