/**
 * @file  LanguageSelect.cpp
 *
 * @brief Implements the Language Selection dialog class (which contains the language data)
 */
#include "StdAfx.h"
#include "Merge.h"
#include "VersionData.h"
#include "resource.h"
#include "LanguageSelect.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "DirFrame.h"
#include "coretools.h"
#include <locale.h>

// Escaped character constants in range 0x80-0xFF are interpreted in current codepage
// Using C locale gets us direct mapping to Unicode codepoints
#pragma setlocale("C")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/** @brief Relative path to WinMerge executable for lang files. */
static const TCHAR szRelativePath[] = _T("\\Languages\\");

static HANDLE NTAPI FindFile(HANDLE h, LPCTSTR path, WIN32_FIND_DATA *fd);

/**
 * @brief A class holding information about language file.
 */
class LangFileInfo
{
public:
	LANGID id; /**< Language ID. */

	static LANGID LangId(const char *lang, const char *sublang);
	
	/**
	 * A constructor taking a language id as parameter.
	 * @param [in] id Language ID to use.
	 */
	LangFileInfo(LANGID id): id(id) { };
	
	LangFileInfo(LPCTSTR path);
	String GetString(LCTYPE type) const;

private:
	struct rg
	{
		LANGID id;
		const char *lang;
	};
	static const struct rg rg[];
};

/**
 * @brief An array holding language IDs and names.
 */
