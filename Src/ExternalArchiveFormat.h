class CExternalArchiveFormat : public Merge7z::Format
{
private:
	typedef stl::map<String, stl::auto_ptr<CExternalArchiveFormat> > Map;
	static Map m_map;
	static String GetShortPathName(LPCTSTR, DWORD = 0);
	int SetPath(String &, LPCTSTR, LPCTSTR, DWORD = 0) const;
	static int RunModal(LPCTSTR, LPCTSTR, UINT);
	class Profile;
	friend Profile;
	String m_strCmdDeCompress;			// decompression command with placeholders
	String m_strCmdCompress;			// compression command with placeholders
	int m_nBulkSize;					// maximum # of items to pass on command line
	String::size_type m_cchCmdMax;		// maximum length of command line
	BOOL m_bLongPathPrefix;				// whether to allow a long path prefix
	int m_nShowConsole;
public:
	static Merge7z::Format *GuessFormat(LPCTSTR);
	static String GetOpenFileFilterString();
	CExternalArchiveFormat(const Profile &, LPCTSTR);
	virtual HRESULT DeCompressArchive(HWND, LPCTSTR path, LPCTSTR folder);
	virtual HRESULT CompressArchive(HWND, LPCTSTR path, Merge7z::DirItemEnumerator *);
DllBuild_Merge7z_9:
	virtual Inspector *Open(HWND, LPCTSTR) { return NULL; }
	virtual Updater *Update(HWND, LPCTSTR) { return NULL; }
DllBuild_Merge7z_10:
	virtual HRESULT GetHandlerProperty(HWND, PROPID, PROPVARIANT *, VARTYPE) { return E_NOTIMPL; }
	virtual BSTR GetHandlerName(HWND) { return NULL; }
	virtual BSTR GetHandlerClassID(HWND) { return NULL; }
	virtual BSTR GetHandlerExtension(HWND) { return NULL; }
	virtual BSTR GetHandlerAddExtension(HWND) { return NULL; }
	virtual VARIANT_BOOL GetHandlerUpdate(HWND) { return FALSE; }
	virtual VARIANT_BOOL GetHandlerKeepName(HWND) { return FALSE; }
	virtual BSTR GetDefaultName(HWND, LPCTSTR) { return NULL; }
};
