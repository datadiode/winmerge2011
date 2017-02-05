#pragma once

class CSplitState
{
protected:
	//Construction
	CSplitState(LONG const *SplitScript = 0): SplitScript(SplitScript) { }
	//Data
	LONG const *SplitScript;
	//Methods
	BOOL Split(HWND);
	BOOL Scan(HWND, LPCTSTR) const;
	int Dump(HWND, LPTSTR) const;
};
