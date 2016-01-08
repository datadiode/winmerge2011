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
 *  @file DirView.h
 *
 *  @brief Declaration of class CDirView
 */
#pragma once

/////////////////////////////////////////////////////////////////////////////
// CDirView view
#include "Constants.h"
#include "SortHeaderCtrl.h"

class FileActionScript;

struct DIFFITEM;

typedef enum { eMain, eContext } eMenuType;

class CDirDoc;
class CDirFrame;

class PackingInfo;
class PathContext;
class DirCmpReport;
class CDiffContext;
class DirCompProgressDlg;
struct DirColInfo;
class CLoadSaveCodepageDlg;
class CShellContextMenu;
class UniStdioFile;

struct ViewCustomFlags
{
	enum
	{
		// We use extra bits so that no valid values are 0
		// and each set of flags is in a different hex digit
		// to make debugging easier
		// These can always be packed down in the future
		VISIBILITY = 0x3, VISIBLE = 0x1, HIDDEN = 0x2, EXPANDED = 0x4
	};
};

/** Default column width in directory compare */
const UINT DefColumnWidth = 150;

/**
 * @brief Directory compare results view.
 *
 * Directory compare view is list-view based, so it shows one result (for
 * folder or file, commonly called as 'item') in one line. User can select
 * visible columns, re-order columns, sort by column etc.
 *
 * Actual data is stored in CDiffContext in CDirDoc. Dircompare items and
 * CDiffContext items are linked by storing POSITION of CDiffContext item
 * as CDirView listitem key.
 */
class CDirView
	: public OListView
	, public IDropSource
	, public IDataObject
{
	class DirItemEnumerator;
	friend DirItemEnumerator;
	friend CDirFrame;
	friend DirCmpReport;
public:
	CDirView(CDirFrame *);           // protected constructor used by dynamic creation
	virtual ~CDirView();

// Operations
public:
	CDirFrame *const m_pFrame;

	// IUnknown
	STDMETHOD(QueryInterface)(REFIID, void **);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();
	// IDropSource
    STDMETHOD(QueryContinueDrag)(BOOL fEscapePressed, DWORD grfKeyState);
    STDMETHOD(GiveFeedback)(DWORD dwEffect);
	// IDataObject
	STDMETHOD(GetData)(LPFORMATETC, LPSTGMEDIUM);
	STDMETHOD(GetDataHere)(LPFORMATETC, LPSTGMEDIUM);
	STDMETHOD(QueryGetData)(LPFORMATETC);
	STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC, LPFORMATETC);
	STDMETHOD(SetData)(LPFORMATETC, LPSTGMEDIUM, BOOL);
	STDMETHOD(EnumFormatEtc)(DWORD, LPENUMFORMATETC *);
	STDMETHOD(DAdvise)(LPFORMATETC, DWORD, LPADVISESINK, LPDWORD);
	STDMETHOD(DUnadvise)(DWORD);
	STDMETHOD(EnumDAdvise)(LPENUMSTATDATA *);

	void StartCompare();
	void Redisplay();
	void RedisplayChildren(DIFFITEM *diffpos, int level, int &index, int &alldiffs);
	void UpdateResources();
	int GetItemIndex(DIFFITEM *);
	void SortColumnsAppropriately();
	DIFFITEM *GetDiffItem(int sel);

	static bool IsShellMenuCmdID(UINT);
	LRESULT HandleMenuMessage(UINT message, WPARAM wParam, LPARAM lParam);
	void HandleMenuSelect(WPARAM wParam, LPARAM lParam);

	static LPCTSTR SuggestArchiveExtensions();

// Implementation types
private:
	typedef enum { SIDE_LEFT, SIDE_RIGHT } SIDE_TYPE;
	
