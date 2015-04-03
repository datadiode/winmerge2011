/** 
 * @file  OptionsDef.h
 *
 * @brief Constants for option-names
 */
#ifndef operator
#define operator(args)
#endif

// User's language
extern COptionDef
<int> OPT_SELECTED_LANGUAGE operator((_T("Locale/LanguageId"), 0x409));

// View-menu
extern COptionDef
<bool> OPT_SHOW_UNIQUE_LEFT operator((_T("Settings/ShowUniqueLeft"), true));
extern COptionDef
<bool> OPT_SHOW_UNIQUE_RIGHT operator((_T("Settings/ShowUniqueRight"), true));
extern COptionDef
<bool> OPT_SHOW_DIFFERENT operator((_T("Settings/ShowDifferent"), true));
extern COptionDef
<bool> OPT_SHOW_IDENTICAL operator((_T("Settings/ShowIdentical"), true));
extern COptionDef
<bool> OPT_SHOW_BINARIES operator((_T("Settings/ShowBinaries"), true));
extern COptionDef
<bool> OPT_SHOW_SKIPPED operator((_T("Settings/ShowSkipped"), false));

// Show/hide toolbar/statusbar/tabbar
extern COptionDef
<bool> OPT_SHOW_TOOLBAR operator((_T("Settings/ShowToolbar"), true));
extern COptionDef
<bool> OPT_SHOW_STATUSBAR operator((_T("Settings/ShowStatusbar"), true));
extern COptionDef
<bool> OPT_SHOW_TABBAR operator((_T("Settings/ShowTabbar"), true));
extern COptionDef
<int> OPT_SHOW_LOCATIONBAR operator((_T("Settings/ShowLocationBar"), 1));
extern COptionDef
<int> OPT_SHOW_DIFFVIEWBAR operator((_T("Settings/ShowDiffViewBar"), 1));

extern COptionDef
<int> OPT_TOOLBAR_SIZE operator((_T("Settings/ToolbarSize"), 0));
extern COptionDef
<bool> OPT_RESIZE_PANES operator((_T("Settings/AutoResizePanes"), false));

extern COptionDef
<bool> OPT_SYNTAX_HIGHLIGHT operator((_T("Settings/HiliteSyntax"), true));
extern COptionDef
<bool> OPT_DISABLE_SPLASH operator((_T("Settings/DisableSplash"), false));
extern COptionDef
<bool> OPT_VIEW_WHITESPACE operator((_T("Settings/ViewWhitespace"), false));
extern COptionDef
<int> OPT_CONNECT_MOVED_BLOCKS operator((_T("Settings/ConnectMovedBlocks"), 0));
extern COptionDef
<bool> OPT_SCROLL_TO_FIRST operator((_T("Settings/ScrollToFirst"), false));

// Difference (in-line) highlight
extern COptionDef
<bool> OPT_WORDDIFF_HIGHLIGHT operator((_T("Settings/HiliteWordDiff"), true));
extern COptionDef
<bool> OPT_BREAK_ON_WORDS operator((_T("Settings/BreakOnWords"), true));
extern COptionDef
<int> OPT_BREAK_TYPE operator((_T("Settings/BreakType"), 0));
extern COptionDef
<String> OPT_BREAK_SEPARATORS operator((_T("Settings/HiliteBreakSeparators"), _T(".,:;")));

// Backup options
extern COptionDef
<bool> OPT_BACKUP_FOLDERCMP operator((_T("Backup/EnableFolder"), false));
extern COptionDef
<bool> OPT_BACKUP_FILECMP operator((_T("Backup/EnableFile"), true));
extern COptionDef
<int> OPT_BACKUP_LOCATION operator((_T("Backup/Location"), 0));
extern COptionDef
<String> OPT_BACKUP_GLOBALFOLDER operator((_T("Backup/GlobalFolder"), _T("")));
extern COptionDef
<bool> OPT_BACKUP_ADD_BAK operator((_T("Backup/NameAddBak"), true));
extern COptionDef
<bool> OPT_BACKUP_ADD_TIME operator((_T("Backup/NameAddTime"), false));

extern COptionDef
<int> OPT_DIRVIEW_SORT_COLUMN operator((_T("Settings/DirViewSortCol"), -1));
extern COptionDef
<bool> OPT_DIRVIEW_SORT_ASCENDING operator((_T("Settings/DirViewSortAscending"), true));
extern COptionDef
<bool> OPT_DIRVIEW_ENABLE_SHELL_CONTEXT_MENU operator((_T("Settings/DirViewEnableShellContextMenu"), false));

