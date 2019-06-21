/**
 * @file  OptionsDef.h
 *
 * @brief Constants for option-names
 */
#ifndef inline
#define inline(args)

/** @enum COMPARE_TYPE
 * @brief Different foldercompare methods.
 * These values are the foldercompare methods WinMerge supports.
 */

/** @var CMP_CONTENT
 * @brief Normal by content compare.
 * This compare type is first, default and all-seeing compare type.
 * diffutils is used for producing compare results. So all limitations
 * of diffutils (like buffering) apply to this compare method. But this
 * is also currently only compare method that produces difference lists
 * we can use in file compare.
 */

/** @var CMP_QUICK_CONTENT
 * @brief Faster byte per byte -compare.
 * This type of compare was a response for needing faster compare results
 * in folder compare. It independent from diffutils, and fully customised
 * for WinMerge. It basically does byte-per-byte compare, still implementing
 * different whitespace ignore options.
 *
 * Optionally this compare type can be stopped when first difference is found.
 * Which gets compare as fast as possible. But misses sometimes binary files
 * if zero bytes aren't found before first difference. Also difference counts
 * are not useful with that option.
 */

/** @var CMP_DATE
 * @brief Compare by modified date.
 * This compare type was added after requests and realization that in some
 * situations difference in file's timestamps is enough to judge them
 * different. E.g. when modifying files in local machine, file timestamps
 * are certainly different after modifying them. This method doesn't even
 * open files for reading them. It only reads file's infos for timestamps
 * and compares them.
 *
 * This is no doubt fastest way to compare files.
 */

/** @var CMP_DATE_SIZE
 * @brief Compare by date and then by size.
 * This method is basically same than CMP_DATE, but it adds check for file
 * sizes if timestamps are identical. This is because there are situations
 * timestamps can't be trusted alone, especially with network shares. Adding
 * checking for file sizes adds some more reliability for results with
 * minimal increase in compare time.
 */

/** @var CMP_SIZE
 * @brief Compare by file size.
 * This compare method compares file sizes. This isn't quite accurate method,
 * other than it can detect files that certainly differ. But it can show lot of
 * different files as identical too. Advantage is in some use cases where different
 * size always means files are different. E.g. automatically created logs - when
 * more data is added size increases.
 */
enum COMPARE_TYPE
{
	CMP_CONTENT,
	CMP_QUICK_CONTENT,
	CMP_DATE,
	CMP_DATE_SIZE,
	CMP_SIZE,
};

/**
 * @brief TAB handling options
 */
enum
{
	/* Radio options */
	TAB_TYPE_INSERT_TABS,
	TAB_TYPE_INSERT_SPACES,
	/* Check options */
	TAB_TYPE_HONOR_MODELINES = 0x02,        /**< honor embedded modelines */
	TAB_TYPE_HONOR_EDITOR_CONFIG = 0x04,    /**< honor .editorconfig files */
	/* Masks to separate between radio & check options */
	TAB_TYPE_CHECK_OPTIONS_MASK = 0x08 - TAB_TYPE_HONOR_MODELINES,
	TAB_TYPE_RADIO_OPTIONS_MASK = TAB_TYPE_HONOR_MODELINES - 0x01,
};

/**
 * @brief Options to control some phony effects with no actual relevance
 */
enum
{
	PHONY_EFFECTS_DISABLE_SPLASH = 0x01,
	PHONY_EFFECTS_ENABLE_SCROLL_ANIMATION = 0x02,
};

#endif

// User's language
extern COptionDef
<int> OPT_SELECTED_LANGUAGE inline((_T("Locale/LanguageId"), 0x409));

// View-menu
extern COptionDef
<bool> OPT_SHOW_UNIQUE_LEFT inline((_T("Settings/ShowUniqueLeft"), true));
extern COptionDef
<bool> OPT_SHOW_UNIQUE_RIGHT inline((_T("Settings/ShowUniqueRight"), true));
extern COptionDef
<bool> OPT_SHOW_DIFFERENT inline((_T("Settings/ShowDifferent"), true));
extern COptionDef
<bool> OPT_SHOW_IDENTICAL inline((_T("Settings/ShowIdentical"), true));
extern COptionDef
<bool> OPT_SHOW_BINARIES inline((_T("Settings/ShowBinaries"), true));
extern COptionDef
<bool> OPT_SHOW_SKIPPED inline((_T("Settings/ShowSkipped"), false));

