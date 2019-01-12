; NSIS script for a Wine compatible WinMerge 2011 installer
; Modified by Jochen Neubeck from:
; basic script template for NSIS installers
; Written by Philip Chu
; Copyright (c) 2004-2005 Technicat, LLC
; SPDX-License-Identifier: Zlib

!define version "0.2011.009.027"
!define srcdir "..\Build\WinMerge\Win32\Release"
!define unxutils "..\3rdparty\unxutils"
!define setup "..\Build\WinMerge\Win32\WinMerge_${version}_wine_setup.exe"

!define script56 "$%ProgramFiles%\Windows Script 5.6"
!define script56url "http://informax.serveftp.com/files/progtools/wsh56-scr56en.exe"
!define script56hash "f356ee7b0ea496ec3725b340e2aab03847beb504928a6608e6ec0b5bd8fb1f3c"

; registry stuff

!define regkey "Software\Thingamahoochie\WinMerge"
!define uninstkey "Software\Microsoft\Windows\CurrentVersion\Uninstall\WinMerge 2011"

!define startmenu "$SMPROGRAMS"
!define uninstaller "uninstall.exe"

!include LogicLib.nsh
!include WordFunc.nsh
!include x64.nsh

;--------------------------------

XPStyle on
ShowInstDetails hide
ShowUninstDetails hide

Name "WinMerge ${version}"
Caption "WinMerge ${version}"

OutFile "${setup}"

SetDateSave on
SetDatablockOptimize on
CRCCheck on
SilentInstall normal

InstallDir "$PROGRAMFILES\WinMerge2011"
InstallDirRegKey HKLM "${regkey}" ""

LicenseText "License"
LicenseData "${srcdir}\Docs\COPYING"

; pages
; we keep it simple - leave out selectable installation types

Page license

; Page components
Page directory
Page components
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

AutoCloseWindow false
ShowInstDetails show

Function .onInit
	IfFileExists "$SYSDIR\winecfg.exe" EndVerifyPlatform
		${If} ${RunningX64}
			StrCpy $6 "_x64"
		${Else}
			StrCpy $6 ""
		${EndIf}
		MessageBox MB_OK "This installer is for Wine only.$\nPlease run WinMerge_${version}$6_setup.cpl instead."
		Quit
	EndVerifyPlatform:
	System::Call "ntdll::wine_get_version() t .r0 ? c"
	${VersionCompare} "$0" "3.0.4" $1
	IntCmp $1 1 EndVerifyWineVersion EndVerifyWineVersion
		MessageBox MB_OK "You're on Wine $0.$\nThis installer requires Wine 3.0.4 or higher."
		Quit
	EndVerifyWineVersion:
FunctionEnd

; install main application
Section "Main Application (GNU GPLv3)"

	; write executable path to (ir)relevant places
	WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\WinMerge.exe" "" "$INSTDIR\WinMergeU.exe"
	WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\WinMergeU.exe" "" "$INSTDIR\WinMergeU.exe"
	WriteRegStr HKLM "${regkey}" "Executable" "$INSTDIR\WinMergeU.exe"

	; write uninstall strings
	WriteRegStr HKLM "${uninstkey}" "DisplayIcon" "$INSTDIR\WinMergeU.exe"
	WriteRegStr HKLM "${uninstkey}" "DisplayName" "WinMerge 2011"
	WriteRegStr HKLM "${uninstkey}" "DisplayVersion" "WinMerge ${version}"
	WriteRegStr HKLM "${uninstkey}" "InstallLocation" "$INSTDIR"
	WriteRegStr HKLM "${uninstkey}" "Publisher" "Jochen Neubeck"
	WriteRegStr HKLM "${uninstkey}" "UninstallString" '"$INSTDIR\${uninstaller}"'

	WriteRegStr HKCR ".WinMerge" "" "WinMerge.Project.File"
	WriteRegStr HKCR "WinMerge.Project.File\Shell\open\command\" "" '"$INSTDIR\WinMergeU.exe" "%1"'
	WriteRegStr HKCR "WinMerge.Project.File\DefaultIcon" "" '"$INSTDIR\WinMergeU.exe",1'

	SetOutPath $INSTDIR

	; package all files, recursively, preserving attributes
	; assume files are in the correct places
	File /a /r /x *.lib /x *.exp /x *.pdb /x *.hta /x *.json /x *.log /x *.bak /x *.zip /x xdoc2txt "${srcdir}\*.*"

	; include sort.exe from https://sf.net/projects/unxutils, to compensate for Wine's lack of same
	File /a "${unxutils}\sort.exe"

	RegDLL "$INSTDIR\ShellExtensionU.dll"

	WriteUninstaller "${uninstaller}"

SectionEnd

Section "Launch script creation script (usage: sudo sh guess_what_goes_here.exe.sh)"
	FileOpen $0 "$EXEPATH.sh" w
	FileWrite $0 'echo "#!/bin/sh" > /usr/local/bin/WinMerge$\n'
	FileWrite $0 'echo "wine \"$INSTDIR\WinMergeU.exe\" \$${WINMERGE_OPTIONS} \$$* 2> /dev/null" >> /usr/local/bin/WinMerge$\n'
	FileWrite $0 'chmod +x /usr/local/bin/WinMerge$\n'
	FileClose $0
SectionEnd

; download and install Windows Script 5.6
Section "Windows Script 5.6 (separate EULA)"

	IfFileExists "$SYSDIR\winecfg.exe" 0 EndInstallScript56

		StrCpy $7 ${script56hash} 7
		IfFileExists "${script56}\setup-$7.exe" EndDownloadScript56
			CreateDirectory "${script56}"
			InetLoad::load /POPUP "Windows Script 5.6" "${script56url}" "${script56}\setup-$7.exe"
		EndDownloadScript56:

		; verify the SHA2 hash of the file
		Crypto::HashFile "SHA2" "${script56}\setup-$7.exe"
		Pop $0
		StrCmp $0 ${script56hash} +2
		Abort "${script56}\setup-$7.exe is corrupt"

		ExecWait '"${script56}\setup-$7.exe" /C /T:"${script56}"'

		RegDLL "${script56}\jscript.dll"
		RegDLL "${script56}\vbscript.dll"
		RegDLL "${script56}\scrrun.dll"
		RegDLL "${script56}\scrobj.dll"
		RegDLL "${script56}\wshext.dll"
		RegDLL "${script56}\wshcon.dll"
		RegDLL "${script56}\wshom.ocx"

		Delete "${script56}\ADVPACK.DLL"
		Delete "${script56}\W95INF16.DLL"
		Delete "${script56}\W95INF32.DLL"
		Delete "${script56}\wscript.hlp"
		Delete "${script56}\scriptde.inf"
		Delete "${script56}\scriptde.cat"
	EndInstallScript56:

SectionEnd

; create shortcuts
Section

	SetOutPath $INSTDIR ; for working directory
	CreateShortCut "${startmenu}\WinMerge.lnk" "$INSTDIR\WinMergeU.exe"

	ExecShell "open" "$INSTDIR\Docs\ReleaseNotes.html"

SectionEnd

; Uninstaller
; All section names prefixed by "Un" will be in the uninstaller

UninstallText "This will uninstall WinMerge 2011 from registry but leave the files in place."

Section "Uninstall"

	DeleteRegKey HKLM "${uninstkey}"
	DeleteRegKey HKLM "${regkey}"

	Delete "${startmenu}\WinMerge.lnk"

	UnRegDLL $INSTDIR\ShellExtensionU.dll

SectionEnd
