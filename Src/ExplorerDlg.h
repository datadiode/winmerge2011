/*
 * Copyright (c) 2017 Jochen Neubeck
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/**
 * @file  ExplorerDlg.h
 *
 * @brief Declaration file for ExplorerDlg class.
 */
#pragma once

#include "Common/SortHeaderCtrl.h"
#include "Common/FloatState.h"
#include "Common/SplitState.h"

class ExplorerDlg
	: ZeroInit<ExplorerDlg>
	, public OResizableDialog
	, protected CSplitState
{
public:
	ExplorerDlg(UINT idd);
	~ExplorerDlg();

	String m_title;
	String m_path;
	String m_filters;
	String m_filename;
	LPCWSTR m_defext;
	bool m_bEnableFolderSelect;
	bool m_bEnableMultiSelect;
	bool m_bWarnIfExists;

private:
	HTreeView *m_pTv;
	HListView *m_pLv;
	HTabCtrl *m_pTcFilter;
	HButton *m_pPbNewFolder;
	HComboBox *m_pCbFilter;
	HEdit *m_pEdFileName;
	IShellFolder *m_shell;
	IShellFolder *m_network;
	HTREEITEM m_hDrives;
	HTREEITEM m_hDocuments;
	HTREEITEM m_hNetwork;
	CSortHeaderCtrl m_ctlSortHeader;
    int m_nSortIndex;
    bool m_bSortAscending;
	bool m_bClosing;
	bool const m_bHasListView;
	UINT m_uChanged;

	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

	LRESULT WndProcFilter(WNDPROC, HWND, UINT, WPARAM, LPARAM);
	LRESULT OnSelchanged(NMTREEVIEW *);
	LRESULT OnItemExpanding(NMTREEVIEW *);
	LRESULT OnCustomDraw(NMTVCUSTOMDRAW *);
	LRESULT OnKeydown(NMTVKEYDOWN *);
	LRESULT OnEndLabelEdit(NMTVDISPINFO *);
	LRESULT OnDeleteItem(NMTREEVIEW *);
	LRESULT OnGetDispinfo(NMLVDISPINFO *);
	LRESULT OnCustomDraw(NMLVCUSTOMDRAW *);
	LRESULT OnColumnClick(NMLISTVIEW *);
	LRESULT OnItemChanged(NMLISTVIEW *);
	LRESULT OnDeleteItem(NMLISTVIEW *);
	LRESULT OnNotify(UNotify *);
	void UpdateSelection();
	void Refresh();
	void OnNewFolder();
	void OnOK();
	void OnCancel();
	BOOL EndLabelEditNow(BOOL);

	void loadColumns();
	void saveColumns();
	static int CALLBACK SortFunc(LPARAM, LPARAM, LPARAM);

	IShellFolder *GetShellFolder(HTREEITEM) const;
	void PopulateTreeItem(HTREEITEM, int cAssumeChildren = 0);
	void PopulateTreeView();
	HTREEITEM ExpandPartial(HTREEITEM, LPWSTR);
	void setSelectedFullPath(std::wstring const &);
	DWORD getPath(HTREEITEM, std::wstring &);
	LPWSTR getPath(LPITEMIDLIST, LPWSTR);
	static int getImage(LPITEMIDLIST);
	static int getImage(LPCWSTR, DWORD = FILE_ATTRIBUTE_DIRECTORY);
	HTREEITEM GetItemToExpand(HTREEITEM);
	void RefreshListView();
	void ToggleFilter(int);
};
