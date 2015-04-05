/**
 *  @file   UniFile.cpp
 *  @author Perry Rapp, Creator, 2003-2006
 *  @author Kimmo Varis, 2004-2006
 *  @date   Created: 2003-10
 *  @date   Edited:  2013-04-27 Jochen Neubeck
 *
 *  @brief Implementation of Unicode enabled file classes.
 *  Classes include memory-mapped reader class and Stdio replacement class.
 */

/* The MIT License
Copyright (c) 2003 Perry Rapp
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "StdAfx.h"
#include "UniFile.h"
#include "unicoder.h"
#include "codepage.h"
#include "paths.h" // paths_GetLongPath()

namespace convert_utf
{
#	include "convert_utf/ConvertUTF.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static void Append(String &strBuffer, LPCTSTR pchTail,
	String::size_type cchTail, String::size_type cchBufferMin = 1024);

/**
 * @brief The constructor.
 */
UniFile::UniError::UniError()
{
	ClearError();
}

/**
 * @brief Check if there is error.
 * @return true if there is an error.
 */
bool UniFile::UniError::HasError() const
{
	return !apiname.empty() || !desc.empty();
}

/**
 * @brief Clears the existing error.
 */
void UniFile::UniError::ClearError()
{
	apiname.clear();
	syserrnum = ERROR_SUCCESS;
	desc.clear();
}

/**
 * @brief Get the error string.
 * @return Error string.
 */
String UniFile::UniError::GetError() const
{
	String sError;
	if (apiname.empty())
		sError = desc;
	else
		sError = GetSysError(syserrnum);
	return sError;
}

/** @brief Record an API call failure */
void UniFile::LastError(LPCTSTR apiname, int syserrnum)
{
	m_lastError.ClearError();

	m_lastError.apiname = apiname;
	m_lastError.syserrnum = syserrnum;
}

/** @brief Record a custom error */
void UniFile::LastErrorCustom(LPCTSTR desc)
{
	m_lastError.ClearError();

	m_lastError.desc = desc;
}

/////////////
// UniLocalFile
/////////////

/** @brief Create disconnected UniLocalFile, but with name */
UniLocalFile::UniLocalFile()
{
	Clear();
}

/** @brief Reset all variables to empty */
void UniLocalFile::Clear()
{
	m_statusFetched = 0;
	m_filesize.int64 = 0;
	m_unicoding = NONE;
	m_charsize = 1;
	m_codepage = getDefaultCodepage();
	m_txtstats.clear();
	m_bom = false;
}

/**
 * @brief Get file status into member variables
 *
 * Reads file's status (size and full path).
 * @return true on success, false on failure.
 * @note Function sets filesize member to zero, and status as read
 * also when failing. That is needed so caller doesn't need to waste
 * time checking if file already exists (and ignores return value).
 */
bool UniMemFile::GetFileStatus(LPCTSTR filename)
{
	m_statusFetched = -1;
	m_lastError.ClearError();
	C_ASSERT(sizeof m_filesize == sizeof(LARGE_INTEGER));
	if (GetFileSizeEx(m_handle, reinterpret_cast<LARGE_INTEGER *>(&m_filesize)))
	{
		if (m_filesize.int64 == 0)
		{
			// if m_filesize equals zero, the file size is really zero or the file is a symbolic link.
			// use GetCompressedFileSize() to get the file size of the symbolic link target whether the file is symbolic link or not.
			// if the file is not symbolic link, GetCompressedFileSize() will return zero.
			// NOTE: GetCompressedFileSize() returns error for pre-W2K windows versions
			DWORD dwFileSizeHigh;
			DWORD dwFileSizeLow = GetCompressedFileSize(filename, &dwFileSizeHigh);
			if (GetLastError() == 0)
			{
				m_filesize.Lo = dwFileSizeLow;
				m_filesize.Hi = dwFileSizeHigh;
			}
		}
		m_statusFetched = 1;

		return true;
	}
	else
	{
		m_filesize.int64 = 0;
		m_statusFetched = 1; // Yep, done for this file still
		LastError(_T("GetFileSizeEx"), GetLastError());
		return false;
	}
}

