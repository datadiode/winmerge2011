#define DllBuild_Merge7z 10 // Minimum DllBuild of Merge7z plugin required

// We include dllpstub.h for Merge7z.h
// Merge7z::Proxy embeds a DLLPSTUB
#include "dllpstub.h"
#include "../ArchiveSupport/Merge7z/Merge7z.h"

#include "DirView.h"

/**
 * @brief Exception class for more explicit error message.
 */
class C7ZipMismatchException
{
public:
	C7ZipMismatchException(DWORD dwVer7zInstalled, DWORD dwVer7zLocal, LPCTSTR cause)
	{
		m_dwVer7zInstalled = dwVer7zInstalled;
		m_dwVer7zLocal = dwVer7zLocal;
		m_cause = cause;
	}
	~C7ZipMismatchException()
	{
	}
	virtual int ReportError(HWND, UINT nType = MB_OK, UINT nMessageID = 0);
protected:
	DWORD m_dwVer7zInstalled;
	DWORD m_dwVer7zLocal;
	String m_cause;
	BOOL m_bShowAllways;
	static const DWORD m_dwVer7zRecommended;
	static const TCHAR m_strRegistryKey[];
	static const TCHAR m_strDownloadURL[];
	static INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
	static DWORD FormatVersion(LPTSTR, LPTSTR, DWORD);
};

extern Merge7z::Proxy Merge7z;

Merge7z::Format *ArchiveGuessFormat(LPCTSTR);

String NTAPI GetClearTempPath(LPVOID pOwner, LPCTSTR pchExt);

/**
 * @brief temp path context
 */
class CTempPathContext
{
public:
	CTempPathContext *m_pParent;
	String m_strLeftDisplayRoot;
	String m_strRightDisplayRoot;
	String m_strLeftRoot;
	String m_strRightRoot;
	CTempPathContext *DeleteHead();
};

/**
 * @brief Merge7z::DirItemEnumerator to compress a single item.
 */
class SingleItemEnumerator : public Merge7z::DirItemEnumerator
{
	LPCTSTR FullPath;
	LPCTSTR Name;
public:
	virtual UINT Open();
	virtual Merge7z::Envelope *Enum(Item &);
	SingleItemEnumerator(LPCTSTR path, LPCTSTR FullPath, LPCTSTR Name = _T(""));
};

/**
 * @brief Merge7z::DirItemEnumerator to compress items from DirView.
 */
class CDirView::DirItemEnumerator : public Merge7z::DirItemEnumerator
{
private:
	CDirView *const m_pView;
	int const m_nFlags;
	int m_nIndex;
	struct Envelope : public Merge7z::Envelope
	{
		String Name;
		String FullPath;
		virtual void Free()
		{
			delete this;
		}
	};
	stl::list<String> m_rgFolderPrefix;
	stl::list<String>::iterator m_curFolderPrefix;
	String m_strFolderPrefix;
	BOOL m_bRight;
	stl::map<String, void *> m_rgImpliedFoldersLeft;
	stl::map<String, void *> m_rgImpliedFoldersRight;
//	helper methods
	const DIFFITEM &Next();
	bool MultiStepCompressArchive(LPCTSTR);
public:
	enum
	{
		Left = 0x00,
		Right = 0x10,
		Original = 0x20,
		Altered = 0x40,
		DiffsOnly = 0x80,
		BalanceFolders = 0x100
	};
	DirItemEnumerator(CDirView *, int);
	virtual UINT Open();
	virtual Merge7z::Envelope *Enum(Item &);
	void CompressArchive(LPCTSTR = 0);
};

int NTAPI HasZipSupport();
void NTAPI Recall7ZipMismatchError(HWND);

DWORD NTAPI VersionOf7z(BOOL bLocal = FALSE);

/**
 * @brief assign BSTR to String, and return BSTR for optional SysFreeString()
 */
inline BSTR Assign(String &dst, BSTR src)
{
	dst = src;
	return src;
}
