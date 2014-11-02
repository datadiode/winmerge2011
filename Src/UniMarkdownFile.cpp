/**
 *  @file UniMarkdownFile.cpp
 *
 *  @brief Implementation of UniMarkdownFile class.
 */ 
#include "StdAfx.h"
#include "UniMarkdownFile.h"
#include "markdown.h"
#include "unicoder.h"

/**
 * @brief Constructor.
 */
UniMarkdownFile::UniMarkdownFile()
: m_pMarkdown(NULL)
{
}

/**
 * @brief Open the XML file.
 * @param [in] filename Filename (and path) of the file to open.
 * @return true if succeeds, false otherwise.
 */
bool UniMarkdownFile::OpenReadOnly(LPCTSTR filename)
{
	m_depth = 0;
	bool bOpen = UniMemFile::OpenReadOnly(filename);
	if (bOpen)
	{
		// CMarkdown wants octets, so we may need to transcode to UTF8.
		// As transcoding strips the BOM, we must check for it in advance.
		ReadBom();
		if (GetUnicoding() != NONE)
			m_codepage = CP_UTF8;
		// The CMarkdown::File constructor cares about transcoding.
		CMarkdown::File f(
			reinterpret_cast<LPCTSTR>(m_base),
			m_filesize.Lo,
			CMarkdown::File::Mapping | CMarkdown::File::Octets);
		// The file mapping may have been recreated due to transcoding.
		m_data = m_current = m_base = f.pbImage;
		m_filesize.Lo = f.cbImage;
		// Prevent the CMarkdown::File destructor from unmapping the view.
		f.pvImage = NULL;
		m_pMarkdown = new CMarkdown(f);
		Move();
	}
	return bOpen;
}

/**
 * @brief Close the file.
 */
void UniMarkdownFile::Close()
{
	UniMemFile::Close();
	delete m_pMarkdown;
	m_pMarkdown = NULL;
}

/**
 * @brief Collapse whitespace characters from the given line.
 * @param [in, out] Line to handle.
 */
static void CollapseWhitespace(String &line)
{
	int nEatSpace = -2;
	for (int i = line.length() ; i-- ; )
	{
		switch (line[i])
		{
		case '\r':
		case '\n':
		case '\t':
		case ' ':
			if (++nEatSpace < 0 || nEatSpace == 0 && line[i + 1] == '<')
				++nEatSpace;
			line[i] = ' ';
			break;
		case '>':
			if (nEatSpace >= 0 && line[i + 1 + nEatSpace] != '<')
				++nEatSpace;
		default:
			if (nEatSpace > 0)
				line.erase(i + 1, nEatSpace);
			nEatSpace = -1;
			break;
		}
	}
	if (++nEatSpace > 0)
		line.erase(0, nEatSpace);
}

void UniMarkdownFile::Move()
{
	m_bMove = m_pMarkdown->Move();
	const char *first = m_pMarkdown->first;
	const char *ahead = m_pMarkdown->ahead;
	m_transparent = NULL;
	if (first < ahead)
	{
		switch (*++first)
		{
		case '?':
			m_pMarkdown->Scan();
			m_transparent = (LPBYTE)m_pMarkdown->upper;
			break;
		case '!':
			if (first < ahead)
			{
				switch (*++first)
				{
				case '[':
				case '-':
					m_pMarkdown->Scan();
					m_transparent = (LPBYTE)m_pMarkdown->upper;
					break;
				}
			}
			break;
		}
	}
}

String UniMarkdownFile::maketstring(LPCSTR lpd, int len)
{
	bool lossy = false;
	String s;
	ucr::maketstring(s, lpd, len, m_codepage, &lossy);
	if (lossy)
		++m_txtstats.nlosses;
	return s;
}

bool UniMarkdownFile::ReadString(String &line, String &eol, bool *lossy)
{
	line.clear();
	eol.clear();
	int nlosses = m_txtstats.nlosses;
	int nDepth = 0;
	bool bDone = false;
	if (m_current < (LPBYTE)m_pMarkdown->lower)
	{
		line = maketstring((const char *)m_current, static_cast<int>(m_pMarkdown->lower - (const char *)m_current));
		CollapseWhitespace(line);
		bDone = !line.empty();
		m_current = (LPBYTE)m_pMarkdown->lower;
	}
	while (m_current < m_base + m_filesize.Lo && !bDone)
	{
		if (m_current < m_transparent)
		{
			// Leave whitespace alone when inside <? ?>, <!-- -->, or <![*[ ]]>.
			LPBYTE current = m_current;
			if (m_current == (LPBYTE)m_pMarkdown->first)
			{
				nDepth = m_depth;
			}
			while (m_current < m_transparent && *m_current != '\r' && *m_current != '\n')
			{
				++m_current;
			}
			line = maketstring((const char *)current, static_cast<int>(m_current - current));
			if (m_current < m_transparent)
			{
				BYTE eol = *m_current++;
				if (m_current < m_transparent && *m_current == (eol ^ ('\r'^'\n')))
				{
					++m_current;
					++m_txtstats.ncrlfs;
				}
				else
				{
					++(eol == '\r' ? m_txtstats.ncrs : m_txtstats.nlfs);
				}
			}
			bDone = true;
		}
		else
		{
			while (m_current < m_base + m_filesize.Lo && isspace(*m_current))
			{
				BYTE eol = *m_current++;
				if (eol == '\r' || eol == '\n')
				{
					if (m_current < m_base + m_filesize.Lo && *m_current == (eol ^ ('\r'^'\n')))
					{
						++m_current;
						++m_txtstats.ncrlfs;
					}
					else
					{
						++(eol == '\r' ? m_txtstats.ncrs : m_txtstats.nlfs);
					}
				}
			}
			nDepth = m_depth;
			bool bPull = false;
			if (m_bMove && m_pMarkdown->Pull())
			{
				++m_depth;
				bPull = true;
			}
			Move();
			bDone = m_bMove;
			if (!bDone)
			{
				--m_depth;
				bDone = m_pMarkdown->Push();
				if (bPull && bDone)
					Move();
			}
			if (bDone)
			{
				line = maketstring((const char *)m_current, static_cast<int>(m_pMarkdown->first - (const char *)m_current));
				CollapseWhitespace(line);
				m_current = (LPBYTE)m_pMarkdown->first;
			}
			else if (m_current < m_base + m_filesize.Lo)
			{
				bDone = true;
				line = maketstring((const char *)m_current, static_cast<int>(m_base + m_filesize.Lo - m_current));
				CollapseWhitespace(line);
				m_current = m_base + m_filesize.Lo;
			}
			bDone = !line.empty();
		}
	}
	ASSERT(line.find_first_of(_T("\r\n")) == String::npos);
	if (nDepth > 0)
		line.insert(0U, nDepth, _T('\t'));
	if (bDone)
		eol = _T("\n");
	if (lossy)
		*lossy = nlosses != m_txtstats.nlosses;
	return bDone;
}
