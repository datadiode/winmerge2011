/** 
 * @file  codepage_detect.cpp
 *
 * @brief Deducing codepage from file contents, when we can
 *
 */
#include "StdAfx.h"
#include "codepage_detect.h"
#include "unicoder.h"
#include "codepage.h"
#include "EncodingInfo.h"
#include "markdown.h"
#include "FileTextEncoding.h"
#include "Utf8FileDetect.h"
#include "Common/coretools.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern HINSTANCE hCharsets = NULL;

/** @brief Buffer size used in this file. */
static const int BufSize = 4 * 1024;

/**
 * @brief Try to to match codepage name from codepages module, & watch for f_wincp_prefixes aliases
 */
static EncodingInfo const *LookupEncoding(char *name)
{
	// Try name as given, except for disregarding character case & leading zeros
	std::replace(name, name + RemoveLeadingZeros(CharLowerA(name)), '_', '-');
	EncodingInfo const *ei = EncodingInfo::From<IMAGE_NT_HEADERS32>(hCharsets, name);
	if (ei == NULL)
	{
		// Prefixes to handle when searching for codepage names
		// NB: prefixes ending in '-' must go first!
		static const char *const prefixes[] =
		{
			"WINDOWS-", "WINDOWS", "CP-", "CP", "MSDOS-", "MSDOS"
		};
		// Handle purely numeric values (codepages)
		char *ahead = 0;
		unsigned codepage = strtol(name, &ahead, 10);
		int i = 0;
		while (*ahead != '\0' && i < _countof(prefixes))
		{
			if (const char *remainder = EatPrefix(name, prefixes[i]))
			{
				codepage = strtol(remainder, &ahead, 10);
			}
			++i;
		}
		if (*ahead == '\0')
		{
			ei = EncodingInfo::From(codepage);
		}
	}
	return ei;
}

static unsigned ReadCodepageFromContentType(char *pchKey)
{
	while (size_t cchKey = strcspn(pchKey += strspn(pchKey, "; \t\r\n"), ";="))
	{
		char *pchValue = pchKey + cchKey;
		size_t cchValue = strcspn(pchValue += strspn(pchValue, "= \t\r\n"), "; \t\r\n");
		if (cchKey >= 7 && _memicmp(pchKey, "charset", 7) == 0 && (cchKey == 7 || strchr(" \t\r\n", pchKey[7])))
		{
			pchValue[cchValue] = '\0';
			// Is it an encoding name known to charsets module ?
			if (EncodingInfo const *encodinginfo = LookupEncoding(pchValue))
				return encodinginfo->GetCodePage();
			return 0;
		}
		pchKey = pchValue + cchValue;
	}
	return 0;
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
				if (CMarkdown::HSTR hstr = markdown.GetAttribute("content"))
				{
					return ReadCodepageFromContentType(CMarkdown::String(hstr).A);
				}
			}
		}
		else if (CMarkdown::HSTR hstr = markdown.GetAttribute("charset"))
		{
			// HTML5 encoding specifier
			if (EncodingInfo const *encodinginfo = LookupEncoding(CMarkdown::String(hstr).A))
				return encodinginfo->GetCodePage();
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
			if (EncodingInfo const *encodinginfo = LookupEncoding(encoding.A))
				return encodinginfo->GetCodePage();
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
 * @brief Parser for po files to find encoding information
 * @note sscanf() requires first argument to be zero-terminated so we must
 * copy lines to temporary buffer.
 */
static unsigned demoGuessEncoding_po(const char *src, stl_size_t len)
{
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
		if (int len = static_cast<int>(src - base))
		{
			char line[80];
			lstrcpynA(line, base, len < sizeof line ? len + 1 : sizeof line);
			if (char *pchKey = EatPrefix(line, "\"Content-Type:"))
			{
				Unslash(CP_ACP, pchKey);
				return ReadCodepageFromContentType(pchKey);
			}
		}
	} while (len);
	return 0;
}

/**
 * @brief Try to deduce encoding for this file.
 * @param [in] ext File extension.
 * @param [in] src File contents (as a string).
 * @param [in] len Size of the file contents string.
 * @return Codepage number.
 */
unsigned GuessEncoding_from_bytes(LPCTSTR ext, LPCTSTR pastext, const char *src, stl_size_t len)
{
	if (len > BufSize)
		len = BufSize;
	unsigned cp = 0;
	if (EatPrefix(ext, _T(".rc")) == pastext)
	{
		cp = demoGuessEncoding_rc(src, len);
	}
	else if (EatPrefix(ext, _T(".htm")) == pastext || EatPrefix(ext, _T(".html")) == pastext)
	{
		cp = demoGuessEncoding_html(src, len);
	}
	else if (EatPrefix(ext, _T(".xml")) == pastext || EatPrefix(ext, _T(".xsl")) == pastext)
	{
		cp = demoGuessEncoding_xml(src, len);
	}
	else if (EatPrefix(ext, _T(".po")) == pastext || EatPrefix(ext, _T(".pot")) == pastext)
	{
		cp = demoGuessEncoding_po(src, len);
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
		LPCTSTR pastext = ext;
		if (int len = lstrlen(ext))
		{
			pastext += len;
		}
		else if (LPCTSTR atat = StrStr(filepath, _T("@@")))
		{
			if (LPCTSTR backslash = StrRChr(filepath, atat, _T('\\')))
				filepath = backslash;
			if (LPCTSTR dot = StrRChr(filepath, atat, _T('.')))
			{
				ext = dot; // extension including dot
				pastext = atat;
			}
		}
		if (unsigned cp = GuessEncoding_from_bytes(ext, pastext, fi.pcImage, fi.cbImage))
		{
			encoding->SetCodepage(cp);
			encoding->m_guessed = true;
		}
	}
}
