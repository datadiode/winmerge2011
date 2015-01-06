/**
 *  @file version.h
 *
 *  @brief Declaration of CVersionInfo class
 */ 
#pragma once

/**
 * @brief Class providing access to version information of a file.
 * This class reads version information block from a file. Version information
 * consists of version numbers, copyright, descriptions etc. Since that is
 * many strings to read, there is constructor taking BOOL parameter and
 * only reading version numbers. That constructor is suggested to be used
 * if string information is not needed.
 */
class CVersionInfo : public VS_FIXEDFILEINFO
{
private:
	BYTE *m_pVffInfo; /**< Pointer to version information block */
	WORD m_wLanguage; /**< Language-ID to use (if given) */
	WORD m_wCodepage;
public:
	CVersionInfo(LPCTSTR szFileName);
	CVersionInfo(HINSTANCE hModule = NULL);
	~CVersionInfo();
	String GetFileVersion() const;
	String GetCompanyName() const;
	String GetFileDescription() const;
	String GetInternalName() const;
	String GetLegalCopyright() const;
	String GetOriginalFilename() const;
	String GetProductVersion() const;
	String GetComments() const;
	String GetSpecialBuild() const;
	String GetPrivateBuild() const;
	String GetFixedProductVersion();
	String GetFixedFileVersion();
	BOOL GetFixedFileVersion(DWORD &versionMS, DWORD &versionLS);

protected:
	void GetVersionInfo(LPCTSTR szFileName);
	String QueryValue(LPCTSTR szId) const;
	String MakeVersionString(DWORD hi, DWORD lo) const;
	WORD GetCodepageForLanguage(WORD wLanguage);
};
