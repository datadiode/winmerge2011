/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997-2000  Thingamahoochie Software
//    Author: Dean Grimm
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
/////////////////////////////////////////////////////////////////////////////
/** 
 * @file  PropVss.cpp
 *
 * @brief VSS properties dialog implementation.
 */
#include "StdAfx.h"
#include "OptionsPanel.h"
#include "resource.h"
#include "LanguageSelect.h"
#include "FileOrFolderSelect.h"
#include "VSSHelper.h" // VCS_* constants
#include "PropVss.h"
#include "Common/MyCom.h"
#include "Common/stream_util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const TCHAR HandlerPathWinMerge[] =
	// These are the two kinds of hardcoded paths created by legacy installers:
	_T("*\\WinMerge.exe;")
	_T("*\\WinMergeU.exe;")
	// Rather more appropriately, point ClearCase to a proper registry entry:
	_T("HKLM\\Software\\Thingamahoochie\\WinMerge\\Executable");

namespace NClearCase
{
	enum EVerbs //: DWORD
	{
		none,
		compare = 1,
		xcompare = 2,
		merge = 4,
		xmerge = 8,
		all = 16 - 1
	};

	class CTypeMgrMapRule
	{
	private:
		TCHAR path[260]; // path to associated tool
	public:
		TCHAR type[40]; // content type
		DWORD Scan(const char *line)
		{
			// following code assumes to be compiled for UNICODE
			C_ASSERT(sizeof(TCHAR) == sizeof(WCHAR));
			if (sscanf(line, "%39ls compare %259ls", type, path) == 2)
				return compare;
			if (sscanf(line, "%39ls xcompare %259ls", type, path) == 2)
				return xcompare;
			if (sscanf(line, "%39ls merge %259ls", type, path) == 2)
				return merge;
			if (sscanf(line, "%39ls xmerge %259ls", type, path) == 2)
				return xmerge;
			return none;
		}
		BOOL IsHandler(LPCTSTR handlerPath) const
		{
			return PathMatchSpec(path, handlerPath);
		}
	};

	class CTypeMgrMapStream : public CMyComPtr<IStream>
	{
	public:
		CTypeMgrMapStream(DWORD grfMode)
		{
			static const TCHAR key[] = _T("SOFTWARE\\Atria\\ClearCase\\CurrentVersion");
			static const TCHAR value[] = _T("ProductHome");
			if (SHRegGetPath(HKEY_LOCAL_MACHINE, key, value, path, 0) == ERROR_SUCCESS)
			{
				PathAppend(path, _T("lib\\mgrs\\map"));
				if (grfMode & STGM_CREATE)
				{
					PathAddExtension(path, _T(".CreatedByWinMerge"));
					DeleteFile(path);
				}
				SHCreateStreamOnFile(path, grfMode, operator &());
			}
		}
		HRESULT WriteRule(LPCTSTR type, DWORD verbs, LPCTSTR path)
		{
			// The canonical form of the path is listed last.
			if (LPCTSTR semicolon = _tcsrchr(path, _T(';')))
				path = semicolon + 1;
			// Pad type field with 1 to 4 tabs for proper column alignment.
			size_t tabs2eat = (_tcslen(type) + 7) / 8;
			if (tabs2eat > 3)
				tabs2eat = 3;
			const char *padding = "\t\t\t\t" + tabs2eat;
			static const char fmt_compare[] =
				"%ls"	"%s" "compare\t\t\t\t"	"%ls\r\n";
			static const char fmt_xcompare[] =
				"%ls"	"%s" "xcompare\t\t\t"	"%ls\r\n";
			static const char fmt_merge[] =
				"%ls"	"%s" "merge\t\t\t\t"	"%ls\r\n";
			static const char fmt_xmerge[] =
				"%ls"	"%s" "xmerge\t\t\t\t"	"%ls\r\n";
			HRESULT hr = S_FALSE;
			char buf[4 * 360];
			// TODO: Make this harder to read.
			return (verbs & compare) && FAILED(hr = operator->()->Write(buf,
					sprintf(buf, fmt_compare, type, padding, path), NULL))
				|| (verbs & xcompare) && FAILED(hr = operator->()->Write(buf,
					sprintf(buf, fmt_xcompare, type, padding, path), NULL))
				|| (verbs & merge) && FAILED(hr = operator->()->Write(buf,
					sprintf(buf, fmt_merge, type, padding, path), NULL))
				|| (verbs & xmerge) && FAILED(hr = operator->()->Write(buf,
					sprintf(buf, fmt_xmerge, type, padding, path), NULL))
				? hr : S_OK;
		}
		LPCTSTR Finish()
		{
			Release();
			return path;
		}
	private:
		TCHAR path[MAX_PATH];
	};
}