const struct LangFileInfo::rg LangFileInfo::rg[] =
{
	{
		LANG_AFRIKAANS,		"AFRIKAANS\0"
	},
	{
		LANG_ALBANIAN,		"ALBANIAN\0"
	},
	{
		LANG_ARABIC,		"ARABIC\0"						"SAUDI_ARABIA\0"
															"IRAQ\0"
															"EGYPT\0"
															"LIBYA\0"
															"ALGERIA\0"
															"MOROCCO\0"
															"TUNISIA\0"
															"OMAN\0"
															"YEMEN\0"
															"SYRIA\0"
															"JORDAN\0"
															"LEBANON\0"
															"KUWAIT\0"
															"UAE\0"
															"BAHRAIN\0"
															"QATAR\0"
	},
	{
		LANG_ARMENIAN,		"ARMENIAN\0"
	},
	{
		LANG_ASSAMESE,		"ASSAMESE\0"
	},
	{
		LANG_AZERI,			"AZERI\0"						"LATIN\0"
															"CYRILLIC\0"
	},
	{
		LANG_BASQUE,		"BASQUE\0"
	},
	{
		LANG_BELARUSIAN,	"BELARUSIAN\0"
	},
	{
		LANG_BENGALI,		"BENGALI\0"
	},
	{
		LANG_BULGARIAN,		"BULGARIAN\0"
	},
	{
		LANG_CATALAN,		"CATALAN\0"
	},
	{
		LANG_CHINESE,		"CHINESE\0"						"TRADITIONAL\0"
															"SIMPLIFIED\0"
															"HONGKONG\0"
															"SINGAPORE\0"
															"MACAU\0"
	},
	{
		LANG_CROATIAN,		"CROATIAN\0"
	},
	{
		LANG_CZECH,			"CZECH\0"
	},
	{
		LANG_DANISH,		"DANISH\0"
	},
	{
		LANG_DIVEHI,		"DIVEHI\0"
	},
	{
		MAKELANGID(LANG_DUTCH, SUBLANG_DUTCH),				"DUTCH\0"
															"BELGIAN\0"
	},
	{
		LANG_ENGLISH,		"ENGLISH\0"						"US\0"
															"UK\0"
															"AUS\0"
															"CAN\0"
															"NZ\0"
															"EIRE\0"
															"SOUTH_AFRICA\0"
															"JAMAICA\0"
															"CARIBBEAN\0"
															"BELIZE\0"
															"TRINIDAD\0"
															"ZIMBABWE\0"
															"PHILIPPINES\0"
	},
	{
		LANG_ESTONIAN,		"ESTONIAN\0"
	},
	{
		LANG_FAEROESE,		"FAEROESE\0"
	},
	{
		LANG_FARSI,			"FARSI\0"
	},
	{
		LANG_FINNISH,		"FINNISH\0"
	},
	{
		MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH),			"FRENCH\0"
															"BELGIAN\0"
															"CANADIAN\0"
															"SWISS\0"
															"LUXEMBOURG\0"
															"MONACO\0"
	},
	{
		LANG_GALICIAN,		"GALICIAN\0"
	},
	{
		LANG_GEORGIAN,		"GEORGIAN\0"
	},
	{
		MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN),			"GERMAN\0"
															"SWISS\0"
															"AUSTRIAN\0"
															"LUXEMBOURG\0"
															"LIECHTENSTEIN"
	},
	{
		LANG_GREEK,			"GREEK\0"
	},
	{
		LANG_GUJARATI,		"GUJARATI\0"
	},
	{
		LANG_HEBREW,		"HEBREW\0"
	},
	{
		LANG_HINDI,			"HINDI\0"
	},
	{
		LANG_HUNGARIAN,		"HUNGARIAN\0"
	},
	{
		LANG_ICELANDIC,		"ICELANDIC\0"
	},
	{
		LANG_INDONESIAN,	"INDONESIAN\0"
	},
	{
		MAKELANGID(LANG_ITALIAN, SUBLANG_ITALIAN),			"ITALIAN\0"
															"SWISS\0"
	},
	{
		LANG_JAPANESE,		"JAPANESE\0"
	},
	{
		LANG_KANNADA,		"KANNADA\0"
	},
	{
		MAKELANGID(LANG_KASHMIRI, SUBLANG_DEFAULT),			"KASHMIRI\0"
															"SASIA\0"
	},
	{
		LANG_KAZAK,			"KAZAK\0"
	},
	{
		LANG_KONKANI,		"KONKANI\0"
	},
	{
		MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN),			"KOREAN\0"
	},
	{
		LANG_KYRGYZ,		"KYRGYZ\0"
	},
	{
		LANG_LATVIAN,		"LATVIAN\0"
	},
	{
		MAKELANGID(LANG_LITHUANIAN, SUBLANG_LITHUANIAN),	"LITHUANIAN\0"
	},
	{
		LANG_MACEDONIAN,	"MACEDONIAN\0"
	},
	{
		LANG_MALAY,			"MALAY\0"						"MALAYSIA\0"
															"BRUNEI_DARUSSALAM\0"
	},
	{
		LANG_MALAYALAM,		"MALAYALAM\0"
	},
	{
		LANG_MANIPURI,		"MANIPURI\0"
	},
	{
		LANG_MARATHI,		"MARATHI\0"
	},
	{
		LANG_MONGOLIAN,		"MONGOLIAN\0"
	},
	{
		MAKELANGID(LANG_NEPALI, SUBLANG_DEFAULT),			"NEPALI\0"
															"INDIA\0"
	},
	{
		LANG_NORWEGIAN,		"NORWEGIAN\0"					"BOKMAL\0"
															"NYNORSK\0"
	},
	{
		LANG_ORIYA,			"ORIYA\0"
	},
	{
		LANG_POLISH,		"POLISH\0"
	},
	{
		MAKELANGID(LANG_PORTUGUESE, SUBLANG_PORTUGUESE),	"PORTUGUESE\0"
															"BRAZILIAN\0"
	},
	{
		LANG_PUNJABI,		"PUNJABI\0"
	},
	{
		LANG_ROMANIAN,		"ROMANIAN\0"
	},
	{
		LANG_RUSSIAN,		"RUSSIAN\0"
	},
	{
		LANG_SANSKRIT,		"SANSKRIT\0"
	},
	{
		MAKELANGID(LANG_SERBIAN, SUBLANG_DEFAULT),			"SERBIAN\0"
															"LATIN\0"
															"CYRILLIC\0"
	},
	{
		LANG_SINDHI,		"SINDHI\0"
	},
	{
		LANG_SLOVAK,		"SLOVAK\0"
	},
	{
		LANG_SLOVENIAN,		"SLOVENIAN\0"
	},
	{
		MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH),			"SPANISH\0"
															"MEXICAN\0"
															"MODERN\0"
															"GUATEMALA\0"
															"COSTA_RICA\0"
															"PANAMA\0"
															"DOMINICAN_REPUBLIC\0"
															"VENEZUELA\0"
															"COLOMBIA\0"
															"PERU\0"
															"ARGENTINA\0"
															"ECUADOR\0"
															"CHILE\0"
															"URUGUAY\0"
															"PARAGUAY\0"
															"BOLIVIA\0"
															"EL_SALVADOR\0"
															"HONDURAS\0"
															"NICARAGUA\0"
															"PUERTO_RICO\0"
	},
	{
		LANG_SWAHILI,		"SWAHILI\0"
	},
	{
		MAKELANGID(LANG_SWEDISH, SUBLANG_SWEDISH),			"SWEDISH\0"
															"FINLAND\0"
	},
	{
		LANG_SYRIAC,		"SYRIAC\0"
	},
	{
		LANG_TAMIL,			"TAMIL\0"
	},
	{
		LANG_TATAR,			"TATAR\0"
	},
	{
		LANG_TELUGU,		"TELUGU\0"
	},
	{
		LANG_THAI,			"THAI\0"
	},
	{
		LANG_TURKISH,		"TURKISH\0"
	},
	{
		LANG_UKRAINIAN,		"UKRAINIAN\0"
	},
	{
		LANG_URDU,			"URDU\0"						"PAKISTAN\0"
															"INDIA\0"
	},
	{
		LANG_UZBEK,			"UZBEK\0"						"LATIN\0"
															"CYRILLIC\0"
	},
	{
		LANG_VIETNAMESE,	"VIETNAMESE\0"
	},
};

/**
 * @brief Get a language ID for given language + sublanguage.
 * @param [in] lang Language name.
 * @param [in] sublang Sub language name.
 * @return Language ID.
 */
LANGID LangFileInfo::LangId(const char *lang, const char *sublang)
{
	// binary search the array for passed in lang
	size_t lower = 0;
	size_t upper = _countof(rg);
	while (lower < upper)
	{
		size_t match = (upper + lower) >> 1;
		int cmp = strcmp(rg[match].lang, lang);
		if (cmp >= 0)
			upper = match;
		if (cmp <= 0)
			lower = match + 1;
	}
	if (lower <= upper)
		return 0;
	LANGID baseid = rg[upper].id;
	// figure out sublang
	if ((baseid & ~0x3ff) && *sublang == '\0')
		return baseid;
	LANGID id = PRIMARYLANGID(baseid);
	if (0 == strcmp(sublang, "DEFAULT"))
		return MAKELANGID(id, SUBLANG_DEFAULT);
	const char *sub = rg[upper].lang;
	do
	{
		do
		{
			id += MAKELANGID(0, 1);
		} while (id == baseid);
		sub += strlen(sub) + 1;
		if (0 == strcmp(sublang, sub))
			return id;
	} while (*sub);
	return 0;
}

