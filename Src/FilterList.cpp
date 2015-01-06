/** 
 * @file  FilterList.h
 *
 * @brief Implementation file for FilterList.
 */
#include "StdAfx.h"
#include "FilterList.h"
#include "LineFiltersList.h"
#include "DiffWrapper.h"
#include "pcre.h"
#include "unicoder.h"
#include "coretools.h"
#include "Environment.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern "C" size_t apply_prediffer(struct file_data *current, short side, char *buf, size_t len)
{
	CDiffWrapper *const pOptions = CDiffWrapper::m_pActiveInstance;
	if (pOptions->HasPrediffers())
	{
		const String &filename = pOptions->GetCompareFile(side);
		pOptions->ResetPrediffers(side);
		const char *src = buf;
		char *dst = buf;
		size_t s = 0;
		while (s < len)
		{
			stl_size_t d = static_cast<stl_size_t>(s);
			switch (char c = src[s++])
			{
			case '\r':
			case '\n':
				if (d > 0)
				{
					if (pOptions->HasPredifferScripts())
					{
						BSTR chunk = ::SysAllocStringLen(NULL, d);
						stl_size_t i;
						for (i = 0 ; i < d ; ++i)
							chunk[i] = static_cast<BYTE>(src[i]);
						chunk = pOptions->ApplyPredifferScripts(filename.c_str(), chunk);
						i = ::SysStringLen(chunk);
						if (i <= d)
						{
							d = i;
							for (i = 0 ; i < d ; ++i)
								dst[i] = static_cast<char>(chunk[i]);
						}
						else if (dst != src)
						{
							memcpy(dst, src, d);
						}
						::SysFreeString(chunk);
					}
					if (pOptions->HasPredifferRegExps())
					{
						d = pOptions->ApplyPredifferRegExps(filename.c_str(), dst, src, d);
					}
					dst += d;
				}
				*dst++ = c;
				src += s;
				len -= s;
				s = 0;
				break;
			}
		}
		len = dst - buf;
	}
	return len;
}

/**
 * @brief Constructor.
 */
FilterList::FilterList(): m_utf8(false)
{
}

/**
 * @brief Destructor.
 */
FilterList::~FilterList()
{
	RemoveAllFilters();
}

/**
 * @brief Add new regular expression to the list.
 * This function adds new regular expression to the list of expressions.
 * The regular expression is compiled and studied for better performance.
 * @param [in] regularExpression Regular expression string.
 * @param [in] encoding Expression encoding.
 */
void FilterList::AddRegExp(LPCTSTR regularExpression)
{
	if (int len = static_cast<int>(_tcslen(regularExpression)))
	{
		regexp_item item;
		if (item.assign(regularExpression, len))
		{
			if (item.options & PCRE_UTF8)
				m_utf8 = true;
			m_list.push_back(item);
		}
	}
}

void FilterList::AddFromIniFile(LPCTSTR inifile)
{
	String filter;
	TCHAR buffer[0x8000];
	if (GetPrivateProfileSection(_T("*"), buffer, _countof(buffer), inifile))
	{
		LPTSTR section = buffer;
		while (const size_t n = _tcslen(section))
		{
			if (LPTSTR check = _tcschr(section, _T('=')))
			{
				*check++ = _T('\0');
				if (_tcschr(check, _T('1')) != 0)
				{
					TCHAR buffer[0x8000];
					if (GetPrivateProfileSection(section, buffer, _countof(buffer), inifile))
					{
						LPTSTR p = buffer;
						DWORD index = 0;
						DWORD count = static_cast<DWORD>(_tcslen(p));
						while (const size_t n = _tcslen(p))
						{
							if (index < count && check[index] == _T('1'))
							{
								if (LPTSTR q = _tcschr(p, _T('=')))
								{
									LPTSTR filterStr = q + 1;
									AddFilter(filter, filterStr, NULL);
								}
							}
							p += n + 1;
							++index;
						}
					}
				}
			}
			section += n + 1;
		}
	}
	AddRegExp(filter.c_str());
}

void FilterList::AddFrom(LineFiltersList &list)
{
	stl_size_t i = 0;
	stl_size_t n = list.GetCount();
	String filter;
	while (i < n)
	{
		LineFilterItem &item = list.GetAt(i++);
		if (item.usage)
			item.hr = AddFilter(filter, item.filterStr.c_str(), &item);
	}
	AddRegExp(filter.c_str());
}