// Show/hide toolbar/statusbar/tabbar
extern COptionDef
<bool> OPT_SHOW_TOOLBAR inline((_T("Settings/ShowToolbar"), true));
extern COptionDef
<bool> OPT_SHOW_STATUSBAR inline((_T("Settings/ShowStatusbar"), true));
extern COptionDef
<bool> OPT_SHOW_TABBAR inline((_T("Settings/ShowTabbar"), true));
extern COptionDef
<int> OPT_SHOW_LOCATIONBAR inline((_T("Settings/ShowLocationBar"), 1));
extern COptionDef
<int> OPT_SHOW_DIFFVIEWBAR inline((_T("Settings/ShowDiffViewBar"), 1));

extern COptionDef
<int> OPT_TOOLBAR_SIZE inline((_T("Settings/ToolbarSize"), 0));
extern COptionDef
<bool> OPT_RESIZE_PANES inline((_T("Settings/AutoResizePanes"), false));

extern COptionDef
<bool> OPT_SYNTAX_HIGHLIGHT inline((_T("Settings/HiliteSyntax"), true));
extern COptionDef
<int> OPT_PHONY_EFFECTS inline((_T("Settings/PhonyEffects"), 0));
extern COptionDef
<bool> OPT_VIEW_WHITESPACE inline((_T("Settings/ViewWhitespace"), false));
extern COptionDef
<int> OPT_CONNECT_MOVED_BLOCKS inline((_T("Settings/ConnectMovedBlocks"), 0));
extern COptionDef
<bool> OPT_SCROLL_TO_FIRST inline((_T("Settings/ScrollToFirst"), false));

// Difference (in-line) highlight
extern COptionDef
<bool> OPT_WORDDIFF_HIGHLIGHT inline((_T("Settings/HiliteWordDiff"), true));
extern COptionDef
<bool> OPT_CHAR_LEVEL inline((_T("Settings/CharLevel"), false));
extern COptionDef
<int> OPT_BREAK_TYPE inline((_T("Settings/BreakType"), 0));
extern COptionDef
<String> OPT_BREAK_SEPARATORS inline((_T("Settings/HiliteBreakSeparators"), _T(".,:;")));

// Backup options
extern COptionDef
<bool> OPT_BACKUP_FOLDERCMP inline((_T("Backup/EnableFolder"), false));
extern COptionDef
<bool> OPT_BACKUP_FILECMP inline((_T("Backup/EnableFile"), true));
extern COptionDef
<int> OPT_BACKUP_LOCATION inline((_T("Backup/Location"), 0));
extern COptionDef
<String> OPT_BACKUP_GLOBALFOLDER inline((_T("Backup/GlobalFolder"), _T("")));
extern COptionDef
<bool> OPT_BACKUP_ADD_BAK inline((_T("Backup/NameAddBak"), true));
extern COptionDef
<bool> OPT_BACKUP_ADD_TIME inline((_T("Backup/NameAddTime"), false));

extern COptionDef
<int> OPT_DIRVIEW_SORT_COLUMN inline((_T("Settings/DirViewSortCol"), -1));
extern COptionDef
<bool> OPT_DIRVIEW_SORT_ASCENDING inline((_T("Settings/DirViewSortAscending"), true));
extern COptionDef
<bool> OPT_DIRVIEW_ENABLE_SHELL_CONTEXT_MENU inline((_T("Settings/DirViewEnableShellContextMenu"), false));

// File compare
extern COptionDef
<bool> OPT_AUTOMATIC_RESCAN inline((_T("Settings/AutomaticRescan"), false));
extern COptionDef
<bool> OPT_ALLOW_MIXED_EOL inline((_T("Settings/AllowMixedEOL"), false));
extern COptionDef
<bool> OPT_SEPARATE_COMBINING_CHARS inline((_T("Settings/SeparateCombiningChars"), false));
extern COptionDef
<int> OPT_TAB_SIZE inline((_T("Settings/TabSize"), 4));
extern COptionDef
<int> OPT_TAB_TYPE inline((_T("Settings/TabType"), 0));
extern COptionDef
<bool> OPT_WORDWRAP inline((_T("Settings/WordWrap"), false));
extern COptionDef
<bool> OPT_VIEW_LINENUMBERS inline((_T("Settings/ViewLineNumbers"), false));
extern COptionDef
<bool> OPT_VIEW_FILEMARGIN inline((_T("Settings/ViewFileMargin"), false));
extern COptionDef
<bool> OPT_MERGEEDITVIEW_ENABLE_SHELL_CONTEXT_MENU inline((_T("Settings/MergeEditViewEnableShellContextMenu"), false));

extern COptionDef
<bool> OPT_AUTOMATIC_BPL inline((_T("Settings/AutomaticBPL"), true));
extern COptionDef
<int> OPT_BYTES_PER_LINE inline((_T("Settings/BytesPerLine"), 16));

