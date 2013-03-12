; NSIS script for a Wine compatible WinMerge 2011 installer
; Altered by Jochen Neubeck
;
; Used to be: basic script template for NSIS installers
;
; Written by Philip Chu
; Copyright (c) 2004-2005 Technicat, LLC
;
; This software is provided 'as-is', without any express or implied warranty.
; In no event will the authors be held liable for any damages arising from the use of this software.

; Permission is granted to anyone to use this software for any purpose,
; including commercial applications, and to alter it ; and redistribute
; it freely, subject to the following restrictions:

;    1. The origin of this software must not be misrepresented; you must not claim that
;       you wrote the original software. If you use this software in a product, an
;       acknowledgment in the product documentation would be appreciated but is not required.

;    2. Altered source versions must be plainly marked as such, and must not be
;       misrepresented as being the original software.

;    3. This notice may not be removed or altered from any source distribution.

!define version "0.2011.XXX.XXX"
!define srcdir "..\Build\WinMerge\Win32\Release"
!define setup "..\Build\WinMerge\Win32\WinMerge_${version}_setup.exe"

!define script56 "$%ProgramFiles%\Windows Script 5.6"
!define script56url "ftp://ftp.uni-rostock.de/pub/tools/microsoft/Scripting/us/WSH_5.6/scripten.exe"

; registry stuff

!define regkey "Software\Thingamahoochie\WinMerge"
!define uninstkey "Software\Microsoft\Windows\CurrentVersion\Uninstall\WinMerge 2011"

!define startmenu "$SMPROGRAMS"
!define uninstaller "uninstall.exe"

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
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

AutoCloseWindow false
ShowInstDetails show

; beginning (invisible) section
Section

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

	File /r /x *.lib /x *.exp /x *.pdb /x *.wsf /x ShellExtension*.dll "${srcdir}\*.*"

	File "/oname=$INSTDIR\ShellExtensionU.new" ..\Build\WinMerge\Win32\Release\ShellExtensionU.dll
	Delete /REBOOTOK "$INSTDIR\ShellExtensionU.dll"
	Rename /REBOOTOK "$INSTDIR\ShellExtensionU.new" "$INSTDIR\ShellExtensionU.dll"

	File "/oname=$INSTDIR\ShellExtensionX64.new" ..\Build\WinMerge\x64\Release\ShellExtensionX64.dll
	Delete /REBOOTOK "$INSTDIR\ShellExtensionX64.dll"
	Rename /REBOOTOK $INSTDIR\ShellExtensionX64.new" "$INSTDIR\ShellExtensionX64.dll"

	RegDLL "$INSTDIR\ShellExtensionU.dll"
	RegDLL "$INSTDIR\ShellExtensionX64.dll"

	IfFileExists "$SYSDIR\winecfg.exe" 0 EndInstallScript56
	IfFileExists "${script56}" EndInstallScript56
		InetLoad::load "${script56url}" "$INSTDIR\script56.exe"
		ExecWait '"$INSTDIR\script56.exe" /C /T:"${script56}"'
		Delete "$INSTDIR\script56.exe"

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

	WriteUninstaller "${uninstaller}"

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
	UnRegDLL $INSTDIR\ShellExtensionX64.dll

SectionEnd
