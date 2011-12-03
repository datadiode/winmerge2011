/////////////////////////////////////////////////////////////////////////////
//    License (GPLv2+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or (at
//    your option) any later version.
//    
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
/////////////////////////////////////////////////////////////////////////////
/** 
 * @file  VSSHelper.cpp
 *
 * @brief Implementation file for VSSHelper class
 */
// ID line follows -- this is updated by SVN
// $Id$


#include "StdAfx.h"
#include "resource.h"
#include "VSSHelper.h"
#include "coretools.h"
#include "paths.h"
#include "Environment.h"
#include "LanguageSelect.h"

String VSSHelper::GetProjectBase()
{
	return m_strVssProjectBase;
}

BOOL VSSHelper::SetProjectBase(String strPath)
{
	if (strPath.size() < 2)
		return FALSE;

	m_strVssProjectBase = strPath;
	string_replace(m_strVssProjectBase, _T("/"), _T("\\"));

	// Check if m_strVssProjectBase has leading $\\, if not put them in:
	if (m_strVssProjectBase[0] != '$' && m_strVssProjectBase[1] != '\\')
		m_strVssProjectBase.insert(0, _T("$\\"));
	
	if (m_strVssProjectBase[m_strVssProjectBase.size() - 1] == '\\')
		m_strVssProjectBase.resize(m_strVssProjectBase.size() - 1);
	return TRUE;
}

BOOL VSSHelper::ReLinkVCProj(String strSavePath, String * psError)
{
	const UINT nBufferSize = 1024;
	TCHAR buffer[nBufferSize] = {0};
	BOOL bVCPROJ = FALSE;

	String tempPath = env_GetTempPath();
	if (tempPath.empty())
	{
		LogErrorString(_T("CMainFrame::ReLinkVCProj() - couldn't get temppath"));
		return FALSE;
	}
	String tempFile = env_GetTempFileName(tempPath.c_str(), _T("_LT"));
	if (tempFile.empty())
	{
		LogErrorString(_T("CMainFrame::ReLinkVCProj() - couldn't get tempfile"));
		return FALSE;
	}

	if (PathMatchSpec(strSavePath.c_str(), _T("*.vcproj;*.sln")))
	{
		GetFullVSSPath(strSavePath, bVCPROJ);

		SetFileAttributes(strSavePath.c_str(), FILE_ATTRIBUTE_NORMAL);
		
		HANDLE hfile = CreateFile(strSavePath.c_str(),
				GENERIC_ALL,              // open for writing
				FILE_SHARE_READ,           // share for reading 
				NULL,                      // no security 
				OPEN_EXISTING,             // existing file only 
				FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,     // normal file 
				NULL);
		HANDLE tfile = CreateFile(tempFile.c_str(),
				GENERIC_ALL,              // open for writing
				FILE_SHARE_READ,           // share for reading 
				NULL,                      // no security 
				CREATE_ALWAYS,             // existing file only 
				FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,     // normal file 
				NULL);  
		
		if (hfile == INVALID_HANDLE_VALUE || tfile == INVALID_HANDLE_VALUE)
		{
			if (hfile == INVALID_HANDLE_VALUE)
			{
				string_format msg(_T("CMainFrame::ReLinkVCProj() ")
					_T("- failed to open file: %s"), strSavePath.c_str());
				LogErrorString(msg.c_str());
				String errMsg = GetSysError(GetLastError());
				*psError = LanguageSelect.FormatMessage(
					IDS_ERROR_FILEOPEN, errMsg.c_str(), strSavePath.c_str());
			}
			if (tfile == INVALID_HANDLE_VALUE)
			{
				string_format msg(_T("CMainFrame::ReLinkVCProj() ")
					_T("- failed to open temporary file: %s"), tempFile.c_str());
				LogErrorString(msg.c_str());
				String errMsg = GetSysError(GetLastError());
				*psError = LanguageSelect.FormatMessage(
					IDS_ERROR_FILEOPEN, errMsg.c_str(), tempFile.c_str());
			}
			return FALSE;
		}

		static TCHAR charset[] = _T(" \t\n\r=");
		DWORD numwritten = 0;
		BOOL succeed = TRUE;
	
		while (succeed && GetWordFromFile(hfile, buffer, nBufferSize, charset))
		{
			if (!WriteFile(tfile, buffer, _tcslen(buffer), &numwritten, NULL))
				succeed = FALSE;
			if (bVCPROJ)
			{
				if (_tcscmp(buffer, _T("SccProjectName")) == 0)
				{
					if (!GetVCProjName(hfile, tfile))
						succeed = FALSE;
				}
			}
			else
			{//sln file
				//find sccprojectname inside this string
				if (_tcsstr(buffer, _T("SccProjectUniqueName")) == buffer)
				{
					if (!GetSLNProjUniqueName(hfile, tfile, buffer))
						succeed = FALSE;
				}
				else if (_tcsstr(buffer, _T("SccProjectName")) == buffer)
				{
					if (!GetSLNProjName(hfile, tfile, buffer))
						succeed = FALSE;
				}
			}
		}
		CloseHandle(hfile);
		CloseHandle(tfile);

		if (succeed)
		{
			if (!CopyFile(tempFile.c_str(), strSavePath.c_str(), FALSE))
			{
				*psError = GetSysError(GetLastError()).c_str();
				DeleteFile(tempFile.c_str());
				return FALSE;
			}
			else
				DeleteFile(tempFile.c_str());
		}
		else
		{
			String errMsg = GetSysError(GetLastError());
			*psError = LanguageSelect.FormatMessage(
				IDS_ERROR_FILEOPEN, strSavePath.c_str(), errMsg.c_str());
			return FALSE;
		}
	}
	return TRUE;
}

