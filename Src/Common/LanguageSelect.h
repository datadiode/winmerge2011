/** 
 * @file  LanguageSelect.h
 *
 * @brief Declaration file for CLanguageSelect dialog.
 */
// ID line follows -- this is updated by SVN
// $Id$

#if !defined(AFX_LANGUAGESELECT_H__4395A84F_E8DF_11D1_BBCB_00A024706EDC__INCLUDED_)
#define AFX_LANGUAGESELECT_H__4395A84F_E8DF_11D1_BBCB_00A024706EDC__INCLUDED_

class CStatusBar;

/////////////////////////////////////////////////////////////////////////////
// CLanguageSelect dialog

class LocalString
{
	friend class CLanguageSelect;
	HLOCAL handle;
	UINT id;
	LocalString(HLOCAL handle, UINT id): handle(handle), id(id) { }
	LocalString(const LocalString &);
public:
	~LocalString() { LocalFree(handle); }
	operator LPCTSTR()
	{
		return handle ? reinterpret_cast<LPCTSTR>(handle) : _T("<null>");
	}
	int MsgBox(UINT type = MB_OK);
};

/**
 * @brief Dialog for selecting GUI language.
 *
 * Language select dialog shows list of installed GUI languages and
 * allows user to select one for use.
 */
extern class CLanguageSelect : public OResizableDialog
{
// Construction
public:
	CLanguageSelect(); // standard constructor
	BOOL AreLangsInstalled() const;
	WORD GetLangId() const { return m_wCurLanguage; };
	void InitializeLanguage();
	void ReloadMenu();
	void TranslateDialog(HWND) const;
	String LoadString(UINT) const;
	stl::wstring LoadDialogCaption(LPCTSTR lpDialogTemplateID) const;
	HMenu *LoadMenu(UINT) const;
	HINSTANCE FindResourceHandle(LPCTSTR id, LPCTSTR type) const;
	HACCEL LoadAccelerators(UINT) const;
	HICON LoadIcon(UINT) const;
	HICON LoadSmallIcon(UINT) const;
	HImageList *LoadImageList(UINT, int cx, int cGrow = 0) const;
	bool TranslateString(LPCSTR, stl::string &) const;
	bool TranslateString(LPCWSTR, stl::wstring &) const;
	int DoModal(ODialog &, HWND parent = NULL) const;
	HWND Create(ODialog &, HWND parent = NULL) const;

	LocalString FormatMessage(UINT, ...) const;
	LocalString Format(UINT, ...) const;

	int MsgBox(UINT, UINT = MB_OK) const;
	int MsgBox(UINT, LPCTSTR, UINT = MB_OK) const;
// Implementation data
private:
	HINSTANCE m_hCurrentDll;
	LANGID m_wCurLanguage;
	class strarray
	{
		stl::vector<stl::vector<stl::string> > m_partition;
	public:
		strarray(): m_partition(3)
		{
			clear();
		}
		void clear()
		{
			size_t i = m_partition.size();
			do
			{
				m_partition[--i].resize(1);
			} while (i > 0);
		}
		stl::string &operator[](unsigned i)
		{
			assert(i > 0);
			assert((i >> 16) < m_partition.size());
			stl::vector<stl::string> &partition = m_partition[i >> 16];
			i &= 0xFFFF;
			if (partition.size() <= i)
				partition.resize(i + 1);
			return partition[i];
		}
		const stl::string &operator[](unsigned i) const
		{
			assert((i >> 16) < m_partition.size());
			const stl::vector<stl::string> &partition = m_partition[i >> 16];
			i &= 0xFFFF;
			return partition[i < partition.size() ? i : 0];
		}
	} m_strarray;
	unsigned m_codepage;
// Implementation methods
private:
	String GetFileName(LANGID);
	BOOL LoadLanguageFile(LANGID);
	BOOL SetLanguage(LANGID);
	void LoadAndDisplayLanguages();
	void TranslateMenu(HMENU) const;

	HListBox *m_ctlLangList;

protected:
	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

	// Generated message map functions
	void OnOK();
} LanguageSelect;

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LANGUAGESELECT_H__4395A84F_E8DF_11D1_BBCB_00A024706EDC__INCLUDED_)
