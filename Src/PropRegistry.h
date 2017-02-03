/////////////////////////////////////////////////////////////////////////////
//    License (GPLv2+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
/////////////////////////////////////////////////////////////////////////////
/**
 * @file  PropRegistry.h
 *
 * @brief Declaration file PropRegistry
 */

/**
 * @brief Property page for system options; used in options property sheet.
 *
 * This class implements property sheet for what we consider System-options.
 * It allows user to select options like whether to use Recycle Bin for
 * deleted files and External text editor.
 */
class PropRegistry : public OptionsPanel
{
// Construction
public:
	PropRegistry();

// Implement IOptionsPanel
	virtual void ReadOptions();
	virtual void WriteOptions();
	virtual void UpdateScreen();

	String	m_strEditorPath;
	BOOL	m_bUseShellFileOperations;
	BOOL	m_bUseRecycleBin;
	BOOL	m_bUseShellFileBrowseDialogs;
	String	m_supplementFolder;
	int		m_tempFolderType;
	String	m_tempFolder;

// Implementation methods
protected:
	template<DDX_Operation>
			bool UpdateData();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	virtual BOOL OnInitDialog();
	void OnBrowseEditor();
	void OnBrowseSupplementFolder();
	void OnBrowseTmpFolder();
};
