/** 
 * @file  SyntaxColors.cpp
 *
 * @brief Implementation for SyntaxColors class.
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "SettingStore.h"
#include "Common/RegKey.h"
#include "SyntaxColors.h"

/** @brief Setting name for default colors. */
static const TCHAR DefColorsPath[] =_T("DefaultSyntaxColors");
/** @brief Setting name for user-defined custom colors. */
static const TCHAR CustomColors[] = _T("Custom Colors");

CSyntaxColors SyntaxColors;

/**
 * @brief Initialize color table.
 * @param [in] pOptionsMgr pointer to OptionsMgr used as storage.
 */
void CSyntaxColors::LoadFromRegistry()
{
	if (CRegKeyEx key = SettingStore.GetSectionKey(DefColorsPath))
	{
		for (UINT i = COLORINDEX_NONE; i < COLORINDEX_LAST; i++)
		{
			SetDefault(i);
			TCHAR name[40];
			wsprintf(name, _T("Color%02u"), i);
			m_colors[i] = key.ReadDword(name, m_colors[i]);
			wsprintf(name, _T("Bold%02u"), i);
			m_bolds[i] = key.ReadDword(name, m_bolds[i]) != 0;
		}
	}
	if (CRegKeyEx key = SettingStore.GetSectionKey(CustomColors))
	{
		for (UINT i = 0; i < _countof(m_rgCustColors); i++)
		{
			TCHAR name[40];
			_itot(i, name, 10);
			m_rgCustColors[i] = key.ReadDword(name, m_rgCustColors[i]);
		}
	}
}

/**
 * @brief Checks if given color is themeable.
 * @param [in] nColorIndex Index of color to check.
 * @return true if color is themeable, false otherwise.
 */
bool CSyntaxColors::IsThemeableColorIndex(UINT nColorIndex) const
{
	int temp = 0;
	return GetSystemColorIndex(nColorIndex, &temp);
}

/**
 * @brief Get system color for this index.
 * Returns the system color for given index in the case it varies by
 * used theme.
 * @param [in] nColorIndex Index of color to get.
 * @param [out] pSysIndex System color index, if any.
 * @return true if system color index was found, false otherwise.
 */
bool CSyntaxColors::GetSystemColorIndex(UINT nColorIndex, int * pSysIndex) const
{
	switch (nColorIndex)
	{
	case COLORINDEX_WHITESPACE:
	case COLORINDEX_BKGND:
		*pSysIndex = COLOR_WINDOW;
		return true;
	case COLORINDEX_NORMALTEXT:
		*pSysIndex = COLOR_WINDOWTEXT;
		return true;
	case COLORINDEX_SELMARGIN:
		*pSysIndex = COLOR_SCROLLBAR;
		return true;
	}
	return false;
}

/**
 * @brief Set default color values.
 */
void CSyntaxColors::SetDefault(UINT i)
{
	COLORREF color;

	int nSysIndex = 0;
	if (GetSystemColorIndex(i, &nSysIndex))
	{
		// Colors that vary with Windows theme
		color = GetSysColor(nSysIndex);
	}
	else switch (i)
	{
		// Theme colors are handled above by GetSystemColorIndex
		//
		// COLORINDEX_WHITESPACE
		// COLORINDEX_BKGND:
		// COLORINDEX_NORMALTEXT:
		// COLORINDEX_SELMARGIN:

		// Hardcoded defaults
	case COLORINDEX_PREPROCESSOR:
		color = RGB (0, 128, 192);
		break;
	case COLORINDEX_COMMENT:
		color = RGB (0, 128, 0);
		break;
	case COLORINDEX_NUMBER:
		color = RGB (0xff, 0x00, 0x00);
		break;
	case COLORINDEX_OPERATOR:
		color = RGB (96, 96, 96);
		break;
	case COLORINDEX_KEYWORD:
		color = RGB (0, 0, 255);
		break;
	case COLORINDEX_FUNCNAME:
		color = RGB (128, 0, 128);
		break;
	case COLORINDEX_USER1:
		color = RGB (0, 0, 128);
		break;
	case COLORINDEX_USER2:
		color = RGB (0, 128, 192);
		break;
	case COLORINDEX_SELBKGND:
		color = RGB (0, 0, 0);
		break;
	case COLORINDEX_SELTEXT:
		color = RGB (255, 255, 255);
		break;
	case COLORINDEX_HIGHLIGHTBKGND1:
		color = RGB (255, 160, 160);
		break;
	case COLORINDEX_HIGHLIGHTTEXT1:
		color = RGB (0, 0, 0);
		break;
	case COLORINDEX_HIGHLIGHTBKGND2:
		color = RGB (255, 255, 0);
		break;
	case COLORINDEX_HIGHLIGHTTEXT2:
		color = RGB (0, 0, 0);
		break;
	default:
		color = RGB (128, 0, 0);
		break;
	}
	m_colors[i] = color;
	m_bolds[i] = i == COLORINDEX_KEYWORD;
}