/////////////
// UniMemFile
/////////////

UniMemFile::UniMemFile()
	: m_handle(INVALID_HANDLE_VALUE)
	, m_hMapping(INVALID_HANDLE_VALUE)
	, m_base(NULL)
	, m_data(NULL)
	, m_current(NULL)
{
}

void UniMemFile::Close()
{
	Clear();
	if (m_base)
	{
		UnmapViewOfFile(m_base);
		m_base = 0;
	}
	m_data = NULL;
	m_current = NULL;
	if (m_hMapping != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hMapping);
		m_hMapping = INVALID_HANDLE_VALUE;
	}
	if (m_handle != INVALID_HANDLE_VALUE)
	{
		FlushFileBuffers(m_handle);
		CloseHandle(m_handle);
		m_handle = INVALID_HANDLE_VALUE;
	}
}

/** @brief Is it currently attached to a file ? */
bool UniMemFile::IsOpen() const
{
	return m_handle != INVALID_HANDLE_VALUE;
}

/** @brief Open file for generic read-only access */
bool UniMemFile::OpenReadOnly(LPCTSTR filename)
{
	return Open(filename,
		GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
		OPEN_EXISTING, PAGE_READONLY, FILE_MAP_READ);
}

/** @brief Open file with specified arguments */
bool UniMemFile::Open(LPCTSTR filename, DWORD dwOpenAccess, DWORD dwOpenShareMode, DWORD dwOpenCreationDispostion, DWORD dwMappingProtect, DWORD dwMapViewAccess)
{
	// We use an internal workhorse to make it easy to close on any error
	if (!DoOpen(filename, dwOpenAccess, dwOpenShareMode, dwOpenCreationDispostion, dwMappingProtect, dwMapViewAccess))
	{
		Close();
		return false;
	}
	return true;
}

/** @brief Internal implementation of Open */
bool UniMemFile::DoOpen(LPCTSTR filename, DWORD dwOpenAccess, DWORD dwOpenShareMode, DWORD dwOpenCreationDispostion, DWORD dwMappingProtect, DWORD dwMapViewAccess)
{
	Close();

	m_handle = CreateFile(filename, dwOpenAccess, dwOpenShareMode, NULL, dwOpenCreationDispostion, 0, 0);
	if (m_handle == INVALID_HANDLE_VALUE)
	{
		LastError(_T("CreateFile"), GetLastError());
		return false;
	}
	if (!GetFileStatus(filename))
		return false;

	if (m_filesize.Hi || m_filesize.Lo > 0x7FFFFFFF)
	{
		LastErrorCustom(_T("UniMemFile cannot handle files over 2 gigabytes"));
		return false;
	}

	if (m_filesize.Lo == 0)
	{
		// Allow opening empty file, but memory mapping doesn't work on such
		// m_base and m_current are 0 from the Close call above
		// so ReadString will correctly return empty EOF immediately
		return true;
	}

	m_hMapping = CreateFileMapping(m_handle, NULL, dwMappingProtect, m_filesize.Hi, m_filesize.Lo, NULL);
	if (!m_hMapping)
	{
		LastError(_T("CreateFileMapping"), GetLastError());
		return false;
	}

	m_base = (LPBYTE)MapViewOfFile(m_hMapping, dwMapViewAccess, 0, 0, m_filesize.Lo);
	if (!m_base)
	{
		LastError(_T("MapViewOfFile"), GetLastError());
		return false;
	}
	m_data = m_base;
	m_current = m_base;

	return true;
}

/**
 * @brief Check for Unicode BOM (byte order mark) at start of file
 *
 * @note This code only checks for UCS-2LE, UCS-2BE, and UTF-8 BOMs (no UCS-4).
 */
