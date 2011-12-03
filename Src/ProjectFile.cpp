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
 * @file  ProjectFile.cpp
 *
 * @brief Implementation file for ProjectFile class.
 */
// ID line follows -- this is updated by CVS
// $Id$

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
: m_bHasLeft(FALSE)
, m_bHasRight(FALSE)
, m_bHasFilter(FALSE)
, m_bHasSubfolders(FALSE)
, m_subfolders(-1)
, m_bLeftReadOnly(FALSE)
, m_bRightReadOnly(FALSE)
{
}

/** 
 * @brief Open given path-file and read data from it to member variables.
 * @param [in] path Path to project file.
 * @param [out] sError Error string if error happened.
 * @return TRUE if reading succeeded, FALSE if error happened.
 */
bool ProjectFile::Read(LPCTSTR path, String &sError)
{
	bool loaded = false;
	CMarkdown::File file = path;
	CMarkdown::EntityMap em;
	CMarkdown::Load(em);
	if (CMarkdown &project = CMarkdown(file).Move(Root_element_name).Pop())
	{
		if (CMarkdown &paths = CMarkdown(project).Move(Paths_element_name).Pop())
		{
			if (CMarkdown &node = CMarkdown(paths).Move(Left_element_name))
			{
				m_leftFile = OString(node.GetInnerText()->Uni(em)).W;
				m_bHasLeft = TRUE;
			}
			if (CMarkdown &node = CMarkdown(paths).Move(Right_element_name))
			{
				m_rightFile = OString(node.GetInnerText()->Uni(em)).W;
				m_bHasRight = TRUE;
			}
			if (CMarkdown &node = CMarkdown(paths).Move(Filter_element_name))
			{
				m_filter = OString(node.GetInnerText()->Uni(em)).W;
				m_bHasFilter = TRUE;
			}
			if (CMarkdown &node = CMarkdown(paths).Move(Subfolders_element_name))
			{
				m_subfolders = atoi(OString(node.GetInnerText()).A);
				m_bHasSubfolders = TRUE;
			}
			if (CMarkdown &node = CMarkdown(paths).Move(Left_ro_element_name))
			{
				m_bLeftReadOnly = atoi(OString(node.GetInnerText()).A) != 0;
			}
			if (CMarkdown &node = CMarkdown(paths).Move(Right_ro_element_name))
			{
				m_bLeftReadOnly = atoi(OString(node.GetInnerText()).A) != 0;
			}
			loaded = true;
		}
	}
	return loaded;
}

/** 
 * @brief Save data from member variables to path-file.
 * @param [in] path Path to project file.
 * @param [out] sError Error string if error happened.
 * @return TRUE if saving succeeded, FALSE if error happened.
 */
bool ProjectFile::Save(LPCTSTR path, String &sError)
{
	static const TCHAR id[] = _T("Project.WinMerge");
	HINSTANCE hinst = LanguageSelect.FindResourceHandle(id, RT_RCDATA);
	HRSRC hrsrc = FindResource(hinst, id, RT_RCDATA);
	PVOID pv = LoadResource(hinst, hrsrc);
	DWORD cb = SizeofResource(hinst, hrsrc);
	OString xml = HString::Oct(reinterpret_cast<LPCSTR>(pv), cb)->Uni(CP_UTF8);
	if (HString *tail = xml.SplitAt(L"&project.paths.left;"))
	{
		xml.Append(CMarkdown::Entities(m_leftFile.c_str()));
		xml.Append(tail);
	}
	if (HString *tail = xml.SplitAt(L"&project.paths.right;"))
	{
		xml.Append(CMarkdown::Entities(m_rightFile.c_str()));
		xml.Append(tail);
	}
	if (HString *tail = xml.SplitAt(L"&project.paths.filter;"))
	{
		xml.Append(CMarkdown::Entities(m_filter.c_str()));
		xml.Append(tail);
	}
	if (HString *tail = xml.SplitAt(L"&project.paths.subfolders;"))
	{
		xml.Append(m_subfolders ? L"1" : L"0");
		xml.Append(tail);
	}
	if (HString *tail = xml.SplitAt(L"&project.paths.left-readonly;"))
	{
		xml.Append(m_bLeftReadOnly ? L"1" : L"0");
		xml.Append(tail);
	}
	if (HString *tail = xml.SplitAt(L"&project.paths.right-readonly;"))
	{
		xml.Append(m_bRightReadOnly ? L"1" : L"0");
		xml.Append(tail);
	}
	UniStdioFile file;
	bool success = false;
	if (file.OpenCreateUtf8(path) &&
		file.WriteString(xml.B, xml.Len()))
	{
		success = true;
	}
	else
	{
		const UniFile::UniError &err = file.GetLastUniError();
		sError = err.apiname.empty() ? err.desc : GetSysError(err.syserrnum);
	}
	return success;
}

