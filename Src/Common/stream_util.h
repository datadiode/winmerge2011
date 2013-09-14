/** 
 * @file  stream_util.h
 *
 * @brief ISequentialStream implementations which may be of some utility.
 */
// ID line follows -- this is updated by SVN
// $Id$

#ifndef _STREAM_UTIL_H_
#define _STREAM_UTIL_H_

/** 
 * @brief A sequential stream.
 */
class SequentialStream : public ISequentialStream
{
public:
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();
};

/**
 * @brief A sequential read stream.
 */
class SequentialReadStream : public SequentialStream
{
public:
	virtual HRESULT STDMETHODCALLTYPE Write(const void *pv, ULONG cb, ULONG *pcbWritten);
};

/**
 * @brief A stream that reads from an OS handle.
 */
class HandleReadStream : public SequentialReadStream
{
	HANDLE const handle;
public:
	HandleReadStream(HANDLE handle)
		: handle(handle)
	{
	}
	virtual HRESULT STDMETHODCALLTYPE Read(void *pv, ULONG cb, ULONG *pcbRead);
};

/**
 * @brief A sequential write stream.
 */
class SequentialWriteStream : public SequentialStream
{
public:
	virtual HRESULT STDMETHODCALLTYPE Read(void *pv, ULONG cb, ULONG *pcbRead);
};

/**
 * @brief A stream that simulates write and sums up the sizes of the passed-in data.
 */
class NulWriteStream : public SequentialWriteStream
{
public:
	NulWriteStream()
		: m_size(0)
	{
	}
	ULONG GetSize()
	{
		return m_size;
	}
	virtual HRESULT STDMETHODCALLTYPE Write(const void *pv, ULONG cb, ULONG *pcbWritten);
private:
	ULONG m_size;
};

/**
 * @brief A stream that writes the passed-in data to a memory buffer.
 */
class MemWriteStream : public SequentialWriteStream
{
public:
	MemWriteStream(void *buffer, ULONG size)
		: m_buffer(reinterpret_cast<BYTE *>(buffer))
		, m_size(size)
	{
	}
	ULONG GetRemainingCapacity()
	{
		return m_size;
	}
	virtual HRESULT STDMETHODCALLTYPE Write(const void *pv, ULONG cb, ULONG *pcbWritten);
private:
	BYTE *m_buffer;
	ULONG m_size;
};

/**
 * @brief A stream that writes the passed-in data to an IHTMLDocument2.
 */
class HtmWriteStream : public SequentialWriteStream
{
public:
	HtmWriteStream(IHTMLDocument2 *pDocument)
		: m_pDocument(pDocument)
		, m_psa(SafeArrayCreateVector(VT_VARIANT, 0, 1))
	{
		SafeArrayAccessData(m_psa, (void **)&m_pvar);
		V_VT(m_pvar) = VT_BSTR;
		V_BSTR(m_pvar) = NULL;
	}
	~HtmWriteStream()
	{
		SafeArrayUnaccessData(m_psa);
		SafeArrayDestroy(m_psa);
	}
	virtual HRESULT STDMETHODCALLTYPE Write(const void *pv, ULONG cb, ULONG *pcbWritten);
private:
	IHTMLDocument2 *const m_pDocument;
	SAFEARRAY *const m_psa;
	VARIANT *m_pvar;
};

/**
 * @brief A reader that reads textual data line by line from a stream.
 */
class StreamLineReader
{
	ISequentialStream *const pstm;
	ULONG index;
	ULONG ahead;
	char chunk[256];
public:
	StreamLineReader(ISequentialStream *pstm)
		: pstm(pstm), index(0), ahead(0)
	{
	}
	stl::string::size_type readLine(stl::string &);
};

#endif // _STREAM_UTIL_H_
