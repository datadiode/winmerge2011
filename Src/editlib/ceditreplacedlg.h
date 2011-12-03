////////////////////////////////////////////////////////////////////////////
//  File:       ceditreplacedlg.h
//  Version:    1.0.0.0
//  Created:    29-Dec-1998
//
//  Author:     Stcherbatchenko Andrei
//  E-mail:     windfall@gmx.de
//
//  Declaration of the CEditReplaceDlg dialog, a part of Crystal Edit -
//  syntax coloring text editor.
//
//  You are free to use or modify this code to the following restrictions:
//  - Acknowledge me somewhere in your about box, simple "Parts of code by.."
//  will be enough. If you can't (or don't want to), contact me personally.
//  - LEAVE THIS HEADER INTACT
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//  19-Jul-99
//      Ferdinand Prantl:
//  +   FEATURE: see cpps ...
//
//  ... it's being edited very rapidly so sorry for non-commented
//        and maybe "ugly" code ...
////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_CEDITREPLACEDLG_H__759417E3_7B18_11D2_8C50_0080ADB86836__INCLUDED_)
#define AFX_CEDITREPLACEDLG_H__759417E3_7B18_11D2_8C50_0080ADB86836__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ceditreplacedlg.h : header file
//

#include "resource.h"
#include "SuperComboBox.h"
#include "cfindtextdlg.h" // for structure LastSearchInfos

class CCrystalEditView;

/////////////////////////////////////////////////////////////////////////////
// CEditReplaceDlg dialog

class EDITPADC_CLASS CEditReplaceDlg
	: ZeroInit<CEditReplaceDlg>
	, public ODialog
{
private:
	CCrystalEditView *const m_pBuddy;
	BOOL m_bFound;
	POINT m_ptFoundAt;
	BOOL DoHighlightText(BOOL bNotifyIfNotFound);
	void UpdateControls();

	// Construction
public:
	CEditReplaceDlg(CCrystalEditView *pBuddy);
	void SetScope(bool bWithSelection);

	bool m_bEnableScopeSelection;
	bool m_bConfirmed;
	POINT m_ptCurrentPos;
	POINT m_ptBlockBegin, m_ptBlockEnd;

	BOOL m_bMatchCase;
	BOOL m_bWholeWord;
	BOOL m_bRegExp;
	String m_sText;
	String m_sNewText;
	int m_nScope;
	BOOL m_bNoWrap;

	HSuperComboBox *m_pCbFindText;
	HSuperComboBox *m_pCbReplText;

  protected:
	template<DDX_Operation>
			bool UpdateData();
	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

	void UpdateRegExp();

	// Generated message map functions
	void OnChangeEditText();
	void OnChangeSelected();
	void OnCancel();
	void OnEditReplace();
	void OnEditReplaceAll();
	void OnEditSkip();
	void OnRegExp();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CEDITREPLACEDLG_H__759417E3_7B18_11D2_8C50_0080ADB86836__INCLUDED_)