void UniMemFile::ReadBom()
{
	if (!IsOpen())
		return;

	if (m_filesize.Hi != 0)
		return;

	PBYTE lpByte = m_base;
	m_current = m_data = m_base;
	m_charsize = 1;
	unsigned bom = 0;

	m_unicoding = DetermineEncoding(lpByte, m_filesize.Lo, &bom);
	switch (m_unicoding)
	{
	case UCS2LE:
	case UCS2BE:
		m_charsize = 2;
		break;
	case UCS4LE:
	case UCS4BE:
		m_charsize = 4;
		break;
	case UTF8:
		break;
	case NONE:
		bom = 0;
		break;
	}

	m_bom = bom != 0;
	m_data = lpByte + bom;
	m_current = m_data;
}

/**
 * @brief Append characters to string.
 * This function appends characters to the string. The storage for the string
 * is grown exponentially to avoid unnecessary allocations and copying.
 * @param [in, out] strBuffer A string to wich new characters are appended.
 * @param [in] ccHead Index in the string where new chars are appended.
 * @param [in] pchTaíl Characters to append.
 * @param [in] cchTail Amount of characters to append.
 * @param [in] cchBufferMin Minimum size for the buffer.
 * @return New length of the string.
 */
static void Append(String &strBuffer, LPCTSTR pchTail,
	String::size_type cchTail, String::size_type cchBufferMin)
{
	String::size_type cchBuffer = strBuffer.capacity();
	String::size_type cchHead = strBuffer.length();
	String::size_type cchLength = cchHead + cchTail;
	while (cchBuffer < cchLength)
	{
		ASSERT((cchBufferMin & cchBufferMin - 1) == 0); // must be a power of 2
		cchBuffer &= ~(cchBufferMin - 1); // truncate to a multiple of cchBufferMin
		cchBuffer += cchBuffer;
		if (cchBuffer < cchBufferMin)
			cchBuffer = cchBufferMin;
	}
	strBuffer.reserve(cchBuffer);
	strBuffer.append(pchTail, cchTail);
}

/**
 * @brief Read one (DOS or UNIX or Mac) line.
 * @param [out] line Line read.
 * @param [out] eol EOL bytes read (if any).
 * @param [out] lossy TRUE if there were lossy encoding.
 * @return true if there is more lines to read, false when last line is read.
 */
