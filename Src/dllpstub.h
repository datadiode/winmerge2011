#ifndef DECORATE_U

/**
 * @brief define the suffixes to decorate TCHAR width specific module/function names
 */

#ifdef _UNICODE
#define DECORATE_A
#define DECORATE_W "W"
#define DECORATE_U "U"
#else
#define DECORATE_A "A"
#define DECORATE_W
#define DECORATE_U
#endif
#define DECORATE_AW DECORATE_A DECORATE_W

/**
 * @brief stub class to help implement DLL proxies
 *
 * A DLLPSTUB must be embedded in an object followed immediately by an array
 * of names, comprising the DLL's name followed by the exported symbol names.
 *
 * If dll is not found, DLLPSTUB::Load will throw an exception
 * If any of dwMajorVersion, dwMinorVersion, dwBuildNumber are non-zero
 * the DLLPSTUB::Load will throw an exception (CO_S_NOTALLINTERFACES) unless
 * the dll exports DllGetVersion and reports a version at least as high as
 * requested by these members
 */
struct DLLPSTUB
{
	DWORD dwMajorVersion;					// Major version
	DWORD dwMinorVersion;					// Minor version
	DWORD dwBuildNumber;					// Build number
	DWORD dwPadding;						// Pad to 64 bit boundary
	static void Throw(LPCSTR name, HMODULE, DWORD dwError, BOOL bFreeLibrary);
	HMODULE Load();
};

template<class T>
struct DllProxy
{
	DLLPSTUB stub;
	LPCSTR names[&static_cast<T *>(0)->END - &static_cast<T *>(0)->BEGIN];
	HMODULE handle;
	T *operator->()
	{
		stub.Load();
		return (T *) names;
	}
};

// ICONV dll interface
struct ICONV
{
	HMODULE BEGIN;
	HANDLE (*iconv_open)(const char *tocode, const char *fromcode);
	size_t (*iconv)(HANDLE, const char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft);
	int (*iconv_close)(HANDLE);
	int (*iconvctl)(HANDLE, int request, void *argument);
	void (*iconvlist)
	(
		int (*do_one)(unsigned int namescount, const char *const *names, void *data),
		void *data
	);
	int *_libiconv_version;
	HMODULE END;
};

extern DllProxy<struct ICONV> ICONV;

// MSHTML dll interface
struct URLMON
{
	HMODULE BEGIN;
	HRESULT(STDAPICALLTYPE*CreateURLMoniker)(IMoniker *, LPCWSTR, IMoniker **);
	HMODULE END;
};

extern DllProxy<struct URLMON> URLMON;

// MSHTML dll interface
struct MSHTML
{
	HMODULE BEGIN;
	HRESULT(STDAPICALLTYPE*CreateHTMLPropertyPage)(IMoniker *, IPropertyPage **);
	HRESULT(STDAPICALLTYPE*ShowHTMLDialogEx)(HWND, IMoniker *, DWORD, VARIANT *, LPWSTR, VARIANT *);
	HMODULE END;
};

extern DllProxy<struct MSHTML> MSHTML;

#endif
