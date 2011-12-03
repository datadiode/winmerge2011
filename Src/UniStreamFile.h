/**
 *  @file TempFile.h
 *
 *  @brief Declaration of UniMarkdownFile class.
 */
// ID line follows -- this is updated by SVN
// $Id: UniMarkdownFile.h 6089 2008-11-16 15:27:10Z jtuc $

#include "Common/UniFile.h"

/**
 * @brief ISequentialStream based file (write-only access)
 * (ReadString methods have never been implemented,
 *  because UniMemFile is good for reading)
 */
class UniStreamFile : public UniLocalFile
{
public:
	UniStreamFile();
	~UniStreamFile();

	void Attach(ISequentialStream *pstm);

	virtual bool OpenReadOnly(LPCTSTR filename);
	virtual bool OpenCreate(LPCTSTR filename);
	virtual bool OpenCreateUtf8(LPCTSTR filename);
	virtual bool Open(LPCTSTR filename, DWORD grfMode);
	virtual void Close();
	virtual bool IsOpen() const;

	virtual bool ReadBom();
	virtual bool HasBom();
	virtual void SetBom(bool bom);

protected:
	virtual bool ReadString(String & line, bool * lossy);
	virtual bool ReadString(String & line, String & eol, bool * lossy);

// Implementation methods
protected:

public:
	virtual __int64 GetPosition() const;

	void WriteBom();
	using UniFile::WriteString; // make inherited overloads accessible
	virtual bool WriteString(LPCTSTR line, size_t length);

// Implementation data
private:
	ISequentialStream *m_pstm;
	void *m_pucrbuff;
};
