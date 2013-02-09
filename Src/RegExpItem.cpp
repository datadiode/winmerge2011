#include "StdAfx.h"
#include "pcre.h"
#include "RegExpItem.h"

static size_t FindSlash(LPCTSTR buf, size_t len)
{
	if (len > 0 && *buf == '/')
	{
		int e = 0;
		size_t i = 0;
		while (i < len)
		{
			switch (buf[++i])
			{
			case '\\':
				e ^= 1;
				break;
			case '/':
				if (e == 0)
					return i;
				break;
			default:
				e = 0;
				break;
			}
		}
	}
	return 0;
}

const char *regexp_item::assign(LPCTSTR pch, size_t cch)
{
	UINT codepage = CP_ACP;
	if (const size_t i = FindSlash(pch, cch))
	{
		size_t j = i;
		while (++j < cch)
		{
			switch (pch[j])
			{
			case _T('i'):
				options |= PCRE_CASELESS;
				break;
			case _T('u'):
				options |= PCRE_UTF8;
				codepage = CP_UTF8;
				break;
			case _T('g'):
				global = true;
				break;
			case _T(':'):
			case _T('<'):
				if (const wchar_t *const p = wmemchr(pch + j, _T('<'), cch - j))
				{
					const wchar_t *const q = p + 1;
					injectString = HString::Uni(q,
						static_cast<UINT>(pch + cch - q))->Oct(codepage);
					// Exclude the <injectString from further processing.
					cch = p - pch;
					// Bail out if the colon was omitted.
					if (cch == j)
						break;
				}
				filenameSpec = HString::Uni(pch + j + 1, cch - j - 1);
				cch = 0;
				break;
			}
		}
		++pch;
		cch = i - 1;
	}
	filterString = HString::Uni(pch, cch)->Oct(codepage);
	const char *errormsg = NULL;
	int erroroffset = 0;
	if (pcre *regexp = pcre_compile(
		filterString->A, options, &errormsg, &erroroffset, NULL))
	{
		errormsg = NULL;
		pRegExp = regexp;
		pRegExpExtra = pcre_study(regexp, 0, &errormsg);
	}
	else
	{
		filterString->Free();
		filterString = NULL;
	}
	return filterString->A;
}

size_t regexp_item::process(const stl::vector<regexp_item> &relist,
	char *dst, const char *src, size_t len, LPCTSTR filename)
{
	stl::vector<regexp_item>::const_iterator iter = relist.begin();
	while (iter != relist.end())
	{
		const regexp_item &filter = *iter++;
		if (filename && filter.filenameSpec && !::PathMatchSpec(filename, filter.filenameSpec->T))
			continue;
		char *buf = dst;
		size_t i = 0;
		while (i < len)
		{
			int ovector[33];
			int matches = filter.execute(src, len, i, 0, ovector, _countof(ovector) - 1);
			if ((matches <= 0) || (ovector[1] == 0))
			{
				ovector[1] = len;
				matches = 0;
			}
			int matches2 = matches * 2;
			ovector[matches2] = ovector[1];
			ovector[1] = ovector[0];
			HString *const injectString = filter.injectString;
			size_t const injectLength = injectString->ByteLen();
			int index = 1;
			size_t j;
			do
			{
				j = ovector[index];
				if (i < j)
				{
					size_t d = j - i;
					if (index == 1 || (index & 1) == 0)
					{
						memcpy(buf, src + i, d);
						buf += d;
					}
					else if (injectLength <= d)
					{
						memcpy(buf, injectString, injectLength);
						buf += injectLength;
					}
					i = j;
				}
			} while (++index <= matches2);
			if (!filter.global)
			{
				j = len;
				if (i < j)
				{
					size_t d = j - i;
					memcpy(buf, src + i, d);
					buf += d;
				}
			}
			i = j;
		}
		len = buf - dst;
		src = dst;
	}
	return len;
}

bool regexp_item::indifferent(const stl::vector<regexp_item> &relist, LPCTSTR p, LPCTSTR q)
{
	OString str1 = HString::Uni(p)->Oct(CP_UTF8);
	OString str2 = HString::Uni(q)->Oct(CP_UTF8);
	size_t len1 = regexp_item::process(relist, str1.A, str1.A, str1.ByteLen());
	size_t len2 = regexp_item::process(relist, str2.A, str2.A, str2.ByteLen());
	return (len1 == len2) && (_memicmp(str1.A, str2.A, len1) == 0);
}
