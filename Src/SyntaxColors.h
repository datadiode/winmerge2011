/** 
 * @file  SyntaxColors.h
 *
 * @brief Declaration file for SyntaxColors class
 */
#pragma once

/** 
 * @brief Indexes to color table
 */
enum COLORINDEX
{
	//
	COLORINDEX_NONE,
    //  Base colors
    COLORINDEX_WHITESPACE,
    COLORINDEX_BKGND,
    COLORINDEX_NORMALTEXT,
    COLORINDEX_SELMARGIN,
    COLORINDEX_SELBKGND,
    COLORINDEX_SELTEXT,
    //  Syntax colors
    COLORINDEX_KEYWORD,
    COLORINDEX_FUNCNAME,
    COLORINDEX_COMMENT,
    COLORINDEX_NUMBER,
    COLORINDEX_OPERATOR,
    COLORINDEX_STRING,
    COLORINDEX_PREPROCESSOR,
    //
    COLORINDEX_HIGHLIGHTBKGND1, // standard
    COLORINDEX_HIGHLIGHTTEXT1,
    COLORINDEX_HIGHLIGHTBKGND2, // changed
    COLORINDEX_HIGHLIGHTTEXT2,
    COLORINDEX_HIGHLIGHTBKGND3, //  not selected insert/delete
	COLORINDEX_HIGHLIGHTBKGND4, // selected insert/delete
	//
	COLORINDEX_USER1,
    COLORINDEX_USER2,
    //  ...
    //  Expandable: custom elements are allowed.
	COLORINDEX_LAST, // Please keep this as last item (not counting masks or
	                 // other special values)
    //
    COLORINDEX_APPLYFORCE = 0x80000000,
};

const int COLORINDEX_COUNT = COLORINDEX_LAST - COLORINDEX_NONE;

/** 
 * @brief Wrapper for Syntax coloring colors.
 *
 * This class is wrapper for syntax colors. We can use this class in editor
 * class and everywhere we need to refer to syntax colors. Class uses our
 * normal options-manager for loading / saving values to storage.
 *
 * @todo We don't really need those arrays to store color values since we now
 * use options-manager.
 */
extern class CSyntaxColors
{
public:
	COLORREF GetColor(UINT index) const { return m_colors[index]; }
	void SetColor(UINT index, COLORREF color);
	bool GetBold(UINT index) const { return m_bolds[index]; }
	void SetBold(UINT index, bool bold);
	void SetDefault(UINT index);
	void LoadFromRegistry();
	void SaveToRegistry();
	bool ChooseColor(HWND hDlg, int nIDDlgItem);
	DWORD ExportOptions(LPCTSTR filename);
	DWORD ImportOptions(LPCTSTR filename);
// Implementation methods
private:
	bool IsThemeableColorIndex(UINT nColorIndex) const;
	bool GetSystemColorIndex(UINT nColorIndex, int * pSysIndex) const;

// Implementation data
private:
	COLORREF m_colors[COLORINDEX_COUNT]; /**< Syntax highlight colors */
	COLORREF m_rgCustColors[16];
	bool m_bolds[COLORINDEX_COUNT]; /**< Bold font enable/disable */
} SyntaxColors;
