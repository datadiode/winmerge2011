/**
 *  @file SortHeaderCtrl.cpp
 *
 *  @brief Implementation of CSortHeaderCtrl
 */ 
#include "StdAfx.h"
#include "SortHeaderCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// CSortHeaderCtrl

CSortHeaderCtrl::CSortHeaderCtrl()
: m_bSortAsc(TRUE)
, m_nSortCol(-1)
{
}

int CSortHeaderCtrl::SetSortImage(int nCol, BOOL bAsc)
{
	int nPrevCol = m_nSortCol;

	m_nSortCol = nCol;
	m_bSortAsc = bAsc;

	LRESULT ccVer = SendMessage(CCM_GETVERSION);
	// Clear HDF_SORTDOWN and HDF_SORTUP flag in all columns.
	int i = GetItemCount();
	while (i > 0)
	{
		HD_ITEM hditem;
		hditem.mask = HDI_FORMAT;
		GetItem(--i, &hditem);
		hditem.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
		if (ccVer < 6)
		{
			hditem.fmt &= ~(HDF_STRING | HDF_BITMAP | HDF_BITMAP_ON_RIGHT);
			hditem.fmt |= HDF_OWNERDRAW;
		}
		else if (i == nCol)
		{
			hditem.fmt |= bAsc ? HDF_SORTUP : HDF_SORTDOWN;
		}
		SetItem(i, &hditem);
	}
	// Invalidate header control so that it gets redrawn
	Invalidate();
	return nPrevCol;
}

void CSortHeaderCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	HSurface *pdc = reinterpret_cast<HSurface *>(lpDrawItemStruct->hDC);
	// Get the column rect
	RECT rcLabel = lpDrawItemStruct->rcItem;
	// Save DC
	int nSavedDC = pdc->SaveDC();
	// Set clipping region to limit drawing within column
	pdc->IntersectClipRect(rcLabel.left, rcLabel.top, rcLabel.right, rcLabel.bottom);
	// Labels are offset by a certain amount	
	// This offset is related to the width of a space character
	SIZE ext;
	pdc->GetTextExtent(_T("  "), 2, &ext);
	// Get the column text and format
	TCHAR buf[256];
	HD_ITEM hditem;
	hditem.mask = HDI_TEXT | HDI_FORMAT;
	hditem.pszText = buf;
	hditem.cchTextMax = _countof(buf);
	GetItem(lpDrawItemStruct->itemID, &hditem);
	// Determine format for drawing column label
	UINT uFormat = (4 >> (hditem.fmt & HDF_JUSTIFYMASK)) & (DT_CENTER | DT_RIGHT) |
		DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER | DT_END_ELLIPSIS;
	// Adjust the rect if the mouse button is pressed on it
	if (lpDrawItemStruct->itemState == ODS_SELECTED)
	{
		rcLabel.left++;
		rcLabel.top += 2;
		rcLabel.right++;
	}

	// Draw the Sort arrow
	if (lpDrawItemStruct->itemID == m_nSortCol)
	{
		RECT rcIcon = lpDrawItemStruct->rcItem;

		// Set up pens to use for drawing the triangle
		HPen *penLight = HPen::Create(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));
		HPen *penShadow = HPen::Create(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
		HGdiObj *pOldPen = pdc->SelectObject(penLight);

		int offset = (rcIcon.bottom - rcIcon.top) / 4;
		if (m_bSortAsc)
		{
			// Draw triangle pointing upwards
			pdc->MoveTo(rcIcon.right - 2 * offset, offset);
			pdc->LineTo(rcIcon.right - offset, rcIcon.bottom - offset - 1);
			pdc->LineTo(rcIcon.right - 3 * offset - 2, rcIcon.bottom - offset - 1);
			pdc->MoveTo(rcIcon.right - 3 * offset - 1, rcIcon.bottom - offset - 1);
			pdc->SelectObject(penShadow);
			pdc->LineTo(rcIcon.right - 2 * offset, offset - 1);
		}
		else
		{
			// Draw triangle pointing downwards
			pdc->MoveTo(rcIcon.right - offset - 1, offset);
			pdc->LineTo(rcIcon.right - 2 * offset - 1, rcIcon.bottom - offset);
			pdc->MoveTo(rcIcon.right - 2 * offset - 2, rcIcon.bottom - offset);
			pdc->SelectObject(penShadow);
			pdc->LineTo(rcIcon.right - 3*offset - 1, offset);
			pdc->LineTo(rcIcon.right - offset - 1, offset);
		}
		// Restore the pen
		pdc->SelectObject(pOldPen);
		penLight->DeleteObject();
		penShadow->DeleteObject();
		// Adjust the rect further
		rcLabel.right -= 3 * ext.cx;
	}

	rcLabel.left += ext.cx;
	rcLabel.right -= ext.cx;

	// Draw column label
	if (rcLabel.left < rcLabel.right)
		pdc->DrawText(buf, -1, &rcLabel, uFormat);

	// Restore dc
	pdc->RestoreDC(nSavedDC);
}
