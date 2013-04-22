/**
 *  @file LineFiltersDlg.cpp
 *
 *  @brief Implementation of Line Filter dialog
 */ 
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "Merge.h"
#include "LanguageSelect.h"
#include "MainFrm.h"
#include "LineFiltersDlg.h"
#include "pcre.h"
#include "coretools.h"
#include "paths.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define WM_APP_DoInPlaceEditingEvents	(WM_APP + 1)
// http://www.apijunkie.com/APIJunkie/blog/post/2007/11/11/Handling-tree-control-check-box-selectionde-selection-in-Win32MFC.aspx
#define WM_APP_CheckStateChange			(WM_APP + 2)

/** @brief Location for file compare specific help to open. */
static const TCHAR FilterHelpLocation[] = _T("::/htmlhelp/Filters.html");

/////////////////////////////////////////////////////////////////////////////
// LineFiltersDlg property page

/**
 * @brief Constructor.
 */
LineFiltersDlg::LineFiltersDlg()
: ODialog(IDD_PROPPAGE_FILTER)
, m_TvFilter(NULL)
, m_EdFilter(NULL)
, m_pEdTestCase(NULL)
, m_pLvTestCase(NULL)
, m_bIgnoreRegExp(FALSE)
, m_bLineFiltersDirty(FALSE)
{
	m_strCaption = LanguageSelect.LoadDialogCaption(m_idd).c_str();
}

void LineFiltersDlg::OnCustomdraw(HSurface *pDC)
{
	String strExpression;
	if (m_EdFilter)
	{
		m_EdFilter->GetWindowText(strExpression);
	}
	else if (HTREEITEM hItem = m_TvFilter->GetSelection())
	{
		TVITEM item;
		item.mask = TVIF_CHILDREN;
		item.hItem = hItem;
		m_TvFilter->GetItem(&item);
		if (item.cChildren == 0)
		{
			strExpression = m_TvFilter->GetItemText(hItem);
			if (m_TvFilter->GetParentItem(hItem))
			{
				strExpression.erase(0, strExpression.find(_T('=')) + 1);
			}
		}
	}

	LPCTSTR pchExpression = strExpression.c_str();
	String::size_type cchExpression = strExpression.length();
	if (LPCTSTR p = EatPrefix(pchExpression, _T("regexp:")))
	{
		cchExpression -= static_cast<String::size_type>(p - pchExpression);
		pchExpression = p;
	}

	regexp_item item;
	if (item.assign(pchExpression, cchExpression))
	{
		HGdiObj *pTmp = pDC->SelectObject(m_pEdTestCase->GetFont());
		int org = -m_pEdTestCase->GetScrollPos(SB_HORZ);
		if (wine_version)
		{
			SCROLLINFO si;
			si.cbSize = sizeof si;
			si.fMask = SIF_POS | SIF_TRACKPOS;
			m_pEdTestCase->GetScrollInfo(SB_HORZ, &si);
			SCROLLBARINFO sbi;
			sbi.cbSize = sizeof sbi;
			m_pEdTestCase->GetScrollBarInfo(OBJID_HSCROLL, &sbi);
			org = 3 - (sbi.rgstate[3] & STATE_SYSTEM_PRESSED ? si.nTrackPos : si.nPos);
		}
		int pos = org;
		char buf[1024];
		int len = m_pEdTestCase->GetWindowTextA(buf, _countof(buf));
		int i = 0;
		while (i < len)
		{
			int ovector[33];
			int matches = item.execute(buf, len, i, 0, ovector, _countof(ovector) - 1);
			if ((matches <= 0) || (ovector[1] == 0))
			{
				ovector[1] = len;
				matches = 0;
			}
			int matches2 = matches * 2;
			ovector[matches2] = ovector[1];
			ovector[1] = ovector[0];
			int index = 1;
			int j;
			do
			{
				j = ovector[index];
				if (i < j)
				{
					RECT rc = { org, 0, org, 0 };
					pDC->DrawTextA(buf, j, &rc, DT_CALCRECT | DT_EDITCONTROL | DT_NOPREFIX | DT_EXPANDTABS);
					if (index > 1)
					{
						rc.left = pos;
						rc.bottom = 2;
						pDC->SetBkColor(index & 1 ? RGB(255,99,71) : RGB(0,192,0));
						pDC->ExtTextOut(0, 0, ETO_OPAQUE, &rc, NULL, 0);
					}
					pos = rc.right;
					i = j;
				}
			} while (++index <= matches2);
			i = item.global ? j : len;
		}
		item.dispose();
		pDC->SelectObject(pTmp);
	}
}

