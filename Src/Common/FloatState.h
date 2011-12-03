#pragma once

class CFloatFlags
{
public:
	enum
	{
		L2L = 0x00010000,
		T2T = 0x00020000,
		L2R = 0x00040000,
		T2B = 0x00080000,
		R2L = 0x00100000,
		B2T = 0x00200000,
		R2R = 0x00400000,
		B2B = 0x00800000
	};
	template<int by>struct BY
	{
		enum
		{
			X2L = by>>2<<000,
			Y2T = by>>2<<010,
			X2R = by>>2<<020,
			Y2B = by>>2<<030
		};
	};
};

class CFloatState : public CFloatFlags
{
public:
	//Construction
	CFloatState(const LONG *FloatScript = 0): FloatScript(FloatScript) { }
	//Data
	int cx;
	int cy;
	DWORD flags;
	const LONG *FloatScript;
	//Methods
	void Clear();
	BOOL Float(WINDOWPOS *);
	static BOOL AdjustMax(HWND, MINMAXINFO *);
	UINT AdjustHit(UINT);
	LRESULT CallWindowProc(WNDPROC, HWND, UINT, WPARAM, LPARAM);
};