bool UniMemFile::ReadString(String & line, String & eol, bool * lossy)
{
	line.clear();
	eol.clear();
	LPCTSTR pchLine = (LPCTSTR)m_current;

	// shortcut methods in case file is in the same encoding as our Strings

	if (m_unicoding == UCS2LE)
	{
		int cchLine = 0;
		// If there aren't any wchars left in the file, return FALSE to indicate EOF
		if (m_current - m_base + 1 >= m_filesize.int64)
			return false;
		// Loop through wchars, watching for eol chars or zero
		while (m_current - m_base + 1 < m_filesize.int64)
		{
			wchar_t wch = *(wchar_t *)m_current;
			m_current += 2;
			if (wch == '\n' || wch == '\r')
			{
				eol += wch;
				if (wch == '\r')
				{
					if (m_current - m_base + 1 < m_filesize.int64 && *(wchar_t *)m_current == '\n')
					{
						eol += '\n';
						m_current += 2;
						++m_txtstats.ncrlfs;
					}
					else
					{
						++m_txtstats.ncrs;
					}
				}
				else
				{
					++m_txtstats.nlfs;
				}
				line.assign(pchLine, cchLine);
				return true;
			}
			if (!wch)
			{
				++m_txtstats.nzeros;
			}
			++cchLine;
		}
		line.assign(pchLine, cchLine);
		return true;
	}

	if (m_current - m_base + (m_charsize - 1) >= m_filesize.int64)
		return false;

	// Handle 8-bit strings in line chunks because of multibyte codings (eg, 936)
	if (m_unicoding == NONE)
	{
		bool eof = true;
		LPBYTE eolptr = 0;
		for (eolptr = m_current; (eolptr - m_base + (m_charsize - 1) < m_filesize.int64); ++eolptr)
		{
			if (*eolptr == '\n' || *eolptr == '\r')
			{
				eof = false;
				break;
			}
			if (*eolptr == 0)
			{
				++m_txtstats.nzeros;
			}
		}
		bool success = ucr::maketstring(line, (LPCSTR)m_current,
			static_cast<int>(eolptr - m_current), m_codepage, lossy);
		if (!success)
		{
			return false;
		}
		if (lossy && *lossy)
			++m_txtstats.nlosses;
		if (!eof)
		{
			eol += (TCHAR) * eolptr;
			if (*eolptr == '\r')
			{
				if (eolptr - m_base + (m_charsize - 1) < m_filesize.int64 && eolptr[1] == '\n')
				{
					eol += '\n';
					++m_txtstats.ncrlfs;
				}
				else
					++m_txtstats.ncrs;
			}
			else
				++m_txtstats.nlfs;
		}
		m_current = eolptr + eol.length();
		// TODO: What do we do if save was lossy ?
		return !eof;
	}

	while (m_current - m_base + (m_charsize - 1) < m_filesize.int64)
	{
		ULONG ch = 0;
		UINT len = m_charsize;
		bool doneline = false;

		if (m_unicoding == UTF8)
		{
			const convert_utf::UTF8 *sourceStart = m_current;
			convert_utf::UTF32 *targetStart = &ch;
			convert_utf::ConversionResult result = convert_utf::ConvertUTF8toUTF32(
				&sourceStart, m_base + m_filesize.int64,
				&targetStart, targetStart + 1,
				convert_utf::lenientConversion);
			switch (result)
			{
			case convert_utf::conversionOK:
			case convert_utf::targetExhausted:
				len = static_cast<UINT>(sourceStart - m_current);
				break;
			case convert_utf::sourceExhausted:
				m_current = m_base + m_filesize.int64;
				doneline = true;
				// fall through
			default:
				ch = '?';
				++m_txtstats.nlosses;
				break;
			}
		}
		else
		{
			ch = ucr::get_unicode_char(m_current, m_unicoding, m_codepage);
		}
		// convert from Unicode codepoint to TCHAR string
		// could be multicharacter if decomposition took place, for example
		if (ch == '\r')
		{
			doneline = true;
			bool crlf = false;
			// check for crlf pair
			if (m_current - m_base + 2 * m_charsize - 1 < m_filesize.int64)
			{
				// For UTF-8, this ch will be wrong if character is non-ASCII
				// but we only check it against \n here, so it doesn't matter
				UINT ch = ucr::get_unicode_char(m_current + m_charsize, m_unicoding);
				if (ch == '\n')
				{
					crlf = true;
				}
			}
			if (crlf)
			{
				eol = _T("\r\n");
				++m_txtstats.ncrlfs;
				// advance an extra character to skip the following lf
				m_current += m_charsize;
			}
			else
			{
				eol = _T("\r");
				++m_txtstats.ncrs;
			}
		}
		else if (ch == '\n')
		{
			eol = _T("\n");
			doneline = true;
			++m_txtstats.nlfs;
		}
		else if (ch == '\0')
		{
			++m_txtstats.nzeros;
		}
		// always advance to next character
		m_current += len;
		if (doneline)
		{
			return true;
		}
		TCHAR targetBuf[2];
		convert_utf::UTF16 *targetStart = targetBuf;
		const convert_utf::UTF32 *sourceStart = &ch;
		convert_utf::ConversionResult result = convert_utf::ConvertUTF32toUTF16(
			&sourceStart, sourceStart + 1,
			&targetStart, targetStart + 2,
			convert_utf::lenientConversion);
		Append(line, targetBuf, static_cast<String::size_type>(targetStart - targetBuf));
	}
	return true;
}

/////////////
// UniStdioFile
/////////////

UniStdioFile::UniStdioFile()
: m_fp(0), m_ucrbuff(128)
{
	m_pstm = this;
}

UniStdioFile::~UniStdioFile()
{
	m_pstm->Release();
	Close();
}

void UniStdioFile::Attach(ISequentialStream *pstm)
{
	m_pstm = pstm;
	m_pstm->AddRef();
}

HGLOBAL UniStdioFile::CreateStreamOnHGlobal()
{
	HGLOBAL hMem = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, 0);
	if (hMem && FAILED(::CreateStreamOnHGlobal(hMem, FALSE, (IStream **)&m_pstm)))
	{
		::GlobalFree(hMem);
		hMem = NULL;
	}
	return hMem;
}

