/**
 *  @file   UniFile.h
 *  @author Perry Rapp, Creator, 2003-2006
 *  @date   Created: 2003-10
 *  @date   Edited:  2006-02-20 (Perry Rapp)
 *
 *  @brief  Declaration of Unicode file classes.
 */
#pragma once

#include "unicoder.h"

/**
 * @brief Interface to file classes in this module
 */
class UniFile
{
public:

	/**
	 * @brief A struct for error message or error code.
	 */
	struct UniError
	{
		String apiname;
		int syserrnum; // valid if apiname nonempty
		String desc; // valid if apiname empty

		UniError();
		bool HasError() const;
		void ClearError();
		String GetError() const;
	};

	struct txtstats
	{
		int ncrs;
		int nlfs;
		int ncrlfs;
		int nzeros;
		int nlosses;
		void clear()
		{
			ncrs = nlfs = ncrlfs = nzeros = nlosses = 0;
		}
	private:
		friend UniFile;
		txtstats() { clear(); }
	};

	virtual ~UniFile() { }

	virtual bool OpenReadOnly(LPCTSTR filename) = 0;
	virtual bool OpenCreate(LPCTSTR filename) = 0;
	virtual void Close() = 0;
	virtual bool IsOpen() const = 0;
	virtual void ReadBom() = 0;
	virtual void WriteBom() = 0;
	virtual bool ReadString(String &line, String &eol, bool *lossy) = 0;
	virtual bool WriteString(LPCTSTR, stl_size_t) = 0;

	const UniError &GetLastUniError() const { return m_lastError; }
	const txtstats &GetTxtStats() const { return m_txtstats; }

	UNICODESET GetUnicoding() const { return m_unicoding; }
	void SetUnicoding(UNICODESET unicoding) { m_unicoding = unicoding; }
	int GetCodepage() const { return m_codepage; }
	void SetCodepage(int codepage) { m_codepage = codepage; }
	bool HasBom() const { return m_bom; }
	void SetBom(bool bom) { m_bom = bom; }

protected:
	void LastError(LPCTSTR apiname, int syserrnum);
	void LastErrorCustom(LPCTSTR desc);

protected:
	UniError m_lastError;
	txtstats m_txtstats;
	UNICODESET m_unicoding;
	int m_codepage; // only valid if m_unicoding == NONE
	bool m_bom; /**< Did the file have a BOM when reading? */
};

/**
 * @brief Local file access code used by both UniMemFile and UniStdioFile
 *
 * This class lacks an actual handle to a file
 */
class UniLocalFile : public UniFile
{
public:
	UniLocalFile();

	virtual void ReadBom() { }
	virtual bool ReadString(String & line, String & eol, bool * lossy)
	{
		ASSERT(0); // unimplemented
		return false;
	}
	virtual void WriteBom() { }
	virtual bool WriteString(LPCTSTR, stl_size_t)
	{
		ASSERT(0); // unimplemented
		return false;
	}

protected:
	void Clear();

protected:
	int m_statusFetched; // 0 not fetched, -1 error, +1 success
	CY m_filesize;
	int m_charsize; // 2 for UCS-2, else 1
};

/**
 * @brief Memory-Mapped disk file (read-only access)
 */
class UniMemFile : public UniLocalFile
{
	friend class UniMarkdownFile;
public:
	UniMemFile();
	virtual ~UniMemFile() { Close(); }

	virtual bool OpenReadOnly(LPCTSTR filename);
	virtual bool OpenCreate(LPCTSTR filename) { return false; }
	virtual void Close();
	virtual bool IsOpen() const;
	virtual void ReadBom();
	virtual bool ReadString(String &line, String &eol, bool *lossy);

// Implementation methods
protected:
	bool DoOpen(LPCTSTR filename, DWORD dwOpenAccess, DWORD dwOpenShareMode, DWORD dwOpenCreationDispostion, DWORD dwMappingProtect, DWORD dwMapViewAccess);
	bool Open(LPCTSTR filename, DWORD dwOpenAccess, DWORD dwOpenShareMode, DWORD dwOpenCreationDispostion, DWORD dwMappingProtect, DWORD dwMapViewAccess);
	bool GetFileStatus(LPCTSTR filename);

// Implementation data
private:
	HANDLE m_handle;
	HANDLE m_hMapping;
	LPBYTE m_base; // points to base of mapping
	LPBYTE m_data; // similar to m_base, but after BOM if any
	LPBYTE m_current; // current location in file
};

/**
 * @brief Regular buffered file (write-only access)
 * (ReadString methods have never been implemented,
 *  because UniMemFile above is good for reading)
 */
class UniStdioFile
	: public UniLocalFile
	, private ISequentialStream
{
public:
	UniStdioFile();
	~UniStdioFile();

	void Attach(ISequentialStream *pstm);
	HGLOBAL CreateStreamOnHGlobal();

	bool Open(LPCTSTR filename, LPCTSTR mode);
	bool OpenCreateUtf8(LPCTSTR filename);

	virtual bool OpenReadOnly(LPCTSTR filename);
	virtual bool OpenCreate(LPCTSTR filename);
	virtual void Close();
	virtual bool IsOpen() const;
	virtual void WriteBom();
	virtual bool WriteString(LPCTSTR line, String::size_type length);

	bool WriteString(const String &line)
	{
		return WriteString(line.c_str(), line.length());
	}

// Implementation methods
protected:
	bool DoOpen(LPCTSTR filename, LPCTSTR mode);

// ISequentialStream methods
private:
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();
	virtual HRESULT STDMETHODCALLTYPE Read(void *pv, ULONG cb, ULONG *pcbRead);
	virtual HRESULT STDMETHODCALLTYPE Write(const void *pv, ULONG cb, ULONG *pcbWritten);

// Implementation data
private:
	FILE *m_fp;
	ucr::buffer m_ucrbuff;
	ISequentialStream *m_pstm;
};
