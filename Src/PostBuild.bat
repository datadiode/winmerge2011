@echo on

if not exist %1 rem: goto :usage
if not exist %2 rem: goto :usage

REM $Id$
REM Enable echo lines below if you need to debug this script
REM echo %0
REM echo $(IntDir) = %1
REM echo $(OutDir) = %2

REM copy WinMerge.chm
copy ..\..\WM2011_Help\Build\Manual\htmlhelp\WinMerge.chm %~2\Docs\

REM Create English.pot and MergeLang.rc from Merge.rc
cd ..\Translations\WinMerge
cscript CreateMasterPotFile.vbs

REM Create MergeLang.dll from MergeLang.rc
rc /fo%~1\MergeLang.res /i..\..\Src MergeLang.rc
link /DLL /NOENTRY /MACHINE:IX86 /OUT:%~2\MergeLang.dll %~1\MergeLang.res

REM Create a WinMergeU.dat which is usable for everyone
del "%~2\..\..\WinMergeU.dat"
reg save HKCU\{08CEC68E-416D-4fae-962D-16A8E838C6F5} "%~2\..\..\WinMergeU.dat"
exit

:usage
echo ####################################################################
echo # Merge.vcproj post-build script                                   #
echo # Not intended for direct invocation through user interface        #
echo # Post-build command line: PostBuild.bat "$(IntDir)" "$(OutDir)"   #
echo ####################################################################
pause