/**
 * @brief Set color value
 * @param [in] Index index of color to set (COLORINDEX).
 * @param [in] color New color value.
 */
void CSyntaxColors::SetColor(UINT index, COLORREF color)
{
	m_colors[index] = color;
}

/**
 * @brief Set bold value.
 * @param [in] Index index of color to set (COLORINDEX).
 * @param [in] bold If TRUE bold is enabled.
 */
void CSyntaxColors::SetBold(UINT index, bool bold)
{
	m_bolds[index] = bold;
}

/**
 * @brief Save color values to storage
 */
void CSyntaxColors::SaveToRegistry()
{
	if (CRegKeyEx key = SettingStore.GetSectionKey(DefColorsPath, CREATE_ALWAYS))
	{
		key.WriteDword(_T("Values"), COLORINDEX_COUNT);
		for (UINT i = COLORINDEX_NONE; i < COLORINDEX_LAST; i++)
		{
			TCHAR name[40];
			wsprintf(name, _T("Color%02u"), i);
			key.WriteDword(name, m_colors[i]);
			wsprintf(name, _T("Bold%02u"), i);
			key.WriteDword(name, m_bolds[i]);
		}
	}
	// Save the custom colors
	if (CRegKeyEx key = SettingStore.GetSectionKey(CustomColors, CREATE_ALWAYS))
	{
		for (UINT i = 0; i < _countof(m_rgCustColors); i++)
		{
			TCHAR name[40];
			_itot(i, name, 10);
			if (m_rgCustColors[i] == RGB(255, 255, 255))
				key.DeleteValue(name);
			else 
				key.WriteDword(name, m_rgCustColors[i]);
		}
	}
}

bool CSyntaxColors::ChooseColor(HWND hDlg, int nIDDlgItem)
{
	CHOOSECOLOR cc;
	ZeroMemory(&cc, sizeof cc);
	cc.lStructSize = sizeof cc;
	cc.hwndOwner = hDlg;
	cc.Flags = CC_RGBINIT;
	cc.lpCustColors = m_rgCustColors;
	cc.rgbResult = GetDlgItemInt(hDlg, nIDDlgItem, NULL, FALSE);
	if (!::ChooseColor(&cc))
		return false;
	SetDlgItemInt(hDlg, nIDDlgItem, cc.rgbResult, FALSE);
	return true;
}

DWORD CSyntaxColors::ExportOptions(LPCTSTR filename)
{
	DWORD error = ERROR_SUCCESS;
	UINT i;
	for (i = COLORINDEX_NONE; i < COLORINDEX_LAST; i++)
	{
		TCHAR name[_countof(DefColorsPath) + 40];
		wsprintf(name, _T("%s/Color%02u"), DefColorsPath, i);
		TCHAR num[16];
		LPCTSTR value = _itot(m_colors[i], num, 10);
		if (!WritePrivateProfileString(_T("WinMerge"), name, value, filename))
		{
			error = GetLastError();
		}
		wsprintf(name, _T("%s/Bold%02u"), DefColorsPath, i);
		value = _itot(m_bolds[i], num, 10);
		if (!WritePrivateProfileString(_T("WinMerge"), name, value, filename))
		{
			error = GetLastError();
		}
	}
	// Save the custom colors
	for (i = 0; i < _countof(m_rgCustColors); i++)
	{
		TCHAR name[_countof(CustomColors) + 40];
		wsprintf(name, _T("%s/%u"), CustomColors, i);
		TCHAR num[16];
		COLORREF cr = m_rgCustColors[i];
		LPCTSTR value = cr != RGB(255,255,255) ? _itot(cr, num, 10) : NULL;
		if (!WritePrivateProfileString(_T("WinMerge"), name, value, filename))
		{
			error = GetLastError();
		}
	}
	return error;
}

DWORD CSyntaxColors::ImportOptions(LPCTSTR filename)
{
	DWORD error = ERROR_SUCCESS;
	UINT i;
	for (i = COLORINDEX_NONE; i < COLORINDEX_LAST; i++)
	{
		TCHAR name[_countof(DefColorsPath) + 40];
		wsprintf(name, _T("%s/Color%02u"), DefColorsPath, i);
		m_colors[i] = GetPrivateProfileInt(_T("WinMerge"), name, m_colors[i], filename);
		wsprintf(name, _T("%s/Bold%02u"), DefColorsPath, i);
		m_bolds[i] = GetPrivateProfileInt(_T("WinMerge"), name, m_bolds[i], filename) != 0;
	}
	// Save the custom colors
	for (i = 0; i < _countof(m_rgCustColors); i++)
	{
		TCHAR name[_countof(CustomColors) + 40];
		wsprintf(name, _T("%s/%u"), CustomColors, i);
		m_rgCustColors[i] = GetPrivateProfileInt(_T("WinMerge"), name, m_rgCustColors[i], filename);
	}
	return error;
}