extern COptionDef
<String> OPT_EXT_EDITOR_CMD inline((_T("Settings/ExternalEditor"), CMergeApp::GetDefaultEditor()));
extern COptionDef
<bool> OPT_USE_SHELL_FILE_OPERATIONS inline((_T("Settings/UseShellFileOperations"), true));
extern COptionDef
<bool> OPT_USE_SHELL_FILE_BROOWSE_DIALOGS inline((_T("Settings/UseShellFileBrowseDialogs"), true));
extern COptionDef
<bool> OPT_USE_RECYCLE_BIN inline((_T("Settings/UseRecycleBin"), true));
extern COptionDef
<bool> OPT_SINGLE_INSTANCE inline((_T("Settings/SingleInstance"), false));
extern COptionDef
<bool> OPT_MERGE_MODE inline((_T("Settings/MergingMode"), false));
extern COptionDef
<bool> OPT_CLOSE_WITH_ESC inline((_T("Settings/CloseWithEsc"), true));
extern COptionDef
<int> OPT_LOGGING inline((_T("Settings/Logging"), 0));
extern COptionDef
<bool> OPT_VERIFY_OPEN_PATHS inline((_T("Settings/VerifyOpenPaths"), true));
extern COptionDef
<int> OPT_AUTO_COMPLETE_SOURCE inline((_T("Settings/AutoCompleteSource"), 0));
extern COptionDef
<bool> OPT_IGNORE_SMALL_FILETIME inline((_T("Settings/IgnoreSmallFileTime"), false));
extern COptionDef
<bool> OPT_ASK_MULTIWINDOW_CLOSE inline((_T("Settings/AskClosingMultipleWindows"), false));
extern COptionDef
<bool> OPT_PRESERVE_FILETIMES inline((_T("Settings/PreserveFiletimes"), false));
extern COptionDef
<bool> OPT_TREE_MODE inline((_T("Settings/TreeMode"), false));

extern COptionDef
<int> OPT_CP_DEFAULT_MODE inline((_T("Settings/CodepageDefaultMode"), 0));
extern COptionDef
<int> OPT_CP_DEFAULT_CUSTOM inline((_T("Settings/CodepageDefaultCustomValue"), GetACP()));
extern COptionDef
<bool> OPT_CP_DETECT inline((_T("Settings/CodepageDetection"), false));

extern COptionDef
<int> OPT_READ_ONLY inline((_T("Settings/ReadOnly"), 0));

extern COptionDef
<String> OPT_PROJECTS_PATH inline((_T("Settings/ProjectsPath"), _T("")));
extern COptionDef
<bool> OPT_USE_SYSTEM_TEMP_PATH inline((_T("Settings/UseSystemTempPath"), true));
extern COptionDef
<String> OPT_CUSTOM_TEMP_PATH inline((_T("Settings/CustomTempPath"), _T("")));

