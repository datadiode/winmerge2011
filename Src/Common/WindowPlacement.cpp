#include "stdafx.h"
#include "WindowPlacement.h"
#include "SettingStore.h"

bool CWindowPlacement::RegQuery(HKEY key, LPCTSTR subkey)
{
	// must allow for up to 12 chars per item
	TCHAR sz[3 * (sizeof *this - offsetof(CWindowPlacement, showCmd)) + 1];
	DWORD cb = sizeof sz - sizeof *sz;
	*sz = _T(',');
	DWORD type = REG_NONE;
	SettingStore.RegQueryValueEx(key, subkey, &type, (LPBYTE)(sz + 1), &cb);
	if (type != REG_SZ)
		return false;
	UINT *p = reinterpret_cast<UINT *>(this + 1);
	while (*sz && --p >= &showCmd)
		*p = PathParseIconLocation(sz);
	return p == &showCmd;
}

void CWindowPlacement::RegWrite(HKEY key, LPCTSTR subkey)
{
	// must allow for up to 12 chars per item
	TCHAR sz[3 * (sizeof *this - offsetof(CWindowPlacement, showCmd)) + 1];
	int i = 0;
	UINT *p = &showCmd;
	while (p < reinterpret_cast<UINT *>(this + 1))
		i += wsprintf(sz + i, _T(",%d"), *p++);
	SettingStore.RegSetValueEx(key, subkey, REG_SZ, (LPBYTE)(sz + 1), i * sizeof(TCHAR));
}
