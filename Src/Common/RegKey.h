/** 
 * @file  RegKey.cpp
 *
 * @brief Declaration of CRegKeyEx C++ wrapper class for reading Windows registry
 */
// ID line follows -- this is updated by SVN
// $Id$

/**
 * @brief Class for reading/writing registry.
 */
class CRegKeyEx
{

// Construction
public:
	CRegKeyEx(HKEY hKey = NULL);
	~CRegKeyEx();

// Operations
public:
	operator HKEY() const { return m_hKey; }
	void Close();
	LONG Open(HKEY hKeyRoot, LPCTSTR pszPath);
	LONG OpenWithAccess(HKEY hKeyRoot, LPCTSTR pszPath, REGSAM regsam);
	LONG OpenNoCreateWithAccess(HKEY hKeyRoot, LPCTSTR pszPath, REGSAM regsam);
	LONG QueryRegMachine(LPCTSTR key);
	LONG QueryRegUser(LPCTSTR key);

	LONG WriteDword(LPCTSTR pszKey, DWORD dwVal) const;
	LONG WriteString(LPCTSTR pszKey, LPCTSTR pszVal) const;

	DWORD ReadDword(LPCTSTR pszKey, DWORD defval) const;
	template<size_t cb>
	struct Blob : BLOB
	{
		BYTE buf[cb];
		Blob()
		{
			cbSize = cb;
			pBlobData = buf;
		}
		operator BLOB *()
		{
			return this;
		}
	};
	typedef Blob<MAX_PATH * sizeof(TCHAR)> MaxPath;
	LPCTSTR ReadString(LPCTSTR pszKey, LPCTSTR defval, BLOB * = MaxPath()) const;

	LONG DeleteValue(LPCTSTR pszKey) const;

protected:
	HKEY m_hKey; /**< Open key (HKLM, HKCU, etc). */
};