// Implementation in DirActions.cpp
private:
	String GetSelectedFileName(SIDE_TYPE stype);
	void GetItemFileNames(const DIFFITEM *, String &strLeft, String &strRight);
	void FormatEncodingDialogDisplays(CLoadSaveCodepageDlg &);
	BOOL IsItemOpenable(const DIFFITEM *) const;
	BOOL AreItemsOpenable(const DIFFITEM *, const DIFFITEM *) const;
	void DoCopyLeftToRight();
	void DoCopyRightToLeft();
	void DoDelLeft();
	void DoDelRight();
	void DoDelBoth();
	void DoDelAll();
	void DoCopyLeftTo();
	void DoCopyRightTo();
	void DoMoveLeftTo();
	void DoMoveRightTo();
	void DoOpen(SIDE_TYPE);
	void DoOpenWith(SIDE_TYPE);
	void DoOpenWithEditor(SIDE_TYPE, LPCTSTR = NULL);
	void DoOpenWithFrhed(SIDE_TYPE);
	void DoOpenFolder(SIDE_TYPE);
	bool ConfirmActionList(FileActionScript &);
	void PerformActionList(FileActionScript &);
	void UpdateAfterFileScript(FileActionScript &);
	int MarkSelectedForRescan();
	void DoFileEncodingDialog();
	bool DoItemRename(int iItem, LPCTSTR szNewItemName);
	bool RenameOnSameDir(LPCTSTR szOldFileName, LPCTSTR szNewFileName);
// End DirActions.cpp
	void ReflectGetdispinfo(NMLVDISPINFO *);
	LRESULT ReflectKeydown(NMLVKEYDOWN *);
	LRESULT ReflectBeginLabelEdit(NMLVDISPINFO *);
	LRESULT ReflectEndLabelEdit(NMLVDISPINFO *);
	LRESULT ReflectCustomDraw(NMLVCUSTOMDRAW *);
	void ReflectColumnClick(NM_LISTVIEW *);
	void ReflectClick(NMITEMACTIVATE *);
	void ReflectItemActivate(NMITEMACTIVATE *);
	void ReflectBeginDrag();
// Implementation in DirViewColHandler.cpp
public:
	// class CompareState is used to pass parameters to the PFNLVCOMPARE callback function.
	class CompareState
	{
	private:
		const CDirView *const pView;
		const CDiffContext *const pCtxt;
		const int sortCol;
		const bool bSortAscending;
	public:
		CompareState(const CDirView *, int sortCol, bool bSortAscending);
		static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	} friend;
	void UpdateDiffItemStatus(UINT nIdx);
private:
	void UpdateColumns(UINT lvcf);
	int AddNewItem(int i, const DIFFITEM *di, int iImage, int iIndent);
	int ColPhysToLog(int i) const { return m_invcolorder[i]; }
	int ColLogToPhys(int i) const { return m_colorder[i]; } /**< -1 if not displayed */
	String GetColDisplayName(int col) const;
	String GetColDescription(int col) const;
	void LoadColumnOrders();
	void ValidateColumnOrdering();
	void ClearColumnOrders();
	void ResetColumnOrdering();
	void MoveColumn(int psrc, int pdest);
	String ColGetTextToDisplay(const CDiffContext *pCtxt, int col, const DIFFITEM *di);
	int ColSort(const CDiffContext *pCtxt, int col, const DIFFITEM *ldi, const DIFFITEM *rdi) const;
// End DirViewCols.cpp

// Implementation in DirViewColItems.cpp
// DirViewColItems typedefs
	typedef String (*ColGetFncPtrType)(const CDiffContext *, const void *);
	typedef int (*ColSortFncPtrType)(const CDiffContext *, const void *, const void *);

	struct DirColInfo /**< Information about one column of dirview list info */
	{
		LPCTSTR regName; /**< Internal name used for registry entries etc */
		// localized string resources
		WORD idName; /**< Displayed name, ID of string resource */
		WORD idDesc; /**< Description, ID of string resource */
		ColGetFncPtrType getfnc; /**< Handler giving display string */
		ColSortFncPtrType sortfnc; /**< Handler for sorting this column */
		WORD offset;
		short physicalIndex; /**< Current physical index, -1 if not displayed */
		bool defSortUp; /**< Does column start with ascending sort (most do) */
		char alignment; /**< Column alignment */
	};
	static const DirColInfo f_cols[];
	static const int g_ncols;