void LineFiltersDlg::DoInPlaceEditingEvents()
{
	MSG msg;
	while (m_EdFilter && GetMessage(&msg, NULL, 0, 0))
	{
		// Protect against nested invocations when receiving a bogus WM_APP.
		if (msg.message == WM_APP_DoInPlaceEditingEvents)
			continue;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (m_EdFilter && m_EdFilter->GetModify())
		{
			m_EdFilter->SetModify(FALSE);
			m_pLvTestCase->Invalidate();
		}
	}
	m_pLvTestCase->Invalidate();
}

LRESULT LineFiltersDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case MAKEWPARAM(IDC_LFILTER_ADDBTN, BN_CLICKED):
			OnBnClickedLfilterAddBtn();
			break;
		case MAKEWPARAM(IDC_LFILTER_EDITBTN, BN_CLICKED):
			OnBnClickedLfilterEditbtn();
			break;
		case MAKEWPARAM(IDC_LFILTER_REMOVEBTN, BN_CLICKED):
			OnBnClickedLfilterRemovebtn();
			break;
		case MAKEWPARAM(IDC_LFILTER_EDIT, EN_CHANGE):
			if (wine_version)
			{
				m_pEdTestCase->Invalidate();
			}
			break;
		}
		break;
	case WM_HELP:
		theApp.m_pMainWnd->ShowHelp(FilterHelpLocation);
		return 0;
	case WM_NOTIFY:
		if (LRESULT lResult = OnNotify(reinterpret_cast<UNotify *>(lParam)))
			return lResult;
		break;
	case WM_CTLCOLOREDIT:
		if (m_pLvTestCase)
			m_pLvTestCase->Invalidate();
		break;
	case WM_APP_DoInPlaceEditingEvents:
		DoInPlaceEditingEvents();
		break;
	case WM_APP_CheckStateChange:
		OnCheckStateChange(reinterpret_cast<HTREEITEM>(lParam));
		break;
	case WM_HOTKEY:
		switch (wParam)
		{
		case MAKEWPARAM(IDC_LFILTER_LIST, VK_F2):
			EditSelectedFilter();
			break;
		case MAKEWPARAM(IDC_LFILTER_LIST, VK_SPACE):
			if (HTREEITEM hItem = m_TvFilter->GetSelection())
			{
				m_TvFilter->SendMessage(WM_KEYDOWN, VK_SPACE);
				PostMessage(WM_APP_CheckStateChange,
					reinterpret_cast<WPARAM>(m_TvFilter),
					reinterpret_cast<LPARAM>(hItem));
			}
			break;
		}
		break;
	}
	return ODialog::WindowProc(message, wParam, lParam);
}

