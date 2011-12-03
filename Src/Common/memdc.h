#ifndef _MEMDC_H_
#define _MEMDC_H_

//////////////////////////////////////////////////
// CMemDC - memory DC
//
// Author: Keith Rule
// Email:  keithr@europa.com
// Copyright 1996-2002, Keith Rule
//
// You may freely use or modify this code provided this
// Copyright is included in all derived versions.
//
// History - 10/3/97 Fixed scrolling bug.
//                   Added print support. - KR
//
//           11/3/99 Fixed most common complaint. Added
//                   background color fill. - KR
//
//           11/3/99 Added support for mapping modes other than
//                   MM_TEXT as suggested by Lee Sang Hun. - KR
//
//           02/11/02 Added support for CScrollView as supplied
//                    by Gary Kirkham. - KR
//
// This class implements a memory Device Context which allows
// flicker free drawing.

class CMemDC : public H2O::Surface<H2O::Object>
{
private:
	HBitmap		*m_bitmap;		// Offscreen bitmap
	HGdiObj		*m_oldBitmap;	// bitmap originally found in CMemDC
	HSurface	*m_dstDC;		// Saves CDC passed in constructor
	RECT		m_rect;			// Rectangle of drawing area.
public:
	using H2O::Surface<H2O::Object>::m_pDC;
	CMemDC(HSurface *dstDC, const RECT *pRect = NULL)
	{
		ASSERT(dstDC != NULL); 
		// Some initialization
		m_dstDC = dstDC;
		m_rect = *pRect;
		// Create a Memory DC
		m_pDC = dstDC->CreateCompatibleDC();
		m_bitmap = dstDC->CreateCompatibleBitmap(
			m_rect.right - m_rect.left, m_rect.bottom - m_rect.top);
		m_oldBitmap = SelectObject(m_bitmap);
	}
	~CMemDC()	
	{		
		// Copy the offscreen bitmap onto the screen.
		m_dstDC->BitBlt(m_rect.left, m_rect.top,
			m_rect.right - m_rect.left, m_rect.bottom - m_rect.top,
			m_pDC, m_rect.left, m_rect.top, SRCCOPY);
		
		//Swap back the original bitmap.
		SelectObject(m_oldBitmap);
		m_bitmap->DeleteObject();
		DeleteDC();
	}
	
};

#endif