/**
 * @brief Constructor.
 * @param [in] optionsMgr Pointer to options manager.
 */
PropVss::PropVss()
: OptionsPanel(IDD_PROP_VSS, sizeof *this)
{
}

template<ODialog::DDX_Operation op>
bool PropVss::UpdateData()
{
	DDX_Text<op>(IDC_PATH_EDIT, m_strPath);
	DDX_CBIndex<op>(IDC_VER_SYS, m_nVerSys);
	return true;
}

DWORD PropVss::GetClearCaseVerbs()
{
	DWORD verbs = NClearCase::none;
	if (IsDlgButtonChecked(IDC_COMPARE))
		verbs |= NClearCase::compare;
	if (IsDlgButtonChecked(IDC_XCOMPARE))
		verbs |= NClearCase::xcompare;
	if (IsDlgButtonChecked(IDC_MERGE))
		verbs |= NClearCase::merge;
	if (IsDlgButtonChecked(IDC_XMERGE))
		verbs |= NClearCase::xmerge;
	return verbs;
}

void PropVss::SetClearCaseVerbs(DWORD verbs)
{
	CheckDlgButton(IDC_COMPARE,
		verbs & NClearCase::compare ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_XCOMPARE,
		verbs & NClearCase::xcompare ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_MERGE,
		verbs & NClearCase::merge ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_XMERGE,
		verbs & NClearCase::xmerge ? BST_CHECKED : BST_UNCHECKED);
}

LRESULT PropVss::OnNotify(UNotify *pNM)
{
	switch (pNM->HDR.idFrom)
	{
	case IDC_TV_CLEARCASE_TYPEMGR_SETUP:
		switch (pNM->HDR.code)
		{
		case TVN_GETDISPINFO:
			if ((pNM->TVDISPINFO.item.lParam ^ pNM->TVDISPINFO.item.state) & TVIS_STATEIMAGEMASK)
			{
				if (!m_bClearCaseTypeMgrSetupModified)
				{
					if (GetClearCaseVerbs() == NClearCase::none)
						SetClearCaseVerbs(NClearCase::all);
					m_bClearCaseTypeMgrSetupModified = true;
				}
			}
			break;
		case NM_DBLCLK:
			if (HTREEITEM hItem = m_pTvClearCaseTypeMgrSetup->GetSelection())
			{
				const UINT stateMask = TVIS_STATEIMAGEMASK | TVIS_BOLD;
				UINT state = m_pTvClearCaseTypeMgrSetup->GetItemState(hItem, stateMask);
				state = state & TVIS_BOLD ? INDEXTOSTATEIMAGEMASK(1) : INDEXTOSTATEIMAGEMASK(2) | TVIS_BOLD;
				m_pTvClearCaseTypeMgrSetup->SetItemState(hItem, state, stateMask);
			}
			break;
		}
		break;
	}
	return 0;
}

