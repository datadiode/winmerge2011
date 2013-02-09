/** 
 * @file  codepage_detect.cpp
 *
 * @brief Deducing codepage from file contents, when we can
 *
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "codepage_detect.h"
#include "unicoder.h"
#include "codepage.h"
#include "charsets.h"
#include "markdown.h"
#include "FileTextEncoding.h"
#include "Utf8FileDetect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/** @brief Buffer size used in this file. */
static const int BufSize = 4 * 1024;

/**
 * @brief Prefixes to handle when searching for codepage names
 * NB: prefixes ending in '-' must go first!
 */
static const char *f_wincp_prefixes[] =
{
	"WINDOWS-", "WINDOWS", "CP-", "CP", "MSDOS-", "MSDOS"
};

/**
 * @brief Remove prefix from the text.
 * @param [in] text Text to process.
 * @param [in] prefix Prefix to remove.
 * @return Text without the prefix.
 */
static const char *EatPrefix(const char *text, const char *prefix)
{
	size_t len = strlen(prefix);
	if (len)
		if (_memicmp(text, prefix, len) == 0)
			return text + len;
	return 0;
}

/**
 * @brief Try to to match codepage name from codepages module, & watch for f_wincp_prefixes aliases
 */
static int
FindEncodingIdFromNameOrAlias(const char *encodingName)
{
	// Try name as given
	unsigned encodingId = GetEncodingIdFromName(encodingName);
	if (encodingId == 0)
	{
		// Handle purely numeric values (codepages)
		char *ahead = 0;
		unsigned codepage = strtol(encodingName, &ahead, 10);
		int i = 0;
		while (*ahead != '\0' && i < _countof(f_wincp_prefixes))
		{
			if (const char *remainder = EatPrefix(encodingName, f_wincp_prefixes[i]))
			{
				codepage = strtol(remainder, &ahead, 10);
			}
			++i;
		}
		if (*ahead == '\0')
		{
			encodingId = GetEncodingIdFromCodePage(codepage);
		}
	}
	return encodingId;
}

/**
 * @brief Parser for HTML files to find encoding information
 */
static unsigned demoGuessEncoding_html(const char *src, stl_size_t len)
{
	CMarkdown markdown(src, src + len, CMarkdown::Html);
	//As <html> and <head> are optional, there is nothing to pull...
	//markdown.Move("html").Pop().Move("head").Pop();
	while (markdown.Move("meta"))
	{
		if (CMarkdown::HSTR hstr = markdown.GetAttribute("http-equiv"))
		{
			if (lstrcmpiA(CMarkdown::String(hstr).A, "content-type") == 0)
			{
				CMarkdown::String content = markdown.GetAttribute("content");
				if (char *pchKey = content.A)
				{
					while (size_t cchKey = strcspn(pchKey += strspn(pchKey, "; \t\r\n"), ";="))
					{
						char *pchValue = pchKey + cchKey;
						size_t cchValue = strcspn(pchValue += strspn(pchValue, "= \t\r\n"), "; \t\r\n");
						if (cchKey >= 7 && _memicmp(pchKey, "charset", 7) == 0 && (cchKey == 7 || strchr(" \t\r\n", pchKey[7])))
						{
							pchValue[cchValue] = '\0';
							// Is it an encoding name known to charsets module ?
							if (unsigned encodingId = FindEncodingIdFromNameOrAlias(pchValue))
								return GetEncodingCodePageFromId(encodingId);
							return 0;
						}
						pchKey = pchValue + cchValue;
					}
				}
			}
		}
		else if (CMarkdown::HSTR hstr = markdown.GetAttribute("charset"))
		{
			// HTML5 encoding specifier
			if (unsigned encodingId = FindEncodingIdFromNameOrAlias(CMarkdown::String(hstr).A))
				return GetEncodingCodePageFromId(encodingId);
		}
	}
	return 0;
}

/**
 * @brief Parser for XML files to find encoding information
 */
static unsigned demoGuessEncoding_xml(const char *src, stl_size_t len)
{
	CMarkdown xml(src, src + len);
	if (xml.Move("?xml"))
	{
		CMarkdown::String encoding = xml.GetAttribute("encoding");
		if (encoding.A)
		{
			// Is it an encoding name we can find in charsets module ?
			unsigned encodingId = FindEncodingIdFromNameOrAlias(encoding.A);
			if (encodingId)
			{
				return GetEncodingCodePageFromId(encodingId);
			}
		}
	}
	return 0;
}

/**
 * @brief Parser for rc files to find encoding information
 * @note sscanf() requires first argument to be zero-terminated so we must
 * copy lines to temporary buffer.
 */
static unsigned demoGuessEncoding_rc(const char *src, stl_size_t len)
{
	unsigned cp = 0;
	char line[80];
	do
	{
		while (len && (*src == '\r' || *src == '\n'))
		{
			++src;
			--len;
		}
		const char *base = src;
		while (len && (*src != '\r' && *src != '\n'))
		{
			++src;
			--len;
		}
		lstrcpynA(line, base, len < sizeof line ? len + 1 : sizeof line);
	} while (len && sscanf(line, "#pragma code_page(%d)", &cp) != 1);
	return cp;
}

/**
 * @brief Try to deduce encoding for this file.
 * @param [in] ext File extension.
 * @param [in] src File contents (as a string).
 * @param [in] len Size of the file contents string.
 * @return Codepage number.
 */
unsigned GuessEncoding_from_bytes(LPCTSTR ext, const char *src, stl_size_t len)
{
	if (len > BufSize)
		len = BufSize;
	unsigned cp = 0;
	if (lstrcmpi(ext, _T(".rc")) ==  0)
	{
		cp = demoGuessEncoding_rc(src, len);
	}
	else if (lstrcmpi(ext, _T(".htm")) == 0 || lstrcmpi(ext, _T(".html")) == 0)
	{
		cp = demoGuessEncoding_html(src, len);
	}
	else if (lstrcmpi(ext, _T(".xml")) == 0 || lstrcmpi(ext, _T(".xsl")) == 0)
	{
		cp = demoGuessEncoding_xml(src, len);
	}
	return cp;
}

/**
 * @brief Try to deduce encoding for this file.
 * @param [in] filepath Full path to the file.
 * @param [in,out] encoding Structure getting the encoding info.
 * @param [in] bGuessEncoding Try to guess codepage (not just unicode encoding).
 */
void GuessCodepageEncoding(LPCTSTR filepath, FileTextEncoding *encoding, bool bGuessEncoding, HANDLE osfhandle)
{
	CMarkdown::FileImage fi(
		osfhandle ? reinterpret_cast<LPCTSTR>(osfhandle) : filepath,
		BufSize,
		osfhandle ? CMarkdown::FileImage::Handle : 0);
	encoding->SetCodepage(getDefaultCodepage());
	encoding->m_bom = false;
	encoding->m_guessed = false;
	encoding->m_binary = false;
	unsigned bom = 0;
	if (UNICODESET ucs = DetermineEncoding(fi.pbImage, fi.cbImage, &bom))
	{
		encoding->SetUnicoding(ucs);
		encoding->m_bom = bom != 0;
	}
	else if (bom)
	{
		encoding->m_binary = true;
	}
	if (bGuessEncoding && bom == 0)
	{
		LPCTSTR ext = PathFindExtension(filepath);
		if (unsigned cp = GuessEncoding_from_bytes(ext, fi.pcImage, fi.cbImage))
		{
			encoding->SetCodepage(cp);
			encoding->m_guessed = true;
		}
	}
}
