// H2O2.h
//
// H2O makes OS handles apear like pointers to C++ objects.
// H2O2 implements traditional OS handle wrapper classes.
// Both H2O and H2O2 share a common set of decorator templates.
//
// Copyright (c) 2005-2010  David Nash (as of Win32++ v7.0.2)
// Copyright (c) 2011-2013  Jochen Neubeck
//
// Permission is hereby granted, free of charge, to
// any person obtaining a copy of this software and
// associated documentation files (the "Software"),
// to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify,
// merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom
// the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice
// shall be included in all copies or substantial portions
// of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
// SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "FloatState.h"

namespace H2O
{
	class Object
	// The union members are protected by default.
	// Derived classes unprotect the ones they use.
	{
	protected:
		union
		{
			HWND m_hWnd;
			HWindow *m_pWnd;
			HDC m_hDC;
			HSurface *m_pDC;
			// SysString
			HString *m_pStr;
			OLECHAR *B;
			CHAR *A;
			TCHAR *T;
			WCHAR *W;
		};
		Object() { }
	private:
		Object(const Object &); // disallow copy construction
		void operator=(const Object &); // disallow assignment
	};

	class OException
	{
	private:
		// OException lives in the stack frame, and is not affected by stack
		// unwinding since there is no destructor. For consistency with well-
		// established coding style, still provide a delete operator to be
		// called from the catch block even if it is a nop.
		void *operator new(size_t);
	public:
		static void Throw(DWORD, LPCTSTR = NULL);
		static void ThrowSilent();
		static void Check(HRESULT);
		TCHAR msg[1024];
		int ReportError(HWND, UINT = MB_ICONSTOP) const;
		OException(DWORD, LPCTSTR = NULL);
		void operator delete(void *) { }
	};

