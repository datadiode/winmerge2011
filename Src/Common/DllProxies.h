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

struct DllProxy
{
	LPCSTR Names[1];
	LPVOID Load() throw();
	LPVOID EnsureLoad();
	void FormatMessage(LPTSTR);
	template<class T> struct Instance;
};

template<class T> struct DllProxy::Instance
{
	union
	{
		LPCSTR Names[sizeof(T) / sizeof(LPCSTR)];
		DllProxy Proxy;
	};
	HMODULE H;
	operator T *() throw()
	{
		return static_cast<T *>(Proxy.Load());
	}
	T *operator->()
	{
		return static_cast<T *>(Proxy.EnsureLoad());
	}
};

// ICONV dll proxy
struct ICONV
{
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
	HMODULE H;
};

extern DllProxy::Instance<struct ICONV> ICONV;

// MSHTML dll proxy
struct MSHTML
{
	HRESULT(STDAPICALLTYPE*CreateHTMLPropertyPage)(IMoniker *, IPropertyPage **);
	HRESULT(STDAPICALLTYPE*ShowHTMLDialogEx)(HWND, IMoniker *, DWORD, VARIANT *, LPWSTR, VARIANT *);
	HMODULE H;
};

extern DllProxy::Instance<struct MSHTML> MSHTML;

// KERNEL32 dll proxy for XP+
struct KERNEL32V51
{
	BOOL(WINAPI*AttachConsole)(DWORD);
	HMODULE H;
};

extern DllProxy::Instance<struct KERNEL32V51> KERNEL32V51;

#endif