LRESULT PropVss::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		if (IsUserInputCommand(wParam))
			UpdateData<Get>();
		switch (wParam)
		{
		case MAKEWPARAM(IDC_BROWSE_BUTTON, BN_CLICKED):
			OnBrowseButton();
			break;
		case MAKEWPARAM(IDC_VER_SYS, CBN_SELCHANGE):
			UpdateScreen();
			break;
		case MAKEWPARAM(IDC_COMPARE, BN_CLICKED):
		case MAKEWPARAM(IDC_XCOMPARE, BN_CLICKED):
		case MAKEWPARAM(IDC_MERGE, BN_CLICKED):
		case MAKEWPARAM(IDC_XMERGE, BN_CLICKED):
			m_bClearCaseTypeMgrSetupModified = true;
			break;
		}
		break;
	case WM_NOTIFY:
		if (LRESULT lResult = OnNotify(reinterpret_cast<UNotify *>(lParam)))
			return lResult;
		break;
	}
	return OptionsPanel::WindowProc(message, wParam, lParam);
}

/** 
 * @brief Reads options values from storage to UI.
 */
void PropVss::ReadOptions()
{
	m_nVerSys = COptionsMgr::Get(OPT_VCS_SYSTEM);
	m_strPath = COptionsMgr::Get(OPT_VSS_PATH);
}

/** 
 * @brief Writes options values from UI to storage.
 */
void PropVss::WriteOptions()
{
	COptionsMgr::SaveOption(OPT_VCS_SYSTEM, m_nVerSys);
	COptionsMgr::SaveOption(OPT_VSS_PATH, m_strPath);
	if (m_bClearCaseTypeMgrSetupModified)
	{
		NClearCase::CTypeMgrMapStream src(STGM_READ | STGM_SHARE_DENY_WRITE);
		NClearCase::CTypeMgrMapStream dst(STGM_WRITE | STGM_CREATE);
		if (src && dst)
		{
			DWORD verbs = GetClearCaseVerbs();
			TVITEM item;
			item.mask = TVIF_TEXT | TVIF_STATE | TVIF_PARAM;
			TCHAR text[40];
			item.pszText = text;
			item.cchTextMax = _countof(text);
			// Place rules for checked types at beginning of map file.
			item.hItem = m_pTvClearCaseTypeMgrSetup->GetRootItem();
			while (item.hItem)
			{
				const UINT stateMask = INDEXTOSTATEIMAGEMASK(2);
				m_pTvClearCaseTypeMgrSetup->GetItem(&item);
				if ((item.state & stateMask) == stateMask)
					OException::Check(dst.WriteRule(item.pszText, verbs, HandlerPathWinMerge));
				item.hItem = m_pTvClearCaseTypeMgrSetup->GetNextSiblingItem(item.hItem);
			}
			// Leave the whole bunch of rules not referring to WinMerge as is.
			stl::string line;
			StreamLineReader reader = src;
			while (reader.readLine(line))
			{
				NClearCase::CTypeMgrMapRule rule;
				if (!rule.Scan(line.c_str()) || !rule.IsHandler(HandlerPathWinMerge))
					OException::Check(dst->Write(line.c_str(), line.length(), NULL));
			}
			// Place rules for unchecked but bold types at end of map file.
			item.hItem = m_pTvClearCaseTypeMgrSetup->GetRootItem();
			while (item.hItem)
			{
				const UINT stateMask = INDEXTOSTATEIMAGEMASK(1) | TVIS_BOLD;
				m_pTvClearCaseTypeMgrSetup->GetItem(&item);
				if ((item.state & stateMask) == stateMask)
					OException::Check(dst.WriteRule(item.pszText, verbs, HandlerPathWinMerge));
				item.hItem = m_pTvClearCaseTypeMgrSetup->GetNextSiblingItem(item.hItem);
			}
			CopyFile(dst.Finish(), src.Finish(), FALSE);
		}
	}
}

/**
 * @brief Called when Browse-button is selected.
 */
void PropVss::OnBrowseButton()
{
	String s;
	if (SelectFile(m_hWnd, s))
		SetDlgItemText(IDC_PATH_EDIT, s.c_str());
}

/**
 * @brief Initialized the dialog.
 * @return Always TRUE.
 */
