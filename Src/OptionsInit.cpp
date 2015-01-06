/**
 * @file  OptionsInit.cpp
 *
 * @brief Options initialisation.
 */
#include "StdAfx.h"
#include "OptionsDef.h"
#include "SettingStore.h"
#include "Common/RegKey.h"

IOptionDef *IOptionDef::First = NULL;

/**
 * @brief Split option name to path (in registry) and
 * valuename (in registry).
 *
 * Option names are given as "full path", e.g. "Settings/AutomaticRescan".
 * This function splits that to path "Settings/" and valuename
 * "AutomaticRescan".
 * @param [in] strName Option name
 * @param [out] srPath Path (key) in registry
 * @param [out] strValue Value in registry
 */
static void SplitName(String strName, String &strPath, String &strValue)
{
	int pos = strName.rfind('/');
	if (pos > 0)
	{
		int len = strName.length();
		strValue = strName.substr(pos + 1, len - pos - 1);
		strPath = strName.substr(0, pos);
	}
	else
	{
		strValue = strName;
		strPath.clear();
	}
}

/**
 * @brief Load option from registry.
 * @note Currently handles only integer and string options!
 */
void IOptionDef::LoadOption(HKEY key)
{
	CRegKeyEx section = NULL;
	String path, entry;
	SplitName(name, path, entry);
	section.OpenWithAccess(key, path.c_str(), KEY_READ);
	UOptionPtr u = { this + 1 };
	switch (type)
	{
	case VT_UI1:
		*u.pByte = static_cast<BYTE>(section.ReadDword(entry.c_str(), *u.pByte));
		break;
	case VT_I4:
	case VT_UI4:
		*u.pDWord = section.ReadDword(entry.c_str(), *u.pDWord);
		break;
	case VT_BOOL:
		*u.pBool = section.ReadDword(entry.c_str(), *u.pBool) != 0;
		break;
	case VT_BSTR:
		*u.pString = section.ReadString(entry.c_str(), u.pString->c_str());
		break;
	case VT_FONT:
		u.pLogFont->Parse(
			section.ReadString(entry.c_str(), u.pLogFont->Print()));
		break;
	}
}

/**
 * @brief Save option to registry
 * @note Currently handles only integer and string options!
 */
LSTATUS IOptionDef::SaveOption(HKEY key)
{
	CRegKeyEx section = NULL;
	String path, entry;
	SplitName(name, path, entry);
	LSTATUS error = section.OpenWithAccess(key, path.c_str(), KEY_WRITE);
	if (error == ERROR_SUCCESS)
	{
		if (!IsDefault())
		{
			UOptionPtr u = { this + 1 };
			switch (type)
			{
			case VT_UI1:
				error = section.WriteDword(entry.c_str(), *u.pByte);
				break;
			case VT_I4:
			case VT_UI4:
				error = section.WriteDword(entry.c_str(), *u.pDWord);
				break;
			case VT_BOOL:
				error = section.WriteDword(entry.c_str(), *u.pBool);
				break;
			case VT_BSTR:
				error = section.WriteString(entry.c_str(), u.pString->c_str());
				break;
			case VT_FONT:
				error = section.WriteString(entry.c_str(), u.pLogFont->Print());
				break;
			}
		}
		else
		{
			error = section.DeleteValue(entry.c_str());
		}
	}
	return error;
}

LSTATUS IOptionDef::SaveOption()
{
	CRegKeyEx rk = SettingStore.GetAppRegistryKey();
	return SaveOption(rk);
}

int IOptionDef::ExportOptions(LPCTSTR filename)
{
	LONG error = ERROR_SUCCESS;
	IOptionDef *p = First;
	while (p)
	{
		UOptionPtr u = { p + 1 };
		LPCTSTR val = NULL;
		if (!p->IsDefault())
		{
			TCHAR num[128]; // Mind the VT_FONT case
			switch (p->type)
			{
			case VT_UI1:
				val = _itot(*u.pByte, num, 10);
				break;
			case VT_I4:
			case VT_UI4:
				val = _itot(*u.pDWord, num, 10);
				break;
			case VT_BOOL:
				val = *u.pBool ? _T("1") : _T("0");
				break;
			case VT_BSTR:
				val = u.pString->c_str();
				break;
			case VT_FONT:
				val = u.pLogFont->Print(num);
				break;
			}
		}
		if (!WritePrivateProfileString(_T("WinMerge"), p->name, val, filename))
		{
			error = GetLastError();
		}
		p = static_cast<IOptionDef *>(p->Next);
	}
	return error;
}

int IOptionDef::ImportOptions(LPCTSTR filename)
{
	LONG error = ERROR_SUCCESS;
	IOptionDef *p = First;
	while (p)
	{
		UOptionPtr u = { p + 1 };
		TCHAR val[MAX_PATH];
		switch (p->type)
		{
		case VT_UI1:
			*u.pByte = static_cast<BYTE>(GetPrivateProfileInt(_T("WinMerge"), p->name, *u.pByte, filename));
			break;
		case VT_I4:
		case VT_UI4:
			*u.pDWord = GetPrivateProfileInt(_T("WinMerge"), p->name, *u.pDWord, filename);
			break;
		case VT_BOOL:
			*u.pBool = GetPrivateProfileInt(_T("WinMerge"), p->name, *u.pBool, filename) != 0;
			break;
		case VT_BSTR:
			GetPrivateProfileString(_T("WinMerge"), p->name, u.pString->c_str(), val, _countof(val), filename);
			*u.pString = val;
			break;
		case VT_FONT:
			GetPrivateProfileString(_T("WinMerge"), p->name, u.pLogFont->Print(), val, _countof(val), filename);
			u.pLogFont->Parse(val);
			break;
		}
		p = static_cast<IOptionDef *>(p->Next);
	}
	return error;
}

void IOptionDef::Parse(LPCTSTR value)
{
	UOptionPtr u = { this + 1 };
	switch (type)
	{
	case VT_UI1:
		*u.pByte = static_cast<BYTE>(_ttol(value));
		break;
	case VT_I4:
	case VT_UI4:
		*u.pDWord = _ttol(value);
		break;
	case VT_BOOL:
		*u.pBool = _ttol(value) != 0;
		break;
	case VT_BSTR:
		*u.pString = value;
		break;
	}
}

void IOptionDef::InitOptions(HKEY loadkey, HKEY savekey)
{
	IOptionDef *p = First;
	while (p)
	{
		p->Reset();
		if (loadkey)
			p->LoadOption(loadkey);
		if (savekey)
			p->SaveOption(savekey);
		p = static_cast<IOptionDef *>(p->Next);
	}
}
