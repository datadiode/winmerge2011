/**
 * @file TextBlock.h
 *
 * @brief Declaration for TextBlock class.
 *
 */
#pragma once

#include "SyntaxColors.h"

class CCrystalTextBuffer;

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
		int m_nActualItems;
		explicit Array(TextBlock *pBuf)
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
	typedef void (CCrystalTextBuffer::*ParseProc)(Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, Array &pBuf);
};