/**
 * @brief A constructor taking a path to language file as parameter.
 * @param [in] path Full path to the language file.
 */
LangFileInfo::LangFileInfo(LPCTSTR path)
: id(0)
{
	if (FILE *f = _tfopen(path, _T("r")))
	{
		char buf[1024 + 1];
		while (fgets(buf, sizeof buf - 1, f))
		{
			int i = 0;
			strcat(buf, "1");
			sscanf(buf, "msgid \" LANG_ENGLISH , SUBLANG_ENGLISH_US \" %d", &i);
			if (i)
			{
				if (fgets(buf, sizeof buf, f))
				{
					char *lang = strstr(buf, "LANG_");
					char *sublang = strstr(buf, "SUBLANG_");
					if (lang && sublang)
					{
						strtok(lang, ",\" \t\r\n");
						strtok(sublang, ",\" \t\r\n");
						lang += sizeof "LANG";
						sublang += sizeof "SUBLANG";
						if (0 != strcmp(sublang, "DEFAULT"))
						{
							sublang = EatPrefix(sublang, lang);
							if (sublang && *sublang)
								sublang = EatPrefix(sublang, "_");
						}
						if (sublang)
							id = LangId(lang, sublang);
					}
				}
				break;
			}
		}
		fclose(f);
	}
}

String LangFileInfo::GetString(LCTYPE type) const
{
	String s;
	if (int cch = GetLocaleInfo(id, type, 0, 0))
	{
		s.resize(cch - 1);
		GetLocaleInfo(id, type, &s.front(), cch);
	}
	return s;
}

static HANDLE NTAPI FindFile(HANDLE h, LPCTSTR path, WIN32_FIND_DATA *fd)
{
	if (h == INVALID_HANDLE_VALUE)
	{
		h = FindFirstFile(path, fd);
	}
	else if (fd->dwFileAttributes == INVALID_FILE_ATTRIBUTES || !FindNextFile(h, fd))
	{
		FindClose(h);
		h = INVALID_HANDLE_VALUE;
	}
	return h;
}

static unsigned NTAPI TransformIndex(LPCSTR rc)
{
	unsigned i;
	if (sscanf(rc, " Merge.rc:%u", &i) == 1)
		return i;
	return 0;
}