LRESULT LineFiltersDlg::OnNotify(UNotify *pNM)
{
	switch (pNM->HDR.idFrom)
	{
	case IDC_LFILTER_LIST:
		switch (pNM->HDR.code)
		{
		case NM_SETFOCUS:
			RegisterHotKey(m_hWnd, MAKEWPARAM(IDC_LFILTER_LIST, VK_F2), 0, VK_F2);
			RegisterHotKey(m_hWnd, MAKEWPARAM(IDC_LFILTER_LIST, VK_SPACE), 0, VK_SPACE);
			break;
		case NM_KILLFOCUS:
			UnregisterHotKey(m_hWnd, MAKEWPARAM(IDC_LFILTER_LIST, VK_F2));
			UnregisterHotKey(m_hWnd, MAKEWPARAM(IDC_LFILTER_LIST, VK_SPACE));
			break;
		case TVN_ITEMEXPANDING:
			if (pNM->TREEVIEW.action == TVE_EXPAND &&
				(pNM->TREEVIEW.itemNew.state & TVIS_EXPANDEDONCE) == 0)
			{
				PopulateSection(pNM->TREEVIEW.itemNew.hItem);
			}
			break;
		case TVN_BEGINLABELEDIT:
			if (!GetEditableItem())
				return 1;
			m_EdFilter = m_TvFilter->GetEditControl();
			PostMessage(WM_APP_DoInPlaceEditingEvents);
			break;
		case TVN_ENDLABELEDIT:
			m_EdFilter = NULL;
			return 1;
		case TVN_SELCHANGED:
			OnSelchanged();
			break;
		case NM_CLICK:
			OnClick(pNM);
			break;
		case NM_CUSTOMDRAW:
			switch (pNM->CUSTOMDRAW.dwDrawStage)
			{
			case CDDS_PREPAINT:
				return CDRF_NOTIFYITEMDRAW;
			case CDDS_ITEMPREPAINT:
				if (FAILED(pNM->CUSTOMDRAW.lItemlParam))
				{
					pNM->TVCUSTOMDRAW.clrText = RGB(255,0,0);
					if (pNM->CUSTOMDRAW.uItemState & CDIS_SELECTED)
					{
						if (pNM->pwndFrom != HWindow::GetFocus())
						{
							pNM->TVCUSTOMDRAW.clrTextBk = GetSysColor(COLOR_BTNFACE);
						}
						else
						{
							pNM->TVCUSTOMDRAW.clrTextBk = pNM->TVCUSTOMDRAW.clrText;
							pNM->TVCUSTOMDRAW.clrText = m_TvFilter->GetBkColor();
						}
						pNM->CUSTOMDRAW.uItemState &= ~CDIS_SELECTED;
					}
				}
				break;
			}
			break;
		}
		break;
	case IDC_LFILTER_EDIT:
		switch (pNM->HDR.code)
		{
		case NM_CUSTOMDRAW:
			switch (pNM->CUSTOMDRAW.dwDrawStage)
			{
			case CDDS_PREPAINT:
				OnCustomdraw(reinterpret_cast<HSurface *>(pNM->CUSTOMDRAW.hdc));
				return CDRF_SKIPDEFAULT;
			}
			break;
		}
		break;
	default:
		switch (pNM->HDR.code)
		{
		case PSN_APPLY:
			OnApply();
			break;
		}
		break;
	}
	return 0;
}

/**
 * @brief Initialize the dialog.
 */
BOOL LineFiltersDlg::OnInitDialog()
{
	ODialog::OnInitDialog();
	LanguageSelect.TranslateDialog(m_hWnd);
	m_TvFilter = static_cast<HTreeView *>(GetDlgItem(IDC_LFILTER_LIST));
	// http://social.msdn.microsoft.com/Forums/en-US/vcgeneral/thread/cf8f9db4-9420-43e3-b423-0fa8e1e2e547/
	m_TvFilter->SetStyle(m_TvFilter->GetStyle() | TVS_CHECKBOXES);
	m_pEdTestCase = static_cast<HEdit *>(GetDlgItem(IDC_LFILTER_EDIT));
	m_pEdTestCase->SetWindowText(m_strTestCase.c_str());
	HSurface *pSurface = GetDC();
	pSurface->SelectObject(m_pEdTestCase->GetFont());
	TEXTMETRIC tm;
	pSurface->GetTextMetrics(&tm);
	ReleaseDC(pSurface);
	RECT rc;
	m_pEdTestCase->GetRect(&rc);
	rc.bottom = rc.top + tm.tmHeight;
	m_pEdTestCase->SetRect(&rc);
	m_pLvTestCase = HListView::Create(WS_CHILD | WS_VISIBLE | WS_DISABLED,
		rc.left, rc.bottom + rc.top, rc.right - rc.left, 2, m_pWnd, IDC_LFILTER_EDIT);
	m_pLvTestCase->SetParent(m_pEdTestCase);
	m_EdFilter = NULL;
	InitList();
	CheckDlgButton(IDC_IGNOREREGEXP, m_bIgnoreRegExp);
	return TRUE;
}