// End DirViewColItems.cpp

public:
	void UpdateFont();
	void OnInitialUpdate();
	LRESULT ReflectNotify(UNotify *);

	static HImageList *AcquireSharedResources();
	static void ReleaseSharedResources();
	static HFont *ReplaceFont(const LOGFONT *);

// Implementation
protected:
	int GetFocusedItem();
	int GetFirstDifferentItem();
	int GetLastDifferentItem();
	int AddSpecialItems();
	bool OpenOneItem(DIFFITEM *, String &path1, String &path2);
	bool OpenTwoItems(DIFFITEM *, DIFFITEM *, String &path1, String &path2);

// Implementation data
protected:
	CSortHeaderCtrl m_ctlSortHeader;
	static HFont *m_font; /**< User-selected font */
	static HImageList *m_imageList;
	static HImageList *m_imageState;
	int m_numcols;
	int m_dispcols;
	std::vector<int> m_colorder; /**< colorder[logical#]=physical# */
	std::vector<int> m_invcolorder; /**< invcolorder[physical]=logical# */
	int m_nHiddenItems; /**< Count of items we have hidden */
	int m_nSpecialItems; /**< Count of special items */
	bool m_bTreeMode; /**< TRUE if tree mode is on*/
	DirCompProgressDlg *m_pCmpProgressDlg;
	clock_t m_compareStart; /**< Starting process time of the compare */
	String m_lastCopyFolder; /**< Last Copy To -target folder. */
	String m_lastLeftPath;
	String m_lastRightPath;

	CShellContextMenu *m_pShellContextMenuLeft; /**< Shell context menu for group of left files */
	HMENU m_hShellContextMenuLeft;

	CShellContextMenu *m_pShellContextMenuRight; /**< Shell context menu for group of right files */
	HMENU m_hShellContextMenuRight;

	HMenu *m_pCompareAsScriptMenu;

	void OnDestroy();
	void OnFirstdiff();
	void OnLastdiff();
	void OnNextdiff();
	void OnPrevdiff();
	void OnCurdiff();
	void OnUpdateUIMessage();
	void OnEditColumns();
	void OnCustomizeColumns();
	void OnToolsGenerateReport();
	void OnToolsSaveToXLS(LPCTSTR verb, int flags = LVNI_ALL);
	void OnCtxtDirZipLeft();
	void OnCtxtDirZipRight();
	void OnCtxtDirZipBoth();
	void OnCtxtDirZipBothDiffsOnly();
	void OnSelectAll();
	void OnCopyLeftPathnames();
	void OnCopyRightPathnames();
	void OnCopyBothPathnames();
	void OnCopyFilenames();
	void OnItemRename();
	void OnHideFilenames();
	void OnUpdateStatusNum();
	void OnViewShowHiddenItems();
	void OnViewTreeMode();
	void OnViewExpandAllSubdirs();
	void OnViewCollapseAllSubdirs();
	void OnViewCompareStatistics();
	void OnEditCopy();
	void OnEditCut();
	void OnEditPaste();
	void OnEditUndo();

	LRESULT WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void DoDefaultAction(int sel);
private:
	void OpenSelection(LPCTSTR szCompareAs, UINT idCompareAs);
	int GetSelectedItems(DIFFITEM **);
	void OpenParentDirectory();
	bool IsItemNavigableDiff(const DIFFITEM *) const;
	void MoveFocus(int, int);
	void SaveColumnWidths();
	void SaveColumnOrders();
	void HeaderContextMenu(POINT);
	void ListContextMenu(POINT);
	HMENU ListShellContextMenu(SIDE_TYPE);
	void ReloadColumns();
	void ResetColumnWidths();
	void DeleteChildren(const DIFFITEM *, int);
	void DetachItem(int);
	int CollapseSubdir(int);
	int ExpandSubdir(int);
	void DeepExpandSubdir(int);
	void PrepareDragData(UniStdioFile &);
};
