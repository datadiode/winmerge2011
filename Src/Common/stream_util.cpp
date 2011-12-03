/** 
 * @file  stream_util.cpp
 *
 * @brief ISequentialStream implementations which may be of some utility.
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "stdafx.h"
#include "stream_util.h"

HRESULT STDMETHODCALLTYPE SequentialStream::QueryInterface(REFIID, void **)
{
	return E_NOTIMPL;
}

ULONG STDMETHODCALLTYPE SequentialStream::AddRef()
{
	return 1;
}

ULONG STDMETHODCALLTYPE SequentialStream::Release()
{
	return 1;
}

HRESULT STDMETHODCALLTYPE SequentialWriteStream::Read(void *, ULONG, ULONG *)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE NulWriteStream::Write(const void *pv, ULONG cb, ULONG *pcbWritten)
{
	if (cb > ULONG_MAX - m_size)
		return E_FAIL;
	m_size += cb;
	if (pcbWritten)
		*pcbWritten = cb;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE MemWriteStream::Write(const void *pv, ULONG cb, ULONG *pcbWritten)
{
	if (cb > m_size)
		return E_FAIL;
	memcpy(m_buffer, pv, cb);
	m_buffer += cb;
	m_size -= cb;
	if (pcbWritten)
		*pcbWritten = cb;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE HtmWriteStream::Write(const void *pv, ULONG cb, ULONG *pcbWritten)
{
	HRESULT hr = E_OUTOFMEMORY;
	if (SysReAllocStringLen(&V_BSTR(m_pvar), reinterpret_cast<LPCOLESTR>(pv), cb / sizeof(OLECHAR)))
	{
		hr = m_pDocument->write(m_psa);
		if (pcbWritten)
			*pcbWritten = cb;
	}
	return hr;
}

size_t StreamLineReader::readLine(stl::string &s)
{
	s.resize(0);
	do 
	{
		size_t n = s.size();
		s.resize(n + ahead);
		char *lower = &s[n];
		if (char *upper = (char *)_memccpy(lower, chunk + index, '\n', ahead))
		{
			n = upper - lower;
			index += n;
			ahead -= n;
			s.resize(upper - &s.front());
			break;
		}
		index = ahead = 0;
		pstm->Read(chunk, sizeof chunk, &ahead);
	} while (ahead != 0);
	return s.size();
}