// File compare
extern COptionDef
<bool> OPT_AUTOMATIC_RESCAN operator((_T("Settings/AutomaticRescan"), false));
extern COptionDef
<bool> OPT_ALLOW_MIXED_EOL operator((_T("Settings/AllowMixedEOL"), false));
extern COptionDef
<bool> OPT_SEPARATE_COMBINING_CHARS operator((_T("Settings/SeparateCombiningChars"), false));
extern COptionDef
<int> OPT_TAB_SIZE operator((_T("Settings/TabSize"), 4));
extern COptionDef
<int> OPT_TAB_TYPE operator((_T("Settings/TabType"), 0));
extern COptionDef
<bool> OPT_WORDWRAP operator((_T("Settings/WordWrap"), false));
extern COptionDef
<bool> OPT_VIEW_LINENUMBERS operator((_T("Settings/ViewLineNumbers"), false));
extern COptionDef
<bool> OPT_VIEW_FILEMARGIN operator((_T("Settings/ViewFileMargin"), false));
extern COptionDef
<bool> OPT_MERGEEDITVIEW_ENABLE_SHELL_CONTEXT_MENU operator((_T("Settings/MergeEditViewEnableShellContextMenu"), false));

extern COptionDef
<bool> OPT_AUTOMATIC_BPL operator((_T("Settings/AutomaticBPL"), true));
extern COptionDef
<int> OPT_BYTES_PER_LINE operator((_T("Settings/BytesPerLine"), 16));

extern COptionDef
<String> OPT_EXT_EDITOR_CMD operator((_T("Settings/ExternalEditor"), CMergeApp::GetDefaultEditor()));
extern COptionDef
<bool> OPT_USE_RECYCLE_BIN operator((_T("Settings/UseRecycleBin"), true));
extern COptionDef
<bool> OPT_SINGLE_INSTANCE operator((_T("Settings/SingleInstance"), false));
extern COptionDef
<bool> OPT_MERGE_MODE operator((_T("Settings/MergingMode"), false));
extern COptionDef
<bool> OPT_CLOSE_WITH_ESC operator((_T("Settings/CloseWithEsc"), true));
extern COptionDef
<int> OPT_LOGGING operator((_T("Settings/Logging"), 0));
extern COptionDef
<bool> OPT_VERIFY_OPEN_PATHS operator((_T("Settings/VerifyOpenPaths"), true));
extern COptionDef
<int> OPT_AUTO_COMPLETE_SOURCE operator((_T("Settings/AutoCompleteSource"), 0));
extern COptionDef
<bool> OPT_IGNORE_SMALL_FILETIME operator((_T("Settings/IgnoreSmallFileTime"), false));
extern COptionDef
<bool> OPT_ASK_MULTIWINDOW_CLOSE operator((_T("Settings/AskClosingMultipleWindows"), false));
extern COptionDef
<bool> OPT_PRESERVE_FILETIMES operator((_T("Settings/PreserveFiletimes"), false));
extern COptionDef
<bool> OPT_TREE_MODE operator((_T("Settings/TreeMode"), false));

extern COptionDef
<int> OPT_CP_DEFAULT_MODE operator((_T("Settings/CodepageDefaultMode"), 0));
extern COptionDef
<int> OPT_CP_DEFAULT_CUSTOM operator((_T("Settings/CodepageDefaultCustomValue"), GetACP()));
extern COptionDef
<bool> OPT_CP_DETECT operator((_T("Settings/CodepageDetection"), false));

extern COptionDef
<int> OPT_READ_ONLY operator((_T("Settings/ReadOnly"), 0));

extern COptionDef
<String> OPT_PROJECTS_PATH operator((_T("Settings/ProjectsPath"), _T("")));
extern COptionDef
<bool> OPT_USE_SYSTEM_TEMP_PATH operator((_T("Settings/UseSystemTempPath"), true));
extern COptionDef
<String> OPT_CUSTOM_TEMP_PATH operator((_T("Settings/CustomTempPath"), _T("")));

