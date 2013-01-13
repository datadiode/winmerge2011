class CMarkdown
{
//	Core class
public:
	class Converter
	{
	//	ICONV wrapper
	public:
		HANDLE handle;
		Converter(const char *tocode, const char *fromcode);
		~Converter();
		size_t iconv(const char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft) const;
		size_t Convert(const char *, size_t, char *, size_t) const;
	};
	typedef stl::map<stl::wstring, stl::wstring> EntityMap;
	static void Load(EntityMap &);
	void Load(EntityMap &, const Converter &);
	class FileImage;
	class File;
	// An _HSTR is a handle to a string
	// It may be any one (and only one) of the following
	//  - CHAR (use _HSTR::A)
	//  - WCHAR (use _HSTR::W)
	//  - TCHAR (use __HSTR::T)
	//  - OLECHAR (use __HSTR::B)
	typedef class _HSTR : public HString
	{
	public:
		_HSTR *Convert(const Converter &);
		_HSTR *Trim(const OLECHAR *);
		HString *Uni(const EntityMap &, unsigned = CP_UTF8);
	} *HSTR;
	static HString *Entitify(const OLECHAR *);
	typedef H2O::OString String;
	const char *first;	// first char of current markup (valid after Move)
	const char *lower;	// beginning of enclosed text (valid after Move)
	const char *upper;	// end of enclosed text (initially beginning of file)
	const char *ahead;	// last char of file
	enum
	{
		IgnoreCase = 0x10,
		HtmlUTags = 0x20,			// check for unbalanced tags
		Html = IgnoreCase|HtmlUTags	// shortcut
	};
	CMarkdown(const char *upper, const char *ahead, unsigned flags = 0);
	operator bool();				// is node ahead?
	void Scan();					// find closing tag
	CMarkdown &Move();				// move to next node
	CMarkdown &Move(const char *);	// move to next node with given name
	bool Pull();					// pull child nodes into view
	CMarkdown &Pop();				// irreversible pull for chained calls
	bool Push();					// reverse pull
	HSTR GetTagName();				// tag name
	HSTR GetTagText();				// tag name plus attributes
	HSTR GetInnerText();			// text between enclosing tags
	HSTR GetOuterText();			// text including enclosing tags
	HSTR GetAttribute(const char *, const void * = 0); // random or enumerate
private:
	int (__cdecl *const memcmp)(const void *, const void *, size_t);
	const char *const utags;
	int FindTag(const char *, const char *);
	class Token;
};

class CMarkdown::FileImage
{
//	Map a file into process memory. Optionally convert UCS2 source to UTF8.
public:
	DWORD cbImage;
	union
	{
		PVOID pvImage;
		PBYTE pbImage;
		PCHAR pcImage;
	};
	enum
	{
		Handle = 0x01,
		Octets = 0x02 + 0x04,
		Mapping = 0x40
	};
	int nByteOrder;
	FileImage(LPCTSTR, DWORD trunc = 0, int flags = 0);
	~FileImage();
	static LPVOID NTAPI MapFile(HANDLE hFile, DWORD dwSize);
	static int NTAPI GuessByteOrder(DWORD);
};

class CMarkdown::File : public CMarkdown::FileImage, public CMarkdown
{
//	Construct CMarkdown object from file.
public:
	File(LPCTSTR path, DWORD trunc = 0, unsigned flags = Octets)
		: FileImage(path, trunc, flags)
		, CMarkdown(pcImage, pcImage + cbImage, flags)
	{
	}
};
