/** 
 * @file  LogFile.h
 *
 * @brief Declaration file for CLogFile
 *
 */
#pragma once

/**
 * @brief Class for writing log files.
 *
 * Allows setting masks and levels for messages. They are used for
 * filtering messages written to log. For example usually its not
 * needed to see all informal messages, but errors are always good
 * to log. For simpler usage, default is that all messages are written
 * and functions with take just message in are provided.
 *
 * Outputting messages can be limited by giving a max level for outputted
 * messages. Only lower level messages are then outputted. For example if
 * level is set to LERROR only error messages are outputted.
 *
 * Written messages are given using printf() -style messageformat specifiers.
 * See MSDN documentation about printf() function for more information.
 *
 * @note User can easily define more levels, just add new constant to
 * struct LOGLEVEL above, and possibly prefix to GetPrefix(UINT level).
 */
extern class CLogFile  
{
public:
	/** @brief Messagelevels for log writing. */
	enum 
	{
		LALL = -1, /**< All messages written */
		LERROR = 0x1, /**< Error messages */
		LWARNING = 0x2, /**< Warning messages */ 
		LNOTICE = 0x4, /**< Important messages */
		LMSG = 0x8, /**< Normal messages */
		LCODEFLOW = 0x10, /**< Code flow messages */
		LCOMPAREDATA = 0x20, /**< Compare data (dump) */

		/* These flags are not loglevels, but have special meaning */
		LFILE = 0x8000, /**< Append message to file */
		LDEBUG = 0x4000, /**< Append message to debug window */
		LSILENTVERIFY = 0x2000, /**< No VERIFY popup, please */
		LOSERROR = 0x1000, /**< Append description of last error */
	};

	CLogFile();
	~CLogFile();

	void SetFile(LPCTSTR strFile, BOOL bDelExisting = FALSE);
	void EnableLogging(BOOL bEnable);
	UINT GetDefaultLevel() const;
	void SetDefaultLevel(UINT logLevel);
	UINT GetMaskLevel() const;
	void SetMaskLevel(UINT maskLevel);

	UINT Write(UINT, LPCTSTR);

	template<class any>
	UINT Write(UINT level, any fmt, ...)
	{
		if (m_bEnabled)
		{
			if (level == 0)
				level = m_nDefaultLevel;
			if (level & m_nMaskLevel)
			{
				va_list args;
				va_start(args, fmt);
				String s;
				s.append_sprintf_va_list(fmt, args);
				Write(level, s.c_str());
				va_end(args);
			}
		}
		return m_nMaskLevel & LSILENTVERIFY;
	}

	void SetMaxLogSize(DWORD dwMax) { m_nMaxSize = dwMax; }
	String GetPath() const { return m_strLogPath; }

protected:
	void Prune(FILE *f);
	LPCTSTR GetPrefix(UINT level) const;
	void WriteRaw(LPCTSTR msg);

private:
	HANDLE    m_hLogMutex; /**< Mutex protecting log writing */
	DWORD     m_nMaxSize; /**< Max size of the log file */
	BOOL      m_bEnabled; /**< Is logging enabled? */
	String    m_strLogPath; /**< Full path to log file */
	UINT      m_nDefaultLevel; /**< Default level for log messages */
	UINT      m_nMaskLevel; /**< Level to mask messages written to log */
} LogFile;