/**
 * @brief Initialize the filter list in the dialog.
 * This function adds current line filters to the filter list.
 */
void LineFiltersDlg::InitList()
{
	m_inifile = COptionsMgr::Get(OPT_SUPPLEMENT_FOLDER);
	m_inifile = paths_ConcatPath(m_inifile, _T("LineFilters.ini"));
	TCHAR buffer[0x8000];
	if (!GetPrivateProfileSection(_T("*"), buffer, _countof(buffer), m_inifile.c_str()))
	{
		static const TCHAR toc[] =
			_T("UnifyDateFormat=\0");
		static const TCHAR UnifyDateFormat[] =
			// ISO date format - remove leading zeros
			_T("ISO date format						= ")
			_T("regexp:/0*(\\d+)-0*(\\d+)-0*(\\d+)/gp<$1-$2-$3\0")
			// US civilian vernacular date format - remove leading zeros and permute accordingly
			_T("US civilian vernacular date format	= ")
			_T("regexp:/0*(\\d+)\\/0*(\\d+)\\/0*(\\d+)/gp<$3-$1-$2\0")
			// Traditional German date format - remove leading zeros and permute accordingly
			_T("Traditional German date format		= ")
			_T("regexp:/0*(\\d+)\\.0*(\\d+)\\.0*(\\d+)/gp<$3-$2-$1\0");
		if (WritePrivateProfileSection(_T("*"), toc, m_inifile.c_str()) &&
			WritePrivateProfileSection(_T("UnifyDateFormat"), UnifyDateFormat, m_inifile.c_str()))
		{
			memcpy(buffer, toc, sizeof toc);
		}
	}
	LPTSTR p = buffer;
	while (const size_t n = _tcslen(p))
	{
		if (LPTSTR q = _tcschr(p, _T('=')))
		{
			*q++ = _T('\0');
			string_format text(_T("[%s]"), p);
			AddRow(text.c_str(), _tcschr(q, _T('1')) != 0 ? 1 : 0, 1);
		}
		p += n + 1;
	}

	int count = globalLineFilters.GetCount();
	for (int i = 0; i < count; i++)
	{
		const LineFilterItem &item = globalLineFilters.GetAt(i);
		HTREEITEM hItem = AddRow(item.filterStr.c_str(), item.usage);
		m_TvFilter->SetItemData(hItem, item.hr);
	}

	if (HTREEITEM hItem = m_TvFilter->GetRootItem())
		m_TvFilter->SelectItem(hItem);
}

void LineFiltersDlg::PopulateSection(HTREEITEM hItem)
{
	TCHAR section[MAX_PATH];
	m_TvFilter->GetItemText(hItem, section, _countof(section));
	TCHAR buffer[0x8000];
	if (StrTrim(section, _T("[]")) &&
		GetPrivateProfileSection(section, buffer, _countof(buffer), m_inifile.c_str()))
	{
		LPTSTR p = buffer;
		TCHAR check[100];
		DWORD index = 0;
		DWORD count = GetPrivateProfileString(_T("*"), section, NULL, check, _countof(check), m_inifile.c_str());
		while (const size_t n = _tcslen(p))
		{
			TVINSERTSTRUCT tvis;
			tvis.hParent = hItem;
			tvis.hInsertAfter = TVI_LAST;
			tvis.item.mask = TVIF_TEXT | TVIF_STATE;
			tvis.item.pszText = p;
			tvis.item.state = index < count && check[index] == _T('1') ?
				INDEXTOSTATEIMAGEMASK(2) : INDEXTOSTATEIMAGEMASK(1);
			tvis.item.stateMask = TVIS_STATEIMAGEMASK;
			m_TvFilter->InsertItem(&tvis);
			p += n + 1;
			++index;
		}
	}
}

