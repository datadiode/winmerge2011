/** 
 * @file DiffFileData.cpp
 *
 * @brief Code for DiffFileData class
 *
 * @date  Created: 2003-08-22
 */
#include <StdAfx.h>
#include <io.h>
#include "coretypes.h"
#include "DiffItem.h"
#include "FileLocation.h"
#include "Diff.h"
#include "DiffFileData.h"

/**
 * @brief Simple initialization of DiffFileData
 * @note Diffcounts are initialized to invalid values, not zeros.
 */
DiffFileData::DiffFileData(int codepage)
: m_inf(new file_data[2])
, m_FileLocation(2)
, m_textStats(2)
, m_sDisplayFilepath(2)
{
	memset(m_inf, 0, 2 * sizeof *m_inf);
	m_used = false;
	Reset();
	// Set default codepages
	for (stl_size_t i = 0 ; i < m_FileLocation.size() ; ++i)
	{
		m_FileLocation[i].encoding.SetCodepage(codepage);
	}
}

/** @brief deallocate member data */
DiffFileData::~DiffFileData()
{
	Reset();
	delete [] m_inf;
}

/** @brief Open file descriptors in the inf structure (return false if failure) */
bool DiffFileData::OpenFiles(LPCTSTR szFilepath1, LPCTSTR szFilepath2)
{
	m_FileLocation[0].filepath = szFilepath1;
	m_FileLocation[1].filepath = szFilepath2;
	bool b = DoOpenFiles();
	if (!b)
		Reset();
	return b;
}

HANDLE DiffFileData::GetFileHandle(int i)
{
	int d = m_inf[i].desc;
	return d > 0 ? reinterpret_cast<HANDLE>(_get_osfhandle(d)) : NULL;
}

/** @brief stash away true names for display, before opening files */
void DiffFileData::SetDisplayFilepaths(LPCTSTR szTrueFilepath1, LPCTSTR szTrueFilepath2)
{
	m_sDisplayFilepath[0] = szTrueFilepath1;
	m_sDisplayFilepath[1] = szTrueFilepath2;
}

/** @brief Allocate a buffer in the inf structure (return NULL if failure) */
void *DiffFileData::AllocBuffer(int i, size_t len, size_t alloc_extra)
{
	typedef unsigned word;
	m_inf[i].desc = i;
	m_inf[i].name = allocated_buffer_name;
	m_inf[i].buffered_chars = len;
	len += (alloc_extra & len / 2) + sizeof(word) + 1;
	m_inf[i].bufsize = len;
	void *buffer = malloc(len);
	m_inf[i].buffer = (char *)buffer;

	m_used = true;
	return buffer;
}

namespace msvcrt
{
	extern "C" _CRTIMP int __cdecl _topen(const TCHAR *, int, ...);
}

/** @brief Open file descriptors in the inf structure (return false if failure) */
bool DiffFileData::DoOpenFiles()
{
	Reset();

	for (int i = 0; i < 2; ++i)
	{
		// Fill in 8-bit versions of names for diffutils (WinMerge doesn't use these)
		// Actual paths are m_FileLocation[i].filepath
		// but these are often temporary files
		// Displayable (original) paths are m_sDisplayFilepath[i]
		m_inf[i].name = HString::Uni(m_sDisplayFilepath[i].c_str())->Oct()->A;
		if (m_inf[i].name == NULL)
			return false;

		// Open up file descriptors
		// Always use O_BINARY mode, to avoid terminating file read on ctrl-Z (DOS EOF)
		// Also, WinMerge-modified diffutils handles all three major eol styles
		if (m_inf[i].desc == 0)
		{
			m_inf[i].desc = msvcrt::_topen(m_FileLocation[i].filepath.c_str(),
					O_RDONLY | O_BINARY, _S_IREAD);
		}
		if (m_inf[i].desc < 0)
			return false;

		// Get file stats (diffutils uses these)
		if (_fstati64(m_inf[i].desc, &m_inf[i].stat) != 0)
		{
			return false;
		}
		
		if (_tcsicmp(m_FileLocation[0].filepath.c_str(), m_FileLocation[1].filepath.c_str()) == 0)
		{
			m_inf[1].desc = m_inf[0].desc;
		}
	}

	m_used = true;
	return true;
}

/** @brief Clear inf structure to pristine */
void DiffFileData::Reset()
{
	// If diffutils put data in, have it cleanup
	if (m_used)
	{
		cleanup_file_buffers(m_inf);
		m_used = false;
	}
	// clean up any open file handles, and zero stuff out
	// open file handles might be leftover from a failure in DiffFileData::OpenFiles
	for (int i = 0; i < 2; ++i)
	{
		if (m_inf[i].name != allocated_buffer_name)
		{
			if (m_inf[1].desc == m_inf[0].desc)
			{
				m_inf[1].desc = 0;
			}
			SysFreeString((BSTR)m_inf[i].name);
			m_inf[i].name = NULL;

			if (m_inf[i].desc > 0)
			{
				_close(m_inf[i].desc);
			}
			m_inf[i].desc = 0;
			memset(&m_inf[i], 0, sizeof(m_inf[i]));
		}
	}
}
