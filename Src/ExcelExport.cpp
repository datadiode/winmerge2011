/* ExcelExport.cpp

Last edit: 2013-04-21 Jochen Neubeck

See https://github.com/Lafriks/nix-spreadsheet if in need for C# code.
See http://dmcritchie.mvps.org/excel/colors.htm for default Excel colors.

[The MIT license]

Copyright (c) 2013 Jochen Neubeck

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "stdafx.h"
#include "DiffItem.h"
#include "OptionsMgr.h"
#include "ExcelExport.h"

#undef EOF

struct CExcelExport::BiffRecord
{
	enum RecordType
	{
		BOF				= 0x0809, // Beginning of File
		EOF				= 0x000A, // End of File
		FONT			= 0x0031, // Font Description
		FORMAT			= 0x041E, // Number Format
		LABEL			= 0x0204, // Cell Value, String Constant
		WINDOWPROTECT	= 0x0019, // Windows Are Protected
		XF				= 0x00E0, // Extended Format
		HEADER			= 0x0014, // Print Header on Each Page
		FOOTER			= 0x0015, // Print Footer on Each Page
		CODEPAGE		= 0x0042, // Default Code Page
		NUMBER			= 0x0203, // Cell Value, Floating-Point Number
		COLINFO			= 0x007D, // Column Formatting Information
		TABID			= 0x013D, // Sheet Tab Index Array
		WINDOW1			= 0x003D, // Window Information
		WINDOW2			= 0x023E, // Sheet Window Information
		WSBOOL			= 0x0081, // Additional Workspace Information
		BOUNDSHEET		= 0x0085, // Sheet Information
		PANE			= 0x0041, // Number of Panes and Their Position
		PRINTGRIDLINES	= 0x002B, // Print Gridlines Flag
		SUPBOOK			= 0x01AE, // Supporting Workbook
		EXTERNSHEET		= 0x0017, // External Reference
		NAME			= 0x0018, // Defined Name
		PALETTE			= 0x0092, // Palette
	};
	WORD id;
	WORD size;
	BYTE data[2048];
	BiffRecord(RecordType id): id(id), size(0)
	{
	}
	template<class T>
	BiffRecord &Append(T value)
	{
		reinterpret_cast<T&>(data[size]) = value;
		size += sizeof value;
		return *this;
	}
	BiffRecord &Append(const void *pv, WORD cb)
	{
		memcpy(data + size, pv, cb);
		size += cb;
		return *this;
	}
	template<class COUNT>
	BiffRecord &AppendString(LPCSTR value)
	{
		COUNT len = static_cast<COUNT>(lstrlenA(value));
		Append<COUNT>(len);
		Append<BYTE>(0x00);
		Append(value, len * sizeof *value);
		return *this;
	}
	template<class COUNT>
	BiffRecord &AppendString(LPCWSTR value)
	{
		COUNT len = static_cast<COUNT>(lstrlenW(value));
		Append<COUNT>(len);
		Append<BYTE>(0x01);
		Append(value, len * sizeof *value);
		return *this;
	}
	void WriteTo(ISequentialStream *pstm)
	{
		pstm->Write(this, offsetof(BiffRecord, data) + size, NULL);
	}
};

CExcelExport::CExcelExport()
	: hr(S_OK)
	, pstg(NULL)
	, pstm(NULL)
	, fPrintGrid(false)
	, wBorderStyle(BorderStyle::THIN)
	, crBorderColor(RGB(192,192,192))
	, sSheetName("Sheet1")
	, nShowViewer(0)
{
}

CExcelExport::~CExcelExport()
{
	assert(pstg == NULL);
	assert(pstm == NULL);
}

bool CExcelExport::Open(LPCWSTR path)
{
	const DWORD stgm = STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE;
	if (FAILED(hr = StgCreateDocfile(path, stgm, 0, &pstg)))
		return false;
	if (FAILED(hr = pstg->CreateStream(L"Workbook", stgm, 0, 0, &pstm)))
		return false;
	return true;
}

void CExcelExport::Close(LPCTSTR lpVerb)
{
	LPTSTR lpFile = NULL;
	if (pstm)
	{
		pstm->Release();
		pstm = NULL;
		if (pstg)
		{
			if (lpVerb != NULL)
			{
				STATSTG stat;
				pstg->Stat(&stat, STATFLAG_DEFAULT);
				STRRET strret;
				strret.uType = STRRET_WSTR;
				strret.pOleStr = stat.pwcsName;
				StrRetToStr(&strret, NULL, &lpFile);
			}
			pstg->Release();
			pstg = NULL;
		}
	}
	if (lpFile != NULL)
	{
		SHELLEXECUTEINFO sei;
		ZeroMemory(&sei, sizeof sei);
		sei.cbSize = sizeof sei;
		sei.fMask = SEE_MASK_CLASSNAME | SEE_MASK_NOCLOSEPROCESS | SEE_MASK_WAITFORINPUTIDLE;
		sei.nShow = nShowViewer;
		sei.lpFile = lpFile;
		sei.lpVerb = lpVerb;
		sei.lpClass = sViewer.c_str();
		ShellExecuteEx(&sei);
		// Cope with possible DDE conversation issues
		DWORD dwWait;
		do
		{
			dwWait = MsgWaitForMultipleObjects(1, &sei.hProcess, FALSE, INFINITE, QS_ALLINPUT);
			MSG msg;
			while (dwWait == WAIT_OBJECT_0 + 1 && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				DispatchMessage(&msg);
				if (HWND hWnd = GetForegroundWindow())
				{
					DWORD dwForegroundProcessId = 0;
					GetWindowThreadProcessId(hWnd, &dwForegroundProcessId);
					if (dwForegroundProcessId == GetCurrentProcessId())
					{
						dwWait = WAIT_OBJECT_0;
					}
				}
			}
		} while (dwWait != WAIT_OBJECT_0);
		CloseHandle(sei.hProcess);
		CoTaskMemFree(lpFile);
	}
}

void CExcelExport::ApplyProfile(LPCTSTR app, LPCTSTR ini, bool fWriteDefaults)
{
	TCHAR tmp[1024];
	const LPCTSTR fWriteDefaultFlag = &_T("\0") _T("%d")[fWriteDefaults];
	const LPCTSTR fWriteDefaultHex4 = &_T("\0") _T("0x%04X")[fWriteDefaults];
	const LPCTSTR fWriteDefaultHex6 = &_T("\0") _T("0x%06X")[fWriteDefaults];
	if (GetPrivateProfileString(app, _T("PrintGrid"), NULL, tmp, _countof(tmp), ini))
		fPrintGrid = PathMatchSpec(tmp, _T("1;yes;true"));
	else if (wsprintf(tmp, fWriteDefaultFlag, fPrintGrid))
		WritePrivateProfileString(app, _T("PrintGrid"), tmp, ini);
	if (GetPrivateProfileString(app, _T("BorderStyle"), NULL, tmp, _countof(tmp), ini))
		wBorderStyle = static_cast<BorderStyle::WORD_BorderStyle>(_tcstol(tmp, NULL, 0));
	else if (wsprintf(tmp, fWriteDefaultHex4, wBorderStyle))
		WritePrivateProfileString(app, _T("BorderStyle"), tmp, ini);
	if (GetPrivateProfileString(app, _T("BorderColor"), NULL, tmp, _countof(tmp), ini))
		crBorderColor = static_cast<COLORREF>(_tcstol(tmp, NULL, 0));
	else if (wsprintf(tmp, fWriteDefaultHex6, crBorderColor))
		WritePrivateProfileString(app, _T("BorderColor"), tmp, ini);
	if (GetPrivateProfileString(app, _T("Header"), NULL, tmp, _countof(tmp), ini))
		sHeader = tmp;
	else if (fWriteDefaults)
		WritePrivateProfileString(app, _T("Header"), sHeader.c_str(), ini);
	if (GetPrivateProfileString(app, _T("Footer"), NULL, tmp, _countof(tmp), ini))
		sFooter = tmp;
	else if (fWriteDefaults)
		WritePrivateProfileString(app, _T("Footer"), sFooter.c_str(), ini);
	if (GetPrivateProfileString(app, _T("Viewer"), NULL, tmp, _countof(tmp), ini))
	{
		nShowViewer = PathParseIconLocation(tmp);
		sViewer = tmp;
	}
	else if (fWriteDefaults)
	{
		wsprintf(tmp, _T("%s,%d"), sViewer.c_str(), nShowViewer);
		WritePrivateProfileString(app, _T("Viewer"), tmp, ini);
	}
}

void CExcelExport::WriteWorkbook(HListView *pLv, int flags)
{
	HHeaderCtrl *const pHd = pLv->GetHeaderCtrl();
	int const nCols = pHd->GetItemCount();
	std::vector<WORD> rgFormatIndex;
	rgFormatIndex.resize(nCols);
	int cxChar = pLv->GetStringWidth(_T("0"));
	TCHAR szText[INFOTIPSIZE];
	LVITEM item;

	const WORD wCellBorderColors = 8 | 8 << 7; // 1st color from PALETTE record

	BiffRecord(BiffRecord::BOF)
		.Append<WORD>(0x0600)
		.Append<WORD>(0x0005)
		.Append<WORD>(0x0DBB)
		.Append<WORD>(0x07CC)
		.Append<WORD>(0x0000)
		.Append<WORD>(0x0000)
		.Append<WORD>(0x0006)
		.Append<WORD>(0x0000)
		.WriteTo(pstm);

	BiffRecord(BiffRecord::CODEPAGE)
		.Append<WORD>(0x04B0)
		.WriteTo(pstm);

	BiffRecord(BiffRecord::TABID)
		.Append<WORD>(1)
		.WriteTo(pstm);

	BiffRecord(BiffRecord::WINDOW1)
		.Append<WORD>(0x0000)
		.Append<WORD>(0x0000)
		.Append<WORD>(0x4000)
		.Append<WORD>(0x2000)
		.Append<WORD>(0x0038)
		.Append<WORD>(0x0000)
		.Append<WORD>(0x0000)
		.Append<WORD>(0x0000)
		.Append<WORD>(0x0000)
		.WriteTo(pstm);

	BiffRecord(BiffRecord::FONT)
		.Append<WORD>(0x00C8)
		.Append<WORD>(0x0000)
		.Append<WORD>(0x7FFF)
		.Append<WORD>(FW_REGULAR)
		.Append<WORD>(0x0000)
		.Append<WORD>(0x0200)
		.Append<WORD>(0x0000)
		.AppendString<BYTE>(L"Arial")
		.WriteTo(pstm);

	BiffRecord(BiffRecord::FONT)
		.Append<WORD>(0x00C8)
		.Append<WORD>(0x0000)
		.Append<WORD>(0x7FFF)
		.Append<WORD>(FW_REGULAR)
		.Append<WORD>(0x0000)
		.Append<WORD>(0x0000)
		.Append<WORD>(0x0000)
		.AppendString<BYTE>(L"Arial")
		.WriteTo(pstm);

	BiffRecord(BiffRecord::FONT)
		.Append<WORD>(0x00C8)
		.Append<WORD>(0x0000)
		.Append<WORD>(0x7FFF)
		.Append<WORD>(FW_REGULAR)
		.Append<WORD>(0x0000)
		.Append<WORD>(0x0000)
		.Append<WORD>(0x0000)
		.AppendString<BYTE>(L"Arial")
		.WriteTo(pstm);

	BiffRecord(BiffRecord::FONT)
		.Append<WORD>(0x00C8)
		.Append<WORD>(0x0000)
		.Append<WORD>(0x7FFF)
		.Append<WORD>(FW_BOLD)
		.Append<WORD>(0x0000)
		.Append<WORD>(0x0000)
		.Append<WORD>(0x0000)
		.AppendString<BYTE>(L"Arial")
		.WriteTo(pstm);

	BiffRecord(BiffRecord::FORMAT)
		.Append<WORD>(0x00A4)
		.AppendString<WORD>("GENERAL")
		.WriteTo(pstm);

	BiffRecord(BiffRecord::FORMAT)
		.Append<WORD>(0x00A5)
		.AppendString<WORD>("0")
		.WriteTo(pstm);

	BiffRecord(BiffRecord::PALETTE)
		.Append<WORD>(4)
		.Append<COLORREF>(crBorderColor)
		.Append<COLORREF>(COptionsMgr::Get(OPT_LIST_LEFTONLY_BKGD_COLOR))
		.Append<COLORREF>(COptionsMgr::Get(OPT_LIST_RIGHTONLY_BKGD_COLOR))
		.Append<COLORREF>(COptionsMgr::Get(OPT_LIST_SUSPICIOUS_BKGD_COLOR))
		.WriteTo(pstm);

	for (item.iSubItem = 0 ; item.iSubItem < nCols ; ++item.iSubItem)
	{
		LVCOLUMN lvc;
		lvc.mask = LVCF_FMT;
		pLv->GetColumn(item.iSubItem, &lvc);
		if ((lvc.fmt & LVCFMT_JUSTIFYMASK) == LVCFMT_RIGHT)
		{
			rgFormatIndex[item.iSubItem] = 19;
		}
	}

	// XF #0 - #16 as mandated by BIFF8 specification
	int i = 0;
	do switch (i++)
	{
	case 0:
		BiffRecord(BiffRecord::XF)
			.Append<WORD>(0x0000)
			.Append<WORD>(0x00A4)
			.Append<WORD>(0xFFF5)
			.Append<WORD>(0x0020)
			.Append<WORD>(0x0000)
			.Append<WORD>(wBorderStyle)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(0x0000)
			.Append<WORD>(0x20C0)
			.WriteTo(pstm);
		break;
	case 1: case 2:
		BiffRecord(BiffRecord::XF)
			.Append<WORD>(0x0001)
			.Append<WORD>(0x0000)
			.Append<WORD>(0xFFF5)
			.Append<WORD>(0x0020)
			.Append<WORD>(0xF400)
			.Append<WORD>(wBorderStyle)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(0x0000)
			.Append<WORD>(0x20C0)
			.WriteTo(pstm);
		break;
	case 3: case 4:
		BiffRecord(BiffRecord::XF)
			.Append<WORD>(0x0002)
			.Append<WORD>(0x0000)
			.Append<WORD>(0xFFF5)
			.Append<WORD>(0x0020)
			.Append<WORD>(0xF400)
			.Append<WORD>(wBorderStyle)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(0x0000)
			.Append<WORD>(0x20C0)
			.WriteTo(pstm);
		break;
	case 5: case 6: case 7: case 8: case 9:
	case 10: case 11: case 12: case 13: case 14:
		BiffRecord(BiffRecord::XF)
			.Append<WORD>(0x0000)
			.Append<WORD>(0x0000)
			.Append<WORD>(0xFFF5)
			.Append<WORD>(0x0020)
			.Append<WORD>(0xF400)
			.Append<WORD>(wBorderStyle)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(0x0000)
			.Append<WORD>(0x20C0)
			.WriteTo(pstm);
		break;
	case 15:
		BiffRecord(BiffRecord::XF)
			.Append<WORD>(0x0000)
			.Append<WORD>(0x00A4)
			.Append<WORD>(0x0001)
			.Append<WORD>(0x0020)
			.Append<WORD>(0x0000)
			.Append<WORD>(wBorderStyle)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(0x0000)
			.Append<WORD>(0x20C0)
			.WriteTo(pstm);
		break;
	case 16:
		BiffRecord(BiffRecord::XF)
			.Append<WORD>(0x0000)
			.Append<WORD>(0x00A4)
			.Append<WORD>(0x0001)
			.Append<WORD>(0x0020)
			.Append<WORD>(0x0800)
			.Append<WORD>(wBorderStyle)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(0x0000)
			.Append<WORD>(0x20C0)
			.WriteTo(pstm);
		break;
	case 17: // bold
		BiffRecord(BiffRecord::XF)
			.Append<WORD>(0x0003)
			.Append<WORD>(0x0000)
			.Append<WORD>(0x0001)
			.Append<WORD>(0x0020)
			.Append<WORD>(0x0400)
			.Append<WORD>(wBorderStyle)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(0x0000)
			.Append<WORD>(0x20C0)
			.WriteTo(pstm);
		break;
	case 18: // bold, right-aligned
		BiffRecord(BiffRecord::XF)
			.Append<WORD>(0x0003)
			.Append<WORD>(0x0000)
			.Append<WORD>(0x0001)
			.Append<WORD>(0x0023)
			.Append<WORD>(0x0400)
			.Append<WORD>(wBorderStyle)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(0x0000)
			.Append<WORD>(0x20C0)
			.WriteTo(pstm);
		break;
	case 19: // format "0", right-aligned
		BiffRecord(BiffRecord::XF)
			.Append<WORD>(0x0000)
			.Append<WORD>(0x00A5)
			.Append<WORD>(0x0001)
			.Append<WORD>(0x0023)
			.Append<WORD>(0x0400)
			.Append<WORD>(wBorderStyle)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(0x0000)
			.Append<WORD>(0x20C0)
			.WriteTo(pstm);
		break;
	case 20: // format "0", left-aligned, left-only
		BiffRecord(BiffRecord::XF)
			.Append<WORD>(0x0000) // ifnt
			.Append<WORD>(0x00A5) // ifmt
			.Append<WORD>(0x0001)
			.Append<WORD>(0x0020) // alignment and rotation
			.Append<WORD>(0x0400)
			.Append<WORD>(wBorderStyle)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(0x0400) // solid fill pattern
			.Append<WORD>(0x2088 + 1) // 1st color after grid color
			.WriteTo(pstm);
		break;
	case 21: // format "0", right-aligned, left-only
		BiffRecord(BiffRecord::XF)
			.Append<WORD>(0x0000) // ifnt
			.Append<WORD>(0x00A5) // ifmt
			.Append<WORD>(0x0001)
			.Append<WORD>(0x0023) // alignment and rotation
			.Append<WORD>(0x0400)
			.Append<WORD>(wBorderStyle)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(0x0400) // solid fill pattern
			.Append<WORD>(0x2088 + 1) // 1st color after grid color
			.WriteTo(pstm);
		break;
	case 22: // format "0", left-aligned, right-only
		BiffRecord(BiffRecord::XF)
			.Append<WORD>(0x0000) // ifnt
			.Append<WORD>(0x00A5) // ifmt
			.Append<WORD>(0x0001)
			.Append<WORD>(0x0020) // alignment and rotation
			.Append<WORD>(0x0400)
			.Append<WORD>(wBorderStyle)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(0x0400) // solid fill pattern
			.Append<WORD>(0x2088 + 2) // 2nd color after grid color
			.WriteTo(pstm);
		break;
	case 23: // format "0", right-aligned, right-only
		BiffRecord(BiffRecord::XF)
			.Append<WORD>(0x0000) // ifnt
			.Append<WORD>(0x00A5) // ifmt
			.Append<WORD>(0x0001)
			.Append<WORD>(0x0023) // alignment and rotation
			.Append<WORD>(0x0400)
			.Append<WORD>(wBorderStyle)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(0x0400) // solid fill pattern
			.Append<WORD>(0x2088 + 2) // 2nd color after grid color
			.WriteTo(pstm);
		break;
	case 24: // format "0", left-aligned, otherwise suspicious
		BiffRecord(BiffRecord::XF)
			.Append<WORD>(0x0000) // ifnt
			.Append<WORD>(0x00A5) // ifmt
			.Append<WORD>(0x0001)
			.Append<WORD>(0x0020) // alignment and rotation
			.Append<WORD>(0x0400)
			.Append<WORD>(wBorderStyle)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(0x0400) // solid fill pattern
			.Append<WORD>(0x2088 + 3) // 3rd color after grid color
			.WriteTo(pstm);
		break;
	case 25: // format "0", right-aligned, otherwise suspicious
		BiffRecord(BiffRecord::XF)
			.Append<WORD>(0x0000) // ifnt
			.Append<WORD>(0x00A5) // ifmt
			.Append<WORD>(0x0001)
			.Append<WORD>(0x0023) // alignment and rotation
			.Append<WORD>(0x0400)
			.Append<WORD>(wBorderStyle)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(wCellBorderColors)
			.Append<WORD>(0x0400) // solid fill pattern
			.Append<WORD>(0x2088 + 3) // 3rd color after grid color
			.WriteTo(pstm);
		break;
	default:
		i = 0;
		break;
	} while (i != 0);

	union
	{
		LARGE_INTEGER in;
		ULARGE_INTEGER out;
	} u = { 0, 0 };
	pstm->Seek(u.in, STREAM_SEEK_CUR, &u.out);

	BiffRecord(BiffRecord::BOUNDSHEET)
		// Here comes the offset of below BiffRecord::BOF
		.Append<DWORD>
		(
			u.out.LowPart
		+	10						// BOUNDSHEET w/o sheet name
		+	2						// Fixed size part of sheet name
		+	sSheetName.length()		// Variable size part of sheet name
		+	8						// SUPBOOK
		+	12						// EXTERNSHEET
		+	31						// NAME
		+	4						// EOF
		)
		.Append<WORD>(0x0000)
		.AppendString<BYTE>(sSheetName.c_str())
		.WriteTo(pstm);

	BiffRecord(BiffRecord::SUPBOOK)
		.Append<WORD>(0x0001)
		.Append<WORD>(0x0401)
		.WriteTo(pstm);

	BiffRecord(BiffRecord::EXTERNSHEET)
		.Append<WORD>(0x0001)		// Number of XTI structures that follow
		.Append<WORD>(0x0000)		// Index (0-based) to table of SUPBOOK redcords
		.Append<WORD>(0x0000)		// Index (0-based) to first sheet tab in the reference
		.Append<WORD>(0x0000)		// Index (0-based) to last sheet tab in the reference
		.WriteTo(pstm);

	BiffRecord(BiffRecord::NAME)
		.Append<WORD>(0x0020)		// Option flags (0x0020 = fBuiltin)
		.Append<BYTE>(0x00)			// chKey
		.Append<BYTE>(0x01)			// cch
		.Append<WORD>(0x000B)		// cce
		.Append<WORD>(0x0000)		// reserved3
		.Append<WORD>(0x0001)		// itab
		.Append<BYTE>(0x00)			// Length of the custom menu text
		.Append<DWORD>(0x00000000)	// reserved4, reserved5, reserved6, reserved7
		.Append<BYTE>(0x07)			// Name definition (7 = Print_Titles)
		.Append<BYTE>(';')			// Beginning of rgce
		.Append<WORD>(0x0000)		//
		.Append<WORD>(0x0000)		// First row
		.Append<WORD>(0x0000)		// Last row
		.Append<WORD>(0x0000)		//
		.Append<WORD>(0x00FF)		// End of rgce
		.WriteTo(pstm);

	BiffRecord(BiffRecord::EOF).WriteTo(pstm);

	BiffRecord(BiffRecord::BOF)
		.Append<WORD>(0x0600)
		.Append<WORD>(0x0010)
		.Append<WORD>(0x0DBB)
		.Append<WORD>(0x07CC)
		.Append<WORD>(0x0000)
		.Append<WORD>(0x0000)
		.Append<WORD>(0x0006)
		.Append<WORD>(0x0000)
		.WriteTo(pstm);

	int iRow = 0;
	for (item.iSubItem = 0 ; item.iSubItem < nCols ; ++item.iSubItem)
	{
		LVCOLUMN lvc;
		lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
		lvc.pszText = szText;
		lvc.cchTextMax = _countof(szText);
		if (pLv->GetColumn(item.iSubItem, &lvc))
		{
			BiffRecord(BiffRecord::COLINFO)
				.Append<WORD>(item.iSubItem)
				.Append<WORD>(item.iSubItem)
				.Append<WORD>(lvc.cx * 256 / cxChar)
				.Append<WORD>(0x000F)
				.Append<WORD>(0x0000)
				.Append<WORD>(0x0000)
				.WriteTo(pstm);

			BiffRecord(BiffRecord::LABEL)
				.Append<WORD>(iRow)
				.Append<WORD>(item.iSubItem)
				.Append<WORD>((lvc.fmt & LVCFMT_JUSTIFYMASK) == LVCFMT_RIGHT ? 18 : 17)
				.AppendString<WORD>(szText)
				.WriteTo(pstm);
		}
	}

	BiffRecord(BiffRecord::WINDOW2)
		.Append<WORD>(0x06BE)
		.Append<WORD>(0)			// Top row visible in the window
		.Append<WORD>(0)			// Leftmost column visible in the window
		.Append<DWORD>(0x00000040)	// Index to color value for row/column headings and gridlines
		.Append<WORD>(0)			// Zoom magnification in page break preview
		.Append<WORD>(0)			// Zoom magnification in normal view
		.Append<DWORD>(0x00000000)	// Reserved
		.WriteTo(pstm);

	BiffRecord(BiffRecord::PANE)
		.Append<WORD>(0)			// Horizontal position of the split; 0 (zero) if none
		.Append<WORD>(1)			// Vertical position of the split; 0 (zero) if none
		.Append<WORD>(1)			// Top row visible in the bottom pane
		.Append<WORD>(1)			// Leftmost column visible in the right pane
		.Append<WORD>(2)			// Pane number of the active pane
		.WriteTo(pstm);

	if (fPrintGrid)
	{
		BiffRecord(BiffRecord::PRINTGRIDLINES)
			.Append<WORD>(1)
			.WriteTo(pstm);
	}

	if (!sHeader.empty())
	{
		BiffRecord(BiffRecord::HEADER)
			.AppendString<WORD>(sHeader.c_str())
			.WriteTo(pstm);
	}

	if (!sFooter.empty())
	{
		BiffRecord(BiffRecord::FOOTER)
			.AppendString<WORD>(sFooter.c_str())
			.WriteTo(pstm);
	}

	item.iItem = -1;
	while ((item.iItem = pLv->GetNextItem(item.iItem, flags)) != -1)
	{
		++iRow;
		item.mask = LVIF_TEXT | LVIF_PARAM;
		for (item.iSubItem = 0 ; item.iSubItem < nCols ; ++item.iSubItem)
		{
			item.pszText = szText;
			item.cchTextMax = _countof(szText);
			if (pLv->GetItem(&item))
			{
				WORD wFormatIndex = 0;
				if (!COptionsMgr::Get(OPT_CLR_DEFAULT_LIST_COLORING))
				{
					if (DIFFITEM *di = reinterpret_cast<DIFFITEM *>(item.lParam))
					{
						switch (di->diffcode & (DIFFCODE::SIDEFLAGS | DIFFCODE::COMPAREFLAGS))
						{
						case DIFFCODE::BOTH | DIFFCODE::NOCMP:
						case DIFFCODE::BOTH | DIFFCODE::SAME:
							// either identical or irrelevant
							break;
						case DIFFCODE::LEFT:
							// left-only
							wFormatIndex = 20;
							break;
						case DIFFCODE::RIGHT:
							// right-only
							wFormatIndex = 22;
							break;
						default:
							// otherwise suspicious
							wFormatIndex = 24;
							break;
						}
					}
				}
				if (WORD wDefFormatIndex = rgFormatIndex[item.iSubItem])
				{
					wFormatIndex = wFormatIndex ? wFormatIndex + 1 : wDefFormatIndex;
				}
				BiffRecord(BiffRecord::LABEL)
					.Append<WORD>(iRow)
					.Append<WORD>(item.iSubItem)
					.Append<WORD>(wFormatIndex)
					.AppendString<WORD>(item.pszText)
					.WriteTo(pstm);
			}
			item.mask = LVIF_TEXT;
		}
	}

	BiffRecord(BiffRecord::EOF).WriteTo(pstm);
}
