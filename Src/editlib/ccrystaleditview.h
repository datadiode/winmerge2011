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

#pragma once

#include "ccrystaltextview.h"
#include "ccrystaltextbuffer.h"

/////////////////////////////////////////////////////////////////////////////
//  Forward class declarations

/////////////////////////////////////////////////////////////////////////////
//  CCrystalEditView view

class CCrystalEditView
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
	void DeleteCurrentSelection();

public:
	bool GetAutoIndent() const { return m_bAutoIndent; }
	void SetAutoIndent(bool bAutoIndent) { m_bAutoIndent = bAutoIndent; }

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

    void ReplaceSelection(LPCTSTR pszNewText, int cchNewText);

    virtual void OnEditOperation(int nAction, LPCTSTR pszText);

    virtual TextDefinition *DoSetTextType(TextDefinition *def);

    // Implementation
protected:
    // Generated message map functions
    bool m_bMergeUndo;
public:
	void OnEditCut();
	void OnEditPaste();
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
	void OnEditDeleteWord();
	void OnEditDeleteWordBack();
	void OnEditRightToLeft();
};
