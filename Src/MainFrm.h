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
 * @file  MainFrm.h
 *
 * @brief Declaration file for CMainFrame
 *
 */
#include "Constants.h"
#include "VSSHelper.h"
#include "MergeCmdLineInfo.h"
#include "../BuildTmp/Merge/midl/WinMerge_h.h"

enum { WM_NONINTERACTIVE = 888 }; // timer value

class CDiffView;
class CDirView;
class CDirFrame;
class CDocFrame;
class CHexMergeFrame;
class CMergeEditView;
class CMergeDiffDetailView;
class LineFiltersList;
struct FileLocation;

class PackingInfo;
class CLanguageSelect;

/**
 * @brief Frame class containing save-routines etc
 */
class CMainFrame
	: public ZeroInit<CMainFrame>
	, public OWindow
	, public IStatusDisplay
	, public CScriptable<IMergeApp>
{
	friend CLanguageSelect;
public:

	static const UINT WM_APP_ConsoleCtrlHandler = WM_APP + 1;

	CMainFrame(HWindow *, const MergeCmdLineInfo &);

	// IElementBehaviorFactory
	STDMETHOD(FindBehavior)(BSTR, BSTR, IElementBehaviorSite *, IElementBehavior **);
	// IMergeApp
	STDMETHOD(put_StatusText)(BSTR bsStatusText);
	STDMETHOD(get_StatusText)(BSTR *pbsStatusText);
	STDMETHOD(get_Strings)(IDispatch **ppDispatch);
	STDMETHOD(ShowHTMLDialog)(LPCOLESTR url, VARIANT *arguments, BSTR features, VARIANT *ret);
	STDMETHOD(ParseCmdLine)(BSTR cmdline, BSTR directory);

// Attributes
public:
	HWindow *m_pWndMDIClient;
	HMenu *m_pMenuDefault;
	HACCEL m_hAccelTable;
	LogFont m_lfDiff; /**< MergeView user-selected font */
	LogFont m_lfDir; /**< DirView user-selected font */
	CDirFrame *m_pCollectingDirFrame;
// Operations
public:
	void InitCmdUI();
	template<UINT CmdID>
	void UpdateCmdUI(BYTE);
	void UpdateSourceTypeUI(UINT sourceType) { m_sourceType = sourceType; }
	void InitialActivate(int nCmdShow);
	void EnableModeless(BOOL bEnable);
	void SetBitmaps(HMenu *);
	bool DoFileOpen(
		FileLocation &filelocLeft,
		FileLocation &filelocRight,
		DWORD dwLeftFlags = FFILEOPEN_DETECT,
		DWORD dwRightFlags = FFILEOPEN_DETECT,
		int nRecursive = 0,
		CDirFrame * = NULL);
	bool DoFileOpen(
		PackingInfo &packingInfo,
		UINT idCompareAs,
		FileLocation &filelocLeft,
		FileLocation &filelocRight,
		DWORD dwLeftFlags,
		DWORD dwRightFlags,
		int nRecursive = 0,
		CDirFrame * = NULL);
	CEditorFrame *ShowMergeDoc(CDirFrame *,
		FileLocation & filelocLeft,
		FileLocation & filelocRight,
		DWORD dwLeftFlags = 0,
		DWORD dwRightFlags = 0,
		PackingInfo * infoUnpacker = NULL,
		LPCTSTR sCompareAs = NULL);
	CHexMergeFrame *ShowHexMergeDoc(CDirFrame *,
		const FileLocation &,
		const FileLocation &,
		BOOL bLeftRO, BOOL bRightRO,
		LPCTSTR sCompareAs);
	void UpdateResources();
	static bool CreateBackup(bool bFolder, LPCTSTR pszPath);
	static String GetFileExt(LPCTSTR sFileName, LPCTSTR sDescription);
	int HandleReadonlySave(String &strSavePath, int choice = 0);
	void SetStatus(UINT);
	void SetStatus(LPCTSTR);
	void UpdateIndicators();
	void ApplyDiffOptions();
	void ApplyViewWhitespace();
	void SetEOLMixed(bool bAllow);
	bool SelectFilter();
	void ShowVSSError(HRESULT, LPCTSTR strItem);
	void ShowHelp(LPCTSTR helpLocation = NULL);
	void UpdateCodepageModule();
	void CheckinToClearCase(LPCTSTR strDestinationPath);
	void StartFlashing();
	bool AskCloseConfirmation();
	void DoOpenMrgman(LPCTSTR);
	bool DoOpenConflict(LPCTSTR);
	bool LoadAndOpenProjectFile(LPCTSTR);
	HWindow *CreateChildHandle() const;
	CDocFrame *GetActiveDocFrame(BOOL *pfActive = NULL);
	void SetActiveMenu(HMenu *);

	HMenu *SetScriptMenu(HMenu *, LPCSTR section);

	BYTE QueryCmdState(UINT id) const;

	static void OpenFileToExternalEditor(LPCTSTR file, LPCTSTR editor = NULL);
	void OpenFileWith(LPCTSTR file) const;
	bool ParseArgsAndDoOpen(const MergeCmdLineInfo &);
	void InitOptions();
	void UpdateDocFrameSettings(const CDocFrame *);
	void RecalcLayout();
	BOOL PreTranslateMessage(MSG *);

	typedef void (CALLBACK *DrawMenuProc)(DRAWITEMSTRUCT *);
	static void CALLBACK DrawMenuDefault(DRAWITEMSTRUCT *);
	static void CALLBACK DrawMenuCheckboxFrame(DRAWITEMSTRUCT *);

// Implementation methods
protected:
	virtual ~CMainFrame();
// Implementation in SourceControl.cpp
	void InitializeSourceControlMembers();
	int SaveToVersionControl(LPCTSTR pszSavePath);
// End SourceControl.cpp


// Public implementation data
public:
	VSSHelper m_vssHelper; /**< Helper class for VSS integration */
	MergeCmdLineInfo::InvocationMode m_invocationMode;
	MergeCmdLineInfo::ExitNoDiff m_bExitIfNoDiff; /**< Exit if files are identical? */

	/**
	 * @name Version Control System (VCS) integration.
	 */
	/*@{*/ 
protected:
	String m_strVssUser; /**< Visual Source Safe User ID */
	String m_strVssPassword; /**< Visual Source Safe Password */
	String m_strVssDatabase; /**< Visual Source Safe database */
	String m_strCCComment; /**< ClearCase comment */
public:
	BOOL m_bCheckinVCS;     /**< TRUE if files should be checked in after checkout */
	BOOL m_CheckOutMulti; /**< Suppresses VSS int. code asking checkout for every file */
	BOOL m_bVssSuppressPathCheck; /**< Suppresses VSS int code asking about different path */
	/*@}*/

	/** @brief Possible toolbar image sizes. */
	enum TOOLBAR_SIZE
	{
		TOOLBAR_SIZE_16x16,
		TOOLBAR_SIZE_32x32,
	};

	HTabCtrl *GetTabBar() { return m_wndTabBar; }
	HStatusBar *GetStatusBar() { return m_wndStatusBar; }
// Implementation data
protected:
	// control bar embedded members
	HStatusBar *m_wndStatusBar;
	HToolBar *m_wndToolBar;
	HTabCtrl *m_wndTabBar;
	HStatic *m_wndCloseBox;

	HImageList *m_imlMenu;
	HImageList *m_imlToolbarEnabled; /**< Images for toolbar */
	HImageList *m_imlToolbarDisabled; /**< Images for toolbar */

	/**
	 * A structure attaching a menu item, icon and menu types to apply to.
	 */
	struct MENUITEM_ICON
	{
		WORD menuitemID;   /**< Menu item's ID. */
		WORD iconResID;    /**< Icon's resource ID. */
	};

	static const MENUITEM_ICON m_MenuIcons[];

// Generated message map functions
protected:
	void GetMrgViewFontProperties();
	void GetDirViewFontProperties();
	template<UINT>
	LRESULT OnWndMsg(WPARAM, LPARAM);
	LRESULT WindowProc(UINT, WPARAM, LPARAM);
	void OnOptionsShowDifferent();
	void OnOptionsShowIdentical();
	void OnOptionsShowUniqueLeft();
	void OnOptionsShowUniqueRight();
	void OnOptionsShowBinaries();
	void OnOptionsShowSkipped();
	void OnFileOpen();
	void OnFileClose();
	void OnHelpGnulicense();
	void OnOptions();
	void OnViewSelectfont();
	void OnViewUsedefaultfont();
	void OnHelpContents();
	void OnViewWhitespace();
	void OnToolsGeneratePatch();
	void OnSaveConfigData();
	void OnFileNew();
	void OnDebugLoadConfig();
	void OnViewStatusBar();
	void OnViewToolbar();
	void OnViewTabBar();
	void OnResizePanes();
	void OnFileStartCollect();
	void OnFileOpenProject();
	void OnWindowCloseAll();
	void OnSaveProject();
	void OnDebugResetOptions();
	void OnToolbarNone();
	void OnToolbarSmall();
	void OnToolbarBig();
	void OnToolTipText(TOOLTIPTEXT *);
	void OnDiffOptionsDropDown(NMTOOLBAR *);
	void OnDiffIgnoreCase();
	void OnDiffIgnoreEOL();
	void OnDiffWhitespace(int);
	void OnCompareMethod(int);
	void OnHelpReleasenotes();
	void OnHelpTranslations();
	void OnFileOpenConflict();

private:
	void addToMru(LPCTSTR szItem, LPCTSTR szRegSubKey, UINT nMaxItems = 20);
	BOOL IsComparing();
	void RedisplayAllDirDocs();
	void SaveWindowPlacement();
	bool PrepareForClosing();
	bool CloseDocFrame(CDocFrame *);
	CChildFrame *GetMergeDocToShow(CDirFrame *);
	CHexMergeFrame *GetHexMergeDocToShow(CDirFrame *);
	CDirFrame *GetDirDocToShow();
	void UpdateDirViewFont();
	void UpdateMrgViewFont();
	void OpenFileOrUrl(LPCTSTR szFile, LPCTSTR szUrl);
	BOOL CreateToobar();
	void LoadToolbarImages();
	HMENU NewMenu(int ID);
	CDocFrame *FindFrameOfType(FRAMETYPE);
	CEditorFrame *ActivateOpenDoc(CDirFrame *, FileLocation &, FileLocation &, LPCTSTR, UINT, FRAMETYPE);

	void LoadFilesMRU();
	void SaveFilesMRU();

	String m_TitleMRU;
	std::vector<String> m_FilesMRU;

	String m_lastCollectFolder;

	bool m_bRemotelyInvoked; /**< Window was invoked through COM */
	bool m_bFlashing; /**< Window is flashing. */

	HMenu *m_pScriptMenu;
	String m_TitleScripts;
	std::vector<String> m_Scripts;

	class Strings : public CMyDispatch<IDispatch>
	{
	public:
		Strings() : CMyDispatch<IDispatch>(__uuidof(IStrings)) { }
		STDMETHOD(Invoke)(DISPID, REFIID, LCID, WORD, DISPPARAMS *, VARIANT *, EXCEPINFO *, UINT *);
	} m_Strings;

	struct CmdState
	{
		BYTE LeftReadOnly;
		BYTE RightReadOnly;
		BYTE Refresh;
		BYTE FileEncoding;
		BYTE TreeMode;
		BYTE ShowHiddenItems;
		BYTE ExpandAllSubdirs;
		BYTE MergeCompare;
		BYTE LeftToRight;
		BYTE RightToLeft;
		BYTE Delete;
		BYTE AllLeft;
		BYTE AllRight;
		BYTE PrevDiff;
		BYTE NextDiff;
		BYTE CurDiff;
		BYTE Save;
		BYTE SaveLeft;
		BYTE SaveRight;
		BYTE Undo;
		BYTE Redo;
		BYTE Cut;
		BYTE Copy;
		BYTE Paste;
		BYTE Replace;
		BYTE SelectLineDiff;
		BYTE EolToDos;
		BYTE EolToUnix;
		BYTE EolToMac;
		BYTE GenerateReport;
		BYTE CollectMode;
		BYTE CompareSelection;
		BYTE ToggleBookmark;
		BYTE NavigateBookmarks;
		BYTE SetSyncPoint;
		BYTE ClearSyncPoints;
		const BYTE *Lookup(UINT id) const;
	} m_cmdState;
	HRESULT m_hrRegister;
	DWORD m_dwRegister;
	UINT m_sourceType;
};

/////////////////////////////////////////////////////////////////////////////
