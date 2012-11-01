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
//
// ID line follows -- this is updated by SVN
// $Id$

#if !defined(AFX_DirView_H__16E7C721_351C_11D1_95CD_444553540000__INCLUDED_)
#define AFX_DirView_H__16E7C721_351C_11D1_95CD_444553540000__INCLUDED_

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
		INVALID_CODE = 0,
		VISIBILITY = 0x3, VISIBLE = 0x1, HIDDEN = 0x2, EXPANDED = 0x4
	};
};

/**
 * @brief Position value for special items (..) in directory compare view.
 */
const UINT_PTR SPECIAL_ITEM_POS = (UINT_PTR) - 1L;

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
	void RedisplayChildren(UINT_PTR diffpos, int level, int &index, int &alldiffs);
	void UpdateResources();
	void LoadColumnHeaderItems();
	UINT_PTR GetItemKey(int idx);
	int GetItemIndex(UINT_PTR key);
	void SortColumnsAppropriately();
	DIFFITEM &GetDiffItem(int sel);

	static bool IsShellMenuCmdID(UINT);
	LRESULT HandleMenuMessage(UINT message, WPARAM wParam, LPARAM lParam);
	void HandleMenuSelect(WPARAM wParam, LPARAM lParam);

	static LPCTSTR SuggestArchiveExtensions();

// Implementation types
private:
	typedef enum { SIDE_LEFT = 1, SIDE_RIGHT } SIDE_TYPE;
	
// Implementation in DirActions.cpp
private:
	String GetSelectedFileName(SIDE_TYPE stype);
	void GetItemFileNames(int sel, String& strLeft, String& strRight);
	void FormatEncodingDialogDisplays(CLoadSaveCodepageDlg &);
	static BOOL IsItemCopyableToLeft(const DIFFITEM & di);
	static BOOL IsItemCopyableToRight(const DIFFITEM & di);
	static BOOL IsItemDeletableOnLeft(const DIFFITEM & di);
	static BOOL IsItemDeletableOnRight(const DIFFITEM & di);
	static BOOL IsItemDeletableOnBoth(const DIFFITEM & di);
	BOOL IsItemOpenable(const DIFFITEM & di) const;
	BOOL AreItemsOpenable(const DIFFITEM & di1, const DIFFITEM & di2) const;
	BOOL IsItemOpenableOnLeft(const DIFFITEM & di) const;
	BOOL IsItemOpenableOnRight(const DIFFITEM & di) const;
	BOOL IsItemOpenableOnLeftWith(const DIFFITEM & di) const;
	BOOL IsItemOpenableOnRightWith(const DIFFITEM & di) const;
	static BOOL IsItemCopyableToOnLeft(const DIFFITEM & di);
	static BOOL IsItemCopyableToOnRight(const DIFFITEM & di);
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
	void DoOpenWithEditor(SIDE_TYPE);
	void ConfirmAndPerformActions(FileActionScript & actions, int selCount);
	BOOL ConfirmActionList(const FileActionScript & actions, int selCount);
	void PerformActionList(FileActionScript & actions);
	void UpdateAfterFileScript(FileActionScript & actionList);
	UINT MarkSelectedForRescan();
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
	void InitiateSort();
	int AddNewItem(int i, UINT_PTR diffpos, int iImage, int iIndent);
	int ColPhysToLog(int i) const { return m_invcolorder[i]; }
	int ColLogToPhys(int i) const { return m_colorder[i]; } /**< -1 if not displayed */
	String GetColDisplayName(int col) const;
	String GetColDescription(int col) const;
	void LoadColumnOrders();
	void ValidateColumnOrdering();
	void ClearColumnOrders();
	void ResetColumnOrdering();
	void MoveColumn(int psrc, int pdest);
	static LPCTSTR GetColRegValueNameBase(int col);
	String ColGetTextToDisplay(const CDiffContext *pCtxt, int col, const DIFFITEM & di);
	int ColSort(const CDiffContext *pCtxt, int col, const DIFFITEM & ldi, const DIFFITEM &rdi) const;
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

private:

public:
	void UpdateFont();
	void OnInitialUpdate();
	LRESULT ReflectNotify(LPARAM);

	static void AcquireSharedResources();
	static void ReleaseSharedResources();
	static HFont *ReplaceFont(const LOGFONT *);

// Implementation
protected:
	int GetFocusedItem();
	int GetFirstDifferentItem();
	int GetLastDifferentItem();
	int AddSpecialItems();
	void OpenSpecialItems(UINT_PTR pos1, UINT_PTR pos2);
	bool OpenOneItem(UINT_PTR pos1, DIFFITEM **di1, DIFFITEM **di2,
			String &path1, String &path2, int &sel1, bool &isDir);
	bool OpenTwoItems(UINT_PTR pos1, UINT_PTR pos2, DIFFITEM **di1, DIFFITEM **di2,
			String &path1, String &path2, int &sel1, int &sel2, bool &isDir);
	bool CreateFoldersPair(const DIFFITEM &di, bool side1, String &newFolder);

// Implementation data
protected:
	CSortHeaderCtrl m_ctlSortHeader;
	static HFont *m_font; /**< User-selected font */
	static HImageList *m_imageList;
	static HImageList *m_imageState;
	int m_numcols;
	int m_dispcols;
	stl::vector<int> m_colorder; /**< colorder[logical#]=physical# */
	stl::vector<int> m_invcolorder; /**< invcolorder[physical]=logical# */
	UINT m_nHiddenItems; /**< Count of items we have hidden */
	UINT m_nSpecialItems; /**< Count of special items */
	bool m_bTreeMode; /**< TRUE if tree mode is on*/
	DirCompProgressDlg *m_pCmpProgressDlg;
	clock_t m_compareStart; /**< Starting process time of the compare */
	String m_lastCopyFolder; /**< Last Copy To -target folder. */

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
	LRESULT OnNotify(LPARAM);

	LRESULT WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void DoDefaultAction(int sel);
private:
	void OpenSelection(LPCTSTR szCompareAs, UINT idCompareAs);
	bool GetSelectedItems(int *sel1, int *sel2);
	void OpenParentDirectory();
	bool IsItemNavigableDiff(const DIFFITEM &) const;
	void MoveFocus(int, int);
	void SaveColumnWidths();
	void SaveColumnOrders();
	void FixReordering();
	void HeaderContextMenu(POINT);
	void ListContextMenu(POINT);
	HMENU ListShellContextMenu(SIDE_TYPE);
	void ReloadColumns();
	void ResetColumnWidths();
	void DeleteChildren(const DIFFITEM &, int);
	int CollapseSubdir(int);
	int ExpandSubdir(int);
	void DeepExpandSubdir(int);
	void PrepareDragData(UniStdioFile &);
};

#endif // !defined(AFX_DirView_H__16E7C721_351C_11D1_95CD_444553540000__INCLUDED_)
