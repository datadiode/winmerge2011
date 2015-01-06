/** 
 * @file  LogFile.h
 *
 * @brief Implementation file for CLogFile
 *
 */
#include "StdAfx.h"
#include "LogFile.h"
#include "paths.h"
#include "Environment.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/** @brief Global name for mutes protecting log file access. */
static const TCHAR MutexName[] = _T("WINMERGE_LOG_MUTEX");

/** @brief Constant for Megabyte. */
static const int MEGA = 1024 * 1024;

CLogFile LogFile;

/**
 * @brief Constructor
 * @param [in] szLogName Logfile name (including path).
 * @param [in] bDeleteExisting Do we delete existing log file with same name?
 */
CLogFile::CLogFile()
	: m_nMaxSize(MEGA)
	, m_bEnabled(FALSE)
	, m_nDefaultLevel(LMSG)
	, m_nMaskLevel(LALL)
{
	m_hLogMutex = CreateMutex(NULL, FALSE, MutexName);
}

/**
 * @brief Destructor.
 */
CLogFile::~CLogFile()
{
	EnableLogging(FALSE);
	CloseHandle(m_hLogMutex);
}

/**
 * @brief Set logfilename.
 * @param [in] strFile Filename and path of logfile.
 * @param bDelExisting If TRUE, existing logfile with same name
 * is deleted.
 * @note If strPath param is empty then system TEMP folder is used.
 */
void CLogFile::SetFile(LPCTSTR strFile, BOOL bDelExisting)
{
	m_strLogPath = strFile;
	if (bDelExisting)
		DeleteFile(m_strLogPath.c_str());
}

/**
 * @brief Enable/Disable writing log.
 * @param [in] bEnable If TRUE logging is enabled.
 */
void CLogFile::EnableLogging(BOOL bEnable)
{
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);

	TCHAR s[80];
	_tcsftime(s, _countof(s), _T(" %A, %B %d, %Y    %H:%M:%S"), tm);

	if (bEnable)
	{
		m_bEnabled = TRUE;
		Write(0, _T("\n*******\nLog Started: %s"), s);
		Write(0, _T("Path: %s\n*******\n"), m_strLogPath.c_str());
	}
	else
	{
		m_bEnabled = FALSE;
		Write(0, _T("\n*******\nLog Stopped: %s\n"), s);
		Write(0, _T("*******\n"));
	}
}

/**
 * @brief Return default level for log messages
 * @return Default log level.
 */
UINT CLogFile::GetDefaultLevel() const
{
	return m_nDefaultLevel;
}

/**
 * @brief Set default level for log messages
 * @param [in] logLevel New default message loglevel.
 */
void CLogFile::SetDefaultLevel(UINT logLevel)
{
	m_nDefaultLevel = logLevel;
}

/**
 * @brief Get log message mask.
 *
 * Mask allows to select which level messages are written to log.
 * @return Current mask Level.
 */
UINT CLogFile::GetMaskLevel() const
{
	return m_nMaskLevel;
}

/**
 * @brief Set log message mask.
 * @param [in] maskLevel New masking level for outputted messages.
 */
void CLogFile::SetMaskLevel(UINT maskLevel)
{
	m_nMaskLevel = maskLevel;
}

/**
 * @brief Internal function for writing the messages.
 * @param [in] level Message's level.
 * @param [in] pszFormat Message's format specifier.
 * @param [in] arglist Message argumets to format string.
 */
UINT CLogFile::Write(UINT level, LPCTSTR text)
{
	if (m_bEnabled)
	{
		if (level == 0)
			level = m_nDefaultLevel;
		if (level & m_nMaskLevel)
		{
			String msg = GetPrefix(level);
			msg += text;
			if (level & LOSERROR)
				msg += GetSysError(GetLastError());
			msg.erase(msg.find_last_not_of(_T("\r\n")) + 1);
			msg += _T("\n");
			if (m_nMaskLevel & LFILE)
				WriteRaw(msg.c_str());
			if (m_nMaskLevel & LDEBUG)
				OutputDebugString(msg.c_str());
		}
	}
	return m_nMaskLevel & LSILENTVERIFY;
}

/**
 * @brief Internal function to write new line to log-file.
 * @param [in] msg Message to add to log-file.
 */
void CLogFile::WriteRaw(LPCTSTR msg)
{
	DWORD dwWaitRes = WaitForSingleObject(m_hLogMutex, 10000);
	if (dwWaitRes == WAIT_OBJECT_0)
	{
		if (FILE *f = _tfopen(m_strLogPath.c_str(), _T("a")))
		{
			_fputts(msg, f);
			// prune the log if it gets too big
			if (ftell(f) >= static_cast<long>(m_nMaxSize))
				Prune(f);
			else
				fclose(f);
		}
		ReleaseMutex(m_hLogMutex);
	}
}

/**
 * @brief Prune log file if it exceeds max given size.
 * @param [in] f Pointer to FILE structure.
 * @todo This is not safe function at all. We should check return values!
 */
void CLogFile::Prune(FILE *f)
{
	String tempfile = env_GetTempFileName(_T("."), _T("LOG"));
	DeleteFile(tempfile.c_str());
	if (FILE *tf = _tfopen(tempfile.c_str(), _T("w")))
	{
		char buf[8196];
		fseek(f, ftell(f) / 4, SEEK_SET);
		_fputts(_T("#### The log has been truncated due to size limits ####\n"), tf);
		while (size_t amt = fread(buf, 1, 8196, f))
			fwrite(buf, amt, 1, tf);
		fclose(tf);
		fclose(f);
		DeleteFile(m_strLogPath.c_str());
		MoveFile(tempfile.c_str(), m_strLogPath.c_str());
	}
}

/**
 * @brief Return message prefix string for given loglevel.
 * @param [in] level Level to query prefix string.
 * @return Pointer to string containing prefix.
 */
LPCTSTR CLogFile::GetPrefix(UINT level) const
{
	switch (level & 0x0FFF)
	{
	case LERROR:		return _T("ERROR: ");
	case LWARNING:		return _T("WARNING: ");
	case LNOTICE:		return _T("NOTICE: ");
	case LCODEFLOW:		return _T("FLOW: ");
	case LCOMPAREDATA:	return _T("COMPARE: ");
	}
	return _T("");
}
