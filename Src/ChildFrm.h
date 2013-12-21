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
 * @file  ChildFrm.h
 *
 * @brief interface of the CChildFrame class
 *
 */
// ID line follows -- this is updated by SVN
// $Id$

#if !defined(AFX_CHILDFRM_H__BBCD4F8E_34E4_11D1_BAA6_00A024706EDC__INCLUDED_)
#define AFX_CHILDFRM_H__BBCD4F8E_34E4_11D1_BAA6_00A024706EDC__INCLUDED_

class UniStdioFile;

#include "LocationView.h"
#include "../BuildTmp/Merge/midl/WinMerge_h.h"

/**
 * @brief Additional action codes for WinMerge.
 * @note Reserve first 100 for CrystalEditor
 */
enum
{
	CE_ACTION_MERGE = 100, /**< Merging action */
};

/**
 * @brief Return statuses of file rescan
 */
enum
{
	RESCAN_OK, /**< Rescan succeeded */
	RESCAN_SUPPRESSED, /**< Rescan not done - suppressed */
	RESCAN_FILE_ERR, /**< Error reading file */
	RESCAN_TEMP_ERR, /**< Error saving to temp file */
};

/**
 * @brief File saving statuses
 */
enum SAVERESULTS_TYPE
{
	SAVE_DONE, /**< Saving succeeded */  
	SAVE_FAILED, /**< Saving failed */  
	SAVE_NO_FILENAME, /**< File has no filename */  
	SAVE_CANCELLED, /**< Saving was cancelled */  
};

/**
 * @brief Result of trying to open file in MergeEditView display
 */
enum OPENRESULTS_TYPE
{
	OPENRESULTS_SUCCESS,       /**< Open succeeded */
	OPENRESULTS_FAILED_BINARY, /**< Open failed because one or both files were binary */
	OPENRESULTS_FAILED_MISC    /**< Open failed for some other reason */
};

enum MERGEVIEW_INDEX_TYPE
{
	MERGEVIEW_LEFT = 0,         /**< Left MergeView */
	MERGEVIEW_RIGHT,            /**< Right MergeView */
	MERGEVIEW_LEFT_DETAIL = 10, /**< Left DetailView */
	MERGEVIEW_RIGHT_DETAIL,     /**< Right DetailView */
};

struct DiffFileInfo;
class CMergeEditView;
class CMergeDiffDetailView;
class CGhostTextView;
class CLocationView;
class PackingInfo;
class CDirFrame;

/** 
 * @brief Frame class for file compare, handles panes, statusbar etc.
 */