/** 
 * @brief Returns if left path is defined in project file.
 * @return TRUE if project file has left path.
 */
bool ProjectFile::HasLeft() const
{
	return m_bHasLeft;
}

/** 
 * @brief Returns if right path is defined in project file.
 * @return TRUE if project file has right path.
 */
bool ProjectFile::HasRight() const
{
	return m_bHasRight;
}

/** 
 * @brief Returns if filter is defined in project file.
 * @return TRUE if project file has filter.
 */
bool ProjectFile::HasFilter() const
{
	return m_bHasFilter;
}

/** 
 * @brief Returns if subfolder is defined in projectfile.
 * @return TRUE if project file has subfolder definition.
 */
bool ProjectFile::HasSubfolders() const
{
	return m_bHasSubfolders;
}

/** 
 * @brief Returns left path.
 * @param [out] pReadOnly TRUE if readonly was specified for path.
 * @return Left path.
 */
String ProjectFile::GetLeft() const
{
	return m_leftFile;
}

/** 
 * @brief Returns if left path is specified read-only.
 * @return TRUE if left path is read-only, FALSE otherwise.
 */
bool ProjectFile::GetLeftReadOnly() const
{
	return m_bLeftReadOnly;
}

/** 
 * @brief Set left path, returns old left path.
 * @param [in] sLeft Left path.
 * @param [in] bReadOnly Will path be recorded read-only?
 */
void ProjectFile::SetLeft(const String& sLeft, bool readOnly)
{
	m_leftFile = sLeft;
	m_bLeftReadOnly = readOnly;
}

/** 
 * @brief Returns right path.
 * @param [out] pReadOnly TRUE if readonly was specified for path.
 * @return Right path.
 */
String ProjectFile::GetRight() const
{
	return m_rightFile;
}

/** 
 * @brief Returns if right path is specified read-only.
 * @return TRUE if right path is read-only, FALSE otherwise.
 */
bool ProjectFile::GetRightReadOnly() const
{
	return m_bRightReadOnly;
}

/** 
 * @brief Set right path, returns old right path.
 * @param [in] sRight Right path.
 * @param [in] bReadOnly Will path be recorded read-only?
 */
void ProjectFile::SetRight(const String& sRight, bool readOnly)
{
	m_rightFile = sRight;
	m_bRightReadOnly = readOnly;
}

/** 
 * @brief Returns filter.
 * @return Filter string.
 */
String ProjectFile::GetFilter() const
{
	return m_filter;
}

/** 
 * @brief Set filter.
 * @param [in] sFilter New filter string to set.
 */
void ProjectFile::SetFilter(const String& sFilter)
{
	m_filter = sFilter;
}

/** 
 * @brief Returns subfolder included -setting.
 * @return != 0 if subfolders are included.
 */
int ProjectFile::GetSubfolders() const
{
	return m_subfolders;
}

/** 
 * @brief set subfolder.
 * @param [in] iSubfolder New value for subfolder inclusion.
 */
void ProjectFile::SetSubfolders(int iSubfolder)
{
	m_subfolders = iSubfolder ? 1 : 0;
}

/**
 * @brief Is specified file a project file?
 * @param [in] filepath Full path to file to check.
 * @return true if file is a projectfile.
 */
bool ProjectFile::IsProjectFile(LPCTSTR filepath)
{
	LPCTSTR ext = PathFindExtension(filepath);
	return lstrcmpi(ext, PROJECTFILE_EXT) == 0;
}
