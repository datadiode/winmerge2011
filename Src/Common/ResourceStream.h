// http://code.google.com/p/mahjong3d/source/browse/trunk/Mahjongg/service/
// Last change: 2013-03-09 by Jochen Neubeck
#pragma once

class ResourceStream : public IStream
{

private:
	ResourceStream(LPBYTE pData, DWORD dwSize);

	~ResourceStream();

public:
	static HRESULT Create(HMODULE hModule, LPCWSTR lpName, LPCWSTR lpType, IStream ** ppStream);

public:
	// IUnknown interface
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObject);

	virtual ULONG STDMETHODCALLTYPE AddRef(void);

	virtual ULONG STDMETHODCALLTYPE Release(void);

public:
	// ISequentialStream Interface
	virtual HRESULT STDMETHODCALLTYPE Read(void* pv, ULONG cb, ULONG* pcbRead);

	virtual HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG* pcbWritten);

public:
	// IStream Interface
	virtual HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER);

	virtual HRESULT STDMETHODCALLTYPE CopyTo(IStream*, ULARGE_INTEGER, ULARGE_INTEGER*, ULARGE_INTEGER*);

	virtual HRESULT STDMETHODCALLTYPE Commit(DWORD);

	virtual HRESULT STDMETHODCALLTYPE Revert(void);

	virtual HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD);

	virtual HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD);

	virtual HRESULT STDMETHODCALLTYPE Clone(IStream **);

	virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER liDistanceToMove, DWORD dwOrigin, ULARGE_INTEGER* lpNewFilePointer);

	virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG* pStatstg, DWORD grfStatFlag);

private:
	LONG _refcount;
	LPBYTE m_pData;
	DWORD m_dwSize;
	DWORD m_dwPos;
};