void VSSHelper::GetFullVSSPath(String strSavePath, BOOL & bVCProj)
{
	if (PathMatchSpec(strSavePath.c_str(), _T("*.vcproj")))
		bVCProj = TRUE;

	string_replace(strSavePath, _T("/"), _T("\\"));
	string_replace(m_strVssProjectBase, _T("/"), _T("\\"));

	//check if m_strVssProjectBase has leading $\\, if not put them in:
	if (m_strVssProjectBase[0] != '$' && m_strVssProjectBase[1] != '\\')
		m_strVssProjectBase.insert(0, _T("$\\"));

	string_makelower(strSavePath);
	string_makelower(m_strVssProjectBase);

	//take out last '\\'
	int nLen = m_strVssProjectBase.size();
	if (m_strVssProjectBase[nLen - 1] == '\\')
		m_strVssProjectBase.resize(nLen - 1);

	String strSearch = m_strVssProjectBase.c_str() + 2; // Don't compare first 2
	int index = strSavePath.find(strSearch); //Search for project base path
	if (index > -1)
	{
		index++;
		m_strVssProjectFull = strSavePath.c_str() + index + strSearch.length();
		if (m_strVssProjectFull[0] == ':')
			m_strVssProjectFull.erase(0, 2);
	}

	String path;
	SplitFilename(m_strVssProjectFull.c_str(), &path, NULL, NULL);
	if (m_strVssProjectBase[m_strVssProjectBase.size() - 1] != '\\')
		m_strVssProjectBase += _T("\\");

	m_strVssProjectFull += m_strVssProjectBase + path;

	//if sln file, we need to replace ' '  with _T("\\u0020")
	if (!bVCProj)
		string_replace(m_strVssProjectFull, _T(" "), _T("\\u0020"));
}

/**
 * @brief Reads words from a file deliminated by charset
 *
 * Reads words from a file deliminated by charset with one slight twist.
 * If the next char in the file to be read is one of the characters inside
 * the delimiter, then the word returned will be a word consisting only
 * of delimiters.
 * @param [in] pfile Opened handle to file
 * @param [in] buffer pointer to buffer read data is put
 * @param [in] dwBufferSize size of buffer
 * @param [in] charset pointer to string containing delimiter chars
 */
BOOL VSSHelper::GetWordFromFile(HANDLE pfile, TCHAR * buffer,
		DWORD dwBufferSize, TCHAR * charset)
{
	TCHAR buf[1024];
	const DWORD bytesToRead = sizeof(buf);
	DWORD bytesRead = 0;
	if (!ReadFile(pfile, (LPVOID)buf, bytesToRead, &bytesRead, NULL))
		return FALSE;
	int charsRead = GetWordFromBuffer(buf, bytesRead, buffer,
		dwBufferSize, charset);
	if (charsRead > 0)
		SetFilePointer(pfile, -1, NULL, FILE_CURRENT);
	return TRUE;
}

int VSSHelper::GetWordFromBuffer(TCHAR *inBuffer, DWORD dwInBufferSize,
		TCHAR * outBuffer, DWORD dwOutBufferSize, TCHAR * charset)
{
	TCHAR ctemp = '\0';
	TCHAR * pcharset = NULL;
	UINT buffercount = 0;
	DWORD numread = sizeof(ctemp);
	BOOL delimword = FALSE;
	BOOL firstRead = FALSE; // First char read ?
	BOOL delimMatch = FALSE;

	ASSERT(inBuffer != NULL && outBuffer != NULL);

	while (buffercount < dwInBufferSize && buffercount < dwOutBufferSize)
	{
		ctemp = *inBuffer;
		if (!firstRead && charset)
		{
			for (pcharset = charset; *pcharset; pcharset++)
			{
				if (ctemp == *pcharset)
					break;
			}
			if (*pcharset != NULL) // Means that cbuffer[0] is a delimiter character
				delimword = TRUE;
			firstRead = TRUE;
		}

		if (!charset)
		{
			if (ctemp != ' ' && ctemp != '\n' && ctemp != '\t' && ctemp != '\r')
			{
				*outBuffer = ctemp;
				buffercount++;
			}
			else
				break;
		}
		else if (delimword == FALSE)
		{
			for (pcharset = charset; *pcharset; pcharset++)
			{
				//if next char is equal to a delimiter or we want delimwords stop the adding
				if (ctemp == *pcharset)
					break;
			}
			if (*pcharset == NULL)
			{
				*outBuffer = ctemp;
				buffercount++;
			}
			else
				break;
		}
		else if (delimword == TRUE)
		{
			delimMatch = FALSE;
			for (pcharset = charset; *pcharset; pcharset++)
			{						
				//if next char is equal to a delimiter or we want delimwords stop the adding
				if (ctemp == *pcharset)
				{
					delimMatch = TRUE;
					break;
				}
			}
			if (delimMatch == TRUE)
			{
				*outBuffer = ctemp;
				buffercount++;
			}
			else
				break;
		}
	
		inBuffer += sizeof(TCHAR);
	}
	if (buffercount >= dwOutBufferSize || numread == 0)
		return 0;
	return buffercount;
}

