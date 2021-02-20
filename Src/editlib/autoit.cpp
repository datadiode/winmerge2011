///////////////////////////////////////////////////////////////////////////
//  File:    autoit.cpp
//  Version: 1.1.0.4
//  Updated: 19-Jul-1998
//
//  Copyright:  Ferdinand Prantl, portions by Stcherbatchenko Andrei
//  E-mail:     prantl@ff.cuni.cz
//
//  AutoIt syntax highlighing definition
//
//  You are free to use or modify this code to the following restrictions:
//  - Acknowledge me somewhere in your about box, simple "Parts of code by.."
//  will be enough. If you can't (or don't want to), contact me personally.
//  - LEAVE THIS HEADER INTACT
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ccrystaltextbuffer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using CommonKeywords::IsNumeric;

// AutoIt keywords
static BOOL IsAutoItKeyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszBasicKeywordList[] =
	{
		_T("Abs"),
		_T("ACos"),
		_T("AdlibRegister"),
		_T("AdlibUnRegister"),
		_T("And"),
		_T("Asc"),
		_T("AscW"),
		_T("ASin"),
		_T("Assign"),
		_T("ATan"),
		_T("AutoItSetOption"),
		_T("AutoItWinGetTitle"),
		_T("AutoItWinSetTitle"),
		_T("Beep"),
		_T("Binary"),
		_T("BinaryLen"),
		_T("BinaryMid"),
		_T("BinaryToString"),
		_T("BitAND"),
		_T("BitNOT"),
		_T("BitOR"),
		_T("BitRotate"),
		_T("BitShift"),
		_T("BitXOR"),
		_T("BlockInput"),
		_T("Break"),
		_T("ByRef"),
		_T("Call"),
		_T("Case"),
		_T("CDTray"),
		_T("Ceiling"),
		_T("Chr"),
		_T("ChrW"),
		_T("ClipGet"),
		_T("ClipPut"),
		_T("ConsoleRead"),
		_T("ConsoleWrite"),
		_T("ConsoleWriteError"),
		_T("Const"),
		_T("ContinueCase"),
		_T("ContinueLoop"),
		_T("ControlClick"),
		_T("ControlCommand"),
		_T("ControlDisable"),
		_T("ControlEnable"),
		_T("ControlFocus"),
		_T("ControlGetFocus"),
		_T("ControlGetHandle"),
		_T("ControlGetPos"),
		_T("ControlGetText"),
		_T("ControlHide"),
		_T("ControlListView"),
		_T("ControlMove"),
		_T("ControlSend"),
		_T("ControlSetText"),
		_T("ControlShow"),
		_T("ControlTreeView"),
		_T("Cos"),
		_T("Dec"),
		_T("Default"),
		_T("Dim"),
		_T("DirCopy"),
		_T("DirCreate"),
		_T("DirGetSize"),
		_T("DirMove"),
		_T("DirRemove"),
		_T("DllCall"),
		_T("DllCallAddress"),
		_T("DllCallbackFree"),
		_T("DllCallbackGetPtr"),
		_T("DllCallbackRegister"),
		_T("DllClose"),
		_T("DllOpen"),
		_T("DllStructCreate"),
		_T("DllStructGetData"),
		_T("DllStructGetPtr"),
		_T("DllStructGetSize"),
		_T("DllStructGetString"),
		_T("DllStructSetData"),
		_T("Do"),
		_T("DriveGetDrive"),
		_T("DriveGetFileSystem"),
		_T("DriveGetLabel"),
		_T("DriveGetSerial"),
		_T("DriveGetType"),
		_T("DriveMapAdd"),
		_T("DriveMapDel"),
		_T("DriveMapGet"),
		_T("DriveSetLabel"),
		_T("DriveSpaceFree"),
		_T("DriveSpaceTotal"),
		_T("DriveStatus"),
		_T("Else"),
		_T("ElseIf"),
		_T("EndFunc"),
		_T("EndIf"),
		_T("EndSelect"),
		_T("EndSwitch"),
		_T("EndWith"),
		_T("Enum"),
		_T("EnvGet"),
		_T("EnvSet"),
		_T("EnvUpdate"),
		_T("Eval"),
		_T("Execute"),
		_T("Exit"),
		_T("ExitLoop"),
		_T("Exp"),
		_T("False"),
		_T("FileChangeDir"),
		_T("FileClose"),
		_T("FileCopy"),
		_T("FileCreateNTFSLink"),
		_T("FileCreateShortcut"),
		_T("FileDelete"),
		_T("FileExists"),
		_T("FileFindFirstFile"),
		_T("FileFindNextFile"),
		_T("FileFlush"),
		_T("FileGetAttrib"),
		_T("FileGetEncoding"),
		_T("FileGetLongName"),
		_T("FileGetPos"),
		_T("FileGetShortcut"),
		_T("FileGetShortName"),
		_T("FileGetSize"),
		_T("FileGetTime"),
		_T("FileGetVersion"),
		_T("FileInstall"),
		_T("FileMove"),
		_T("FileOpen"),
		_T("FileOpenDialog"),
		_T("FileRead"),
		_T("FileReadLine"),
		_T("FileReadToArray"),
		_T("FileRecycle"),
		_T("FileRecycleEmpty"),
		_T("FileSaveDialog"),
		_T("FileSelectFolder"),
		_T("FileSetAttrib"),
		_T("FileSetEnd"),
		_T("FileSetPos"),
		_T("FileSetTime"),
		_T("FileWrite"),
		_T("FileWriteLine"),
		_T("Floor"),
		_T("For"),
		_T("FtpSetProxy"),
		_T("Func"),
		_T("FuncGetStack"),
		_T("FuncName"),
		_T("Global"),
		_T("GUICreate"),
		_T("GUICtrlCreateAvi"),
		_T("GUICtrlCreateButton"),
		_T("GUICtrlCreateCheckbox"),
		_T("GUICtrlCreateCombo"),
		_T("GUICtrlCreateContextMenu"),
		_T("GUICtrlCreateDate"),
		_T("GUICtrlCreateDummy"),
		_T("GUICtrlCreateEdit"),
		_T("GUICtrlCreateGraphic"),
		_T("GUICtrlCreateGroup"),
		_T("GUICtrlCreateIcon"),
		_T("GUICtrlCreateInput"),
		_T("GUICtrlCreateLabel"),
		_T("GUICtrlCreateList"),
		_T("GUICtrlCreateListView"),
		_T("GUICtrlCreateListViewItem"),
		_T("GUICtrlCreateMenu"),
		_T("GUICtrlCreateMenuItem"),
		_T("GUICtrlCreateMonthCal"),
		_T("GUICtrlCreateObj"),
		_T("GUICtrlCreatePic"),
		_T("GUICtrlCreateProgress"),
		_T("GUICtrlCreateRadio"),
		_T("GUICtrlCreateSlider"),
		_T("GUICtrlCreateTab"),
		_T("GUICtrlCreateTabItem"),
		_T("GUICtrlCreateTreeView"),
		_T("GUICtrlCreateTreeViewItem"),
		_T("GUICtrlCreateUpdown"),
		_T("GUICtrlDelete"),
		_T("GUICtrlGetHandle"),
		_T("GUICtrlGetState"),
		_T("GUICtrlRead"),
		_T("GUICtrlRecvMsg"),
		_T("GUICtrlRegisterListViewSort"),
		_T("GUICtrlSendMsg"),
		_T("GUICtrlSendToDummy"),
		_T("GUICtrlSetBkColor"),
		_T("GUICtrlSetColor"),
		_T("GUICtrlSetCursor"),
		_T("GUICtrlSetData"),
		_T("GUICtrlSetDefBkColor"),
		_T("GUICtrlSetDefColor"),
		_T("GUICtrlSetFont"),
		_T("GUICtrlSetGraphic"),
		_T("GUICtrlSetImage"),
		_T("GUICtrlSetLimit"),
		_T("GUICtrlSetOnEvent"),
		_T("GUICtrlSetPos"),
		_T("GUICtrlSetResizing"),
		_T("GUICtrlSetState"),
		_T("GUICtrlSetStyle"),
		_T("GUICtrlSetTip"),
		_T("GUIDelete"),
		_T("GUIGetCursorInfo"),
		_T("GUIGetMsg"),
		_T("GUIGetStyle"),
		_T("GUIRegisterMsg"),
		_T("GUISetAccelerators"),
		_T("GUISetBkColor"),
		_T("GUISetCoord"),
		_T("GUISetCursor"),
		_T("GUISetFont"),
		_T("GUISetHelp"),
		_T("GUISetIcon"),
		_T("GUISetOnEvent"),
		_T("GUISetState"),
		_T("GUISetStyle"),
		_T("GUIStartGroup"),
		_T("GUISwitch"),
		_T("Hex"),
		_T("HotKeySet"),
		_T("HttpSetProxy"),
		_T("HttpSetUserAgent"),
		_T("HWnd"),
		_T("If"),
		_T("In"),
		_T("InetClose"),
		_T("InetGet"),
		_T("InetGetInfo"),
		_T("InetGetSize"),
		_T("InetRead"),
		_T("IniDelete"),
		_T("IniRead"),
		_T("IniReadSection"),
		_T("IniReadSectionNames"),
		_T("IniRenameSection"),
		_T("IniWrite"),
		_T("IniWriteSection"),
		_T("InputBox"),
		_T("Int"),
		_T("IsAdmin"),
		_T("IsArray"),
		_T("IsBinary"),
		_T("IsBool"),
		_T("IsDeclared"),
		_T("IsDllStruct"),
		_T("IsFloat"),
		_T("IsFunc"),
		_T("IsHWnd"),
		_T("IsInt"),
		_T("IsKeyword"),
		_T("IsMap"),
		_T("IsNumber"),
		_T("IsObj"),
		_T("IsPtr"),
		_T("IsString"),
		_T("Local"),
		_T("Log"),
		_T("MapAppend"),
		_T("MapExists"),
		_T("MapKeys"),
		_T("MapRemove"),
		_T("MemGetStats"),
		_T("Mod"),
		_T("MouseClick"),
		_T("MouseClickDrag"),
		_T("MouseDown"),
		_T("MouseGetCursor"),
		_T("MouseGetPos"),
		_T("MouseMove"),
		_T("MouseUp"),
		_T("MouseWheel"),
		_T("MsgBox"),
		_T("Next"),
		_T("Not"),
		_T("Null"),
		_T("Number"),
		_T("ObjCreate"),
		_T("ObjCreateInterface"),
		_T("ObjEvent"),
		_T("ObjGet"),
		_T("ObjName"),
		_T("OnAutoItExitRegister"),
		_T("OnAutoItExitUnRegister"),
		_T("Opt"),
		_T("Or"),
		_T("Ping"),
		_T("PixelChecksum"),
		_T("PixelGetColor"),
		_T("PixelSearch"),
		_T("ProcessClose"),
		_T("ProcessExists"),
		_T("ProcessGetStats"),
		_T("ProcessList"),
		_T("ProcessSetPriority"),
		_T("ProcessWait"),
		_T("ProcessWaitClose"),
		_T("ProgressOff"),
		_T("ProgressOn"),
		_T("ProgressSet"),
		_T("Ptr"),
		_T("Random"),
		_T("ReDim"),
		_T("RegDelete"),
		_T("RegEnumKey"),
		_T("RegEnumVal"),
		_T("RegRead"),
		_T("RegWrite"),
		_T("Return"),
		_T("Round"),
		_T("Run"),
		_T("RunAs"),
		_T("RunAsWait"),
		_T("RunWait"),
		_T("Select"),
		_T("Send"),
		_T("SendKeepActive"),
		_T("SetError"),
		_T("SetExtended"),
		_T("ShellExecute"),
		_T("ShellExecuteWait"),
		_T("Shutdown"),
		_T("Sin"),
		_T("Sleep"),
		_T("SoundPlay"),
		_T("SoundSetWaveVolume"),
		_T("SplashImageOn"),
		_T("SplashOff"),
		_T("SplashTextOn"),
		_T("Sqrt"),
		_T("SRandom"),
		_T("Static"),
		_T("StatusbarGetText"),
		_T("StderrRead"),
		_T("StdinWrite"),
		_T("StdioClose"),
		_T("StdoutRead"),
		_T("Step"),
		_T("String"),
		_T("StringAddCR"),
		_T("StringCompare"),
		_T("StringFormat"),
		_T("StringFromASCIIArray"),
		_T("StringInStr"),
		_T("StringIsAlNum"),
		_T("StringIsAlpha"),
		_T("StringIsASCII"),
		_T("StringIsDigit"),
		_T("StringIsFloat"),
		_T("StringIsInt"),
		_T("StringIsLower"),
		_T("StringIsSpace"),
		_T("StringIsUpper"),
		_T("StringIsXDigit"),
		_T("StringLeft"),
		_T("StringLen"),
		_T("StringLower"),
		_T("StringMid"),
		_T("StringRegExp"),
		_T("StringRegExpReplace"),
		_T("StringReplace"),
		_T("StringReverse"),
		_T("StringRight"),
		_T("StringSplit"),
		_T("StringStripCR"),
		_T("StringStripWS"),
		_T("StringToASCIIArray"),
		_T("StringToBinary"),
		_T("StringTrimLeft"),
		_T("StringTrimRight"),
		_T("StringUpper"),
		_T("Switch"),
		_T("Tan"),
		_T("TCPAccept"),
		_T("TCPCloseSocket"),
		_T("TCPConnect"),
		_T("TCPListen"),
		_T("TCPNameToIP"),
		_T("TCPRecv"),
		_T("TCPSend"),
		_T("TCPShutdown"),
		_T("TCPShutdownSocket"),
		_T("TCPStartup"),
		_T("Then"),
		_T("TimerDiff"),
		_T("TimerInit"),
		_T("To"),
		_T("ToolTip"),
		_T("TrayCreateItem"),
		_T("TrayCreateMenu"),
		_T("TrayGetMsg"),
		_T("TrayItemDelete"),
		_T("TrayItemGetHandle"),
		_T("TrayItemGetState"),
		_T("TrayItemGetText"),
		_T("TrayItemSetOnEvent"),
		_T("TrayItemSetState"),
		_T("TrayItemSetText"),
		_T("TraySetClick"),
		_T("TraySetIcon"),
		_T("TraySetOnEvent"),
		_T("TraySetPauseIcon"),
		_T("TraySetState"),
		_T("TraySetToolTip"),
		_T("TrayTip"),
		_T("True"),
		_T("UBound"),
		_T("UDPBind"),
		_T("UDPCloseSocket"),
		_T("UDPJoinMulticastGroup"),
		_T("UDPOpen"),
		_T("UDPRecv"),
		_T("UDPSend"),
		_T("UDPShutdown"),
		_T("UDPStartup"),
		_T("Until"),
		_T("VarGetType"),
		_T("Volatile"),
		_T("WEnd"),
		_T("While"),
		_T("WinActivate"),
		_T("WinActive"),
		_T("WinClose"),
		_T("WinExists"),
		_T("WinFlash"),
		_T("WinGetCaretPos"),
		_T("WinGetClassList"),
		_T("WinGetClientSize"),
		_T("WinGetHandle"),
		_T("WinGetPos"),
		_T("WinGetProcess"),
		_T("WinGetState"),
		_T("WinGetText"),
		_T("WinGetTitle"),
		_T("WinKill"),
		_T("WinList"),
		_T("WinMenuSelectItem"),
		_T("WinMinimizeAll"),
		_T("WinMinimizeAllUndo"),
		_T("WinMove"),
		_T("WinSetOnTop"),
		_T("WinSetState"),
		_T("WinSetTitle"),
		_T("WinSetTrans"),
		_T("WinWait"),
		_T("WinWaitActive"),
		_T("WinWaitClose"),
		_T("WinWaitNotActive"),
		_T("With"),
	};
	return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszBasicKeywordList);
}