void UniStdioFile::Close()
{
	if (IsOpen())
	{
		fclose(m_fp);
		m_fp = 0;
	}
	m_statusFetched = 0;
	m_filesize.int64 = 0;
	// preserve m_unicoding
	// preserve m_charsize
	// preserve m_codepage
	m_txtstats.clear();
}

/** @brief Is it currently attached to a file ? */
bool UniStdioFile::IsOpen() const
{
	return m_fp != 0;
}

bool UniStdioFile::OpenReadOnly(LPCTSTR filename)
{
	ASSERT(0); // unimplemented
	return false;
}

bool UniStdioFile::OpenCreate(LPCTSTR filename)
{
	return Open(filename, _T("w+b"));
}

bool UniStdioFile::OpenCreateUtf8(LPCTSTR filename)
{
	if (!OpenCreate(filename))
		return false;
	SetUnicoding(UTF8);
	return true;
}

bool UniStdioFile::Open(LPCTSTR filename, LPCTSTR mode)
{
	if (!DoOpen(filename, mode))
	{
		Close();
		return false;
	}
	return true;
}

bool UniStdioFile::DoOpen(LPCTSTR filename, LPCTSTR mode)
{
	Close();
	m_statusFetched = -1;
	m_lastError.ClearError();
	m_fp = _tfopen(filename, mode);
	if (m_fp == 0)
	{
		LastErrorCustom(_tcserror(errno));
		return false;
	}
	return true;
}

/** @brief Write BOM (byte order mark) if Unicode file */
void UniStdioFile::WriteBom()
{
	if (m_bom)
	{
		if (m_unicoding == UCS2LE)
		{
			static const unsigned char bom[] = { '\xFF', '\xFE' };
			m_pstm->Write(bom, sizeof bom, NULL);
		}
		else if (m_unicoding == UCS2BE)
		{
			static const unsigned char bom[] = { '\xFE', '\xFF' };
			m_pstm->Write(bom, sizeof bom, NULL);
		}
		else if (m_unicoding == UTF8)
		{
			static const unsigned char bom[] = { '\xEF', '\xBB', '\xBF' };
			m_pstm->Write(bom, sizeof bom, NULL);
		}
	}
}

/**
 * @brief Write one line (doing any needed conversions)
 */
bool UniStdioFile::WriteString(LPCTSTR line, String::size_type length)
{
	// shortcut the easy cases
	if (m_unicoding == UCS2LE)
	{
		return SUCCEEDED(m_pstm->Write(line, length * sizeof(TCHAR), NULL));
	}
	const BYTE *src = reinterpret_cast<const BYTE *>(line);
	String::size_type srcbytes = length * sizeof(TCHAR);
	BOOL loss = FALSE;
	ucr::convert(UCS2LE, 0, src, srcbytes, m_unicoding, m_codepage, &m_ucrbuff, &loss);
	// TODO: What to do about lossy conversion ?
	return SUCCEEDED(m_pstm->Write(m_ucrbuff.ptr, m_ucrbuff.size, NULL));
}

HRESULT STDMETHODCALLTYPE UniStdioFile::QueryInterface(REFIID, void **)
{
	ASSERT(0); // unimplemented
	return E_NOTIMPL;
}

ULONG STDMETHODCALLTYPE UniStdioFile::AddRef()
{
	return 1;
}

ULONG STDMETHODCALLTYPE UniStdioFile::Release()
{
	return 1;
}

HRESULT STDMETHODCALLTYPE UniStdioFile::Read(void *, ULONG, ULONG *)
{
	ASSERT(0); // unimplemented
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE UniStdioFile::Write(const void *pv, ULONG cb, ULONG *pcbWritten)
{
	cb = static_cast<ULONG>(fwrite(pv, 1, cb, m_fp));
	m_filesize.int64 += cb;
	if (pcbWritten)
		*pcbWritten = cb;
	if (int e = ferror(m_fp))
	{
		LastErrorCustom(_tcserror(e));
		return E_FAIL;
	}
	return S_OK;
}