// Color options
// The difference color
extern COptionDef
<COLORREF> OPT_CLR_DIFF operator((_T("Settings/DifferenceColor"), RGB(239,203,5)));
// The selected difference color
extern COptionDef
<COLORREF> OPT_CLR_SELECTED_DIFF operator((_T("Settings/SelectedDifferenceColor"), RGB(239,119,116)));
// The difference deleted color
extern COptionDef
<COLORREF> OPT_CLR_DIFF_DELETED operator((_T("Settings/DifferenceDeletedColor"), RGB(192, 192, 192)));
// The selected difference deleted color
extern COptionDef
<COLORREF> OPT_CLR_SELECTED_DIFF_DELETED operator((_T("Settings/SelectedDifferenceDeletedColor"), RGB(240, 192, 192)));
// The difference text color
extern COptionDef
<COLORREF> OPT_CLR_DIFF_TEXT operator((_T("Settings/DifferenceTextColor"), RGB(0,0,0)));
// The selected difference text color
extern COptionDef
<COLORREF> OPT_CLR_SELECTED_DIFF_TEXT operator((_T("Settings/SelectedDifferenceTextColor"), RGB(0,0,0)));
// The ignored lines color
extern COptionDef
<COLORREF> OPT_CLR_TRIVIAL_DIFF operator((_T("Settings/TrivialDifferenceColor"), RGB(251,242,191)));
// The ignored and deleted lines color
extern COptionDef
<COLORREF> OPT_CLR_TRIVIAL_DIFF_DELETED operator((_T("Settings/TrivialDifferenceDeletedColor"), RGB(233,233,233)));
// The ignored text color
extern COptionDef
<COLORREF> OPT_CLR_TRIVIAL_DIFF_TEXT operator((_T("Settings/TrivialDifferenceTextColor"), RGB(0,0,0)));
// The moved block color
extern COptionDef
<COLORREF> OPT_CLR_MOVEDBLOCK operator((_T("Settings/MovedBlockColor"), RGB(228,155,82)));
// The moved block deleted lines color
extern COptionDef
<COLORREF> OPT_CLR_MOVEDBLOCK_DELETED operator((_T("Settings/MovedBlockDeletedColor"), RGB(192, 192, 192)));
// The moved block text color
extern COptionDef
<COLORREF> OPT_CLR_MOVEDBLOCK_TEXT operator((_T("Settings/MovedBlockTextColor"), RGB(0,0,0)));
// The selected moved block color
extern COptionDef
<COLORREF> OPT_CLR_SELECTED_MOVEDBLOCK operator((_T("Settings/SelectedMovedBlockColor"), RGB(248,112,78)));
// The selected moved block deleted lines
extern COptionDef
<COLORREF> OPT_CLR_SELECTED_MOVEDBLOCK_DELETED operator((_T("Settings/SelectedMovedBlockDeletedColor"), RGB(252, 181, 163)));
// The selected moved block text color
extern COptionDef
<COLORREF> OPT_CLR_SELECTED_MOVEDBLOCK_TEXT operator((_T("Settings/SelectedMovedBlockTextColor"), RGB(0,0,0)));
// The word difference color
extern COptionDef
<COLORREF> OPT_CLR_WORDDIFF operator((_T("Settings/WordDifferenceColor"), RGB(241,226,173)));
// The word difference deleted color
extern COptionDef
<COLORREF> OPT_CLR_WORDDIFF_DELETED operator((_T("Settings/WordDifferenceDeletedColor"), RGB(255,170,130)));
// The word difference text color
extern COptionDef
<COLORREF> OPT_CLR_WORDDIFF_TEXT operator((_T("Settings/WordDifferenceTextColor"), RGB(0,0,0)));
// The selected word difference color
extern COptionDef
<COLORREF> OPT_CLR_SELECTED_WORDDIFF operator((_T("Settings/SelectedWordDifferenceColor"), RGB(255,160,160)));
// The word difference deleted color
extern COptionDef
<COLORREF> OPT_CLR_SELECTED_WORDDIFF_DELETED operator((_T("Settings/SelectedWordDifferenceDeletedColor"), RGB(200,129,108)));
// The selected word difference text color
extern COptionDef
<COLORREF> OPT_CLR_SELECTED_WORDDIFF_TEXT operator((_T("Settings/SelectedWordDifferenceTextColor"), RGB(0,0,0)));
// Whether to cross-hatch deleted lines
extern COptionDef
<bool> OPT_CROSS_HATCH_DELETED_LINES operator((_T("Settings/CrossHatchDeletedLines"), false));
// Whether to use default (theme) text colors
extern COptionDef
<bool> OPT_CLR_DEFAULT_TEXT_COLORING operator((_T("Settings/DefaultTextColoring"), true));
// Whether to use default (theme) list colors
extern COptionDef
<bool> OPT_CLR_DEFAULT_LIST_COLORING operator((_T("Settings/DefaultListColoring"), true));
extern COptionDef
<COLORREF> OPT_LIST_LEFTONLY_BKGD_COLOR operator((_T("Settings/LeftOnlyColor"), RGB(0xFF, 0xCC, 0xCC)));
extern COptionDef
<COLORREF> OPT_LIST_RIGHTONLY_BKGD_COLOR operator((_T("Settings/RightOnlyColor"), RGB(0xCC, 0xFF, 0xCC)));
extern COptionDef
<COLORREF> OPT_LIST_SUSPICIOUS_BKGD_COLOR operator((_T("Settings/SuspiciousColor"), RGB(0xFF, 0xFF, 0x80)));
// Compare options
extern COptionDef
<int> OPT_CMP_IGNORE_WHITESPACE operator((_T("Settings/IgnoreSpace"), 0));
extern COptionDef
<bool> OPT_CMP_IGNORE_BLANKLINES operator((_T("Settings/IgnoreBlankLines"), false));
extern COptionDef
<bool> OPT_CMP_FILTER_COMMENTLINES operator((_T("Settings/FilterCommentsLines"), false));
extern COptionDef
<bool> OPT_CMP_IGNORE_CASE operator((_T("Settings/IgnoreCase"), false));
extern COptionDef
<bool> OPT_CMP_IGNORE_EOL operator((_T("Settings/IgnoreEol"), false));
extern COptionDef
<int> OPT_CMP_METHOD operator((_T("Settings/CompMethod"), CMP_CONTENT));
extern COptionDef
<bool> OPT_CMP_MOVED_BLOCKS operator((_T("Settings/MovedBlocks"), false));
extern COptionDef
<bool> OPT_CMP_MATCH_SIMILAR_LINES operator((_T("Settings/MatchSimilarLines"), false));
extern COptionDef
<int> OPT_CMP_MATCH_SIMILAR_LINES_MAX operator((_T("Settings/MatchSimilarLinesMax"), 15));
extern COptionDef
<bool> OPT_CMP_STOP_AFTER_FIRST operator((_T("Settings/StopAfterFirst"), false));
extern COptionDef
<UINT> OPT_CMP_QUICK_LIMIT operator((_T("Settings/QuickMethodLimit"), 4 * 1024 * 1024));
extern COptionDef
<int> OPT_CMP_COMPARE_THREADS operator((_T("Settings/CompareThreads"), -1));
extern COptionDef
<bool> OPT_CMP_SELF_COMPARE operator((_T("Settings/SelfCompare"), true));
extern COptionDef
<bool> OPT_CMP_WALK_UNIQUES operator((_T("Settings/WalkUniqueDirs"), true));
extern COptionDef
<bool> OPT_CMP_CACHE_RESULTS operator((_T("Settings/CacheResults"), true));

