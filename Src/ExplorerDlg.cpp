/*
 * Copyright (c) 2017 Jochen Neubeck
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/**
 * @file ExplorerDlg.cpp
 *
 * @brief Implementation of ExplorerDlg class
 * @credits Contains snippets from https://github.com/minfinitum/VFileView.
 */
#include "StdAfx.h"
#include "resource.h"
#include "paths.h"
#include "Common/coretools.h"
#include "Common/coretypes.h"
#include "Common/RegKey.h"
#include "Common/SettingStore.h"
#include "Common/LanguageSelect.h"
#include "Common/PidlManager.h"
#include "ExplorerDlg.h"
#include "SuperComboBox.h"

static COLORREF const COLOUR_NORMAL		= RGB(0, 0, 0);
static COLORREF const COLOUR_HIDDEN		= RGB(127, 127, 127);
static COLORREF const COLOUR_SYSTEM		= RGB(127, 127, 127);
static COLORREF const COLOUR_COMPRESS	= RGB(0, 0, 255);

enum
{
	FILE_NAME_INDEX,
	FILE_CREATED_INDEX,
	FILE_LAST_ACCESSED_INDEX,
	FILE_LAST_WRITE_INDEX,
	FILE_ATTR_INDEX,
	FILE_SIZE_INDEX,
	FILE_INDEX_COUNT
};

enum
{
	LVCFMT_DEFSORTUP = 4,
	LVCFMT_DEFHIDDEN = 8
};

static struct /**< Information about one column of ExplorerDlg list info */
{
	LPCWSTR regName; /**< Internal name used for registry entries etc */
	// localized string resources
	WORD idName; /**< Displayed name, ID of string resource */
	WORD fmt; /**< Column alignment etc. */
}
const cols[] =
{
	{ L"Name",			IDS_COLHDR_FILENAME,	LVCFMT_LEFT | LVCFMT_DEFSORTUP	},
	{ L"Created",		IDS_COLHDR_TIMEC,		LVCFMT_LEFT | LVCFMT_DEFHIDDEN	},
	{ L"Accessed",		IDS_COLHDR_TIMEA,		LVCFMT_LEFT | LVCFMT_DEFHIDDEN	},
	{ L"Modified",		IDS_COLHDR_TIMEM,		LVCFMT_LEFT						},
	{ L"Attributes",	IDS_COLHDR_ATTRIBUTES,	LVCFMT_LEFT | LVCFMT_DEFSORTUP	},
	{ L"Size",			IDS_COLHDR_SIZE,		LVCFMT_RIGHT | LVCFMT_DEFSORTUP	}
};

C_ASSERT(_countof(cols) == FILE_INDEX_COUNT);

static UINT const WM_APP_UpdateSelection = WM_APP + 1;

static DWORD const ProtectiveFileAttributes = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN;

static ULONGLONG GetFileSize(WIN32_FIND_DATAW const *pfd)
{
	ULARGE_INTEGER li = { pfd->nFileSizeLow, pfd->nFileSizeHigh };
	return li.QuadPart;
}

static void AttributesToText(DWORD attr, LPWSTR text)
{
	if (attr & FILE_ATTRIBUTE_READONLY)
		*text++ = L'R';
	if (attr & FILE_ATTRIBUTE_HIDDEN)
		*text++ = L'H';
	if (attr & FILE_ATTRIBUTE_SYSTEM)
		*text++ = L'S';
	if (attr & FILE_ATTRIBUTE_DIRECTORY)
		*text++ = L'D';
	if (attr & FILE_ATTRIBUTE_ARCHIVE)
		*text++ = L'A';
	if (attr & FILE_ATTRIBUTE_NORMAL)
		*text++ = L'N';
	if (attr & FILE_ATTRIBUTE_TEMPORARY)
		*text++ = L'T';
	if (attr & FILE_ATTRIBUTE_COMPRESSED)
		*text++ = L'C';
	*text = L'\0';
}

ExplorerDlg::ExplorerDlg(UINT idd)
	: OResizableDialog(idd)
	, CSplitState(SplitScript)
	, m_bHasListView(m_idd == MAKEINTRESOURCE(IDD_BROWSE_FOR_FILE))
	, m_bEnableFolderSelect(m_idd == MAKEINTRESOURCE(IDD_BROWSE_FOR_FOLDER))
{
	if (m_bHasListView)
	{
		static LONG const FloatScript[] =
		{
			IDC_TREE_FOLDER,		BY<1000>::Y2B,
			IDC_LIST_FILE,			BY<1000>::X2R | BY<1000>::Y2B,
			IDC_STATIC_FILTER,		BY<1000>::Y2T | BY<1000>::Y2B,
			IDC_TAB_FILTER,			BY<1000>::Y2T | BY<1000>::Y2B,
			IDC_BUTTON_NEW_FOLDER,	BY<1000>::Y2T | BY<1000>::Y2B,
			IDC_STATIC_FILENAME,	BY<1000>::Y2T | BY<1000>::Y2B,
			IDC_COMBO_FILTER,		BY<1000>::X2R | BY<1000>::Y2T | BY<1000>::Y2B,
			IDOK,					BY<1000>::X2L | BY<1000>::X2R | BY<1000>::Y2T | BY<1000>::Y2B,
			IDCANCEL,				BY<1000>::X2L | BY<1000>::X2R | BY<1000>::Y2T | BY<1000>::Y2B,
			0
		};
		static LONG const SplitScript[] =
		{
			IDC_TREE_FOLDER, IDC_LIST_FILE, ~20,
			0
		};
		ExplorerDlg::FloatScript = FloatScript;
		ExplorerDlg::SplitScript = SplitScript;
	}
	else
	{
		static LONG const FloatScript[] =
		{
			IDC_TREE_FOLDER,		BY<1000>::X2R | BY<1000>::Y2B,
			IDC_STATIC_FILTER,		BY<1000>::Y2T | BY<1000>::Y2B,
			IDC_TAB_FILTER,			BY<1000>::Y2T | BY<1000>::Y2B,
			IDC_BUTTON_NEW_FOLDER,	BY<1000>::Y2T | BY<1000>::Y2B,
			IDOK,					BY<1000>::X2L | BY<1000>::X2R | BY<1000>::Y2T | BY<1000>::Y2B,
			IDCANCEL,				BY<1000>::X2L | BY<1000>::X2R | BY<1000>::Y2T | BY<1000>::Y2B,
			0
		};
		ExplorerDlg::FloatScript = FloatScript;
	}
	SHGetDesktopFolder(&m_shell);
}

ExplorerDlg::~ExplorerDlg()
{
	if (m_network)
		m_network->Release();
	if (m_shell)
		m_shell->Release();
}

