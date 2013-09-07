/** 
 * @file  Bitmap.h
 *
 * @brief Declaration file for Bitmap helper functions.
 *
 */
// ID line follows -- this is updated by SVN
// $Id$

#ifndef _BITMAP_H_
#define _BITMAP_H_

HBitmap *CopyRectToBitmap(HSurface *pDC, int x, int y, int cx, int cy);
void DrawBitmap(HSurface *pDC, int x, int y, HBitmap *pBitmap);
HBitmap *GetDarkenedBitmap(HSurface *pDC, HBitmap *pBitmap);

#endif // _BITMAP_H_
