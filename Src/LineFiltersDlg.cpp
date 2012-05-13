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
#include "FilterList.h"
#include "LineFiltersList.h"
#include "MainFrm.h"
#include "LineFiltersDlg.h"
#include "pcre.h"
#include "coretools.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define WM_APP_DoInPlaceEditingEvents (WM_APP + 1)

/** @brief Location for file compare specific help to open. */
static const TCHAR FilterHelpLocation[] = _T("::/htmlhelp/Filters.html");

/////////////////////////////////////////////////////////////////////////////
// LineFiltersDlg property page

/**
 * @brief Constructor.
 */
LineFiltersDlg::LineFiltersDlg()
: ODialog(IDD_PROPPAGE_FILTER)
, m_LvFilter(NULL)
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
	else
	{
		int i = m_LvFilter->GetNextItem(-1, LVNI_SELECTED);
		strExpression = m_LvFilter->GetItemText(i, 0);
	}

	LPCTSTR pchExpression = strExpression.c_str();
	size_t cchExpression = strExpression.length();
	if (LPCTSTR p = EatPrefix(pchExpression, _T("regexp:")))
	{
		cchExpression -= p - pchExpression;
		pchExpression = p;
	}

	filter_item item;
	if (item.assign(pchExpression, cchExpression) && item.compile())
	{
		HGdiObj *pTmp = pDC->SelectObject(m_pEdTestCase->GetFont());
		RECT rc = { -m_pEdTestCase->GetScrollPos(SB_HORZ), 0, 0, 0 };
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
					pDC->DrawTextA(buf + i, j - i, &rc, DT_CALCRECT | DT_EDITCONTROL);
					if (index > 1)
					{
						rc.bottom = 2;
						pDC->SetBkColor(index & 1 ? RGB(255,99,71) : RGB(0,192,0));
						pDC->ExtTextOut(0, 0, ETO_OPAQUE, &rc, NULL, 0);
					}
					rc.left = rc.right;
					i = j;
				}
			} while (++index <= matches2);
			if (!item.global)
			{
				j = len;
				if (i < j)
				{
					pDC->DrawTextA(buf + i, j - i, &rc, DT_CALCRECT | DT_EDITCONTROL);
					rc.left = rc.right;
				}
			}
			i = j;
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
		case LVN_ITEMACTIVATE:
			EditSelectedFilter();
			break;
		case LVN_KEYDOWN:
			if (LOWORD(pNM->KEY.nVKey) == VK_F2)
				EditSelectedFilter();
			break;
		case LVN_BEGINLABELEDIT:
			m_EdFilter = m_LvFilter->GetEditControl();
			PostMessage(WM_APP_DoInPlaceEditingEvents);
			break;
		case LVN_ENDLABELEDIT:
			m_EdFilter = NULL;
			return 1;
		case LVN_ITEMCHANGED:
			m_pLvTestCase->Invalidate();
			break;
		case NM_CUSTOMDRAW:
			switch (pNM->CUSTOMDRAW.dwDrawStage)
			{
			case CDDS_PREPAINT:
				return CDRF_NOTIFYITEMDRAW;
			case CDDS_ITEMPREPAINT:
				if (FAILED(pNM->CUSTOMDRAW.lItemlParam))
				{
					// Don't trust NMCUSTOMDRAW::uItemState!
					UINT uItemState = pNM->pLV->GetItemState(
						pNM->CUSTOMDRAW.dwItemSpec, LVIS_SELECTED | LVIS_FOCUSED);
					pNM->LVCUSTOMDRAW.clrText = RGB(255,0,0);
					if (uItemState & LVIS_SELECTED)
					{
						if (pNM->pwndFrom != HWindow::GetFocus())
						{
							pNM->LVCUSTOMDRAW.clrTextBk = GetSysColor(COLOR_BTNFACE);
						}
						else if (uItemState & LVIS_FOCUSED)
						{
							pNM->LVCUSTOMDRAW.clrTextBk = pNM->LVCUSTOMDRAW.clrText;
							pNM->LVCUSTOMDRAW.clrText = m_LvFilter->GetBkColor();
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
	m_LvFilter = static_cast<HListView *>(GetDlgItem(IDC_LFILTER_LIST));
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
	// Show selection across entire row.
	// Also enable infotips.
	m_LvFilter->SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
	String title = LanguageSelect.LoadString(IDS_FILTERLINE_REGEXP);
	m_LvFilter->InsertColumn(0, title.c_str());
	int count = globalLineFilters.GetCount();
	for (int i = 0; i < count; i++)
	{
		const LineFilterItem &item = globalLineFilters.GetAt(i);
		int ind = AddRow(item.filterStr.c_str(), item.usage);
		m_LvFilter->SetItemData(ind, item.hr);
	}
	m_LvFilter->SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
	m_LvFilter->SetItemState(0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
}

/**
 * @brief Add new row to the list control.
 * @param [in] Filter string to add.
 * @param [in] enabled Is filter enabled?
 * @return Index of added row.
 */
int LineFiltersDlg::AddRow(LPCTSTR filter, int usage)
{
	int items = m_LvFilter->GetItemCount();
	int ind = m_LvFilter->InsertItem(items, filter);
	m_LvFilter->SetItemState(ind, (usage + 1) << 12, LVIS_STATEIMAGEMASK);
	return ind;
}

/**
 * @brief Edit currently selected filter.
 */
void LineFiltersDlg::EditSelectedFilter()
{
	m_LvFilter->SetFocus();
	int sel = m_LvFilter->GetNextItem(-1, LVNI_SELECTED);
	if (sel > -1)
	{
		m_LvFilter->EditLabel(sel);
	}
}

/**
 * @brief Called when Add-button is clicked.
 */
void LineFiltersDlg::OnBnClickedLfilterAddBtn()
{
	int ind = AddRow(_T(""));
	if (ind >= 0)
	{
		m_LvFilter->SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
		m_LvFilter->SetItemState(ind, LVIS_SELECTED, LVIS_SELECTED);
		m_LvFilter->EnsureVisible(ind, FALSE);
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
	for (int i = 0; i < m_LvFilter->GetItemCount(); i++)
	{
		String text = m_LvFilter->GetItemText(i, 0);
		int usage = (m_LvFilter->GetItemState(i, LVIS_STATEIMAGEMASK) >> 12) - 1;
		lineFilters.AddFilter(text.c_str(), usage);
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
	int sel = m_LvFilter->GetNextItem(-1, LVNI_SELECTED);
	if (sel != -1)
	{
		m_LvFilter->DeleteItem(sel);
		int newSel = min(m_LvFilter->GetItemCount() - 1, sel);
		if (newSel != -1)
		{
			m_LvFilter->SetItemState(newSel, LVIS_SELECTED, LVIS_SELECTED);
			m_LvFilter->EnsureVisible(newSel, FALSE);
		}
		m_LvFilter->SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
	}
}