LRESULT ExplorerDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MOUSEMOVE:
		CSplitState::Split(m_hWnd);
		break;
	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			OnOK();
			break;
		case IDCANCEL:
			OnCancel();
			break;
		case IDC_BUTTON_NEW_FOLDER:
			OnNewFolder();
			break;
		case MAKEWPARAM(IDC_COMBO_FILTER, CBN_DROPDOWN):
			reinterpret_cast<HSuperComboBox *>(lParam)->AdjustDroppedWidth();
			break;
		case MAKEWPARAM(IDC_COMBO_FILTER, CBN_SELCHANGE):
			m_filename.clear();
			m_pEdFileName->SetWindowText(m_filename.c_str());
			RefreshListView();
			break;
		}
		break;
	case WM_MENUCHAR:
		switch (LOWORD(wParam))
		{
		case 'H': case 'h':
			ToggleFilter(0);
			break;
		case 'S': case 's':
			ToggleFilter(1);
			break;
		}
		break;
	case WM_NOTIFY:
		return OnNotify(reinterpret_cast<UNotify *>(lParam));
	case WM_APP_UpdateSelection:
		UpdateSelection();
		break;
	case WM_DESTROY:
		m_pTv->SetImageList(NULL, TVSIL_NORMAL);
		if (m_bHasListView)
		{
			saveColumns();
			m_pLv->SetImageList(NULL, LVSIL_SMALL);
		}
		break;
	}
	return OResizableDialog::WindowProc(message, wParam, lParam);
}

void ExplorerDlg::ScanExtraLayoutInfo(LPCTSTR pch)
{
	CSplitState::Scan(m_hWnd, pch);
}

void ExplorerDlg::DumpExtraLayoutInfo(LPTSTR pch)
{
	CSplitState::Dump(m_hWnd, pch);
}

LRESULT ExplorerDlg::WndProcFilter(WNDPROC pfnSuper, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_F5:
			Refresh();
			return 0;
		}
		break;
	}
	return ::CallWindowProc(pfnSuper, hWnd, uMsg, wParam, lParam);
}