BOOL VSSHelper::GetVCProjName(HANDLE hFile, HANDLE tFile)
{
	TCHAR buffer[1024] = {0};
	DWORD dwNumWritten = 0;
	
	ASSERT(hFile != NULL && hFile != INVALID_HANDLE_VALUE &&
		tFile != NULL && tFile != INVALID_HANDLE_VALUE);

	//nab the equals sign
	if (!GetWordFromFile(hFile, buffer, _countof(buffer), _T("=")))
		return FALSE;
	if (!WriteFile(tFile, buffer, _tcslen(buffer),
			&dwNumWritten, NULL))
		return FALSE;

	String stemp = _T("\"&quot;") + m_strVssProjectFull + 
		_T("&quot;");
	if (!WriteFile(tFile, stemp.c_str(), stemp.size(),
			&dwNumWritten, NULL))
		return FALSE;

	if (!GetWordFromFile(hFile, buffer, _countof(buffer), _T(",\n"))) //for junking
		return FALSE;
	if (!GetWordFromFile(hFile, buffer, _countof(buffer), _T(",\n"))) //get the next delimiter
		return FALSE;

	if (!_tcscmp(buffer, _T("\n")))
	{
		if (!WriteFile(tFile, _T("\""), 1, &dwNumWritten, NULL))
			return FALSE;
	}
	if (!WriteFile(tFile, buffer, _tcslen(buffer), &dwNumWritten, NULL))
		return FALSE;

	return TRUE;
}

BOOL VSSHelper::GetSLNProjUniqueName(HANDLE hFile, HANDLE tFile, TCHAR * buf)
{
	TCHAR buffer[1024] = {0};
	DWORD dwNumWritten = 0;

	ASSERT(hFile != NULL && hFile != INVALID_HANDLE_VALUE &&
		tFile != NULL && tFile != INVALID_HANDLE_VALUE);
	ASSERT(buf != NULL);

	//nab until next no space, and no =
	if (!GetWordFromFile(hFile, buffer, _countof(buffer), _T(" =")))
		return FALSE;
	if (!WriteFile(tFile, buffer, _tcslen(buffer), &dwNumWritten, NULL))
		return FALSE;
	//nab word
	if (!GetWordFromFile(hFile, buffer, _countof(buffer), _T("\\\n.")))
		return FALSE;
	while (!_tcsstr(buffer, _T(".")))
	{						
		if (buffer[0] != '\\')
			_tcsncat(buf, buffer, _tcslen(buffer));

		if (!WriteFile(tFile, buffer, _tcslen(buffer), &dwNumWritten, NULL))
			return FALSE;
		if (!GetWordFromFile(hFile, buffer, _countof(buffer), _T("\\\n.")))
			return FALSE;
	}
	if (!WriteFile(tFile, buffer, _tcslen(buffer), &dwNumWritten, NULL))
		return FALSE;

	return TRUE;
}

BOOL VSSHelper::GetSLNProjName(HANDLE hFile, HANDLE tFile, TCHAR * buf)
{
	TCHAR buffer[1024] = {0};
	DWORD dwNumWritten = 0;

	ASSERT(hFile != NULL && hFile != INVALID_HANDLE_VALUE &&
		tFile != NULL && tFile != INVALID_HANDLE_VALUE);
	ASSERT(buf != NULL);
	
	String capp;
	if (*buf != '\\' && !_tcsstr(buf, _T(".")))
	{
		//write out \\u0020s for every space in buffer2
		for (TCHAR * pc = buf; *pc; pc++)
		{
			if (*pc == ' ') //insert \\u0020
				capp += _T("\\u0020");
			else
				capp += *pc;
		}

		string_makelower(capp);

		//nab until the no space, and no =
		if (!GetWordFromFile(hFile, buffer, _countof(buffer), _T(" =")))
			return FALSE;
		if (!WriteFile(tFile, buffer, _tcslen(buffer), &dwNumWritten, NULL))
			return FALSE;
		String stemp = _T("\\u0022") + m_strVssProjectFull + capp + _T("\\u0022");
		if (!WriteFile(tFile, stemp.c_str(), stemp.size(),
				&dwNumWritten, NULL))
			return FALSE;
		
		//nab until the first backslash
		if (!GetWordFromFile(hFile, buffer, _countof(buffer), _T(",")))
			return FALSE;
	}
	return TRUE;
}
