////////////////////////////////////////////////////////////////////////////
//  File:       ccrystaleditview.h
//  Version:    1.0.0.0
//  Created:    29-Dec-1998
//
//  Author:     Stcherbatchenko Andrei
//  E-mail:     windfall@gmx.de
//
//  Interface of the CCrystalEditView class, a part of Crystal Edit - syntax
//  coloring text editor.
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

#if !defined(AFX_CCRYSTALEDITVIEW_H__8F3F8B63_6F66_11D2_8C34_0080ADB86836__INCLUDED_)
#define AFX_CCRYSTALEDITVIEW_H__8F3F8B63_6F66_11D2_8C34_0080ADB86836__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "ccrystaltextview.h"
#include "ccrystaltextbuffer.h"

/////////////////////////////////////////////////////////////////////////////
//  Forward class declarations

/////////////////////////////////////////////////////////////////////////////
//  CCrystalEditView view

class EDITPADC_CLASS CCrystalEditView
	: public CCrystalTextView
	, public IDropTarget
  {

public:
    int m_nLastReplaceLen;

protected:
    BOOL m_bLastReplace;
    DWORD m_dwLastReplaceFlags;

protected:
    bool m_bDropPosVisible;
    POINT m_ptSavedCaretPos;
    bool m_bSelectionPushed;
    POINT m_ptSavedSelStart, m_ptSavedSelEnd;
private:
    bool m_bOvrMode;
    POINT m_ptDropPos;
    bool m_bAutoIndent;

public:
    virtual void ResetView();

	STDMETHOD(QueryInterface)(REFIID, void **);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();
	STDMETHOD(DragEnter)(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
	STDMETHOD(DragLeave)();
	STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
	STDMETHOD(Drop)(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

protected:
	virtual DWORD GetDropEffect();
	virtual void OnDropSource(DWORD de);
	void Paste();
	void Cut();
	void DeleteCurrentSelection();

public:
	bool GetAutoIndent() const { return m_bAutoIndent; }
	void SetAutoIndent(bool bAutoIndent) { m_bAutoIndent = bAutoIndent; }
	BOOL GetInsertTabs() const { return m_pTextBuffer->GetInsertTabs(); }
	void SetInsertTabs(BOOL bInsertTabs) { m_pTextBuffer->SetInsertTabs(bInsertTabs); }

    CCrystalEditView(size_t);
    ~CCrystalEditView();

	bool GetOverwriteMode() const { return m_bOvrMode; }
	void SetOverwriteMode(bool bOvrMode = TRUE) { m_bOvrMode = bOvrMode; }

    void ShowDropIndicator(const POINT &point);
    void HideDropIndicator();

    bool DoDropText(HGLOBAL hData, const POINT &ptClient);
    void DoDragScroll(const POINT &point);

	bool DoEditUndo();
    bool DoEditRedo();

    bool QueryEditable();

	virtual void UpdateView(CCrystalTextView * pSource, CUpdateContext * pContext, DWORD dwFlags, int nLineIndex = -1);

    void ReplaceSelection(LPCTSTR pszNewText, int cchNewText, DWORD dwFlags);

    virtual void OnEditOperation(int nAction, LPCTSTR pszText);

    virtual bool DoSetTextType(TextDefinition *def);

    // Implementation
protected :
    // Generated message map functions
    bool m_bMergeUndo;
public:
    void OnEditDelete();
    void OnChar(WPARAM);
    void OnEditDeleteBack();
    void OnEditUntab();
    void OnEditTab();
    void OnEditSwitchOvrmode();
    void OnEditReplace();
    void OnEditLowerCase();
    void OnEditUpperCase();
    void OnEditSwapCase();
    void OnEditCapitalize();
    void OnEditSentence();
	void OnEditGotoLastChange();
	void OnEditDeleteWord ();
	void OnEditDeleteWordBack ();
  };

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CCRYSTALEDITVIEW_H__8F3F8B63_6F66_11D2_8C34_0080ADB86836__INCLUDED_)