// Color options
// The difference color
extern COptionDef
<COLORREF> OPT_CLR_DIFF inline((_T("Settings/DifferenceColor"), RGB(239,203,5)));
// The selected difference color
extern COptionDef
<COLORREF> OPT_CLR_SELECTED_DIFF inline((_T("Settings/SelectedDifferenceColor"), RGB(239,119,116)));
// The difference deleted color
extern COptionDef
<COLORREF> OPT_CLR_DIFF_DELETED inline((_T("Settings/DifferenceDeletedColor"), RGB(192, 192, 192)));
// The selected difference deleted color
extern COptionDef
<COLORREF> OPT_CLR_SELECTED_DIFF_DELETED inline((_T("Settings/SelectedDifferenceDeletedColor"), RGB(240, 192, 192)));
// The difference text color
extern COptionDef
<COLORREF> OPT_CLR_DIFF_TEXT inline((_T("Settings/DifferenceTextColor"), RGB(0,0,0)));
// The selected difference text color
extern COptionDef
<COLORREF> OPT_CLR_SELECTED_DIFF_TEXT inline((_T("Settings/SelectedDifferenceTextColor"), RGB(0,0,0)));
// The ignored lines color
extern COptionDef
<COLORREF> OPT_CLR_TRIVIAL_DIFF inline((_T("Settings/TrivialDifferenceColor"), RGB(251,242,191)));
// The ignored and deleted lines color
extern COptionDef
<COLORREF> OPT_CLR_TRIVIAL_DIFF_DELETED inline((_T("Settings/TrivialDifferenceDeletedColor"), RGB(233,233,233)));
// The ignored text color
extern COptionDef
<COLORREF> OPT_CLR_TRIVIAL_DIFF_TEXT inline((_T("Settings/TrivialDifferenceTextColor"), RGB(0,0,0)));
// The moved block color
extern COptionDef
<COLORREF> OPT_CLR_MOVEDBLOCK inline((_T("Settings/MovedBlockColor"), RGB(228,155,82)));
// The moved block deleted lines color
extern COptionDef
<COLORREF> OPT_CLR_MOVEDBLOCK_DELETED inline((_T("Settings/MovedBlockDeletedColor"), RGB(192, 192, 192)));
// The moved block text color
extern COptionDef
<COLORREF> OPT_CLR_MOVEDBLOCK_TEXT inline((_T("Settings/MovedBlockTextColor"), RGB(0,0,0)));
// The selected moved block color
extern COptionDef
<COLORREF> OPT_CLR_SELECTED_MOVEDBLOCK inline((_T("Settings/SelectedMovedBlockColor"), RGB(248,112,78)));
// The selected moved block deleted lines
extern COptionDef
<COLORREF> OPT_CLR_SELECTED_MOVEDBLOCK_DELETED inline((_T("Settings/SelectedMovedBlockDeletedColor"), RGB(252, 181, 163)));
// The selected moved block text color
extern COptionDef
<COLORREF> OPT_CLR_SELECTED_MOVEDBLOCK_TEXT inline((_T("Settings/SelectedMovedBlockTextColor"), RGB(0,0,0)));
// The word difference color
extern COptionDef
<COLORREF> OPT_CLR_WORDDIFF inline((_T("Settings/WordDifferenceColor"), RGB(241,226,173)));
// The word difference deleted color
extern COptionDef
<COLORREF> OPT_CLR_WORDDIFF_DELETED inline((_T("Settings/WordDifferenceDeletedColor"), RGB(255,170,130)));
// The word difference text color
extern COptionDef
<COLORREF> OPT_CLR_WORDDIFF_TEXT inline((_T("Settings/WordDifferenceTextColor"), RGB(0,0,0)));
// The selected word difference color
extern COptionDef
<COLORREF> OPT_CLR_SELECTED_WORDDIFF inline((_T("Settings/SelectedWordDifferenceColor"), RGB(255,160,160)));
// The word difference deleted color
extern COptionDef
<COLORREF> OPT_CLR_SELECTED_WORDDIFF_DELETED inline((_T("Settings/SelectedWordDifferenceDeletedColor"), RGB(200,129,108)));
// The selected word difference text color
extern COptionDef
<COLORREF> OPT_CLR_SELECTED_WORDDIFF_TEXT inline((_T("Settings/SelectedWordDifferenceTextColor"), RGB(0,0,0)));
// Whether to cross-hatch deleted lines
extern COptionDef
<int> OPT_CROSS_HATCH_DELETED_LINES inline((_T("Settings/CrossHatchDeletedLines"), 0));
// Whether to use default (theme) text colors
extern COptionDef
<bool> OPT_CLR_DEFAULT_TEXT_COLORING inline((_T("Settings/DefaultTextColoring"), true));
// Whether to use default (theme) list colors
extern COptionDef
<bool> OPT_CLR_DEFAULT_LIST_COLORING inline((_T("Settings/DefaultListColoring"), false));
extern COptionDef
<COLORREF> OPT_LIST_LEFTONLY_BKGD_COLOR inline((_T("Settings/LeftOnlyColor"), RGB(0xFF, 0xCC, 0xCC)));
extern COptionDef
<COLORREF> OPT_LIST_RIGHTONLY_BKGD_COLOR inline((_T("Settings/RightOnlyColor"), RGB(0xCC, 0xFF, 0xCC)));
extern COptionDef
<COLORREF> OPT_LIST_SUSPICIOUS_BKGD_COLOR inline((_T("Settings/SuspiciousColor"), RGB(0xFF, 0xFF, 0x80)));
// Compare options
extern COptionDef
<int> OPT_CMP_IGNORE_WHITESPACE inline((_T("Settings/IgnoreSpace"), 0));
extern COptionDef
<bool> OPT_CMP_IGNORE_BLANKLINES inline((_T("Settings/IgnoreBlankLines"), false));
extern COptionDef
<bool> OPT_CMP_FILTER_COMMENTLINES inline((_T("Settings/FilterCommentsLines"), false));
extern COptionDef
<bool> OPT_CMP_IGNORE_CASE inline((_T("Settings/IgnoreCase"), false));
extern COptionDef
<bool> OPT_CMP_IGNORE_EOL inline((_T("Settings/IgnoreEol"), false));
extern COptionDef
<bool> OPT_CMP_MINIMAL inline((_T("Settings/Minimal"), false));
extern COptionDef
<bool> OPT_CMP_SPEED_LARGE_FILES inline((_T("Settings/SpeedLargeFiles"), true));
extern COptionDef
<bool> OPT_CMP_APPLY_HISTORIC_COST_LIMIT inline((_T("Settings/ApplyHistoricCostLimit"), false));
extern COptionDef
<int> OPT_CMP_METHOD inline((_T("Settings/CompMethod"), CMP_CONTENT));
extern COptionDef
<bool> OPT_CMP_MOVED_BLOCKS inline((_T("Settings/MovedBlocks"), false));
extern COptionDef
<bool> OPT_CMP_MATCH_SIMILAR_LINES inline((_T("Settings/MatchSimilarLines"), false));
extern COptionDef
<int> OPT_CMP_MATCH_SIMILAR_LINES_MAX inline((_T("Settings/MatchSimilarLinesMax"), 15));
extern COptionDef
<bool> OPT_CMP_STOP_AFTER_FIRST inline((_T("Settings/StopAfterFirst"), false));
extern COptionDef
<UINT> OPT_CMP_QUICK_LIMIT inline((_T("Settings/QuickMethodLimit"), 4 * 1024 * 1024));
extern COptionDef
<int> OPT_CMP_COMPARE_THREADS inline((_T("Settings/CompareThreads"), -1));
extern COptionDef
<bool> OPT_CMP_SELF_COMPARE inline((_T("Settings/SelfCompare"), false));
extern COptionDef
<bool> OPT_CMP_WALK_UNIQUES inline((_T("Settings/WalkUniqueDirs"), true));
extern COptionDef
<bool> OPT_CMP_CACHE_RESULTS inline((_T("Settings/CacheResults"), true));
extern COptionDef
<int> OPT_CMP_DIFF_ALGORITHM inline((_T("Settings/DiffAlgorithm"), 0));
extern COptionDef
<bool> OPT_CMP_INDENT_HEURISTIC inline((_T("Settings/IndentHeuristic"), false));