/**
 * @brief Add new row to the list control.
 * @param [in] Filter string to add.
 * @param [in] enabled Is filter enabled?
 * @return Index of added row.
 */
HTREEITEM LineFiltersDlg::AddRow(LPCTSTR filter, int usage, int cChildren)
{
	TVINSERTSTRUCT tvis;
	tvis.hParent = TVI_ROOT;
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.mask = TVIF_TEXT | TVIF_STATE | TVIF_CHILDREN;
	tvis.item.pszText = const_cast<LPTSTR>(filter);
	tvis.item.state = (usage + 1) << 12;
	tvis.item.stateMask = TVIS_STATEIMAGEMASK;
	tvis.item.cChildren = cChildren;
	return m_TvFilter->InsertItem(&tvis);
}

HTREEITEM LineFiltersDlg::GetEditableItem()
{
	// Only items with neither children nor a parent are editable.
	HTREEITEM hItem = m_TvFilter->GetSelection();
	if (hItem)
	{
		TVITEM item;
		item.mask = TVIF_CHILDREN;
		item.hItem = hItem;
		m_TvFilter->GetItem(&item);
		if (item.cChildren || m_TvFilter->GetParentItem(hItem))
		{
			hItem = NULL;
		}
	}
	return hItem;
}

/**
 * @brief Edit currently selected filter.
 */
void LineFiltersDlg::EditSelectedFilter()
{
	if (HTREEITEM hItem = GetEditableItem())
	{
		m_TvFilter->SetFocus();
		m_TvFilter->EditLabel(hItem);
	}
}

/**
 * @brief Called when Add-button is clicked.
 */
void LineFiltersDlg::OnBnClickedLfilterAddBtn()
{
	if (HTREEITEM hItem = AddRow(_T("")))
	{
		m_TvFilter->SelectItem(hItem);
		m_TvFilter->EnsureVisible(hItem);
		EditSelectedFilter();
	}
}

/**
 * @brief Called when Edit button is clicked.
 */
void LineFiltersDlg::OnBnClickedLfilterEditbtn()
{
	EditSelectedFilter();
}

/**
 * @brief Save filters to list when exiting the dialog.
 */
void LineFiltersDlg::OnApply()
{
	LineFiltersList lineFilters;
	HTREEITEM hItem = m_TvFilter->GetRootItem();
	while (hItem != NULL)
	{
		TCHAR text[MAX_PATH];
		TVITEM item;
		item.mask = TVIF_TEXT | TVIF_CHILDREN | TVIF_STATE;
		item.hItem = hItem;
		item.pszText = text;
		item.cchTextMax = _countof(text);
		item.stateMask = TVIS_STATEIMAGEMASK;
		m_TvFilter->GetItem(&item);
		if (item.cChildren == 0)
		{
			int usage = ((item.state & item.stateMask) >> 12) - 1;
			lineFilters.AddFilter(text, usage);
		}
		else if (HTREEITEM hChild = m_TvFilter->GetChildItem(hItem))
		{
			String check;
			do
			{
				item.mask = TVIF_STATE;
				item.hItem = hChild;
				item.stateMask = TVIS_STATEIMAGEMASK;
				m_TvFilter->GetItem(&item);
				check.push_back(
					(item.state & item.stateMask) == INDEXTOSTATEIMAGEMASK(2) ?
					_T('1') : _T('0'));
				hChild = m_TvFilter->GetNextSiblingItem(hChild);
			} while (hChild != NULL);
			StrTrim(text, _T("[]"));
			WritePrivateProfileString(_T("*"), text, check.c_str(), m_inifile.c_str());
			m_bLineFiltersDirty = TRUE;
		}
		hItem = m_TvFilter->GetNextSiblingItem(hItem);
	}
	if (!lineFilters.Compare(globalLineFilters))
	{
		globalLineFilters.swap(lineFilters);
		m_bLineFiltersDirty = TRUE;
	}
	BOOL bIgnoreRegExp = IsDlgButtonChecked(IDC_IGNOREREGEXP);
	if (m_bIgnoreRegExp != bIgnoreRegExp)
	{
		m_bIgnoreRegExp = bIgnoreRegExp;
		m_bLineFiltersDirty = TRUE;
	}
}

