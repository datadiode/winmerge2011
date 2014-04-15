#define DllBuild_Merge7z 10 // Minimum DllBuild of Merge7z plugin required

#include "../ArchiveSupport/Merge7z/Merge7z.h"

#include "DirView.h"

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
	String m_strLeftParent;
	String m_strRightParent;
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
	const DIFFITEM *Next();
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

DWORD NTAPI VersionOf7z();

/**
 * @brief assign BSTR to String, and return BSTR for optional SysFreeString()
 */
inline BSTR Assign(String &dst, BSTR src)
{
	dst.assign(src, SysStringLen(src));
	return src;
}