static unsigned NTAPI TransformIndex(LPCWSTR rc)
{
	unsigned i;
	if (swscanf(rc, L" Merge.rc:%u", &i) == 1)
		return i;
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CLanguageSelect dialog

/** @brief Default English language. */
const WORD wSourceLangId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

CLanguageSelect::CLanguageSelect()
: OResizableDialog(IDD_LANGUAGE_SELECT)
, m_ctlLangList(NULL)
, m_hCurrentDll(0)
, m_wCurLanguage(wSourceLangId)
{
	static const LONG FloatScript[] =
	{
		IDC_LANGUAGE_LIST,			BY<1000>::X2R | BY<1000>::Y2B,
		IDOK,						BY<500>::X2L | BY<500>::X2R | BY<1000>::Y2T | BY<1000>::Y2B,
		IDCANCEL,					BY<500>::X2L | BY<500>::X2R | BY<1000>::Y2T | BY<1000>::Y2B,
		0
	};
	CFloatState::FloatScript = FloatScript;
	SetThreadLocale(MAKELCID(m_wCurLanguage, SORT_DEFAULT));
}

/**
 * @brief Load language.file
 * @param [in] wLangId 
 * @return TRUE on success, FALSE otherwise.
 */
bool CLanguageSelect::LoadLanguageFile(LANGID wLangId)
{
	String strPath = GetFileName(wLangId);
	if (strPath.empty())
		return false;
	if (m_hCurrentDll == 0)
		m_hCurrentDll = LoadLibraryEx(_T("MergeLang.dll"), 0, LOAD_LIBRARY_AS_DATAFILE);
	// Failure of LoadLibraryEx() will cause the MERGEPOT blob fail to load.
	DWORD instanceVerMS = 0;
	DWORD instanceVerLS = 0;
	if (const VS_FIXEDFILEINFO *pVffInfo =
		reinterpret_cast<const VS_FIXEDFILEINFO *>(CVersionData::Load()->Data()))
	{
		instanceVerMS = pVffInfo->dwFileVersionMS;
		instanceVerLS = pVffInfo->dwFileVersionLS;
	}
	DWORD resourceVerMS = 0;
	DWORD resourceVerLS = 0;
	if (const VS_FIXEDFILEINFO *pVffInfo =
		reinterpret_cast<const VS_FIXEDFILEINFO *>(CVersionData::Load(m_hCurrentDll)->Data()))
	{
		resourceVerMS = pVffInfo->dwFileVersionMS;
		resourceVerLS = pVffInfo->dwFileVersionLS;
	}
	// There is no point in translating error messages about inoperational
	// translation system, so go without string resources here.
	if (instanceVerMS != resourceVerMS || instanceVerLS != resourceVerLS)
	{
		if (m_hWnd)
			MessageBox(_T("MergeLang.dll version mismatch"), _T("WinMerge"), MB_ICONSTOP);
		return false;
	}
	HRSRC mergepot = FindResource(m_hCurrentDll, _T("MERGEPOT"), RT_RCDATA);
	if (mergepot == 0)
	{
		if (m_hWnd)
			theApp.DoMessageBox(_T("MergeLang.dll is missing or invalid"), MB_ICONSTOP);
		return false;
	}
	size_t size = SizeofResource(m_hCurrentDll, mergepot);
	const char *data = (const char *)LoadResource(m_hCurrentDll, mergepot);
	char buf[1024];
	std::string *ps = 0;
	std::string msgid;
	std::vector<unsigned> lines;
	int unresolved = 0;
	int mismatched = 0;
	while (const char *eol = (const char *)memchr(data, '\n', size))
	{
		size_t len = eol - data;
		if (len >= sizeof buf)
		{
			ASSERT(FALSE);
			break;
		}
		memcpy(buf, data, len);
		buf[len++] = '\0';
		data += len;
		size -= len;
		if (char *p = EatPrefix(buf, "#:"))
		{
			if (unsigned line = TransformIndex(p)) //char *q = strchr(p, ':'))
			{
				lines.push_back(line);
				++unresolved;
			}
		}
		else if (EatPrefix(buf, "msgid "))
		{
			ps = &msgid;
		}
		if (ps)
		{
			char *p = strchr(buf, '"');
			char *q = strrchr(buf, '"');
			if (std::string::size_type n = static_cast<std::string::size_type>(q - p))
			{
				ps->append(p + 1, n - 1);
			}
			else
			{
				ps = 0;
				const char *text = msgid.c_str();
				std::vector<unsigned>::iterator pline = lines.begin();
				while (pline < lines.end())
				{
					unsigned line = *pline++;
					text = m_strarray.setAtGrow(line, text);
				}
				lines.clear();
				msgid.clear();
			}
		}
	}
	FILE *f = _tfopen(strPath.c_str(), _T("r"));
	if (f == 0)
	{
		if (m_hWnd)
		{
			String str = _T("Failed to load ") + strPath;
			theApp.DoMessageBox(str.c_str(), MB_ICONSTOP);
		}
		return false;
	}
	ps = 0;
	msgid.clear();
	lines.clear();
	std::string format;
	std::string msgstr;
	std::string directive;
	while (fgets(buf, sizeof buf, f))
	{
		if (char *p = EatPrefix(buf, "#:"))
		{
			if (unsigned line = TransformIndex(p)) //char *q = strchr(p, ':'))
			{
				lines.push_back(line);
				--unresolved;
			}
		}
		else if (char *p = EatPrefix(buf, "#,"))
		{
			format = p;
			format.erase(0, format.find_first_not_of(" \t\r\n"));
			format.erase(format.find_last_not_of(" \t\r\n") + 1);
		}
		else if (char *p = EatPrefix(buf, "#."))
		{
			directive = p;
			directive.erase(0, directive.find_first_not_of(" \t\r\n"));
			directive.erase(directive.find_last_not_of(" \t\r\n") + 1);
		}
		else if (EatPrefix(buf, "msgid "))
		{
			ps = &msgid;
		}
		else if (EatPrefix(buf, "msgstr "))
		{
			ps = &msgstr;
		}
		if (ps)
		{
			char *p = strchr(buf, '"');
			char *q = strrchr(buf, '"');
			if (std::string::size_type n = static_cast<std::string::size_type>(q - p))
			{
				ps->append(p + 1, n - 1);
			}
			else if (msgid.empty() && !msgstr.empty())
			{
				unslash(CP_ACP, msgstr);
				m_poheader.swap(msgstr);
			}
			else
			{
				ps = 0;
				if (msgstr.empty())
					msgstr = msgid;
				unslash(m_codepage, msgstr);
				const char *text = msgstr.c_str();
				std::vector<unsigned>::iterator pline = lines.begin();
				while (pline < lines.end())
				{
					unsigned line = *pline++;
					if (m_strarray[line] != msgid)
						++mismatched;
				}
				pline = lines.begin();
				while (pline < lines.end())
				{
					unsigned line = *pline++;
					if (m_strarray[line] == msgid)
						text = m_strarray.setAtGrow(line, text);
				}
				lines.clear();
				if (directive == "Codepage")
				{
					m_codepage = strtol(msgstr.c_str(), &p, 10);
					if (!IsValidCodePage(m_codepage))
						m_codepage = 0;
					directive.clear();
				}
				msgid.clear();
				msgstr.clear();
			}
		}
	}
	fclose(f);
	if (m_codepage == 0)
	{
		if (m_hWnd)
		{
			String stm;
			stm += _T("Unsupported codepage - cannot apply translations from\n");
			stm += strPath.c_str();
			theApp.DoMessageBox(stm.c_str(), MB_ICONSTOP);
		}
		return false;
	}
	if (unresolved != 0 || mismatched != 0)
	{
		if (m_hWnd)
		{
			String str = _T("Unresolved or mismatched references detected when ")
				_T("attempting to read translations from\n") + strPath;
			theApp.DoMessageBox(str.c_str(), MB_ICONSTOP);
		}
		return false;
	}
	return true;
}

/**
 * @brief Set UI language.
 * @param [in] wLangId 
 * @return TRUE on success, FALSE otherwise.
 */
bool CLanguageSelect::SetLanguage(LANGID wLangId)
{
	if (wLangId == 0)
		return false;
	if (m_wCurLanguage == wLangId)
		return true;
	if (wLangId == wSourceLangId || !LoadLanguageFile(wLangId))
	{
		m_poheader.clear();
		m_strarray.clear();
		m_codepage = 0;
		// free the existing DLL
		if (m_hCurrentDll)
		{
			FreeLibrary(m_hCurrentDll);
			m_hCurrentDll = 0;
		}
		wLangId = wSourceLangId;
	}
	m_wCurLanguage = wLangId;
	SetThreadLocale(MAKELCID(m_wCurLanguage, SORT_DEFAULT));
	return true;
}

bool CLanguageSelect::GetPoHeaderProperty(const char *name, std::string &value) const
{
	size_t len = strlen(name);
	const char *p = m_poheader.c_str();
	while (const char *q = strchr(p, '\n'))
	{
		if (memcmp(p, name, len) == 0 && p[len] == ':')
		{
			p += len + 1;
			p += strspn(p, " \t");
			bool lossy = false;
			len = strcspn(p, " \t\r\n");
			value.assign(p, static_cast<std::string::size_type>(len));
			return true;
		}
		p = q + 1;
	}
	return false;
}

/**
 * @brief Get a language file for the specified language ID.
 * This function gets a language file name for the given language ID. Language
 * files are currently named as [languagename].po.
 * @param [in] wLangId Language ID.
 * @return Language filename, or empty string if no file for language found.
 */
String CLanguageSelect::GetFileName(LANGID wLangId)
{
	String filename;
	String path = GetModulePath().append(szRelativePath);
	String pattern = path + _T("*.po");
	WIN32_FIND_DATA ff;
	HANDLE h = INVALID_HANDLE_VALUE;
	while ((h = FindFile(h, pattern.c_str(), &ff)) != INVALID_HANDLE_VALUE)
	{
		filename = path + ff.cFileName;
		LangFileInfo lfi = filename.c_str();
		if (lfi.id == wLangId)
			ff.dwFileAttributes = INVALID_FILE_ATTRIBUTES; // terminate loop
		else
			filename.clear();
	}
	return filename;
}

HINSTANCE CLanguageSelect::FindResourceHandle(LPCTSTR id, LPCTSTR type) const
{
	return m_hCurrentDll && FindResource(m_hCurrentDll, id, type) ?
		m_hCurrentDll : GetModuleHandle(NULL);
}

bool CLanguageSelect::TranslateString(LPCSTR rc, std::string &s) const
{
	unsigned line = TransformIndex(rc);
	s = m_strarray[line];
	unsigned codepage = GetACP();
	if (m_codepage != codepage)
	{
		// Attempt to convert to UI codepage
		if (int len = s.length())
		{
			std::wstring ws;
			ws.resize(len);
			len = MultiByteToWideChar(m_codepage, 0, s.c_str(), -1, &ws.front(), len + 1);
			if (len)
			{
				ws.resize(len - 1);
				len = WideCharToMultiByte(codepage, 0, ws.c_str(), -1, 0, 0, 0, 0);
				if (len)
				{
					s.resize(len - 1);
					WideCharToMultiByte(codepage, 0, ws.c_str(), -1, &s.front(), len, 0, 0);
				}
			}
			return true;
		}
	}
	return false;
}

bool CLanguageSelect::TranslateString(LPCWSTR rc, std::wstring &ws) const
{
	unsigned line = TransformIndex(rc);
	const std::string &s = m_strarray[line];
	if (int len = s.length())
	{
		ws.resize(len);
		const char *msgstr = s.c_str();
		len = MultiByteToWideChar(m_codepage, 0, msgstr, -1, &ws.front(), len + 1);
		ws.resize(len - 1);
		return true;
	}
	return false;
}

void CLanguageSelect::TranslateMenu(HMenu *pMenu) const
{
	bool DebugMenu = false;
#ifdef _DEBUG
	DebugMenu = true;
#endif
	int i = pMenu->GetMenuItemCount();
	DWORD fMask = MIIM_BITMAP | MIIM_DATA;
	while (i > 0)
	{
		--i;
		MENUITEMINFO mii;
		mii.cbSize = sizeof mii;
		mii.fMask = MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_STRING | MIIM_FTYPE;
		TCHAR text[80];
		mii.dwTypeData = text;
		mii.cch = _countof(text);
		pMenu->GetMenuItemInfo(i, TRUE, &mii);
		if (HMenu *pSubMenu = reinterpret_cast<HMenu *>(mii.hSubMenu))
		{
			// Conditionally remove debug menu.
			// Finds debug menu by looking for a submenu which
			// starts with item ID_DEBUG_LOADCONFIG.
			if (!DebugMenu && pSubMenu->GetMenuItemID(0) == ID_DEBUG_LOADCONFIG)
			{
				pMenu->DeleteMenu(i, MF_BYPOSITION);
				continue;
			}
			TranslateMenu(pSubMenu);
			mii.wID = reinterpret_cast<UINT>(pSubMenu);
		}
		// Prevent some menues which happen to lack a bitmap from rendering differently
		mii.fMask = fMask;
		mii.hbmpItem = HBMMENU_CALLBACK;
		mii.dwItemData = reinterpret_cast<ULONG_PTR>(CMainFrame::DrawMenuDefault);
		String s;
		if (TranslateString(text, s))
		{
			mii.fMask |= MIIM_STRING;
			mii.dwTypeData = const_cast<LPTSTR>(s.c_str());
		}
		if (mii.fType & MFT_RIGHTJUSTIFY)
		{
			mii.fMask |= MIIM_BITMAP;
			mii.dwItemData = reinterpret_cast<ULONG_PTR>(CMainFrame::DrawMenuCheckboxFrame);
		}
		pMenu->SetMenuItemInfo(i, TRUE, &mii);
		fMask = MIIM_DATA;
	}
}

void CLanguageSelect::TranslateDialog(HWND h) const
{
	UINT gw = GW_CHILD;
	do
	{
		TCHAR text[80];
		::GetWindowText(h, text, _countof(text));
		String s;
		if (TranslateString(text, s))
			::SetWindowText(h, s.c_str());
		h = ::GetWindow(h, gw);
		gw = GW_HWNDNEXT;
	} while (h);
}

String CLanguageSelect::LoadString(UINT id) const
{
	String s;
	if (id)
	{
		HINSTANCE hinst = m_hCurrentDll ? m_hCurrentDll : GetModuleHandle(NULL);
		TCHAR text[1024];
		::LoadString(hinst, id, text, _countof(text));
		if (!TranslateString(text, s))
			s = text;
	}
	return s;
}

std::wstring CLanguageSelect::LoadDialogCaption(LPCTSTR lpDialogTemplateID) const
{
	std::wstring s;
	if (HINSTANCE hInst = FindResourceHandle(lpDialogTemplateID, RT_DIALOG))
	{
		if (HRSRC hRsrc = FindResource(hInst, lpDialogTemplateID, RT_DIALOG))
		{
			if (LPCWSTR text = (LPCWSTR)LoadResource(hInst, hRsrc))
			{
				// Skip DLGTEMPLATE or DLGTEMPLATEEX
				text += text[1] == 0xFFFF ? 13 : 9;
				// Skip menu name string or ordinal
				if (*text == (const WCHAR)-1)
					text += 2; // WCHARs
				else
					while (*text++);
				// Skip class name string or ordinal
				if (*text == (const WCHAR)-1)
					text += 2; // WCHARs
				else
					while (*text++);
				// Caption string is ahead
				if (!TranslateString(text, s))
					s = text;
			}
		}
	}
	return s;
}

HMenu *CLanguageSelect::LoadMenu(UINT id) const
{
	HINSTANCE hinst = m_hCurrentDll ? m_hCurrentDll : GetModuleHandle(NULL);
	HMenu *pMenu = HMenu::LoadMenu(hinst, MAKEINTRESOURCE(id));
	TranslateMenu(pMenu);
	theApp.m_pMainWnd->SetBitmaps(pMenu);
	return pMenu;
}

HACCEL CLanguageSelect::LoadAccelerators(UINT id) const
{
	HINSTANCE hinst = m_hCurrentDll ? m_hCurrentDll : GetModuleHandle(NULL);
	return ::LoadAccelerators(hinst, MAKEINTRESOURCE(id));
}

HICON CLanguageSelect::LoadIcon(UINT id) const
{
	const HINSTANCE hinst = FindResourceHandle(MAKEINTRESOURCE(id), RT_GROUP_ICON);
	return ::LoadIcon(hinst, MAKEINTRESOURCE(id));
}

HICON CLanguageSelect::LoadSmallIcon(UINT id) const
{
	const HINSTANCE hinst = FindResourceHandle(MAKEINTRESOURCE(id), RT_GROUP_ICON);
	return reinterpret_cast<HICON>(::LoadImage(
		hinst, MAKEINTRESOURCE(id), IMAGE_ICON, 16, 16, LR_SHARED));
}

HImageList *CLanguageSelect::LoadImageList(UINT id, int cx, int cGrow) const
{
	const HINSTANCE hinst = FindResourceHandle(MAKEINTRESOURCE(id), RT_BITMAP);
	return HImageList::LoadImage(hinst, MAKEINTRESOURCE(id),
		cx, cGrow, RGB(255, 0, 255), IMAGE_BITMAP, LR_CREATEDIBSECTION);
}

void CLanguageSelect::UpdateResources()
{
	HMenu *rgpGarbageMenu[] = { theApp.m_pMainWnd->m_pMenuDefault, NULL, NULL, NULL };

	theApp.m_pMainWnd->m_pMenuDefault = LoadMenu(IDR_MAINFRAME);
	HWindow *const pWndMDIClient = theApp.m_pMainWnd->m_pWndMDIClient;
	HWindow *pChild = NULL;
	while ((pChild = pWndMDIClient->FindWindowEx(pChild, WinMergeWindowClass)) != NULL)
	{
		CDocFrame *const pDocFrame = static_cast<CDocFrame *>(CDocFrame::FromHandle(pChild));
		FRAMETYPE const frameType = pDocFrame->GetFrameType();
		CDocFrame::HandleSet *const pHandleSet = pDocFrame->m_pHandleSet;
		ASSERT(frameType > 0 && frameType < _countof(rgpGarbageMenu));
		if (frameType < _countof(rgpGarbageMenu) && rgpGarbageMenu[frameType] == NULL)
		{
			rgpGarbageMenu[frameType] = pHandleSet->m_pMenuShared;
			pHandleSet->m_pMenuShared = LoadMenu(pHandleSet->m_id);
		}
	}
	// Replace the active window
	HMenu *pNewMenu = theApp.m_pMainWnd->m_pMenuDefault;
	if (CDocFrame *pDocFrame = theApp.m_pMainWnd->GetActiveDocFrame())
	{
		pNewMenu = pDocFrame->m_pHandleSet->m_pMenuShared;
	}
	theApp.m_pMainWnd->SetActiveMenu(pNewMenu);
	// Do away with the garbage
	for (int i = 0 ; i < _countof(rgpGarbageMenu) ; ++i)
	{
		if (HMenu *pMenu = rgpGarbageMenu[i])
			pMenu->DestroyMenu();
	}
	theApp.m_pMainWnd->UpdateResources();
}

int CLanguageSelect::DoModal(ODialog &dlg, HWND parent) const
{
	HINSTANCE hinst = m_hCurrentDll ? m_hCurrentDll : GetModuleHandle(NULL);
	if (parent == NULL)
		parent = theApp.m_pMainWnd->GetLastActivePopup()->m_hWnd;
	return static_cast<int>(dlg.DoModal(hinst, parent));
}

HWND CLanguageSelect::Create(ODialog &dlg, HWND parent) const
{
	HINSTANCE hinst = m_hCurrentDll ? m_hCurrentDll : GetModuleHandle(NULL);
	return dlg.Create(hinst, parent);
}

LocalString CLanguageSelect::FormatMessage(UINT id, ...) const
{
	va_list args;
	va_start(args, id);
	DWORD const flags = HIWORD(id) |
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING;
	id = LOWORD(id);
	String const fmt = LoadString(id);
	HLOCAL handle;
	if (!::FormatMessage(flags, fmt.c_str(), 0, 0, (LPTSTR)&handle, 0, &args))
		handle = NULL;
	va_end(args);
	return LocalString(handle, id);
}

LocalString CLanguageSelect::Format(UINT id, ...) const
{
	va_list args;
	va_start(args, id);
	String const fmt = LoadString(id);
	int length = _vsctprintf(fmt.c_str(), args) + 1;
	HLOCAL handle = LocalAlloc(LMEM_FIXED, length * sizeof(TCHAR));
	if (handle)
		_vsntprintf(reinterpret_cast<LPTSTR>(handle), length, fmt.c_str(), args);
	va_end(args);
	return LocalString(handle, id);
}

LocalString (CLanguageSelect::FormatStrings)(UINT id, UINT n, ...) const
{
	va_list args;
	va_start(args, n);
	String const fmt = LoadString(id);
	size_t length = 0;
	LPCTSTR p = fmt.c_str();
	while (LPCTSTR q = _tcschr(p, _T('%')))
	{
		length += q - p;
		if (*++q == '%')
		{
			++length;
			p = q + 1;
		}
		else if (UINT i = _tcstol(q, const_cast<LPTSTR *>(&p), 10))
		{
			if (--i < n)
			{
				q = reinterpret_cast<LPCTSTR *>(args)[i];
				length += _tcslen(q);
			}
		}
	}
	length += _tcslen(p) + 1;
	HLOCAL handle = LocalAlloc(LMEM_FIXED, length * sizeof(TCHAR));
	if (LPTSTR string = reinterpret_cast<LPTSTR>(handle))
	{
		p = fmt.c_str();
		while (LPCTSTR q = _tcschr(p, _T('%')))
		{
			length = q - p;
			memcpy(string, p, length * sizeof(TCHAR));
			string += length;
			if (*++q == '%')
			{
				*string++ = _T('%');
				p = q + 1;
			}
			else if (UINT i = _tcstol(q, const_cast<LPTSTR *>(&p), 10))
			{
				if (--i < n)
				{
					q = reinterpret_cast<LPCTSTR *>(args)[i];
					length = _tcslen(q);
					memcpy(string, q, length * sizeof(TCHAR));
					string += length;
				}
			}
		}
		length = _tcslen(p) + 1;
		memcpy(string, p, length * sizeof(TCHAR));
	}
	va_end(args);
	return LocalString(handle, id);
}

/**
 * @brief Lang aware version of AfxMessageBox()
 */
int CLanguageSelect::MsgBox(UINT id, UINT type) const
{
	String string = LoadString(id);
	return theApp.DoMessageBox(string.c_str(), type, id);
}

/**
 * @brief Show messagebox with resource string having parameter.
 * @param [in] msgid Resource string ID.
 * @param [in] arg Argument string.
 * @param [in] nType Messagebox type flags (e.g. MB_OK).
 * @param [in] nIDHelp Help string ID.
 * @return User choice from the messagebox (see MessageBox()).
 */
int CLanguageSelect::MsgBox(UINT id, LPCTSTR arg, UINT type) const
{
	return FormatMessage(id, arg).MsgBox(type);
}

int LocalString::MsgBox(UINT type)
{
	// Replace any CRs which are not followed by LFs with such
	LPTSTR p = reinterpret_cast<LPTSTR>(handle);
	while (p)
	{
		LPTSTR q = p = _tcschr(p, _T('\r'));
		if (p && *++p != _T('\n'))
			*q = _T('\n');
	}
	return theApp.DoMessageBox(*this, type, id);
}

void CLanguageSelect::OnOK() 
{
	int index = m_ctlLangList->GetCurSel();
	if (index < 0)
		return;
	WORD lang = (WORD)m_ctlLangList->GetItemData(index);
	if (lang != m_wCurLanguage)
	{
		if (SetLanguage(lang))
			COptionsMgr::SaveOption(OPT_SELECTED_LANGUAGE, (int)lang);
		theApp.m_pMainWnd->UpdateCodepageModule();
		// Update the current menu
		UpdateResources();
	}
	EndDialog(IDOK);
}

BOOL CLanguageSelect::OnInitDialog()
{
	OResizableDialog::OnInitDialog();
	TranslateDialog(m_hWnd);
	m_ctlLangList = static_cast<HListBox *>(GetDlgItem(IDC_LANGUAGE_LIST));
	LoadAndDisplayLanguages();
	return TRUE;
}

/**
 * @brief Load languages available on disk, and display in list, and select current
 */
void CLanguageSelect::LoadAndDisplayLanguages()
{
	String path = GetModulePath().append(szRelativePath);
	String pattern = path + _T("*.po");
	WIN32_FIND_DATA ff;
	HANDLE h = INVALID_HANDLE_VALUE;
	do
	{
		LangFileInfo &lfi =
			h == INVALID_HANDLE_VALUE
		?	LangFileInfo(wSourceLangId)
		:	LangFileInfo((path + ff.cFileName).c_str());
		String stm;
		stm += lfi.GetString(LOCALE_SLANGUAGE).c_str();
		stm += _T(" - ");
		stm += lfi.GetString(LOCALE_SNATIVELANGNAME|LOCALE_USE_CP_ACP).c_str();
		stm += _T(" (");
		stm += lfi.GetString(LOCALE_SNATIVECTRYNAME|LOCALE_USE_CP_ACP).c_str();
		stm += _T(")");
		/*stm << _T(" - ");
		stm << lfi.GetString(LOCALE_SABBREVLANGNAME|LOCALE_USE_CP_ACP).c_str();
		stm << _T(" (");
		stm << lfi.GetString(LOCALE_SABBREVCTRYNAME|LOCALE_USE_CP_ACP).c_str();
		stm << _T(") ");*/
		stm += _T(" - ");
		stm += lfi.GetString(LOCALE_SENGLANGUAGE).c_str();
		stm += _T(" (");
		stm += lfi.GetString(LOCALE_SENGCOUNTRY).c_str();
		stm += _T(")");
		int i = m_ctlLangList->AddString(stm.c_str());
		m_ctlLangList->SetItemData(i, lfi.id);
		if (lfi.id == m_wCurLanguage)
			m_ctlLangList->SetCurSel(i);
	} while ((h = FindFile(h, pattern.c_str(), &ff)) != INVALID_HANDLE_VALUE);
}

/**
 * @brief Find DLL entry in lang_map for language for specified locale
 */
static WORD GetLangFromLocale(LCID lcid)
{
	TCHAR buff[8] = {0};
	WORD langID = 0;
	if (GetLocaleInfo(lcid, LOCALE_IDEFAULTLANGUAGE, buff, _countof(buff)))
		_stscanf(buff, _T("%4hx"), &langID);
	return langID;
}

void CLanguageSelect::InitializeLanguage()
{
	ASSERT(LangFileInfo::LangId("GERMAN", "") == MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN));
	ASSERT(LangFileInfo::LangId("GERMAN", "DEFAULT") == MAKELANGID(LANG_GERMAN, SUBLANG_DEFAULT));
	ASSERT(LangFileInfo::LangId("GERMAN", "SWISS") == MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN_SWISS));
	ASSERT(LangFileInfo::LangId("PORTUGUESE", "") == MAKELANGID(LANG_PORTUGUESE, SUBLANG_PORTUGUESE));
	ASSERT(LangFileInfo::LangId("NORWEGIAN", "BOKMAL") == MAKELANGID(LANG_NORWEGIAN, SUBLANG_NORWEGIAN_BOKMAL));
	ASSERT(LangFileInfo::LangId("NORWEGIAN", "NYNORSK") == MAKELANGID(LANG_NORWEGIAN, SUBLANG_NORWEGIAN_NYNORSK));

	//TRACE(_T("%hs\n"), LangFileInfo::FileName(MAKELANGID(LANG_NORWEGIAN, SUBLANG_NORWEGIAN_BOKMAL)).c_str());
	//TRACE(_T("%hs\n"), LangFileInfo::FileName(MAKELANGID(LANG_PORTUGUESE, SUBLANG_PORTUGUESE)).c_str());
	//TRACE(_T("%hs\n"), LangFileInfo::FileName(MAKELANGID(LANG_GERMAN, SUBLANG_DEFAULT)).c_str());

	WORD langID = (WORD)COptionsMgr::Get(OPT_SELECTED_LANGUAGE);
	if (langID)
	{
		// User has set a language override
		SetLanguage(langID);
		return;
	}
	// User has not specified a language
	// so look in thread locale, user locale, and then system locale for
	// a language that WinMerge supports
	WORD Lang1 = GetLangFromLocale(GetThreadLocale());
	if (SetLanguage(Lang1))
		return;
	WORD Lang2 = GetLangFromLocale(LOCALE_USER_DEFAULT);
	if (Lang2 != Lang1 && SetLanguage(Lang2))
		return;
	WORD Lang3 = GetLangFromLocale(LOCALE_SYSTEM_DEFAULT);
	if (Lang3 != Lang2 && Lang3 != Lang1 && SetLanguage(Lang3))
		return;
}

LRESULT CLanguageSelect::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
		case MAKEWPARAM(IDC_LANGUAGE_LIST, LBN_DBLCLK):
			OnOK();
			break;
		case IDCANCEL:
			EndDialog(IDCANCEL);
			break;
		}
		break;
	}
	return OResizableDialog::WindowProc(message, wParam, lParam);
}
