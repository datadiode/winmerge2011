/**
 *  @file UniMarkdownFile.cpp
 *
 *  @brief Implementation of UniMarkdownFile class.
 */ 
// ID line follows -- this is updated by SVN
// $Id: UniMarkdownFile.cpp 6936 2009-07-26 17:17:18Z kimmov $

#include "StdAfx.h"
#include <shlwapi.h>
#include "UnicodeString.h"
#include "UniStreamFile.h"
#include "markdown.h"
#include "unicoder.h"

UniStreamFile::UniStreamFile()
: m_pstm(NULL)
{
	m_pucrbuff = new ucr::buffer(128);
}

UniStreamFile::~UniStreamFile()
{
	Close();
	delete m_pucrbuff;
}

void UniStreamFile::Attach(ISequentialStream *pstm)
{
	ASSERT(pstm != NULL);
	ASSERT(m_pstm == NULL);
	m_pstm = pstm;
	m_pstm->AddRef();
}

void UniStreamFile::Close()
{
	if (m_pstm)
		m_pstm->Release();
	m_pstm = NULL;
}

/** @brief Is it currently attached to a file ? */
bool UniStreamFile::IsOpen() const
{
	return m_pstm != NULL;
}

bool UniStreamFile::OpenReadOnly(LPCTSTR filename)
{
	return Open(filename, STGM_READ | STGM_SHARE_DENY_NONE);
}

bool UniStreamFile::OpenCreate(LPCTSTR filename)
{
	return Open(filename, STGM_WRITE | STGM_CREATE | STGM_SHARE_DENY_NONE);
}

bool UniStreamFile::OpenCreateUtf8(LPCTSTR filename)
{
	if (!OpenCreate(filename))
		return false;
	SetUnicoding(ucr::UTF8);
	return true;
}

bool UniStreamFile::Open(LPCTSTR filename, DWORD grfMode)
{
	Close();

	if (FAILED(SHCreateStreamOnFile(filename, grfMode, reinterpret_cast<IStream **>(&m_pstm))))
		return false;

	m_filepath = filename;
	m_filename = filename; // TODO: Make canonical ?

	m_lineno = 0;

	return true;
}

bool UniStreamFile::ReadBom()
{
	return false;
}

/**
 * @brief Returns if file has a BOM bytes.
 * @return true if file has BOM bytes, false otherwise.
 */
bool UniStreamFile::HasBom()
{
	return m_bom;
}

/**
 * @brief Sets if file has BOM or not.
 * @param [in] true to have a BOM in file, false to not to have.
 */
void UniStreamFile::SetBom(bool bom)
{
	m_bom = bom;
}

bool UniStreamFile::ReadString(String &, bool *)
{
	ASSERT(0); // unimplemented -- currently cannot read from a UniStreamFile!
	return false;
}

bool UniStreamFile::ReadString(String &, String &, bool *)
{
	ASSERT(0); // unimplemented -- currently cannot read from a UniStreamFile!
	return false;
}

/** @brief Write BOM (byte order mark) if Unicode file */
void UniStreamFile::WriteBom()
{
	if (m_unicoding == ucr::UCS2LE)
	{
		unsigned char bom[] = "\xFF\xFE";
		m_pstm->Write(bom, 2, NULL);
	}
	else if (m_unicoding == ucr::UCS2BE)
	{
		unsigned char bom[] = "\xFE\xFF";
		m_pstm->Write(bom, 2, NULL);
	}
	else if (m_unicoding == ucr::UTF8 && m_bom)
	{
		unsigned char bom[] = "\xEF\xBB\xBF";
		m_pstm->Write(bom, 3, NULL);
	}
}

/**
 * @brief Write one line (doing any needed conversions)
 */
bool UniStreamFile::WriteString(LPCTSTR line, size_t length)
{
	// shortcut the easy cases
#ifdef _UNICODE
	if (m_unicoding == ucr::UCS2LE)
#else
	if (m_unicoding == ucr::NONE && EqualCodepages(m_codepage, getDefaultCodepage()))
#endif
	{
		size_t bytes = length * sizeof(TCHAR);
		return SUCCEEDED(m_pstm->Write(line, bytes, NULL));
	}

	ucr::buffer * buff = (ucr::buffer *)m_pucrbuff;
	ucr::UNICODESET unicoding1 = ucr::NONE;
	int codepage1 = 0;
	ucr::getInternalEncoding(&unicoding1, &codepage1); // What String & TCHARs represent
	const unsigned char * src = (const UCHAR *)line;
	size_t srcbytes = length * sizeof(TCHAR);
	bool lossy = ucr::convert(unicoding1, codepage1, src, srcbytes, (ucr::UNICODESET)m_unicoding, m_codepage, buff);
	// TODO: What to do about lossy conversion ?
	return SUCCEEDED(m_pstm->Write(buff->ptr, buff->size, NULL));
}

__int64 UniStreamFile::GetPosition() const
{
	ASSERT(0);
	return 0;
}
