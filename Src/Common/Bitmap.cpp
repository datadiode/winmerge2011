/** 
 * @file  Bitmap.cpp
 *
 * @brief Implementation file for Bitmap helper functions.
 *
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include <math.h>
#include "Bitmap.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/**
 * @brief Save an area as a bitmap
 * @param pDC [in] The source device context
 * @param rect [in] The rect to be copied
 * @return The bitmap object
 */
HBitmap *CopyRectToBitmap(HSurface *pDC, int x, int y, int cx, int cy)
{
	HBitmap *pBitmap = NULL;
	if (HSurface *pdcMem = pDC->CreateCompatibleDC())
	{
		pBitmap = pDC->CreateCompatibleBitmap(cx, cy);
		HGdiObj *pOldBitmap = pdcMem->SelectObject(pBitmap);
		pdcMem->BitBlt(0, 0, cx, cy, pDC, x, y, SRCCOPY);
		pdcMem->SelectObject(pOldBitmap);
		pdcMem->DeleteDC();
	}
	return pBitmap;
}

/**
 * @brief Draw a bitmap image
 * @param pDC [in] The destination device context to draw to
 * @param x [in] The x-coordinate of the upper-left corner of the bitmap
 * @param y [in] The y-coordinate of the upper-left corner of the bitmap
 * @param pBitmap [in] the bitmap to draw
 */
void DrawBitmap(HSurface *pDC, int x, int y, HBitmap *pBitmap)
{
	if (HSurface *pdcMem = pDC->CreateCompatibleDC())
	{
		BITMAP bm;
		pBitmap->GetBitmap(&bm);
		HGdiObj *pOldBitmap = pdcMem->SelectObject(pBitmap);
		pDC->BitBlt(x, y, bm.bmWidth, bm.bmHeight, pdcMem, 0, 0, SRCCOPY);
		pdcMem->SelectObject(pOldBitmap);
		pdcMem->DeleteDC();
	}
}

/**
 * @brief Duplicate a bitmap and make it dark
 * @param pDC [in] Device context
 * @param pBitmap [in] the bitmap to darken
 * @return The bitmap object
 */
HBitmap *GetDarkenedBitmap(HSurface *pDC, HBitmap *pBitmap)
{
	HBitmap *pBitmapDarkened = NULL;
	if (HSurface *pdcMem = pDC->CreateCompatibleDC())
	{
		BITMAP bm;
		pBitmap->GetBitmap(&bm);

		BITMAPINFO bi;
		bi.bmiHeader.biSize = sizeof bi.bmiHeader;
		bi.bmiHeader.biWidth = bm.bmWidth;
		bi.bmiHeader.biHeight = bm.bmHeight;
		bi.bmiHeader.biPlanes = 1;
		bi.bmiHeader.biBitCount = 32;
		bi.bmiHeader.biCompression = 0;
		bi.bmiHeader.biSizeImage = bm.bmWidth * 4 * bm.bmHeight;
		bi.bmiHeader.biXPelsPerMeter = 0;
		bi.bmiHeader.biYPelsPerMeter = 0;
		bi.bmiHeader.biClrUsed = 0;
		bi.bmiHeader.biClrImportant = 0;

		BYTE *pbuf = NULL;
		pBitmapDarkened = pDC->CreateDIBSection(&bi, DIB_RGB_COLORS, (void **)&pbuf);
		if (pBitmapDarkened)
		{
			HGdiObj *pOldBitmap = pdcMem->SelectObject(pBitmapDarkened);
			DrawBitmap(pdcMem, 0, 0, pBitmap);
			pdcMem->SelectObject(pOldBitmap);
			int x;
			for (x = 0; x < bm.bmWidth; x++)
			{
				double b = 0.70 + (0.20 * sin(acos((double)x/bm.bmWidth*2.0-1.0)));
				for (int y = 1; y < bm.bmHeight-1; y++)
				{
					int i = x * 4 + y * bm.bmWidth * 4;
					pbuf[i  ] = (BYTE)(pbuf[i] * 0.95);
					pbuf[i+1] = (BYTE)(pbuf[i+1] * b);
					pbuf[i+2] = (BYTE)(pbuf[i+2] * b);
				}
			}
			for (x = 0; x < bm.bmWidth; x++)
			{
				int i = x * 4 + 0 * bm.bmWidth * 4;
				pbuf[i  ] = (BYTE)(pbuf[i] * 0.95);
				pbuf[i+1] = (BYTE)(pbuf[i+1] * 0.7);
				pbuf[i+2] = (BYTE)(pbuf[i+2] * 0.7);
				i = x * 4 + (bm.bmHeight-1) * bm.bmWidth * 4;
				pbuf[i  ] = (BYTE)(pbuf[i] * 0.95);
				pbuf[i+1] = (BYTE)(pbuf[i+1] * 0.7);
				pbuf[i+2] = (BYTE)(pbuf[i+2] * 0.7);
			}
			for (int y = 0; y < bm.bmHeight; y++)
			{
				int i = 0 * 4 + y * bm.bmWidth * 4;
				pbuf[i  ] = (BYTE)(pbuf[i] * 0.95);
				pbuf[i+1] = (BYTE)(pbuf[i+1] * 0.4);
				pbuf[i+2] = (BYTE)(pbuf[i+2] * 0.4);
				i = (bm.bmWidth-1) * 4 + y * bm.bmWidth * 4;
				pbuf[i  ] = (BYTE)(pbuf[i] * 0.95);
				pbuf[i+1] = (BYTE)(pbuf[i+1] * 0.4);
				pbuf[i+2] = (BYTE)(pbuf[i+2] * 0.4);
			}
		}
		pdcMem->DeleteDC();
	}
	return pBitmapDarkened;
}