class CChildFrame
	: ZeroInit<CChildFrame>
	, public CEditorFrame
	, public CScriptable<IMergeDoc>
	, private CFloatFlags
{
	class DiffMap;
	friend DiffMap;
public:
	CChildFrame(CMainFrame *, CDirFrame * = NULL, CChildFrame *pOpener = NULL);

	// IElementBehaviorFactory
	STDMETHOD(FindBehavior)(BSTR, BSTR, IElementBehaviorSite *, IElementBehavior **);
	// IMergeDoc
	STDMETHOD(GetFilePath)(long nSide, BSTR *pbsPath);
	STDMETHOD(GetStyleRules)(long nSide, BSTR *pbsRules);
	STDMETHOD(GetLineCount)(long nSide, long *pnLines);
	STDMETHOD(GetMarginHTML)(long nSide, long nLine, BSTR *pbsHTML);
	STDMETHOD(GetContentHTML)(long nSide, long nLine, BSTR *pbsHTML);
	STDMETHOD(PrepareHTML)(long nLine, long nStop, IDispatch *pFrame, long *pnLine);
	STDMETHOD(WriteReport)(BSTR bsPath);
	STDMETHOD(LimitContext)(long nLines);

	void UpdateResources();
	void SetLastCompareResult(int nResult);
	void UpdateCmdUI();
	void UpdateEditCmdUI();
	void UpdateGeneralCmdUI();
	void UpdateBookmarkUI();
	void UpdateSyncPointUI();
	void UpdateSourceTypeUI();
	void UpdateMergeStatusUI();
	void AlignScrollPositions();

// Attributes

	CSubFrame m_wndLocationBar;
	CSubFrame m_wndDiffViewBar;
	CLocationView m_wndLocationView;

	UINT m_idContextLines;

// Overrides
public:
	virtual FRAMETYPE GetFrameType() const { return FRAME_FILE; }
	void ActivateFrame();
	virtual void RecalcLayout();
	virtual BOOL PreTranslateMessage(MSG *);

// Implementation
private:
	void SavePosition();
	CMergeEditView *CreatePane(int iPane);
	void CreateClient();
	virtual ~CChildFrame();

private:
	static const LONG FloatScript[];
	static const LONG SplitScript[];
	static const LONG FloatScriptLocationBar[];
	static const LONG FloatScriptDiffViewBar[];
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

	template<UINT>
	LRESULT OnWndMsg(WPARAM, LPARAM);

public:
	enum FileChange
	{
		FileNoChange,
		FileChanged,
		FileRemoved,
	};

// Attributes
public:
	CChildFrame *const m_pOpener;
	CDiffTextBuffer *m_ptBuf[2]; /**< Left/Right side text buffer */
	static const int m_nBuffers = 2; /**< Takashi's code is 3-way aware */

	DiffList m_diffList;
	UINT m_nTrivialDiffs; /**< Amount of trivial (ignored) diffs */
	stl::vector<FileTextStats> m_pFileTextStats;

	int GetActiveMergeViewIndexType() const;
	CGhostTextView *GetActiveTextView() const;
	CMergeEditView *GetActiveMergeView() const;
	void UpdateAllViews();
	void UpdateHeaderPath(int pane);
	void UpdateHeaderActivity(int pane, bool bActivate);
	void RefreshOptions();
	OPENRESULTS_TYPE OpenDocs(FileLocation &, FileLocation &, bool bROLeft, bool bRORight);
	void RescanIfNeeded(float timeOutInSecond);
	int Rescan(bool &bIdentical, bool bForced = false);
	int Rescan2(bool &bIdentical);
	void ShowRescanError(int nRescanResult, BOOL bIdentical);
	void AddUndoAction(UINT nBegin, UINT nEnd, UINT nDiff, int nBlanks, BOOL bInsert, CMergeEditView *pList);
	bool Undo();
	void CopyAllList(int srcPane, int dstPane);
	void CopyMultipleList(int srcPane, int dstPane, int firstDiff, int lastDiff);
	bool SanityCheckDiff(const DIFFRANGE *) const;
	bool ListCopy(int srcPane, int dstPane, int nDiff = -1, bool bGroupWithPrevious = false);
	bool TrySaveAs(String &strPath, int &nLastErrorCode, String & sError,
		int nBuffer, PackingInfo * pInfoTempUnpacker);
	bool DoSave(bool &bSaveSuccess, int nBuffer);
	bool DoSaveAs(int nBuffer);
	int RightLineInMovedBlock(int leftLine);
	int LeftLineInMovedBlock(int rightLine);
	void SetEditedAfterRescan(int nBuffer);

	void SetUnpacker(PackingInfo *);
	void SetMergeViews(CMergeEditView * pLeft, CMergeEditView * pRight);
	void SetMergeDetailViews(CMergeDiffDetailView * pLeft, CMergeDiffDetailView * pRight);
	void SwapFiles();

	CMergeEditView *GetLeftView() const { return m_pView[0]; }
	CMergeEditView *GetRightView() const { return m_pView[1]; }
	CMergeEditView *GetView(int pane) const { return m_pView[pane]; }
	CMergeDiffDetailView *GetLeftDetailView() const { return m_pDetailView[0]; }
	CMergeDiffDetailView *GetRightDetailView() const { return m_pDetailView[1]; }
	CMergeDiffDetailView *GetDetailView(int pane) const { return m_pDetailView[pane]; }

	const FileTextEncoding & GetEncoding(int file) const
			{ return m_ptBuf[file]->getEncoding(); }

	void SetSyncPoint();
	void ClearSyncPoint();
	void ClearSyncPoints();
	bool HasSyncPoints() const { return m_bHasSyncPoints; }
	bool IsCursorAtSyncPoint();

	bool SaveModified();
	void SetTitle();

// Implementation in MergeDocLineDiffs.cpp
public:
	typedef enum { BYTEDIFF, WORDDIFF } DIFFLEVEL;
	void Showlinediff(CCrystalTextView *, CMergeEditView *, DIFFLEVEL);
	void GetWordDiffArray(int nLineIndex, stl::vector<wdiff> &worddiffs);
private:
	void Computelinediff(CCrystalTextView *, CCrystalTextView *, int, RECT &, RECT &, DIFFLEVEL);
// End MergeDocLineDiffs.cpp

// Implementation in MergeDocEncoding.cpp
	void DoFileEncodingDialog();
// End MergeDocEncoding.cpp

	void DoPrint();
	void ReloadDocs();

// Implementation
public:
	FileChange IsFileChangedOnDisk(LPCTSTR, DiffFileInfo &, const DiffFileInfo &);
	bool PromptAndSaveIfNeeded(bool bAllowCancel);
	stl::vector<CMergeEditView*> undoTgt;
	stl::vector<CMergeEditView*>::iterator curUndo;
	void FlushAndRescan(bool bForced = false);
	void SetCurrentDiff(int nDiff);
	int GetCurrentDiff() { return m_nCurDiff; }
	int GetContextDiff(int &firstDiff, int &lastDiff);
	bool GetMergingMode() const;
	void SetMergingMode(bool bMergingMode);
	void SetDetectMovedBlocks(bool bDetectMovedBlocks);
	bool IsMixedEOL(int nBuffer) const;

private:
// Implementation data

	int m_nCurDiff; /**< Selected diff, 0-based index, -1 if no diff selected */
	CMergeEditView *m_pView[MERGE_VIEW_COUNT]; /**< Pointer to left/right view */
	CMergeDiffDetailView *m_pDetailView[2];

	stl::vector<DiffFileInfo> m_pSaveFileInfo;
	stl::vector<DiffFileInfo> m_pRescanFileInfo;

	bool m_bEditAfterRescan[2]; /**< Left/right doc edited after rescanning */
	bool m_bInitialReadOnly[2]; /**< Left/right doc initial read-only state */
	bool m_bMergingMode; /**< Merging or Edit mode */
	bool m_bMixedEol; /**< Does this document have mixed EOL style? */
	bool m_bEnableRescan; /**< Automatic rescan enabled/disabled */
	bool m_bHasSyncPoints;
	FileTime m_LastRescan; /**< Time of last rescan (for delaying) */ 
	CDiffWrapper m_diffWrapper;
	/// information about the file packer/unpacker
	const stl::auto_ptr<PackingInfo> m_pInfoUnpacker;
// friend access
	friend class RescanSuppress;

private:
	void OnReadOnly(int nSide);
	void OnEditUndo();
	void OnEditRedo();
	void OnWMGoto();
	void OnOpenFile();
	void OnOpenFileWith();
	void OnOpenFileWithEditor();
	bool OnL2r();
	bool OnR2l();
	void OnAllLeft();
	void OnAllRight();
	void OnFileSave();
	void OnFileSaveLeft();
	void OnFileSaveRight();
	void OnFileSaveAsLeft();
	void OnFileSaveAsRight();
	void OnUpdateStatusNum();
	void OnFileEncoding();
	void OnToolsCompareSelection();
	void OnToolsGenerateReport();
	void WriteReport(UniStdioFile &);
	void PrimeTextBuffers();
	void AdjustDiffBlocks();
	int GetMatchCost(const String &sLine0, const String &sLine1);
	void FlagMovedLines(MovedLines *, CDiffTextBuffer *, CDiffTextBuffer *);
	String GetFileExt(LPCTSTR sFileName, LPCTSTR sDescription);
	static int GetBreakType();
	static bool GetByteColoringOption();
	static bool IsValidCodepageForMergeEditor(unsigned cp);
	static void SanityCheckCodepage(FileLocation &fileinfo);
	FileLoadResult::FILES_RESULT LoadOneFile(int index, bool &readOnly, FileLocation &);
	FileLoadResult::FILES_RESULT ReloadDoc(int index);
	FileLoadResult::FILES_RESULT LoadFile(int nBuffer, bool &readOnly, FileLocation &);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHILDFRM_H__BBCD4F8E_34E4_11D1_BAA6_00A024706EDC__INCLUDED_)
