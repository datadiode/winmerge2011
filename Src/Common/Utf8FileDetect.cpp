/////////////////////////////////////////////////////////////////////////////
//    License (GPLv2+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or (at
//    your option) any later version.
//    
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
/////////////////////////////////////////////////////////////////////////////
/** 
 * @file  Utf8FileDetect.cpp
 *
 * @brief Implementation UTF-8 file detection.
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "Utf8FileDetect.h"

// Algorithm originally from:
// TortoiseMerge - a Diff/Patch program
// Copyright (C) 2007 - TortoiseSVN

/**
 * @brief Check for invalid UTF-8 bytes in buffer.
 * This function checks if there are invalid UTF-8 bytes in the given buffer.
 * If such bytes are found, caller knows this buffer is not valid UTF-8 file.
 * @param [in] pBuffer Pointer to begin of the buffer.
 * @param [in] size Size of the buffer in bytes.
 * @return true if invalid bytes found, false otherwise.
 */
bool CheckForInvalidUtf8(LPBYTE pBuffer, size_t size)
{
	const UINT8 *const q = pBuffer + size;
	const UINT8 *p = q;
	while (p > pBuffer)
	{
		UINT8 c = *--p;
		if ((c == 0xC0) || (c == 0xC1) || (c >= 0xF5))
			return true;
	}
	bool bUTF8 = false;
	while (p < q)
	{
		UINT8 c = *p++;
		if ((c & 0x80) != 0x00)
		{
			if ((c & 0xE0) != 0xC0)
			{
				if ((c & 0xF0) != 0xE0)
				{
					if ((c & 0xF8) != 0xF0)
						return true;
					if (p == q)
						break;
					if ((*p++ & 0xC0) != 0x80)
						return true;
				}
				if (p == q)
					break;
				if ((*p++ & 0xC0) != 0x80)
					return true;
			}
			if (p == q)
				break;
			if ((*p++ & 0xC0) != 0x80)
				return true;
			bUTF8 = true;
		}
	}
	return !bUTF8;
}