#define DEFINE_BLOCK pBuf.DefineBlock

#define COOKIE_COMMENT          0x0001
#define COOKIE_PREPROCESSOR     0x0002
#define COOKIE_STRING           0x000C
#define COOKIE_STRING_SINGLE    0x0004
#define COOKIE_STRING_DOUBLE    0x0008
#define COOKIE_EXT_COMMENT      0xFF00
#define COOKIE_TRANSPARENT      0xFFFFFF00

void TextDefinition::ParseLineAutoIt(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const
{
	DWORD &dwCookie = cookie.m_dwCookie;

	if (nLength == 0)
	{
		dwCookie &= COOKIE_TRANSPARENT;
		return;
	}

	BOOL bFirstChar = TRUE;
	BOOL bRedefineBlock = TRUE;
	BOOL bDecIndex = FALSE;
	int nIdentBegin = -1;
	do
	{
		int const nPrevI = I++;
		if (bRedefineBlock)
		{
			int const nPos = bDecIndex ? nPrevI : I;
			bRedefineBlock = FALSE;
			bDecIndex = FALSE;
			if (dwCookie & (COOKIE_COMMENT | COOKIE_EXT_COMMENT))
			{
				DEFINE_BLOCK(nPos, COLORINDEX_COMMENT);
			}
			else if (dwCookie & COOKIE_PREPROCESSOR)
			{
				DEFINE_BLOCK(nPos, COLORINDEX_PREPROCESSOR);
			}
			else if (dwCookie & COOKIE_STRING)
			{
				DEFINE_BLOCK(nPos, COLORINDEX_STRING);
			}
			else if (xisalnum(pszChars[nPos]) || pszChars[nPos] == '.' && nPos > 0 && (!xisalpha(pszChars[nPos - 1]) && !xisalpha(pszChars[nPos + 1])))
			{
				DEFINE_BLOCK(nPos, COLORINDEX_NORMALTEXT);
			}
			else
			{
				DEFINE_BLOCK(nPos, COLORINDEX_OPERATOR);
				bRedefineBlock = TRUE;
				bDecIndex = TRUE;
			}
		}
		// Can be bigger than length if there is binary data
		// See bug #1474782 Crash when comparing SQL with with binary data
		if (I < nLength)
		{
			if (dwCookie & COOKIE_COMMENT)
			{
				DEFINE_BLOCK(I, COLORINDEX_COMMENT);
				break;
			}

			if (!bFirstChar)
			{
				if (dwCookie & COOKIE_EXT_COMMENT)
				{
					break;
				}
			}
			else if (!xisspace(pszChars[I]))
			{
				bFirstChar = FALSE;
				if (xisspace(xisequal<_tcsnicmp>(pszChars + I, _T("#cs")), pszChars + nLength) ||
					xisspace(xisequal<_tcsnicmp>(pszChars + I, _T("#comments-start")), pszChars + nLength))
				{
					DEFINE_BLOCK(I, COLORINDEX_COMMENT);
					dwCookie = dwCookie & ~COOKIE_EXT_COMMENT | dwCookie - COOKIE_EXT_COMMENT & COOKIE_EXT_COMMENT;
					break;
				}
				if (dwCookie & COOKIE_EXT_COMMENT)
				{
					if (xisspace(xisequal<_tcsnicmp>(pszChars + I, _T("#ce")), pszChars + nLength) ||
						xisspace(xisequal<_tcsnicmp>(pszChars + I, _T("#comments-end")), pszChars + nLength))
					{
						dwCookie = dwCookie & ~COOKIE_EXT_COMMENT | dwCookie + COOKIE_EXT_COMMENT & COOKIE_EXT_COMMENT;
					}
					break;
				}
				if (pszChars[I] == '#')
				{
					DEFINE_BLOCK(I, COLORINDEX_PREPROCESSOR);
					dwCookie |= COOKIE_PREPROCESSOR;
					continue;
				}
			}

			//  String constant "...."
			if (dwCookie & COOKIE_STRING_DOUBLE)
			{
				if (pszChars[I] == '"')
				{
					dwCookie &= ~COOKIE_STRING_DOUBLE;
					bRedefineBlock = TRUE;
				}
				continue;
			}

			//  String constant '....'
			if (dwCookie & COOKIE_STRING_SINGLE)
			{
				if (pszChars[I] == '\'')
				{
					dwCookie &= ~COOKIE_STRING_SINGLE;
					bRedefineBlock = TRUE;
				}
				continue;
			}

			if (pszChars[I] == ';')
			{
				DEFINE_BLOCK(I, COLORINDEX_COMMENT);
				dwCookie |= COOKIE_COMMENT;
				break;
			}

			//  Normal text
			if (pszChars[I] == '"')
			{
				DEFINE_BLOCK(I, COLORINDEX_STRING);
				dwCookie |= COOKIE_STRING_DOUBLE;
				continue;
			}

			if (pszChars[I] == '\'')
			{
				DEFINE_BLOCK(I, COLORINDEX_STRING);
				dwCookie |= COOKIE_STRING_SINGLE;
				continue;
			}

			//  Preprocessor directive #....
			if (dwCookie & COOKIE_PREPROCESSOR)
				continue;

			if (pBuf == NULL)
				continue; // No need to extract keywords, so skip rest of loop

			if (xisalnum(pszChars[I]) || pszChars[I] == '.' && I > 0 && (!xisalpha(pszChars[nPrevI]) && !xisalpha(pszChars[I + 1])))
			{
				if (nIdentBegin == -1)
					nIdentBegin = I;
				continue;
			}
		}
		if (nIdentBegin >= 0)
		{
			if (IsAutoItKeyword(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_KEYWORD);
			}
			else if (IsNumeric(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_NUMBER);
			}
			else
			{
				for (int j = I; j < nLength; j++)
				{
					if (!xisspace(pszChars[j]))
					{
						if (pszChars[j] == '(')
						{
							DEFINE_BLOCK(nIdentBegin, COLORINDEX_FUNCNAME);
						}
						break;
					}
				}
			}
			bRedefineBlock = TRUE;
			bDecIndex = TRUE;
			nIdentBegin = -1;
		}
	} while (I < nLength);

	dwCookie &= COOKIE_TRANSPARENT;
}

TESTCASE
{
	int count = 0;
	BOOL (*pfnIsKeyword)(LPCTSTR, int) = NULL;
	FILE *file = fopen(__FILE__, "r");
	assert(file);
	TCHAR text[1024];
	while (_fgetts(text, _countof(text), file))
	{
		TCHAR c, *p, *q;
		if (pfnIsKeyword && (p = _tcschr(text, '"')) != NULL && (q = _tcschr(++p, '"')) != NULL)
			VerifyKeyword<_tcsnicmp>(pfnIsKeyword, p, static_cast<int>(q - p));
		else if (_stscanf(text, _T(" static BOOL IsAutoItKeyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsAutoItKeyword;
		else if (pfnIsKeyword && _stscanf(text, _T(" } %c"), &c) == 1 && (c == ';' ? ++count : 0))
			VerifyKeyword<_tcsnicmp>(pfnIsKeyword = NULL, NULL, 0);
	}
	fclose(file);
	assert(count == 1);
	return count;
}