	class OWindow : public Window<Object>
	{
	public:
		using Window<Object>::m_hWnd;
		using Window<Object>::m_pWnd;
		OWindow(HWND hWnd = NULL)
			: m_pfnSuper(NULL)
			, m_bAutoDelete(false)
		{
			m_hWnd = hWnd;
		}
		void SubclassWindow(HWindow *pWnd)
		{
			SubclassWindow<GWLP_USERDATA>(pWnd);
		}
		void SubclassDlgItem(int id, OWindow &item)
		{
			item.SubclassWindow(GetDlgItem(id));
		}
		static OWindow *FromHandle(HWindow *pWnd)
		{
			return FromHandle<GWLP_USERDATA>(pWnd);
		}
		virtual ~OWindow();
	protected:
		template<UINT GWLP_THIS>
		void SubclassWindow(HWindow *pWnd)
		{
			m_pWnd = pWnd;
			m_pfnSuper = SetWindowPtr<WNDPROC>(GWLP_WNDPROC, WndProc<GWLP_THIS>);
			assert(m_pfnSuper != WndProc<GWLP_THIS>);
			void *p = SetWindowPtr(GWLP_THIS, this);
			assert(p == NULL);
		}
		template<UINT GWLP_THIS>
		static OWindow *FromHandle(HWindow *pWnd)
		{
			return reinterpret_cast<OWindow *>(::GetWindowLongPtr(pWnd->m_hWnd, GWLP_THIS));
		}
		virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
		WNDPROC m_pfnSuper;
		bool m_bAutoDelete;
	public:
		template<UINT uMsg>
		LRESULT MessageReflect_TopLevelWindow(WPARAM, LPARAM);
		template<UINT>
		LRESULT MessageReflect_WebLinkButton(WPARAM, LPARAM);
		template<UINT>
		LRESULT MessageReflect_ColorButton(WPARAM, LPARAM);
		void SwapPanes(UINT, UINT);
	private:
		template<UINT GWLP_THIS>
		static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			OWindow *const pThis = FromHandle<GWLP_THIS>(reinterpret_cast<HWindow *>(hWnd));
			LRESULT lResult = 0;
			try
			{
				lResult = pThis->WindowProc(uMsg, wParam, lParam);
			}
			catch (OException *e)
			{
				e->ReportError(hWnd);
				delete e;
			}
			return lResult;
		}
		template<class T>
		T SetWindowPtr(int i, T p) const
		{
			return reinterpret_cast<T>(
				::SetWindowLongPtr(m_hWnd, i, reinterpret_cast<LONG_PTR>(p)));
		}
	};

	class ODialog : public OWindow
	{
		friend class OPropertySheet;
	public:
		LPCTSTR const m_idd;
		ODialog(LPCTSTR idd): m_idd(idd) { }
		ODialog(UINT idd): m_idd(MAKEINTRESOURCE(idd)) { }
		virtual INT_PTR DoModal(HINSTANCE hinst, HWND parent);
		virtual HWND Create(HINSTANCE hinst, HWND parent);
		BOOL EndDialog(INT_PTR nResult)
		{
			assert(::IsWindow(m_hWnd));
			return ::EndDialog(m_hWnd, nResult);
		}
		BOOL MapDialogRect(RECT *prc)
		{
			assert(::IsWindow(m_hWnd));
			return ::MapDialogRect(m_hWnd, prc);
		}
		void SetDefID(UINT id)
		{
			SendMessage(DM_SETDEFID, id);
		}
		UINT GetDefID()
		{
			return static_cast<UINT>(SendMessage(DM_GETDEFID));
		}
		void GotoDlgCtrl(HWindow *pCtl)
		{
			SendMessage(WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(pCtl), 1);
		}
		static ODialog *FromHandle(HWindow *pWnd)
		{
			return reinterpret_cast<ODialog *>(::GetWindowLongPtr(pWnd->m_hWnd, DWLP_USER));
		}

		// Dialog data exchange
		enum DDX_Operation { Set, Get };

	protected:
		virtual BOOL OnInitDialog();
		virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

		BOOL IsUserInputCommand(WPARAM wParam);
		void Update3StateCheckBoxLabel(UINT id);

		template<DDX_Operation>
		bool DDX_Text(UINT id, String &);
		template<>
		bool DDX_Text<Set>(UINT id, String &s)
		{
			SetDlgItemText(id, s.c_str());
			return true;
		}
		template<>
		bool DDX_Text<Get>(UINT id, String &s)
		{
			GetDlgItemText(id, s);
			return true;
		}

		template<DDX_Operation>
		bool DDX_Text(UINT id, int &n);
		template<>
		bool DDX_Text<Set>(UINT id, int &n)
		{
			SetDlgItemInt(id, n);
			return true;
		}
		template<>
		bool DDX_Text<Get>(UINT id, int &n)
		{
			n = GetDlgItemInt(id);
			return true;
		}

		template<DDX_Operation>
		bool DDX_Text(UINT id, UINT &n);
		template<>
		bool DDX_Text<Set>(UINT id, UINT &n)
		{
			SetDlgItemInt(id, n, FALSE);
			return true;
		}
		template<>
		bool DDX_Text<Get>(UINT id, UINT &n)
		{
			n = GetDlgItemInt(id, NULL, FALSE);
			return true;
		}

		template<DDX_Operation>
		bool DDX_CBIndex(UINT id, int &n);
		template<>
		bool DDX_CBIndex<Set>(UINT id, int &n)
		{
			SendDlgItemMessage(id, CB_SETCURSEL, static_cast<WPARAM>(n));
			return true;
		}
		template<>
		bool DDX_CBIndex<Get>(UINT id, int &n)
		{
			n = static_cast<int>(SendDlgItemMessage(id, CB_GETCURSEL));
			return true;
		}

		template<DDX_Operation>
		bool DDX_Check(UINT id, int &n);
		template<>
		bool DDX_Check<Set>(UINT id, int &n)
		{
			CheckDlgButton(id, n);
			return true;
		}
		template<>
		bool DDX_Check<Get>(UINT id, int &n)
		{
			n = IsDlgButtonChecked(id);
			return true;
		}

		template<DDX_Operation>
		bool DDX_Check(UINT id, int &n, int value);
		template<>
		bool DDX_Check<Set>(UINT id, int &n, int value)
		{
			CheckDlgButton(id, n == value);
			return true;
		}
		template<>
		bool DDX_Check<Get>(UINT id, int &n, int value)
		{
			if (IsDlgButtonChecked(id))
				n = value;
			return true;
		}

		template<DDX_Operation>
		bool DDX_CBStringExact(UINT id, String &s);
		template<>
		bool DDX_CBStringExact<Set>(UINT id, String &s)
		{
			int i = static_cast<int>(SendDlgItemMessage(id,
				CB_FINDSTRINGEXACT, -1, reinterpret_cast<LPARAM>(s.c_str())));
			SendDlgItemMessage(id, CB_SETCURSEL, i);
			if (i < 0)
				SetDlgItemText(id, s.c_str());
			return true;
		}
		template<>
		bool DDX_CBStringExact<Get>(UINT id, String &s)
		{
			GetDlgItemText(id, s);
			return true;
		}

	private:
		static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
		static INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
	};

	class OPropertySheet
	{
	public:
		PROPSHEETHEADER m_psh;
		String m_caption;
		OPropertySheet();
		PROPSHEETPAGE *AddPage(ODialog &page);
		INT_PTR DoModal(HINSTANCE hinst, HWND parent);
	private:
		std::vector<PROPSHEETPAGE> m_pages;
		static int CALLBACK PropSheetProc(HWND, UINT, LPARAM);
		static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
	};

	class OResizableDialog : public ODialog, protected CFloatState
	{
	public:
		OResizableDialog(LPCTSTR idd): ODialog(idd) { }
		OResizableDialog(UINT idd): ODialog(MAKEINTRESOURCE(idd)) { }
	protected:
		virtual BOOL OnInitDialog();
		virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	};

	class OString : public SysString<Object>
	{
	public:
		using SysString<Object>::m_pStr;
		using SysString<Object>::B;
		using SysString<Object>::A;
		using SysString<Object>::W;
		using SysString<Object>::T;
		OString(HString *pStr = NULL) { m_pStr = pStr; }
		~OString() { SysString<Object>::Free(); }
		void Free()
		{
			SysString<Object>::Free();
			B = NULL;
		}
		void Append(LPCWSTR tail)
		{
			UINT len = Len();
			if (SysReAllocStringLen(&B, NULL, len + lstrlenW(tail)))
			{
				lstrcpyW(W + len, tail);
			}
		}
		void Append(HString *pStr)
		{
			Append(pStr->W);
			pStr->Free();
		}
		HString *SplitAt(LPCWSTR pattern)
		{
			HString *pStr = NULL;
			if (LPCWSTR match = StrStrW(W, pattern))
			{
				pStr = HString::Uni(match + lstrlenW(pattern));
				if (pStr)
				{
					SysReAllocStringLen(&B, NULL, static_cast<UINT>(match - W));
				}
			}
			return pStr;
		}
	};

	typedef ListBox<OWindow> OListBox;
	typedef ComboBox<OWindow> OComboBox;
	typedef Edit<OWindow> OEdit;
	typedef Static<OWindow> OStatic;
	typedef Button<OWindow> OButton;
	typedef HeaderCtrl<OWindow> OHeaderCtrl;
	typedef ListView<OWindow> OListView;
	typedef StatusBar<OWindow> OStatusBar;

	template<class Self>
	class ZeroInit
	{
	protected:
		ZeroInit(size_t cb) { memset(static_cast<Self *>(this), 0, cb); }
		ZeroInit() { memset(static_cast<Self *>(this), 0, sizeof(Self)); }
	};

	// Utility functions
	HWND GetTopLevelParent(HWND);
	HIMAGELIST Create3StateImageList();
	void GetDesktopWorkArea(HWND, LPRECT);
	void CenterWindow(HWindow *, HWindow *pParent = NULL);
}
