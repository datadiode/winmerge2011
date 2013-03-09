// http://code.google.com/p/mahjong3d/source/browse/trunk/Mahjongg/service/
// Last change: 2013-03-09 by Jochen Neubeck
#include "stdafx.h"
#include "ResourceStream.h"

ResourceStream::ResourceStream(LPBYTE pData, DWORD dwSize)
{
	_refcount = 1;
	m_pData = pData;
	m_dwSize = dwSize;
	m_dwPos = 0;
}

ResourceStream::~ResourceStream()
{
}

/**
 * @brief    QueryInterface
 *
 * ResourceStream::QueryInterface
 *
 * @param REFIID iid
 * @param void * * ppvObject
 * @return HRESULT STDMETHODCALLTYPE
 */
HRESULT STDMETHODCALLTYPE ResourceStream::QueryInterface( REFIID iid, void ** ppvObject )
{
	if (iid == __uuidof(IUnknown)
			|| iid == __uuidof(IStream)
			|| iid == __uuidof(ISequentialStream))
	{
		*ppvObject = static_cast<IStream*>(this);
		AddRef();
		return S_OK;
	}
	else
		return E_NOINTERFACE;
}

/**
 * @brief    AddRef
 *
 * ResourceStream::AddRef
 *
 * @param void
 * @return ULONG STDMETHODCALLTYPE
 */
ULONG STDMETHODCALLTYPE ResourceStream::AddRef( void )
{
	return (ULONG)InterlockedIncrement(&_refcount);
}

/**
 * @brief    Release
 *
 * ResourceStream::Release
 *
 * @param void
 * @return ULONG STDMETHODCALLTYPE
 */
ULONG STDMETHODCALLTYPE ResourceStream::Release( void )
{
	ULONG res = (ULONG) InterlockedDecrement(&_refcount);

	if (res == 0)
		delete this;

	return res;
}

/**
 * @brief    Read
 *
 * ResourceStream::Read
 *
 * @param void * pv
 * @param ULONG cb
 * @param ULONG * pcbRead
 * @return HRESULT STDMETHODCALLTYPE
 */
HRESULT STDMETHODCALLTYPE ResourceStream::Read( void* pv, ULONG cb, ULONG* pcbRead )
{
	DWORD dwCopy = min(m_dwSize - m_dwPos, cb);

	// copy data
	memcpy(pv, m_pData + m_dwPos, dwCopy);

	// move pointer
	m_dwPos += dwCopy;

	if (pcbRead != NULL)
		*pcbRead = dwCopy;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE ResourceStream::Write( void const* pv, ULONG cb, ULONG* pcbWritten )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE ResourceStream::SetSize( ULARGE_INTEGER )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE ResourceStream::CopyTo( IStream*, ULARGE_INTEGER, ULARGE_INTEGER*, ULARGE_INTEGER* )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE ResourceStream::Commit( DWORD )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE ResourceStream::Revert( void )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE ResourceStream::LockRegion( ULARGE_INTEGER, ULARGE_INTEGER, DWORD )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE ResourceStream::UnlockRegion( ULARGE_INTEGER, ULARGE_INTEGER, DWORD )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE ResourceStream::Clone( IStream ** )
{
	return E_NOTIMPL;
}

/**
 * @brief    Seek
 *
 * ResourceStream::Seek
 *
 * @param LARGE_INTEGER liDistanceToMove
 * @param DWORD dwOrigin
 * @param ULARGE_INTEGER * lpNewFilePointer
 * @return HRESULT STDMETHODCALLTYPE
 */
HRESULT STDMETHODCALLTYPE ResourceStream::Seek( LARGE_INTEGER liDistanceToMove, DWORD dwOrigin, ULARGE_INTEGER* lpNewFilePointer )
{
	switch (dwOrigin)
	{

	case STREAM_SEEK_SET:
		m_dwPos = liDistanceToMove.LowPart;
		break;

	case STREAM_SEEK_CUR:
		m_dwPos += liDistanceToMove.LowPart;
		break;

	case STREAM_SEEK_END:
		m_dwPos = m_dwSize - liDistanceToMove.LowPart;
		break;

	default:
		return STG_E_INVALIDFUNCTION;
		break;
	}

	if (lpNewFilePointer != NULL)
	{
		lpNewFilePointer->LowPart = m_dwPos;
		lpNewFilePointer->HighPart = 0;
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE ResourceStream::Stat( STATSTG* pStatstg, DWORD grfStatFlag )
{
	pStatstg->cbSize.LowPart = m_dwSize;
	pStatstg->cbSize.HighPart = 0;
	return S_OK;
}

/**
 * @brief    Create
 *
 * ResourceStream::Create
 *
 * @param HMODULE hModule
 * @param LPCWSTR lpName
 * @param LPCWSTR lpType
 * @param IStream * * ppStream
 * @return HRESULT
 */
HRESULT ResourceStream::Create(HMODULE hModule, LPCWSTR lpName, LPCWSTR lpType, IStream ** ppStream)
{
	HRSRC hRsrc = ::FindResource(hModule, lpName, lpType);

	if (hRsrc == NULL)
		return E_FAIL;

	DWORD dwSize = ::SizeofResource(hModule, hRsrc);

	HGLOBAL hGlob = ::LoadResource(hModule, hRsrc);

	if (hGlob == NULL)
		return E_FAIL;

	LPVOID pData = ::LockResource(hGlob);

	if (pData == NULL)
		return E_FAIL;

	*ppStream = new ResourceStream((LPBYTE)pData, dwSize);

	return S_OK;
}