// Multidoc enable/disable per document type
extern COptionDef
<bool> OPT_MULTIDOC_DIRDOCS inline((_T("Settings/MultiDirDocs"), false));
extern COptionDef
<bool> OPT_MULTIDOC_MERGEDOCS inline((_T("Settings/MultiMergeDocs"), true));

/// Are regular expression linefilters enabled?
extern COptionDef
<bool> OPT_LINEFILTER_ENABLED inline((_T("Settings/IgnoreRegExp"), false));
/// Currently selected filefilter
extern COptionDef
<String> OPT_FILEFILTER_CURRENT inline((_T("Settings/FileFilterCurrent"), _T("*.*")));
extern COptionDef
<String> OPT_SUPPLEMENT_FOLDER inline((_T("Settings/SupplementFolder"), CMergeApp::GetDefaultSupplementFolder()));
extern COptionDef
<bool> OPT_FILEFILTER_SHARED inline((_T("Settings/FileFilterShared"), false));

// Version control
extern COptionDef
<int> OPT_VCS_SYSTEM inline((_T("Settings/VersionSystem"), VCS_NONE));
extern COptionDef
<String> OPT_VSS_PATH inline((_T("Settings/VssPath"), _T("")));
extern COptionDef
<String> OPT_CLEARTOOL_PATH inline((_T("Settings/ClearToolPath"), _T("")));
extern COptionDef
<String> OPT_TFS_PATH inline((_T("Settings/TfsPath"), _T("")));

// Archive support
extern COptionDef
<bool> OPT_ARCHIVE_ENABLE inline((_T("Merge7z/Enable"), true));
extern COptionDef
<bool> OPT_ARCHIVE_PROBETYPE inline((_T("Merge7z/ProbeSignature"), false));

// MRU
extern COptionDef
<String> OPT_ARCHIVE_MRU inline((_T("Files/ArchiveMRU"), _T("")));
extern COptionDef
<String> OPT_CONFLICT_MRU inline((_T("Files/ConflictMRU"), _T("")));
extern COptionDef
<String> OPT_REPORT_MRU inline((_T("Files/ReportMRU"), _T("")));

// Startup options
extern COptionDef
<bool> OPT_SHOW_SELECT_FILES_AT_STARTUP inline((_T("Settings/ShowFileDialog"), false));

// Font options
extern COptionDef
<LogFont> OPT_FONT_FILECMP_LOGFONT inline((_T("Settings/Font"), 0));
extern COptionDef
<LogFont> OPT_FONT_DIRCMP_LOGFONT inline((_T("Settings/FontDirCompare"), 0));

#undef inline
