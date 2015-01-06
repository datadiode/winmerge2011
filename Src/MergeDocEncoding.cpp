/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997-2000  Thingamahoochie Software
//    Author: Dean Grimm
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
 * @file  MergeDoc.cpp
 *
 * @brief Implementation file for CMergeDoc
 *
 */
#include "StdAfx.h"
#include "Merge.h"
#include "LanguageSelect.h"
#include "MainFrm.h"
#include "LoadSaveCodepageDlg.h"
#include "unicoder.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/**
 * @brief Display file encoding dialog to user & handle user's choices
 */
void CChildFrame::DoFileEncodingDialog()
{
	if (!PromptAndSaveIfNeeded(true))
		return;
	
	CLoadSaveCodepageDlg dlg;
	dlg.m_nLoadCodepage = m_ptBuf[0]->getCodepage();
	dlg.m_nSaveCodepage = m_ptBuf[0]->getCodepage();
	if (IDOK != LanguageSelect.DoModal(dlg))
		return;

	FileLocation filelocLeft, filelocRight;
	bool bROLeft = m_ptBuf[0]->GetReadOnly();
	bool bRORight = m_ptBuf[1]->GetReadOnly();
	if (dlg.m_bAffectsLeft)
	{
		filelocLeft.encoding.m_unicoding = NONE;
		filelocLeft.encoding.m_codepage = dlg.m_nLoadCodepage;
	}
	else
	{
		filelocLeft.encoding.m_unicoding = m_ptBuf[0]->getUnicoding();
		filelocLeft.encoding.m_codepage = m_ptBuf[0]->getCodepage();
	}
	if (dlg.m_bAffectsRight)
	{
		filelocRight.encoding.m_unicoding = NONE;
		filelocRight.encoding.m_codepage = dlg.m_nLoadCodepage;
	}
	else
	{
		filelocRight.encoding.m_unicoding = m_ptBuf[1]->getUnicoding();
		filelocRight.encoding.m_codepage = m_ptBuf[1]->getCodepage();
	}
	filelocLeft.filepath = m_strPath[0];
	filelocLeft.description = m_strDesc[0];
	filelocRight.filepath = m_strPath[1];
	filelocRight.description = m_strDesc[1];
	OpenDocs(filelocLeft, filelocRight, bROLeft, bRORight);
}