HRESULT FilterList::AddFilter(String &filter, LPCTSTR filterStr, LineFilterItem *item)
{
	HRESULT hr = S_OK;
	if (LPCTSTR regexp = EatPrefix(filterStr, _T("regexp:")))
	{
		regexp_item filter;
		if (filter.assign(regexp, static_cast<int>(_tcslen(regexp))))
		{
			m_predifferRegExps.push_back(filter);
		}
	}
	else if (LPCTSTR path = EatPrefix(filterStr, _T("script:")))
	{
		script_item script(item);
		String moniker = filterStr;
		env_ResolveMoniker(moniker);
		String::size_type colon = moniker.find(
			_T(':'), static_cast<String::size_type>(path - filterStr) + 2);
		C_ASSERT(String::npos == -1);
		if (String::size_type extra = colon + 1)
		{
			script.filenameSpec = HString::Uni(
				moniker.c_str() + extra, moniker.length() - extra);
			moniker.resize(colon);
		}
		if (SUCCEEDED(hr = CoGetObject(moniker.c_str(), NULL, IID_IDispatch,
				reinterpret_cast<void **>(&script.object))) &&
			SUCCEEDED(hr = script.Reset.Init(script.object, L"Reset")) &&
			SUCCEEDED(hr = script.ProcessLine.Init(script.object, L"ProcessLine")))
		{
			m_predifferScripts.push_back(script);
		}
	}
	else
	{
		if (!filter.empty())
			filter.push_back(_T('|'));
		filter += filterStr;
	}
	return hr;
}

/**
 * @brief Removes all expressions from the list.
 */
void FilterList::RemoveAllFilters()
{
	m_utf8 = false;
	dispose(m_list.begin(), m_list.end());
	m_list.clear();
	dispose(m_predifferRegExps.begin(), m_predifferRegExps.end());
	m_predifferRegExps.clear();
	dispose(m_predifferScripts.begin(), m_predifferScripts.end());
	m_predifferScripts.clear();
}

/**
 * @brief Match string against list of expressions.
 * This function matches given @p string against the list of regular
 * expressions. The matching ends when first match is found, so all
 * expressions may not be matched against.
 * @param [in] string string to match.
 * @param [in] codepage codepage of string.
 * @return true if any of the expressions did match the string.
 */
bool FilterList::Match(int stringlen, const char *string, int codepage)
{
	// convert string into UTF-8
	ucr::buffer buf(0);
	if (m_utf8 && codepage != CP_UTF8)
	{
		// If a codepage exists which defines a single byte character that maps
		// to a surrogate pair, transcoding to UTF-16 might grow the data size
		// by a factor of 4.
		buf.resize(stringlen * 4);
		ucr::convert(NONE, codepage, (const unsigned char *)string, 
				stringlen, UTF8, CP_UTF8, &buf);
		string = (const char *)buf.ptr;
		stringlen = buf.size;
	}
	std::vector<regexp_item>::const_iterator iter = m_list.begin();
	while (iter != m_list.end())
	{
		const regexp_item &item = *iter++;
		int ovector[30];
		int result = item.execute(string, stringlen, 0, 0, ovector, 30);
		if (result >= 0)
			return true;
	}
	return false;
}

BSTR FilterList::ApplyPredifferScripts(LPCTSTR filename, BSTR bstr)
{
	std::vector<script_item>::iterator iter = m_predifferScripts.begin();
	VARIANT var;
	V_VT(&var) = VT_BSTR;
	V_BSTR(&var) = bstr;
	while (iter != m_predifferScripts.end())
	{
		script_item &script = *iter++;
		if (script.filenameSpec && !::PathMatchSpec(filename, script.filenameSpec->T))
			continue;
		if (FAILED(script.item->hr))
			continue;
		DISPPARAMS params = { &var, NULL, 1, 0 };
		VARIANT varResult;
		::VariantInit(&varResult);
		script.item->hr = script.ProcessLine.Call(script.object, params, DISPATCH_METHOD, &varResult);
		if (V_VT(&varResult) == VT_BSTR)
		{
			::SysFreeString(V_BSTR(&var));
			V_BSTR(&var) = V_BSTR(&varResult);
		}
		else
		{
			::VariantClear(&varResult);
		}
	}
	return V_BSTR(&var);
}

int FilterList::ApplyPredifferRegExps(LPCTSTR filename, char *dst, const char *src, int len) const
{
	return regexp_item::process(m_predifferRegExps, dst, src, len, filename);
}

void FilterList::ResetPrediffers(short side)
{
	std::vector<script_item>::iterator iter = m_predifferScripts.begin();
	while (iter != m_predifferScripts.end())
	{
		script_item &script = *iter++;
		if (FAILED(script.item->hr))
			continue;
		script.item->hr = script.Reset.Call(script.object, CMyDispParams<1>().Unnamed[side]);
	}
}
