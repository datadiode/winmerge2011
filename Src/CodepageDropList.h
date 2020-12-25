// WildcardDropList.h
// Copyright (c) datadiode
// SPDX-License-Identifier: WTFPL

class CodepageDropList
{
private:
	static BOOL CALLBACK EnumCodePagesProc(LPTSTR);
	static LRESULT CALLBACK SbWndProc(HWND, UINT, WPARAM, LPARAM);
public:
	static void OnItemActivate(HWND, HWND, int);
};
