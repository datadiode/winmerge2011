/** 
 * @file  LanguageSelect.h
 *
 * @brief Declaration file for CLanguageSelect dialog.
 */
#pragma once

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
public:
	CLanguageSelect();
	WORD GetLangId() const { return m_wCurLanguage; };
	void InitializeLanguage();
	void UpdateResources();
	void TranslateDialog(HWND) const;
	String LoadString(UINT) const;
	std::wstring LoadDialogCaption(LPCTSTR lpDialogTemplateID) const;
	HMenu *LoadMenu(UINT) const;
	HINSTANCE FindResourceHandle(LPCTSTR id, LPCTSTR type) const;
	HACCEL LoadAccelerators(UINT) const;
	HICON LoadIcon(UINT) const;
	HICON LoadSmallIcon(UINT) const;
	HImageList *LoadImageList(UINT, int cx, int cGrow = 0) const;
	bool TranslateString(LPCSTR, std::string &) const;
	bool TranslateString(LPCWSTR, std::wstring &) const;
	int DoModal(ODialog &, HWND parent = NULL) const;
	HWND Create(ODialog &, HWND parent = NULL) const;

	LocalString FormatMessage(UINT, ...) const;
	LocalString Format(UINT, ...) const;
	LocalString FormatStrings(UINT, UINT, ...) const;

	int MsgBox(UINT, UINT = MB_OK) const;
	int MsgBox(UINT, LPCTSTR, UINT = MB_OK) const;

	bool GetPoHeaderProperty(const char *name, std::string &value) const;
// Implementation data
private:
	HINSTANCE m_hCurrentDll;
	LANGID m_wCurLanguage;
	std::string m_poheader;
	class strarray
	{
		std::vector<LPCSTR> m_vec;
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
			stl_size_t i = m_vec.size();
			while (i > 1)
			{
				--i;
				LPCSTR text = m_vec[i];
				if (HIWORD(text))
					free(const_cast<LPSTR>(text));
			}
			m_vec.resize(1, "");
		}
		LPCSTR setAtGrow(stl_size_t i, LPCSTR text)
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
		LPCSTR operator[](stl_size_t i) const
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
	bool LoadLanguageFile(LANGID);
	bool SetLanguage(LANGID);
	void LoadAndDisplayLanguages();
	void TranslateMenu(HMenu *) const;

	HListBox *m_ctlLangList;

protected:
	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

	// Generated message map functions
	void OnOK();
} LanguageSelect;

#define FormatStrings(fmt, ...) FormatStrings(fmt, VA_NUM_ARGS(__VA_ARGS__), __VA_ARGS__)