/**
 * @brief Called when Remove button is clicked.
 */
void LineFiltersDlg::OnBnClickedLfilterRemovebtn()
{
	if (HTREEITEM hItem = m_TvFilter->GetSelection())
	{
		m_TvFilter->DeleteItem(hItem);
	}
}

/**
 * @brief Called when an item in the tree view has been clicked.
 */
void LineFiltersDlg::OnClick(UNotify *pNM)
{
	DWORD dwPos = GetMessagePos();
	TVHITTESTINFO ht;
	POINTSTOPOINT(ht.pt, dwPos);
	m_TvFilter->ScreenToClient(&ht.pt);
	if (m_TvFilter->HitTest(&ht) && (TVHT_ONITEMSTATEICON & ht.flags) != 0)
	{
		m_TvFilter->SelectItem(ht.hItem);
		PostMessage(WM_APP_CheckStateChange,
			reinterpret_cast<WPARAM>(m_TvFilter),
			reinterpret_cast<LPARAM>(ht.hItem));
	}
}

/**
 * @brief Called when the selection in the tree view has changed.
 */
void LineFiltersDlg::OnSelchanged()
{
	BOOL bEnable = GetEditableItem() != NULL;
	GetDlgItem(IDC_LFILTER_EDITBTN)->EnableWindow(bEnable);
	GetDlgItem(IDC_LFILTER_REMOVEBTN)->EnableWindow(bEnable);
	m_pLvTestCase->Invalidate();
}

/**
 * @brief Called when an item's check state has changed.
 */
void LineFiltersDlg::OnCheckStateChange(HTREEITEM hItem)
{
	TVITEM item;
	item.mask = TVIF_STATE | TVIF_CHILDREN;
	item.stateMask = TVIS_STATEIMAGEMASK | TVIS_EXPANDEDONCE;
	item.hItem = hItem;
	m_TvFilter->GetItem(&item);
	if (item.cChildren != 0 && (item.state & TVIS_EXPANDEDONCE) == 0)
	{
		PopulateSection(hItem);
		m_TvFilter->SetItemState(hItem, TVIS_EXPANDEDONCE, TVIS_EXPANDEDONCE);
	}
	if (HTREEITEM hChild = m_TvFilter->GetChildItem(hItem))
	{
		item.state &= TVIS_STATEIMAGEMASK;
		do
		{
			m_TvFilter->SetItemState(hChild, item.state, TVIS_STATEIMAGEMASK);
			hChild = m_TvFilter->GetNextSiblingItem(hChild);
		} while (hChild != NULL);
	}
	else if (HTREEITEM hParent = m_TvFilter->GetParentItem(hItem))
	{
		HTREEITEM hChild = m_TvFilter->GetChildItem(hParent);
		item.state = INDEXTOSTATEIMAGEMASK(1);
		do
		{
			if (INDEXTOSTATEIMAGEMASK(2) == (TVIS_STATEIMAGEMASK &
				m_TvFilter->GetItemState(hChild, TVIS_STATEIMAGEMASK)))
			{
				item.state = INDEXTOSTATEIMAGEMASK(2);
			}
			hChild = m_TvFilter->GetNextSiblingItem(hChild);
		} while (hChild != NULL);
		m_TvFilter->SetItemState(hParent, item.state, TVIS_STATEIMAGEMASK);
	}
}
