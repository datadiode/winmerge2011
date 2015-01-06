#include "StdAfx.h"
#include "pcre.h"
#include "RegExpItem.h"

static int FindSlash(LPCTSTR buf, int len)
{
	if (len > 0 && *buf == '/')
	{
		int e = 0;
		int i = 0;
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

const char *regexp_item::assign(LPCTSTR pch, int cch)
{
	UINT codepage = CP_ACP;
	if (const int i = FindSlash(pch, cch))
	{
		int j = i;
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
			case _T('p'):
				permutive = true;
				break;
			case _T(':'):
			case _T('<'):
				if (const wchar_t *const p = wmemchr(pch + j, _T('<'), cch - j))
				{
					const wchar_t *const q = p + 1;
					injectString = HString::Uni(q,
						static_cast<UINT>(pch + cch - q))->Oct(codepage);
					// Exclude the <injectString from further processing.
					cch = static_cast<int>(p - pch);
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

int regexp_item::process(const std::vector<regexp_item> &relist,
	char *dst, const char *src, int len, LPCTSTR filename)
{
	std::vector<regexp_item>::const_iterator iter = relist.begin();
	while (iter != relist.end())
	{
		const regexp_item &filter = *iter++;
		if (filename && filter.filenameSpec && !::PathMatchSpec(filename, filter.filenameSpec->T))
			continue;
		HString *const injectString = filter.injectString;
		UINT const injectLength = injectString->ByteLen();
		char *buf = dst;
		int i = 0;
		while (i < len)
		{
			int ovector[33];
			int matches = filter.execute(src, len, i, 0, ovector, _countof(ovector) - 1);
			if ((matches <= 0) || (ovector[1] == 0))
			{
				ovector[1] = len;
				matches = 0;
			}
			matches *= 2;
			int j = ovector[1];
			if (filter.permutive && matches != 0)
			{
				int tmplen = ovector[1] - ovector[0];
				char *const tmpbuf = new char[tmplen];
				const char *q = injectString->A;
				char *p = tmpbuf;
				while (char c = *q++)
				{
					if (p - tmpbuf >= tmplen) // overflow
					{
						q = NULL;
						break;
					}
					int d = 1;
					*p = c;
					if (c == '$' && *q >= '1' && *q <= '9')
					{
						int arg = (*q++ - '0') * 2;
						if (arg < matches)
						{
							int start = ovector[arg];
							int end = ovector[arg + 1];
							d = end - start;
							if (p - tmpbuf + d > tmplen) // overflow
							{
								q = NULL;
								break;
							}
							memcpy(p, src + start, d);
						}
					}
					p += d;
				}
				if (q != NULL)
				{
					// no overflow -> replace input text with permuted text
					tmplen = static_cast<int>(p - tmpbuf);
					memcpy(buf, tmpbuf, tmplen);
				}
				else
				{
					// overflow -> leave input text alone
					memcpy(buf, src, tmplen);
				}
				buf += tmplen;
				delete[] tmpbuf;
			}
			else
			{
				int index = 1;
				ovector[matches] = ovector[1];
				ovector[1] = ovector[0];
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
				} while (++index <= matches);
			}
			if (!filter.global)
			{
				j = len;
				if (i < j)
				{
					int d = j - i;
					memcpy(buf, src + i, d);
					buf += d;
				}
			}
			i = j;
		}
		len = static_cast<int>(buf - dst);
		src = dst;
	}
	return len;
}

bool regexp_item::indifferent(const std::vector<regexp_item> &relist, LPCTSTR p, LPCTSTR q)
{
	OString str1 = HString::Uni(p)->Oct(CP_UTF8);
	OString str2 = HString::Uni(q)->Oct(CP_UTF8);
	size_t len1 = regexp_item::process(relist, str1.A, str1.A, str1.ByteLen());
	size_t len2 = regexp_item::process(relist, str2.A, str2.A, str2.ByteLen());
	return (len1 == len2) && (_memicmp(str1.A, str2.A, len1) == 0);
}