int CALLBACK ExplorerDlg::SortFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	int nRetVal = 0;

	ExplorerDlg const *const pThis = reinterpret_cast<ExplorerDlg *>(lParamSort);

	WIN32_FIND_DATAW const *const x = reinterpret_cast<WIN32_FIND_DATAW const *>(lParam1);
	WIN32_FIND_DATAW const *const y = reinterpret_cast<WIN32_FIND_DATAW const *>(lParam2);

	// Don't compare entries unless same type (directory always wins over files)
	if ((x->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > (y->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		nRetVal = -1;
	}
	else if ((x->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) < (y->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		nRetVal = 1;
	}
	else
	{
		switch (pThis->m_nSortIndex)
		{
		case FILE_NAME_INDEX:
			nRetVal = lstrcmpiW(x->cFileName, y->cFileName);
			break;
		case FILE_ATTR_INDEX:
			{
				WCHAR attribx[33];
				WCHAR attriby[33];
				AttributesToText(x->dwFileAttributes, attribx);
				AttributesToText(y->dwFileAttributes, attriby);
				nRetVal = lstrcmpiW(attribx, attriby);
			}
			break;
		case FILE_SIZE_INDEX:
			{
				ULONGLONG sizex = GetFileSize(x);
				ULONGLONG sizey = GetFileSize(y);
				nRetVal = sizex > sizey ? 1 : sizex < sizey ? -1 : 0;
			}
			break;
		case FILE_CREATED_INDEX:
			{
				FileTime const &timex(x->ftCreationTime);
				FileTime const &timey(y->ftCreationTime);
				nRetVal = timex > timey ? 1 : timex < timey ? -1 : 0;
			}
			break;
		case FILE_LAST_ACCESSED_INDEX:
			{
				FileTime const &timex(x->ftLastAccessTime);
				FileTime const &timey(y->ftLastAccessTime);
				nRetVal = timex > timey ? 1 : timex < timey ? -1 : 0;
			}
			break;
		case FILE_LAST_WRITE_INDEX:
			{
				FileTime const &timex(x->ftLastWriteTime);
				FileTime const &timey(y->ftLastWriteTime);
				nRetVal = timex > timey ? 1 : timex < timey ? -1 : 0;
			}
			break;
		}
	}
	return pThis->m_bSortAscending ? nRetVal : -nRetVal;
}

BOOL ExplorerDlg::OnInitDialog()
{
	OResizableDialog::OnInitDialog();
	LanguageSelect.TranslateDialog(m_hWnd);

	if (!m_title.empty())
	{
		SetWindowText(m_title.c_str());
	}

	HImageList *const iml = reinterpret_cast<HImageList *>(GetShellImageList());

	m_pTv = static_cast<HTreeView *>(
		SubclassDlgItem<ExplorerDlg, IDC_TREE_FOLDER, &ExplorerDlg::WndProcFilter>());
	m_pTv->SetImageList(iml, TVSIL_NORMAL);

	m_pTcFilter = static_cast<HTabCtrl *>(GetDlgItem(IDC_TAB_FILTER));
	m_pTcFilter->SetExtendedStyle(m_pTcFilter->GetExtendedStyle() & ~TCS_EX_FLATSEPARATORS);

	TCITEM item;
	item.mask = TCIF_TEXT;
	item.pszText = L"H";
	m_pTcFilter->InsertItem(0, &item);
	item.pszText = L"S";
	m_pTcFilter->InsertItem(1, &item);
	m_pTcFilter->SetCurSel(-1);
	RECT rect;
	m_pTcFilter->GetItemRect(0, &rect);
	m_pTcFilter->SetItemSize(rect.bottom - rect.top, rect.bottom - rect.top);

	m_pPbNewFolder = static_cast<HButton *>(GetDlgItem(IDC_BUTTON_NEW_FOLDER));
	
	if (m_bHasListView)
	{
		m_pLv = static_cast<HListView *>(
			SubclassDlgItem<ExplorerDlg, IDC_LIST_FILE, &ExplorerDlg::WndProcFilter>());
		m_pLv->SetImageList(iml, LVSIL_SMALL);
	    m_pLv->SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_INFOTIP);

		if (!m_bEnableMultiSelect)
			m_pLv->SetStyle(m_pLv->GetStyle() | LVS_SINGLESEL);

		loadColumns();

		m_pCbFilter = static_cast<HComboBox *>(
			SubclassDlgItem<ExplorerDlg, IDC_COMBO_FILTER, &ExplorerDlg::WndProcFilter>());

		m_pEdFileName = static_cast<HEdit *>(m_pCbFilter->GetDlgItem(1001));

		int nCurSel = 0;
		if (m_bEnableFolderSelect)
		{
			String tag = LanguageSelect.LoadString(IDS_DIRSEL_TAG);
			nCurSel = m_pCbFilter->AddString(tag.c_str());
		}

		LPCWSTR filter = m_filters.c_str();
		while (*filter != L'\0')
		{
			int i = m_pCbFilter->AddString(filter);
			filter += lstrlenW(filter) + 1;
			m_pCbFilter->SetItemData(i, reinterpret_cast<UINT_PTR>(filter));
			if (LPCWSTR p = EatPrefix(filter, L"*."))
				if (m_defext == p)
					nCurSel = i;
			filter += lstrlenW(filter) + 1;
		}

		if (m_bEnableFolderSelect || m_path.rfind(L'\\') + 1 == m_path.length())
			m_pCbFilter->SetCurSel(nCurSel);
	}

	ShowWindow(SW_SHOW);
	PopulateTreeView();
	setSelectedFullPath(m_path);
	return TRUE;
}

LRESULT ExplorerDlg::OnSelchanged(NMTREEVIEW *pNMTreeView)
{
	m_pTv->EnsureVisible(pNMTreeView->itemNew.hItem);
	if (m_bHasListView)
		RefreshListView();
	return 0;
}

void ExplorerDlg::PopulateTreeItem(HTREEITEM hItem, int cAssumeChildren)
{
	HTREEITEM const hExItem = GetItemToExpand(hItem);
	if (hExItem == NULL || m_bClosing)
		return;

	TVINSERTSTRUCT tvis;
	tvis.hParent = hItem;
	tvis.hInsertAfter = TVI_LAST;

	tvis.item.hItem = hItem;
	tvis.item.mask = TVIF_PARAM;
	if (!m_pTv->GetItem(&tvis.item))
		return;
	tvis.item.hItem = NULL;

	LPITEMIDLIST parent = reinterpret_cast<LPITEMIDLIST>(tvis.item.lParam);

	HCURSOR const hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

	WCHAR exclude[MAX_PATH];
	if (hExItem != tvis.hParent)
	{
		tvis.hInsertAfter = TVI_FIRST;
		tvis.item.hItem = hExItem;
		tvis.item.mask = TVIF_TEXT;
		tvis.item.pszText = exclude;
		tvis.item.cchTextMax = _countof(exclude);
		m_pTv->GetItem(&tvis.item);
	}

	tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvis.item.cChildren = 1;
	tvis.item.lParam = 0;

	WCHAR path[MAX_PATH];
	HTREEITEM hParent = m_pTv->GetParentItem(hItem);
	if (IShellFolder *folder = GetShellFolder(hParent))
	{
		if (SUCCEEDED(folder->BindToObject(parent, NULL, IID_IShellFolder, (void**)&folder)))
		{
			IEnumIDList *enumerator = NULL;
			if (SUCCEEDED(folder->EnumObjects(NULL, SHCONTF_FOLDERS, &enumerator)))
			{
				LPITEMIDLIST child = NULL;
				while (enumerator->Next(1, &child, NULL) == S_OK)
				{
					STRRET str;
					folder->GetDisplayNameOf(child, SHGDN_NORMAL, &str);
					StrRetToBuf(&str, child, path, _countof(path));
					if (tvis.hInsertAfter != TVI_LAST && lstrcmp(path, exclude) == 0)
					{
						tvis.hInsertAfter = TVI_LAST;
						continue;
					}
					tvis.item.pszText = path;
					tvis.item.lParam = reinterpret_cast<LPARAM>(child);
					child = PidlManager::Combine(parent, child);
					while (hParent)
					{
						parent = reinterpret_cast<LPITEMIDLIST>(m_pTv->GetItemData(hParent));
						LPITEMIDLIST p = PidlManager::Combine(parent, child);
						PidlManager::Free(child);
						child = p;
						hParent = m_pTv->GetParentItem(hParent);
					}
					tvis.item.iImage = tvis.item.iSelectedImage = getImage(child);
					PidlManager::Free(child);
					tvis.item.hItem = m_pTv->InsertItem(&tvis);
					if (tvis.item.hItem != NULL)
					{
						if (tvis.hInsertAfter != TVI_LAST)
							tvis.hInsertAfter = tvis.item.hItem;
					}
					else
					{
						PidlManager::Free(reinterpret_cast<LPITEMIDLIST>(tvis.item.lParam));
					}
				}
				enumerator->Release();
			}
			folder->Release();
		}
	}
	else
	{
		std::wstring sPath;
		if (DWORD const dwFilter = getPath(hItem, sPath))
		{
			paths_DoMagic(sPath);
			sPath.append(L"*.*");
			WIN32_FIND_DATAW fd;
			HANDLE h = FindFirstFileW(sPath.c_str(), &fd);
			if (h != INVALID_HANDLE_VALUE)
			{
				do
				{
					if (fd.cFileName[0] && (fd.cFileName[0] != L'.' ||
						fd.cFileName[1] && (fd.cFileName[1] != L'.' ||
						fd.cFileName[2])))
					{
						switch (fd.dwFileAttributes & dwFilter)
						{
						case FILE_ATTRIBUTE_DIRECTORY:
							if (tvis.hInsertAfter != TVI_LAST && lstrcmp(fd.cFileName, exclude) == 0)
							{
								tvis.hInsertAfter = TVI_LAST;
								continue;
							}
							tvis.item.pszText = fd.cFileName;
							tvis.item.iImage = tvis.item.iSelectedImage = getImage(fd.cFileName);
							tvis.item.lParam = fd.dwFileAttributes & 0xFFFF;
							tvis.item.hItem = m_pTv->InsertItem(&tvis);
							if (tvis.item.hItem != NULL)
							{
								if (tvis.hInsertAfter != TVI_LAST)
									tvis.hInsertAfter = tvis.item.hItem;
							}
							break;
						}
					}
				} while (FindNextFileW(h, &fd));
				FindClose(h);
			}
		}
		m_pTv->SortChildren(tvis.hParent, FALSE);
	}

	if (tvis.item.hItem == NULL)
	{
		tvis.item.hItem = tvis.hParent;
		tvis.item.mask = TVIF_CHILDREN;
		tvis.item.cChildren = cAssumeChildren;
		m_pTv->SetItem(&tvis.item);
	}

	SetCursor(hCursor);
}

LRESULT ExplorerDlg::OnItemExpanding(NMTREEVIEW *pNMTreeView)
{
	switch (pNMTreeView->action)
	{
	case TVE_EXPAND:
	case TVE_COLLAPSE: // Beware that TVIS_EXPANDPARTIAL will get lost!
		if (pNMTreeView->itemNew.state & TVIS_EXPANDPARTIAL)
			m_pTv->SetRedraw(FALSE);
		PopulateTreeItem(pNMTreeView->itemNew.hItem);
		if (pNMTreeView->itemNew.state & TVIS_EXPANDPARTIAL)
			m_pTv->SetRedraw(TRUE);
		break;
	}
	return 0;
}

LRESULT ExplorerDlg::OnCustomDraw(NMTVCUSTOMDRAW *pCD)
{
    switch (pCD->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
        // Order is important
		if ((pCD->nmcd.lItemlParam & ~0xFFFF) == 0 && pCD->nmcd.uItemState == 0)
		{
			pCD->clrText = COLOUR_NORMAL;
			if (pCD->nmcd.lItemlParam & FILE_ATTRIBUTE_HIDDEN)
			{
				pCD->clrText = COLOUR_HIDDEN;
			}
			if (pCD->nmcd.lItemlParam & FILE_ATTRIBUTE_SYSTEM)
			{
				pCD->clrText = COLOUR_SYSTEM;
			}
			if (pCD->nmcd.lItemlParam & FILE_ATTRIBUTE_COMPRESSED)
			{
				pCD->clrText = COLOUR_COMPRESS;
			}
		}
        break;
    }
	return CDRF_DODEFAULT;
}

LRESULT ExplorerDlg::OnKeydown(NMTVKEYDOWN *pNM)
{
	switch (pNM->wVKey)
	{
	case VK_F2:
		if (HTREEITEM hSelection = m_pTv->GetSelection())
			m_pTv->EditLabel(hSelection);
		return 1;
	case VK_DELETE:
		if (HTREEITEM hSelection = m_pTv->GetSelection())
		{
			String sPath;
			if (getPath(hSelection, sPath))
			{
				int response = LanguageSelect.FormatStrings(
					IDS_CONFIRM_DELETE_SINGLE, sPath.c_str()
				).MsgBox(MB_ICONWARNING | MB_YESNO);
				if (response == IDYES)
				{
					paths_DoMagic(sPath);
					if (RemoveDirectory(sPath.c_str()))
					{
						m_pTv->DeleteItem(hSelection);
					}
					else
					{
						OException what = GetLastError();
						what.ReportError(m_hWnd, MB_ICONEXCLAMATION);
					}
				}
			}
		}
		return 1;
	}
	return 0;
}

LRESULT ExplorerDlg::OnEndLabelEdit(NMTVDISPINFO *pNM)
{
	if (pNM->item.pszText == NULL)
	{
		// Editing canceled
		if (pNM->item.lParam == 0)
			m_pTv->DeleteItem(pNM->item.hItem);
	}
	else if (pNM->item.lParam == 0)
	{
		// Create new folder
		HTREEITEM hParent = m_pTv->GetParentItem(pNM->item.hItem);
		String sPath;
		if (getPath(hParent, sPath))
		{
			paths_DoMagic(sPath);
			sPath.append(pNM->item.pszText);
			if (CreateDirectory(sPath.c_str(), NULL))
			{
				pNM->item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN;
				if (m_pTv->GetParentItem(hParent))
				{
					pNM->item.lParam = FILE_ATTRIBUTE_DIRECTORY;
				}
				else
				{
					LPITEMIDLIST parent = reinterpret_cast<LPITEMIDLIST>(m_pTv->GetItemData(hParent));
					IShellFolder *folder = NULL;
					if (SUCCEEDED(m_shell->BindToObject(parent, NULL, IID_IShellFolder, (void**)&folder)))
					{
						DWORD cch;
						DWORD dwAttributes;
						LPITEMIDLIST child = NULL;
						if (SUCCEEDED(folder->ParseDisplayName(m_hWnd, NULL, pNM->item.pszText, &cch, &child, &dwAttributes)))
						{
							pNM->item.lParam = reinterpret_cast<LPARAM>(child);
						}
						folder->Release();
					}
				}
				pNM->item.cChildren = 1;
				m_pTv->SetItem(&pNM->item);
				m_pTv->SelectItem(pNM->item.hItem);
			}
			else
			{
				OException what = GetLastError();
				m_pTv->DeleteItem(pNM->item.hItem);
				what.ReportError(m_hWnd, MB_ICONEXCLAMATION);
			}
		}
	}
	else
	{
		// Rename existing folder
		String sPath;
		if (getPath(pNM->item.hItem, sPath))
		{
			paths_DoMagic(sPath);
			sPath.resize(sPath.rfind(L'\\'));
			String sNewPath(sPath.c_str(), sPath.rfind(L'\\') + 1);
			sNewPath.append(pNM->item.pszText);
			if (MoveFile(sPath.c_str(), sNewPath.c_str()))
			{
				pNM->item.mask = TVIF_TEXT;
				m_pTv->SetItem(&pNM->item);
			}
			else
			{
				OException what = GetLastError();
				what.ReportError(m_hWnd, MB_ICONEXCLAMATION);
			}
		}
	}
	return 0;
}

LRESULT ExplorerDlg::OnDeleteItem(NMTREEVIEW *pNM)
{
	if ((pNM->itemOld.lParam & ~0xFFFF) != 0)
	{
		PidlManager::Free(reinterpret_cast<LPITEMIDLIST>(pNM->itemOld.lParam));
	}
	return 0;
}

void ExplorerDlg::loadColumns()
{
	int i;
	int n = 0;

	for (i = 0; i < _countof(cols); ++i)
	{
		if (cols[i].fmt & LVCFMT_DEFHIDDEN)
			continue;
		LVCOLUMN lvc;
		lvc.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
		lvc.iSubItem = i;
		String text = cols[i].idName ?
			LanguageSelect.LoadString(cols[i].idName) : cols[i].regName;
		lvc.pszText = const_cast<LPWSTR>(text.c_str());
		lvc.fmt = cols[lvc.iSubItem].fmt & LVCFMT_JUSTIFYMASK;
		m_pLv->InsertColumn(n++, &lvc);
	}

	bool bAutoSizeColumns = false;
	if (CRegKeyEx rk = SettingStore.GetSectionKey(L"ExplorerDlg"))
	{
		for (i = 0; i < n; ++i)
		{
			LVCOLUMN lvc;
			lvc.mask = LVCF_SUBITEM;
			m_pLv->GetColumn(i, &lvc);
			lvc.cx = rk.ReadDword(cols[lvc.iSubItem].regName, 0);
			if (lvc.cx == 0)
				bAutoSizeColumns = true;
			m_pLv->SetColumnWidth(i, lvc.cx);
		}

		WCHAR szColumnOrder[20 * 12];
		if (LPCWSTR str = rk.ReadString(L"ColumnOrder", NULL,
			CRegKeyEx::StringRef(szColumnOrder, _countof(szColumnOrder))))
		{
			int rgColumnOrder[20];
			i = 0;
			if (*str != L'\0') do
			{
				rgColumnOrder[i++] = wcstol(str, const_cast<LPWSTR *>(&str), 10);
			} while (i < _countof(rgColumnOrder) && *str++ == L';');
			m_pLv->SetColumnOrderArray(i, rgColumnOrder);
		}
	}

	if (bAutoSizeColumns)
	{
		LVITEM lvi;
		lvi.mask = LVIF_TEXT | LVIF_PARAM;
		lvi.iItem = 0;
		lvi.iSubItem = 0;
		lvi.pszText = LPSTR_TEXTCALLBACK;
		WIN32_FIND_DATAW *pParam = new WIN32_FIND_DATAW;
		memset(pParam, 0, sizeof *pParam);
		swprintf(pParam->cFileName, L"%020d", 0);
		pParam->dwFileAttributes = INVALID_FILE_ATTRIBUTES;
		pParam->nFileSizeLow = 1000;
		lvi.lParam = reinterpret_cast<LPARAM>(pParam);
		lvi.iItem = m_pLv->InsertItem(&lvi);
		for (int i = 0; i < n; ++i)
		{
			m_pLv->SetColumnWidth(i, LVSCW_AUTOSIZE);
		}
		m_pLv->DeleteItem(lvi.iItem);
	}

	// Replace standard header with sort header
	m_ctlSortHeader.Subclass(m_pLv->GetHeaderCtrl());
}

void ExplorerDlg::saveColumns()
{
	if (CRegKeyEx rk = SettingStore.GetSectionKey(L"ExplorerDlg"))
	{
		int i;
		int const n = m_ctlSortHeader.GetItemCount();

		for (i = 0; i < n; ++i)
		{
			LVCOLUMN lvc;
			lvc.mask = LVCF_SUBITEM | LVCF_WIDTH;
			m_pLv->GetColumn(i, &lvc);
			rk.WriteDword(cols[lvc.iSubItem].regName, lvc.cx);
		}

		int rgColumnOrder[20];
		if (m_pLv->GetColumnOrderArray(rgColumnOrder, n))
		{
			WCHAR szColumnOrder[20 * 12];
			LPWSTR str = szColumnOrder;
			for (i = 0; i < n; ++i)
			{
				str += swprintf(str, &L";%d"[str == szColumnOrder], rgColumnOrder[i]);
			}
			rk.WriteString(L"ColumnOrder", szColumnOrder);
		}
	}
}

IShellFolder *ExplorerDlg::GetShellFolder(HTREEITEM hItem) const
{
	if (hItem == NULL)
		return m_shell;
	if (hItem == m_hNetwork)
		return m_network;
	return NULL;
}

void ExplorerDlg::PopulateTreeView()
{
    m_pTv->SetRedraw(FALSE);
    HCURSOR const hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    m_pTv->DeleteAllItems();

	LPITEMIDLIST p = NULL;
	WCHAR path[MAX_PATH];

	TVINSERTSTRUCT tvis;
	tvis.hParent = TVI_ROOT;
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.mask = TVIF_TEXT | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
	tvis.item.cChildren = 1;

	if (SUCCEEDED(SHGetFolderLocation(m_hWnd, CSIDL_DRIVES, NULL, 0, &p)))
	{
		tvis.item.pszText = getPath(p, path);
		tvis.item.iImage = tvis.item.iSelectedImage = getImage(p);
		p = PidlManager::ReAlloc(p);
		tvis.item.lParam = reinterpret_cast<LPARAM>(p);
		m_hDrives = m_pTv->InsertItem(&tvis);
	}

	if (SUCCEEDED(SHGetFolderLocation(m_hWnd, CSIDL_MYDOCUMENTS, NULL, 0, &p)))
	{
		tvis.item.pszText = getPath(p, path);
		tvis.item.iImage = tvis.item.iSelectedImage = getImage(p);
		p = PidlManager::ReAlloc(p);
		tvis.item.lParam = reinterpret_cast<LPARAM>(p);
		m_hDocuments = m_pTv->InsertItem(&tvis);
	}

	if (SUCCEEDED(SHGetFolderLocation(m_hWnd, CSIDL_NETWORK, NULL, 0, &p)))
	{
		tvis.item.pszText = getPath(p, path);
		tvis.item.iImage = tvis.item.iSelectedImage = getImage(p);
		p = PidlManager::ReAlloc(p);
		tvis.item.lParam = reinterpret_cast<LPARAM>(p);
		m_hNetwork = m_pTv->InsertItem(&tvis);
		m_shell->BindToObject(p, NULL, IID_IShellFolder, (void**)&m_network);
	}

    SetCursor(hCursor);
    SetRedraw(TRUE);
}

HTREEITEM ExplorerDlg::ExpandPartial(HTREEITEM hRoot, LPWSTR full)
{
	LPWSTR p = PathSkipRootW(full);
	if (p <= full)
		return NULL;

	TVINSERTSTRUCT tvis;
	tvis.hParent = hRoot;
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.mask = TVIF_PARAM;

	tvis.item.hItem = hRoot;
	m_pTv->GetItem(&tvis.item);
	tvis.item.hItem = NULL;

	LPITEMIDLIST parent = reinterpret_cast<LPITEMIDLIST>(tvis.item.lParam);
	IShellFolder *folder = NULL;
	if (SUCCEEDED(m_shell->BindToObject(parent, NULL, IID_IShellFolder, (void**)&folder)))
	{
		WCHAR path[MAX_PATH];
		lstrcpynW(path, full, static_cast<int>(p - full + 1));
		DWORD cch;
		DWORD dwAttributes;
		LPITEMIDLIST child = NULL;
		if (SUCCEEDED(folder->ParseDisplayName(m_hWnd, NULL, path, &cch, &child, &dwAttributes)))
		{
			STRRET str;
			folder->GetDisplayNameOf(child, SHGDN_NORMAL, &str);
			StrRetToBuf(&str, child, path, _countof(path));
			tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
			tvis.item.cChildren = 1;
			tvis.item.pszText = path;
			tvis.item.lParam = reinterpret_cast<LPARAM>(child);
			child = PidlManager::Combine(parent, child);
			tvis.item.iImage = tvis.item.iSelectedImage = getImage(child);
			PidlManager::Free(child);
			tvis.item.hItem = m_pTv->InsertItem(&tvis);
			if (tvis.item.hItem != NULL)
			{
				m_pTv->Expand(tvis.hParent, TVE_EXPAND | TVE_EXPANDPARTIAL);
				while (LPWSTR q = StrChrW(p, L'\\'))
				{
					tvis.hParent = tvis.item.hItem;
					tvis.item.hItem = NULL;
					*q = L'\0';
					WIN32_FIND_DATAW fd;
					HANDLE handle = FindFirstFileW(full, &fd);
					*q = L'\\';
					if (handle == INVALID_HANDLE_VALUE)
						break;
					FindClose(handle);
					if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
						break;
					tvis.item.pszText = fd.cFileName;
					tvis.item.iImage = tvis.item.iSelectedImage = getImage(fd.cFileName);
					tvis.item.lParam = fd.dwFileAttributes & 0xFFFF;
					tvis.item.hItem = m_pTv->InsertItem(&tvis);
					if (tvis.item.hItem == NULL)
						break;
					m_pTv->Expand(tvis.hParent, TVE_EXPAND | TVE_EXPANDPARTIAL);
					p = q + 1;
				}
			}
			else
			{
				PidlManager::Free(reinterpret_cast<LPITEMIDLIST>(tvis.item.lParam));
			}
		}
		folder->Release();
	}
	return tvis.item.hItem;
}

void ExplorerDlg::setSelectedFullPath(const std::wstring &sPath)
{
	HCURSOR const hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
	m_pTv->SetRedraw(FALSE);

	if (HTREEITEM hItem = m_pTv->GetRootItem()) do
	{
		TVITEM item;
		item.hItem = hItem;
		item.mask = TVIF_CHILDREN;
		item.cChildren = 1;
		m_pTv->Expand(hItem, TVE_COLLAPSE | TVE_COLLAPSERESET);
		m_pTv->SetItem(&item);
		hItem = m_pTv->GetNextSiblingItem(hItem);
	} while (hItem);

	LVFINDINFO fi;
	fi.flags = LVFI_STRING;
	fi.psz = NULL;

	HTREEITEM hInitialSelection = m_pTv->GetSelection();
	m_pTv->SelectItem(NULL);
	if (HTREEITEM hItem = ExpandPartial(m_hDrives, const_cast<LPWSTR>(sPath.data())))
	{
		hInitialSelection = hItem;
		if (m_bHasListView)
			fi.psz = PathFindFileName(sPath.c_str());
	}
	else if (hInitialSelection == NULL)
	{
		hInitialSelection = m_hDrives;
	}
	m_pTv->SelectItem(hInitialSelection);
	m_pTv->EnsureVisible(hInitialSelection);

	if (fi.psz)
	{
		int iItem = m_pLv->FindItem(&fi);
		if (iItem != -1)
		{
			m_pLv->SetItemState(-1, 0, LVIS_FOCUSED | LVIS_SELECTED);
			m_pLv->SetItemState(iItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
			m_pLv->EnsureVisible(iItem, FALSE);
		}
	}

	m_pTv->SetRedraw(TRUE);
	SetCursor(hCursor);
}

DWORD ExplorerDlg::getPath(HTREEITEM hItem, std::wstring &sPath)
{
    std::wstring sItem;
	WCHAR path[MAX_PATH];
	TVITEM item;
	item.mask = TVIF_TEXT | TVIF_PARAM;
	item.hItem = hItem;
	item.pszText = path;
	item.cchTextMax = _countof(path);
	UINT state = 0;
	sPath.clear();
    while (item.hItem != NULL)
    {
		m_pTv->GetItem(&item);
		state |= item.state & item.stateMask;
		item.stateMask = 0;
		item.hItem = m_pTv->GetParentItem(item.hItem);
		if (item.hItem == NULL)
		{
			LPCITEMIDLIST child = reinterpret_cast<LPCITEMIDLIST>(item.lParam);
			if (SHGetPathFromIDListW(child, path))
			{
				sItem = path;
			}
		}
		else if (IShellFolder *folder = GetShellFolder(m_pTv->GetParentItem(item.hItem)))
		{
			LPCITEMIDLIST child = reinterpret_cast<LPCITEMIDLIST>(item.lParam);
			m_pTv->GetItem(&item);
			LPCITEMIDLIST parent = reinterpret_cast<LPCITEMIDLIST>(item.lParam);
			if (SUCCEEDED(folder->BindToObject(parent, NULL, IID_IShellFolder, (void**)&folder)))
			{
				STRRET str;
				folder->GetDisplayNameOf(child, SHGDN_FORADDRESSBAR | SHGDN_FORPARSING, &str);
				StrRetToBuf(&str, child, path, _countof(path));
				folder->Release();
				PathAppendW(path, L"target.lnk");
				sItem = ExpandShortcut(path);
				if (sItem.empty())
				{
					PathRemoveFileSpecW(path);
					sItem = path;
				}
			}
			item.hItem = NULL;
		}
		else
		{
			sItem = path;
		}
		if (sItem.rfind('\\') + 1 != sItem.length())
			sItem.push_back('\\');
		sPath = sItem + sPath;
    }
	DWORD dwFilter = 0;
	if (!sPath.empty())
	{
		TCITEM tci;
		tci.mask = TCIF_STATE;
		tci.dwStateMask = TCIS_HIGHLIGHTED;
		dwFilter = FILE_ATTRIBUTE_DIRECTORY;
		if (m_pTcFilter->GetItem(0, &tci) && (tci.dwState & TCIS_HIGHLIGHTED) == 0)
			dwFilter |= FILE_ATTRIBUTE_HIDDEN;
		if (m_pTcFilter->GetItem(1, &tci) && (tci.dwState & TCIS_HIGHLIGHTED) == 0)
			dwFilter |= FILE_ATTRIBUTE_SYSTEM;
	}
	return dwFilter;
}

LPWSTR ExplorerDlg::getPath(LPITEMIDLIST pidl, LPWSTR path)
{
	STRRET str;
	m_shell->GetDisplayNameOf(pidl, SHGDN_NORMAL, &str);
	return SUCCEEDED(StrRetToBuf(&str, pidl, path, MAX_PATH)) ? path : L"?";
}

int ExplorerDlg::getImage(LPITEMIDLIST pidl)
{
	SHFILEINFO shfi;
	ZeroMemory(&shfi, sizeof shfi);
	SHGetFileInfo(reinterpret_cast<LPCWSTR>(pidl), 0, &shfi, sizeof shfi,
		SHGFI_PIDL | SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
	ASSERT(shfi.hIcon == NULL);
	return shfi.iIcon;
}

int ExplorerDlg::getImage(LPCWSTR path, DWORD attr)
{
	SHFILEINFO shfi;
	ZeroMemory(&shfi, sizeof shfi);
	SHGetFileInfo(path, attr, &shfi, sizeof shfi,
		SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
	ASSERT(shfi.hIcon == NULL);
	return shfi.iIcon;
}

HTREEITEM ExplorerDlg::GetItemToExpand(HTREEITEM hItem)
{
	if (m_pTv->GetItemState(hItem, TVIS_EXPANDPARTIAL) & TVIS_EXPANDPARTIAL)
		return m_pTv->GetChildItem(hItem);
	HTREEITEM hSelection = m_pTv->GetSelection();
	if (hSelection == NULL)
		return NULL;
	UINT uState = m_pTv->GetItemState(hSelection, TVIS_EXPANDED | TVIS_EXPANDEDONCE);
	if ((uState & (TVIS_EXPANDED | TVIS_EXPANDEDONCE)) == TVIS_EXPANDEDONCE)
		return NULL;
	if (m_pTv->GetChildItem(hItem) != NULL)
		return NULL;
	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return hItem;
}

void ExplorerDlg::RefreshListView()
{
	m_pLv->DeleteAllItems();

	HTREEITEM const hItem = m_pTv->GetSelection();
	std::wstring sPath;
	DWORD const dwFilter = getPath(hItem, sPath);
	if (dwFilter == 0)
		return;

	paths_DoMagic(sPath);
	sPath.append(L"*.*");

	LPCWSTR filter = NULL;

	int nCurSel = m_pCbFilter->GetCurSel();
	if (nCurSel != -1)
	{
		filter = reinterpret_cast<LPCWSTR>(m_pCbFilter->GetItemData(nCurSel));
		if (m_defext)
			m_defext = EatPrefix(filter, L"*.");
	}
	else
	{
		if (m_filename.find_first_of(L"*?") != String::npos)
			filter = m_filename.c_str();
		if (m_defext)
			m_defext = L"*.";
	}

	HCURSOR const hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
	m_pLv->SetRedraw(FALSE);

	WIN32_FIND_DATAW fd;
	HANDLE const h = FindFirstFileW(sPath.c_str(), &fd);
	if (h != INVALID_HANDLE_VALUE)
	{
		LVITEM lvi;
		lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
		lvi.iItem = 0;
		lvi.iSubItem = 0;
		lvi.pszText = LPSTR_TEXTCALLBACK;
		lvi.iImage = I_IMAGECALLBACK;
		lvi.state = lvi.stateMask = LVIS_FOCUSED;
		do
		{
			switch (fd.dwFileAttributes & dwFilter)
			{
			case 0:
				if (filter != NULL && !PathMatchSpec(fd.cFileName, filter))
					continue;
				lvi.lParam = reinterpret_cast<LPARAM>(new WIN32_FIND_DATAW(fd));
				m_pLv->InsertItem(&lvi);
				++lvi.iItem;
				lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
				break;
			}
		} while (FindNextFileW(h, &fd));
		FindClose(h);
	}
	m_uChanged = 0;

	if ((filter != NULL) && (wcspbrk(filter, L"*?") == NULL))
	{
		// No filter but rather an MRU entry
		sPath.clear();
		sPath.push_back(L'"');
		sPath.append(filter);
		sPath.push_back(L'"');
	}
	else
	{
		m_pEdFileName->GetWindowText(sPath);
	}

	nCurSel = -1;

	LPCWSTR p = NULL;
	LPCWSTR q = sPath.c_str();
	while ((p = wcschr(q, L'"')) != NULL && (q = wcschr(++p, L'"')) != NULL)
	{
		std::wstring sItem(p, static_cast<std::wstring::size_type>(q++ - p));
		LVFINDINFO fi;
		fi.flags = LVFI_STRING;
		fi.psz = sItem.c_str();
		int iItem = m_pLv->FindItem(&fi);
		if (iItem != -1)
		{
			m_pLv->SetItemState(iItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
			nCurSel = iItem;
		}
	}

	if (nCurSel != -1)
	{
		m_pLv->SetSelectionMark(nCurSel);
		m_pLv->EnsureVisible(nCurSel, FALSE);
	}

	m_pLv->SetRedraw(TRUE);
	SetCursor(hCursor);
}

void ExplorerDlg::ToggleFilter(int i)
{
	TCITEM tci;
	tci.mask = TCIF_STATE;
	tci.dwStateMask = TCIS_HIGHLIGHTED;
	m_pTcFilter->GetItem(i, &tci);
	tci.dwState ^= TCIS_HIGHLIGHTED;
	m_pTcFilter->SetItem(i, &tci);
	Refresh();
}

LRESULT ExplorerDlg::OnGetDispinfo(NMLVDISPINFO *pNM)
{
	using locality::TimeString;
	WIN32_FIND_DATAW *pfd = reinterpret_cast
		<WIN32_FIND_DATAW *>(m_pLv->GetItemData(pNM->item.iItem));
	if (pNM->item.mask & LVIF_IMAGE)
	{
		pNM->item.iImage = getImage(pfd->cFileName, pfd->dwFileAttributes);
	}
	if (pNM->item.mask & LVIF_TEXT)
	{
		LVCOLUMN lvc;
		lvc.mask = LVCF_SUBITEM;
		m_pLv->GetColumn(pNM->item.iSubItem, &lvc);
		switch (lvc.iSubItem)
		{
		case FILE_NAME_INDEX:
			pNM->item.pszText = pfd->cFileName;
			break;
		case FILE_ATTR_INDEX:
			AttributesToText(pfd->dwFileAttributes, pNM->item.pszText);
			break;
		case FILE_SIZE_INDEX:
			StrFormatByteSizeW(GetFileSize(pfd), pNM->item.pszText, pNM->item.cchTextMax);
			break;
		case FILE_CREATED_INDEX:
			reinterpret_cast<locality::TimeString *>
				(pNM->item.pszText)->TimeString::TimeString(pfd->ftCreationTime, TimeString::LTime);
			break;
		case FILE_LAST_ACCESSED_INDEX:
			reinterpret_cast<locality::TimeString *>
				(pNM->item.pszText)->TimeString::TimeString(pfd->ftLastAccessTime, TimeString::LTime);
			break;
		case FILE_LAST_WRITE_INDEX:
			reinterpret_cast<locality::TimeString *>
				(pNM->item.pszText)->TimeString::TimeString(pfd->ftLastWriteTime, TimeString::LTime);
			break;
		}
	}
	return 0;
}

LRESULT ExplorerDlg::OnCustomDraw(NMLVCUSTOMDRAW *pCD)
{
	WIN32_FIND_DATAW *pfd = reinterpret_cast<WIN32_FIND_DATAW *>(pCD->nmcd.lItemlParam);
	switch (pCD->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		pCD->clrText = COLOUR_NORMAL;
		if (pfd->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
		{
			pCD->clrText = COLOUR_HIDDEN;
		}
		if (pfd->dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
		{
			pCD->clrText = COLOUR_SYSTEM;
		}
		if (pfd->dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED)
		{
			pCD->clrText = COLOUR_COMPRESS;
		}
		break;
	}
	return CDRF_DODEFAULT;
}

LRESULT ExplorerDlg::OnColumnClick(NMLISTVIEW *pNM)
{
	LVCOLUMN lvc;
	lvc.mask = LVCF_SUBITEM;
	m_pLv->GetColumn(pNM->iSubItem, &lvc);
	m_bSortAscending = m_nSortIndex == lvc.iSubItem ?
		!m_bSortAscending : (cols[lvc.iSubItem].fmt & LVCFMT_DEFSORTUP) != 0;
	m_nSortIndex = lvc.iSubItem;
	m_pLv->SortItems(SortFunc, reinterpret_cast<DWORD_PTR>(this));
	m_ctlSortHeader.SetSortImage(pNM->iSubItem, m_bSortAscending);
	return 0;
}

LRESULT ExplorerDlg::OnItemChanged(NMLISTVIEW *pNM)
{
	if (pNM->uChanged)
	{
		if (m_uChanged == 0)
			PostMessage(WM_APP_UpdateSelection);
		m_uChanged |= pNM->uChanged;
	}
	return 0;
}

LRESULT ExplorerDlg::OnDeleteItem(NMLISTVIEW *pNM)
{
	delete reinterpret_cast<WIN32_FIND_DATAW *>(pNM->lParam);
	return 0;
}

LRESULT ExplorerDlg::OnNotify(UNotify *pNM)
{
	switch (pNM->HDR.idFrom)
	{
	case IDC_TREE_FOLDER:
		switch (pNM->HDR.code)
		{
		case TVN_SELCHANGED:
			return OnSelchanged(&pNM->TREEVIEW);
		case TVN_ITEMEXPANDING:
			return OnItemExpanding(&pNM->TREEVIEW);
		case NM_CUSTOMDRAW:
			return OnCustomDraw(&pNM->TVCUSTOMDRAW);
		case TVN_KEYDOWN:
			return OnKeydown(&pNM->TVKEYDOWN);
		case TVN_ENDLABELEDIT:
			return OnEndLabelEdit(&pNM->TVDISPINFO);
		case TVN_DELETEITEM:
			return OnDeleteItem(&pNM->TREEVIEW);
		}
		break;
	case IDC_LIST_FILE:
		switch (pNM->HDR.code)
		{
		case LVN_GETDISPINFO:
			return OnGetDispinfo(&pNM->LVDISPINFO);
		case NM_CUSTOMDRAW:
			return OnCustomDraw(&pNM->LVCUSTOMDRAW);
		case LVN_DELETEITEM:
			return OnDeleteItem(&pNM->LISTVIEW);
		case LVN_COLUMNCLICK:
			return OnColumnClick(&pNM->LISTVIEW);
		case LVN_ITEMCHANGED:
			return OnItemChanged(&pNM->LISTVIEW);
		case LVN_DELETEALLITEMS:
			m_nSortIndex = -1;
			m_bSortAscending = true;
			m_ctlSortHeader.SetSortImage(-1, m_bSortAscending);
			m_uChanged = 0;
			break;
		case LVN_ITEMACTIVATE:
			OnOK();
			break;
		}
		break;
	case IDC_TAB_FILTER:
		switch (pNM->HDR.code)
		{
		case TCN_SELCHANGE:
			ToggleFilter(m_pTcFilter->SetCurSel(-1));
			break;
		}
		break;
	case 0:
		switch (pNM->HDR.code)
		{
		case HDN_ENDDRAG:
			// Prevent reordering if the first column is involved
			return pNM->HEADER.iItem == 0 || pNM->HEADER.pitem->iOrder == 0;
		}
		break;
	}
	return 0;
}

void ExplorerDlg::UpdateSelection()
{
	if (m_uChanged & LVIF_STATE)
	{
		UINT count = m_pLv->GetSelectedCount();
		UINT const limit = 100;
		if (count == 0)
		{
			if (!m_filename.empty())
				m_pEdFileName->SetWindowText(m_filename.c_str());
		}
		else if (count > limit)
		{
			LanguageSelect.FormatStrings(
				IDS_BROWSE_FOR_FILE_SEL_LIMIT, NumToStr(limit).c_str()
			).MsgBox(MB_ICONWARNING);
			RefreshListView();
		}
		else
		{
			LPCWSTR outerSeparator = L"";
			LPCWSTR innerSeparator = L"\"";
			m_pEdFileName->SetWindowText(outerSeparator);
			int i = -1;
			while ((i = m_pLv->GetNextItem(i, LVNI_SELECTED)) != -1)
			{
				WIN32_FIND_DATAW *pfd = reinterpret_cast<WIN32_FIND_DATAW *>(m_pLv->GetItemData(i));
				m_pEdFileName->ReplaceSel(innerSeparator);
				m_pEdFileName->ReplaceSel(pfd->cFileName);
				outerSeparator = L"\"";
				innerSeparator = L"\", \"";
			}
			m_pEdFileName->ReplaceSel(outerSeparator);
		}
	}
	m_uChanged = 0;
}

void ExplorerDlg::Refresh()
{
	std::wstring sPath;
	getPath(m_pTv->GetSelection(), sPath);
	setSelectedFullPath(sPath);
	PeekMessage(&MSG(), m_hWnd, WM_APP_UpdateSelection, WM_APP_UpdateSelection, PM_REMOVE);
}

void ExplorerDlg::OnOK()
{
	if (EndLabelEditNow(FALSE))
		return;

	bool bFolderSelect = m_bEnableFolderSelect;
	if (m_bHasListView)
	{
		m_filename.clear();
		int nCurSel = m_pCbFilter->GetCurSel();
		if (nCurSel == -1)
			m_pEdFileName->GetWindowText(m_filename);
		else if (m_pCbFilter->GetItemData(nCurSel))
			bFolderSelect = false;
	}

	if (m_filename.find_first_of(L"*?") != String::npos)
	{
		RefreshListView();
		m_pEdFileName->SetSel(0, -1);
		return;
	}

	if (HTREEITEM hSelection = m_pTv->GetSelection())
		getPath(hSelection, m_path);
	else
		m_path.clear();

	if (m_path.empty() || !bFolderSelect && m_filename.empty())
		return;

	if (!m_bEnableMultiSelect)
	{
		// remove quotation marks
		m_filename.erase(std::remove(m_filename.begin(), m_filename.end(), L'"'), m_filename.end());
		m_path.append(m_filename);
		String::size_type d = m_path.find(L'.', m_path.rfind(L'\\') + 1);
		if (d + 1 == m_path.length())
		{
			// remove the trailing dot
			m_path.resize(d);
		}
		else if (d == String::npos && m_defext && *m_defext != L'*')
		{
			// append the default extension
			m_path.push_back(L'.');
			m_path.append(m_defext);
		}
		if (m_bWarnIfExists)
		{
			String path = m_path;
			paths_DoMagic(path);
			DWORD dwAttributes = GetFileAttributes(path.c_str());
			if (dwAttributes != INVALID_FILE_ATTRIBUTES)
			{
				int choice = LanguageSelect.FormatStrings(
					dwAttributes & ProtectiveFileAttributes ? IDS_TRASH_FILE_ATTENTIVE : IDS_TRASH_FILE,
					NULL, m_path.c_str()).MsgBox(MB_YESNO | MB_HIGHLIGHT_ARGUMENTS | MB_ICONWARNING | MB_DEFBUTTON2);
				if (choice != IDYES)
					return;
			}
		}
	}

	m_bClosing = true;
	EndDialog(IDOK);
}

void ExplorerDlg::OnCancel()
{
	if (!EndLabelEditNow(TRUE))
		EndDialog(IDCANCEL);
}

void ExplorerDlg::OnNewFolder()
{
	TVINSERTSTRUCT tvis;
	tvis.hParent = m_pTv->GetSelection();
	if (tvis.hParent == NULL)
		return;
	PopulateTreeItem(tvis.hParent, 1);
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.mask = TVIF_TEXT | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
	tvis.item.cChildren = 1;
	tvis.item.pszText = L"";
	tvis.item.iImage = tvis.item.iSelectedImage = getImage(L":");
	tvis.item.lParam = 0;
	tvis.item.hItem = m_pTv->InsertItem(&tvis);
	if (tvis.item.hItem == NULL)
		return;
	m_pTv->Expand(tvis.hParent, TVE_EXPAND);
	m_pTv->EditLabel(tvis.item.hItem);
}

BOOL ExplorerDlg::EndLabelEditNow(BOOL fCancel)
{
	if (m_pTv->GetEditControl())
	{
		m_pTv->EndEditLabelNow(fCancel);
		return TRUE;
	}
	return FALSE;
}