BOOL PropVss::OnInitDialog()
{
	if (HComboBox *combo = static_cast<HComboBox *>(GetDlgItem(IDC_VER_SYS)))
	{
		/*
		Must be in order to agree with enum in MainFrm.h
		VCS_NONE = 0,
		VCS_VSS4,
		VCS_VSS5,
		VCS_CLEARCASE,
		VCS_TFS,
		*/
		combo->AddString(LanguageSelect.LoadString(IDS_VCS_NONE).c_str());
		combo->AddString(LanguageSelect.LoadString(IDS_VCS_VSS4).c_str());
		combo->AddString(LanguageSelect.LoadString(IDS_VCS_VSS5).c_str());
		combo->AddString(LanguageSelect.LoadString(IDS_VCS_CLEARCASE).c_str());
		combo->AddString(LanguageSelect.LoadString(IDS_VCS_TFS).c_str());
	}
	m_pTvClearCaseTypeMgrSetup = static_cast<HTreeView *>(GetDlgItem(IDC_TV_CLEARCASE_TYPEMGR_SETUP));
	m_pTvClearCaseTypeMgrSetup->SetStyle(m_pTvClearCaseTypeMgrSetup->GetStyle() | TVS_CHECKBOXES);
	if (NClearCase::CTypeMgrMapStream pstm = STGM_READ | STGM_SHARE_DENY_WRITE)
	{
		stl::string line;
		StreamLineReader reader = pstm;
		DWORD verbs = NClearCase::none;
		while (reader.readLine(line))
		{
			NClearCase::CTypeMgrMapRule rule;
			DWORD verb = rule.Scan(line.c_str());
			if (verb == 0)
				continue;
			TVINSERTSTRUCT tvis = { NULL };
			tvis.hParent = TVI_ROOT;
			tvis.hInsertAfter = TVI_SORT;
			tvis.item.mask = TVIF_TEXT | TVIF_STATE | TVIF_CHILDREN | TVIF_PARAM;
			tvis.item.stateMask = TVIS_STATEIMAGEMASK;
			tvis.item.cChildren = I_CHILDRENCALLBACK;
			const BOOL bMatch = rule.IsHandler(HandlerPathWinMerge);
			tvis.item.lParam = tvis.item.state = bMatch ? INDEXTOSTATEIMAGEMASK(2) : INDEXTOSTATEIMAGEMASK(1);
			tvis.item.pszText = rule.type;
			HTREEITEM hItem = m_pTvClearCaseTypeMgrSetup->InsertItem(&tvis);
			if (HTREEITEM hPrevious = m_pTvClearCaseTypeMgrSetup->GetNextItem(hItem, TVGN_PREVIOUS))
			{
				TVITEM item;
				item.mask = TVIF_TEXT;
				TCHAR text[40];
				item.pszText = text;
				item.cchTextMax = _countof(text);
				item.hItem = hPrevious;
				if (m_pTvClearCaseTypeMgrSetup->GetItem(&item) &&
					_tcscmp(item.pszText, tvis.item.pszText) == 0)
				{
					m_pTvClearCaseTypeMgrSetup->DeleteItem(hItem);
					hItem = hPrevious;
				}
			}
			if (bMatch)
			{
				verbs |= verb;
				m_pTvClearCaseTypeMgrSetup->SetItemState(hItem, TVIS_BOLD, TVIS_BOLD);
			}
		}
		SetClearCaseVerbs(verbs ? verbs : NClearCase::none);
	}
	return OptionsPanel::OnInitDialog();
}

/**
 * @brief Called when user has selected VSS version.
 */
void PropVss::UpdateScreen() 
{
	UpdateData<Set>();
	String tempStr = LanguageSelect.LoadString(
		m_nVerSys == VCS_CLEARCASE ? IDS_CC_CMD :
		m_nVerSys == VCS_TFS ? IDS_TF_CMD :
		IDS_VSS_CMD);
	SetDlgItemText(IDC_VSS_L1, tempStr.c_str());
	BOOL enable = m_nVerSys == VCS_VSS4 || m_nVerSys == VCS_CLEARCASE || m_nVerSys == VCS_TFS;
	GetDlgItem(IDC_PATH_EDIT)->EnableWindow(enable);
	GetDlgItem(IDC_VSS_L1)->EnableWindow(enable);
	GetDlgItem(IDC_BROWSE_BUTTON)->EnableWindow(enable);
}
