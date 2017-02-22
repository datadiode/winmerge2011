/** 
 * @file  codepage_detect.h
 *
 * @brief Declaration file for codepage detection routines.
 */
#pragma once

extern HINSTANCE hCharsets;

class FileTextEncoding;

void GuessCodepageEncoding(LPCTSTR filepath, FileTextEncoding *encoding, bool bGuessEncoding, HANDLE osfhandle = NULL);

unsigned GuessEncoding_from_bytes(LPCTSTR ext, const char *src, stl_size_t len);
