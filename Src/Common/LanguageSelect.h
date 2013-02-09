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
		typedef stl::vector<LPCSTR> t_vec;
		t_vec m_vec;
	public:
		strarray()
		{
			clear();
		}
		~strarray()
		{
			clear();
		}
		void clear()
		{
			t_vec::size_type i = m_vec.size();
			while (i > 1)
			{
				--i;
				LPCSTR text = m_vec[i];
				if (HIWORD(text))
					free(const_cast<LPSTR>(text));
			}
			m_vec.resize(1, "");
		}
		LPCSTR setAtGrow(t_vec::size_type i, LPCSTR text)
		{
			if (m_vec.size() <= i)
			{
				m_vec.resize(i + 1, NULL);
			}
			else
			{
				LPCSTR text = m_vec[i];
				if (HIWORD(text))
					free(const_cast<LPSTR>(text));
			}
			LPCSTR link = text;
			if (HIWORD(text))
			{
				text = _strdup(text);
				link = reinterpret_cast<LPCSTR>(i);
			}
			m_vec[i] = text;
			return link;
		}
		LPCSTR operator[](t_vec::size_type i) const
		{
			LPCSTR text = m_vec[i < m_vec.size() ? i : 0];
			if (HIWORD(text) == 0)
				text = m_vec[LOWORD(text)];
			return text;
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