// Multidoc enable/disable per document type
extern COptionDef
<bool> OPT_MULTIDOC_DIRDOCS operator((_T("Settings/MultiDirDocs"), false));
extern COptionDef
<bool> OPT_MULTIDOC_MERGEDOCS operator((_T("Settings/MultiMergeDocs"), true));

/// Are regular expression linefilters enabled?
extern COptionDef
<bool> OPT_LINEFILTER_ENABLED operator((_T("Settings/IgnoreRegExp"), false));
/// Currently selected filefilter
extern COptionDef
<String> OPT_FILEFILTER_CURRENT operator((_T("Settings/FileFilterCurrent"), _T("*.*")));
extern COptionDef
<String> OPT_SUPPLEMENT_FOLDER operator((_T("Settings/SupplementFolder"), CMergeApp::GetDefaultSupplementFolder()));
extern COptionDef
<bool> OPT_FILEFILTER_SHARED operator((_T("Settings/Filters/Shared"), false));

// Version control
extern COptionDef
<int> OPT_VCS_SYSTEM operator((_T("Settings/VersionSystem"), VCS_NONE));
extern COptionDef
<String> OPT_VSS_PATH operator((_T("Settings/VssPath"), _T("")));

// Archive support
extern COptionDef
<bool> OPT_ARCHIVE_ENABLE operator((_T("Merge7z/Enable"), true));
extern COptionDef
<bool> OPT_ARCHIVE_PROBETYPE operator((_T("Merge7z/ProbeSignature"), false));

// Startup options
extern COptionDef
<bool> OPT_SHOW_SELECT_FILES_AT_STARTUP operator((_T("Settings/ShowFileDialog"), false));

// Font options
extern COptionDef
<LogFont> OPT_FONT_FILECMP_LOGFONT operator((_T("Settings/Font"), 0));
extern COptionDef
<LogFont> OPT_FONT_DIRCMP_LOGFONT operator((_T("Settings/FontDirCompare"), 0));

#undef operator
