/** 
 * @file  Bitmap.h
 *
 * @brief Declaration file for Bitmap helper functions.
 *
 */
#pragma once

HBitmap *CopyRectToBitmap(HSurface *pDC, int x, int y, int cx, int cy);
void DrawBitmap(HSurface *pDC, int x, int y, HBitmap *pBitmap);
HBitmap *GetDarkenedBitmap(HSurface *pDC, HBitmap *pBitmap);
