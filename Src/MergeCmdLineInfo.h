/////////////////////////////////////////////////////////////////////////////
//
//    WinMerge: An interactive diff/merge utility
//    Copyright (C) 1997 Dean P. Grimm
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
/////////////////////////////////////////////////////////////////////////////
/** 
 * @file  MergeCmdLineInfo.h
 *
 * @brief MergeCmdLineInfo class declaration.
 *
 */
#pragma once

class IOptionDef;

/** 
 * @brief WinMerge's command line handler.
 * This class calls command line parser(s) and allows reading parsed values
 * from public member variables.
 */
class MergeCmdLineInfo
{
public:
	MergeCmdLineInfo(LPCTSTR);

public:

	enum ExitNoDiff
	{
		Disabled, /**< Don't exit. */
		Exit, /**< Exit and show message. */
		ExitQuiet, /**< Exit and don't show message. */
	} m_bExitIfNoDiff; /**< Exit if files are identical. */

	enum InvocationMode
	{
		InvocationModeNone,
		InvocationModeEscShutdown, /**< If commandline switch -e given ESC closes application */
		InvocationModeCompareTool, /**< WinMerge is executed as an external Rational ClearCase compare tool. */
		InvocationModeMergeTool /**< WinMerge is executed as an external Rational ClearCase merge tool. */
	} m_invocationMode;

	int m_nSingleInstance; /**< Allow only one instance of WinMerge executable. */
	int m_nCmdShow; /**< Initial state of the application's window. */
	int m_nRecursive; /**< Include sub folder in directories compare. */
	bool m_bNonInteractive; /**< Suppress user's notifications. */
	bool m_bShowUsage; /**< Show a brief reminder to command line arguments. */
	int m_nCodepage;  /**< Codepage. */

	DWORD m_dwLeftFlags; /**< Left side file's behavior options. */
	DWORD m_dwRightFlags; /**< Right side file's behavior options. */

	String m_sLeftDesc; /**< Left side file's description. */
	String m_sRightDesc; /**< Right side file's description. */

	String m_sFileFilter; /**< File filter mask. */
	String m_sContentType; /**< Content type. */
	String m_sRunScript; /**< Run this script in the context of the document. */
	String m_sOptionChars; /**< Set of accepted option indicators. */
	String m_sConfigFileName; /**< Where to persist application settings when not using the registry. */

	std::vector<String> m_Files; /**< Files (or directories) to compare. */

private:

	LPCTSTR EatParam(LPCTSTR, String &, bool *flag = 0) const;
	LPCTSTR SetOption(LPCTSTR, IOptionDef &, LPCTSTR value = _T("1")) const;
	void ParseClearCaseCmdLine(LPCTSTR, LPCTSTR basedesc);
	void ParseWinMergeCmdLine(LPCTSTR);
	void AddPath(const String &path);

	/** Operator= is not implemented. */
	MergeCmdLineInfo& operator=(const MergeCmdLineInfo&);
};
