/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997  Dean P. Grimm
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
 * @file  DirFrame.h
 *
 * @brief Declaration file for CDirFrame
 *
 */
// RCS ID line follows -- this is updated by CVS
// $Id$

#if !defined(AFX_DIRFRAME_H__95565903_35C4_11D1_BAA7_00A024706EDC__INCLUDED_)
#define AFX_DIRFRAME_H__95565903_35C4_11D1_BAA7_00A024706EDC__INCLUDED_

/////////////////////////////////////////////////////////////////////////////
// CDirFrame frame

#include "DiffThread.h"

class CDirView;
class CHexMergeFrame;
typedef stl::list<CChildFrame *> MergeDocPtrList;
typedef stl::list<CHexMergeFrame *> HexMergeDocPtrList;
class CTempPathContext;
struct FileActionItem;

/**
 * @brief Frame window for Directory Compare window
 */
class CDirFrame : public CDocFrame, private CFloatFlags
{
// Attributes
public:
	CDirFrame(CMainFrame *);

	CDirView *const m_pDirView;

// Operations
public:
	virtual FRAMETYPE GetFrameType() const { return FRAME_FOLDER; }
	void SetStatus(LPCTSTR szStatus);
	void SetFilterStatusDisplay(LPCTSTR szFilter);
	BOOL GetLeftReadOnly() const { return m_bROLeft; }
	BOOL GetRightReadOnly() const { return m_bRORight; }
	void SetLeftReadOnly(BOOL bReadOnly);
	void SetRightReadOnly(BOOL bReadOnly);
	HStatusBar *m_wndStatusBar;
	void UpdateResources();
	void CreateClient();
	void AlignStatusBar(int cx);

	template<UINT>
	void UpdateCmdUI();

protected:

	BOOL m_bROLeft; /**< Is left side read-only */
	BOOL m_bRORight; /**< Is right side read-only */

	virtual ~CDirFrame();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

	bool PreventFromClosing();

	template<UINT>
	LRESULT OnWndMsg(WPARAM, LPARAM);

private:
	static const LONG FloatScript[];
// Attributes
public:
	CTempPathContext *m_pTempPathContext;
// Operations
public:
	bool CloseMergeDocs();
	CChildFrame *GetMergeDocForDiff();
	CHexMergeFrame *GetHexMergeDocForDiff();
	bool CanFrameClose();

// Implementation
public:
	void InitCompare(LPCTSTR pszLeft, LPCTSTR pszRight, BOOL bRecursive, CTempPathContext *);
	void Rescan();
	BOOL GetRecursive() const { return m_bRecursive; }
	void CompareReady();
	void UpdateChangedItem(const CChildFrame *);
	UINT_PTR FindItemFromPaths(LPCTSTR pathLeft, LPCTSTR pathRight);
	void SetDiffSide(UINT diffcode, int idx);
	void SetDiffCompare(UINT diffcode, int idx);
	void UpdateStatusFromDisk(UINT_PTR diffPos, BOOL bLeft, BOOL bRight);
	void ReloadItemStatus(UINT_PTR diffPos, BOOL bLeft, BOOL bRight);
	void Redisplay();
	void AddMergeDoc(CChildFrame *);
	void AddMergeDoc(CHexMergeFrame *);
	void MergeDocClosing(CChildFrame *);
	void MergeDocClosing(CHexMergeFrame *);
	CDiffThread m_diffThread;
	void SetDiffStatus(UINT diffcode, UINT mask, int idx);
	void SetDiffCounts(UINT diffs, UINT ignored, int idx);
	void UpdateDiffAfterOperation(const FileActionItem & act, UINT_PTR pos);
	void UpdateHeaderPath(BOOL bLeft);
	void AbortCurrentScan();
	bool IsCurrentScanAbortable() const;
	void SetDescriptions(const String &strLeftDesc, const String &strRightDesc);
	void ApplyLeftDisplayRoot(String &);
	void ApplyRightDisplayRoot(String &);

	bool IsShowable(const DIFFITEM &);

	CDiffContext *GetDiffContext() { return m_pCtxt; }
	const DIFFITEM & GetDiffByKey(UINT_PTR key) const { return m_pCtxt->GetDiffAt(key); }
	DIFFITEM & GetDiffRefByKey(UINT_PTR key) { return m_pCtxt->GetDiffRefAt(key); }
	String GetLeftBasePath() const { return m_pCtxt->GetLeftPath(); }
	String GetRightBasePath() const { return m_pCtxt->GetRightPath(); }
	void RemoveDiffByKey(UINT_PTR key) { m_pCtxt->RemoveDiff(key); }
	void SetMarkedRescan() {m_bMarkedRescan = TRUE; }
	struct AllowUpwardDirectory
	{
		enum ReturnCode
		{
			Never,
			No,
			ParentIsRegularPath,
			ParentIsTempPath
		};
	};
	AllowUpwardDirectory::ReturnCode AllowUpwardDirectory(String &leftParent, String &rightParent);
	void SetItemViewFlag(UINT_PTR key, UINT flag, UINT mask);
	void SetItemViewFlag(UINT flag, UINT mask);
	const CompareStats * GetCompareStats() const { return m_pCompareStats; };
	bool IsArchiveFolders();

protected:
	void LoadLineFilterList();

	// Implementation data
private:
	CDiffContext *m_pCtxt; /**< Pointer to compare results-data */
	CompareStats *const m_pCompareStats; /**< Compare statistics */
	MergeDocPtrList m_MergeDocs; /**< List of file compares opened from this compare */
	HexMergeDocPtrList m_HexMergeDocs; /**< List of hex file compares opened from this compare */
	BOOL m_bRecursive; /**< Is current compare recursive? */
	String m_strLeftDesc; /**< Left side desription text */
	String m_strRightDesc; /**< Left side desription text */
	BOOL m_bMarkedRescan; /**< If TRUE next rescan scans only marked items */
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIRFRAME_H__95565903_35C4_11D1_BAA7_00A024706EDC__INCLUDED_)
