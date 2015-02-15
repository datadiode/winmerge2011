/////////////////////////////////////////////////////////////////////////////
//    License (GPLv2+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
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
 * @file  ProjectFile.cpp
 *
 * @brief Implementation file for ProjectFile class.
 */
#include "stdafx.h"
#include "markdown.h"
#include "UniFile.h"
#include "ProjectFile.h"
#include "LanguageSelect.h"
#include "resource.h"

// Constants for xml element names
const char Root_element_name[] = "project";
const char Paths_element_name[] = "paths";
const char Left_element_name[] = "left";
const char Right_element_name[] = "right";
const char Filter_element_name[] = "filter";
const char Subfolders_element_name[] = "subfolders";
const char Left_ro_element_name[] = "left-readonly";
const char Right_ro_element_name[] = "right-readonly";

/** 
 * @brief Standard constructor.
 */
 ProjectFile::ProjectFile()
: m_nRecursive(0)
, m_bLeftPathReadOnly(FALSE)
, m_bRightPathReadOnly(FALSE)
{
}

/** 
 * @brief Open given path-file and read data from it to member variables.
 * @param [in] path Path to project file.
 * @param [out] sError Error string if error happened.
 * @return TRUE if reading succeeded, FALSE if error happened.
 */
bool ProjectFile::Read(LPCTSTR path)
{
	bool loaded = false;
	CMarkdown::File file = path;
	CMarkdown::EntityMap em;
	CMarkdown::Load(em);

	if (HString *hstr = file.Move().GetTagName())
	{
		OString ostr = hstr;
		if (strcmp(ostr.A, "?WinMergeScript") == 0)
		{
			if (HString *hstr = file.GetAttribute("options"))
			{
				OString ostr = hstr;
				bool lossy;
				ucr::maketstring(m_sOptions, ostr.A, ostr.ByteLen(), CP_UTF8, &lossy);
			}
			return false;
		}
	}

	if (CMarkdown &project = CMarkdown(file).Move(Root_element_name).Pop())
	{
		if (CMarkdown &paths = CMarkdown(project).Move(Paths_element_name).Pop())
		{
			if (CMarkdown &node = CMarkdown(paths).Move(Left_element_name))
			{
				m_sLeftFile = OString(node.GetInnerText()->Uni(em)).W;
			}
			if (CMarkdown &node = CMarkdown(paths).Move(Right_element_name))
			{
				m_sRightFile = OString(node.GetInnerText()->Uni(em)).W;
			}
			if (CMarkdown &node = CMarkdown(paths).Move(Filter_element_name))
			{
				m_sFilter = OString(node.GetInnerText()->Uni(em)).W;
			}
			if (CMarkdown &node = CMarkdown(paths).Move(Subfolders_element_name))
			{
				m_nRecursive = atoi(OString(node.GetInnerText()).A);
			}
			if (CMarkdown &node = CMarkdown(paths).Move(Left_ro_element_name))
			{
				m_bLeftPathReadOnly = atoi(OString(node.GetInnerText()).A) != 0;
			}
			if (CMarkdown &node = CMarkdown(paths).Move(Right_ro_element_name))
			{
				m_bRightPathReadOnly = atoi(OString(node.GetInnerText()).A) != 0;
			}
			string_format sConfig(_T("%s.json"), path);
			if (PathFileExists(sConfig.c_str()))
				m_sConfig.swap(sConfig);
			loaded = true;
		}
		else
		{
			String sError = LanguageSelect.LoadString(IDS_UNK_ERROR_READING_PROJECT);
			LanguageSelect.FormatMessage(
				IDS_ERROR_FILEOPEN, path, sError.c_str()
			).MsgBox(MB_ICONSTOP);
		}
	}
	else
	{
		String sError = LanguageSelect.LoadString(IDS_UNK_ERROR_READING_PROJECT);
		LanguageSelect.FormatMessage(
			IDS_ERROR_FILEOPEN, path, sError.c_str()
		).MsgBox(MB_ICONSTOP);
	}
	return loaded;
}

/** 
 * @brief Save data from member variables to path-file.
 * @param [in] path Path to project file.
 * @param [out] sError Error string if error happened.
 * @return TRUE if saving succeeded, FALSE if error happened.
 */
bool ProjectFile::Save(LPCTSTR path)
{
	static const TCHAR id[] = _T("Project.WinMerge");
	HINSTANCE hinst = LanguageSelect.FindResourceHandle(id, RT_RCDATA);
	HRSRC hrsrc = FindResource(hinst, id, RT_RCDATA);
	PVOID pv = LoadResource(hinst, hrsrc);
	DWORD cb = SizeofResource(hinst, hrsrc);
	OString xml = HString::Oct(reinterpret_cast<LPCSTR>(pv), cb)->Uni(CP_UTF8);
	if (HString *tail = xml.SplitAt(L"&project.paths.left;"))
	{
		xml.Append(CMarkdown::Entitify(m_sLeftFile.c_str()));
		xml.Append(tail);
	}
	if (HString *tail = xml.SplitAt(L"&project.paths.right;"))
	{
		xml.Append(CMarkdown::Entitify(m_sRightFile.c_str()));
		xml.Append(tail);
	}
	if (HString *tail = xml.SplitAt(L"&project.paths.filter;"))
	{
		xml.Append(CMarkdown::Entitify(m_sFilter.c_str()));
		xml.Append(tail);
	}
	if (HString *tail = xml.SplitAt(L"&project.paths.subfolders;"))
	{
		WCHAR numbuff[24];
		xml.Append(_ltow(m_nRecursive, numbuff, 10));
		xml.Append(tail);
	}
	if (HString *tail = xml.SplitAt(L"&project.paths.left-readonly;"))
	{
		xml.Append(m_bLeftPathReadOnly ? L"1" : L"0");
		xml.Append(tail);
	}
	if (HString *tail = xml.SplitAt(L"&project.paths.right-readonly;"))
	{
		xml.Append(m_bRightPathReadOnly ? L"1" : L"0");
		xml.Append(tail);
	}
	UniStdioFile file;
	if (!file.OpenCreateUtf8(path) ||
		!file.WriteString(xml.B, xml.Len()))
	{
		const UniFile::UniError &err = file.GetLastUniError();
		String sError = err.apiname.empty() ? err.desc : GetSysError(err.syserrnum);
		if (sError.empty())
			sError = LanguageSelect.LoadString(IDS_UNK_ERROR_SAVING_PROJECT);
		LanguageSelect.FormatMessage(
			IDS_ERROR_FILEOPEN, path, sError.c_str()
		).MsgBox(MB_ICONSTOP);
		return false;
	}
	return true;
}

/**
 * @brief Is specified file a project file?
 * @param [in] filepath Full path to file to check.
 * @return true if file is a projectfile.
 */
BOOL ProjectFile::IsProjectFile(LPCTSTR filepath)
{
	return PathMatchSpec(filepath, _T("*.WinMerge"));
}
