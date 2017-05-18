@echo on

if not exist %1 rem: goto :usage
if not exist %2 rem: goto :usage

REM Enable echo lines below if you need to debug this script
REM echo %0
REM echo $(IntDir) = %1
REM echo $(OutDir) = %2
REM echo $(PlatformName) = %3

goto :PlatformName:%3

:PlatformName:Win32
set $=..\3rdparty\tidy\build32\MinSizeRel\
for %%$ in (
	tidy.exe
) do if not "%%~$$:$" == "" (
	xcopy /y "%%~$$:$" "%~2"
	set $=
)
goto :PlatformName:

:PlatformName:x64
set $=..\3rdparty\tidy\build64\MinSizeRel\
for %%$ in (
	tidy.exe
) do if not "%%~$$:$" == "" (
	xcopy /y "%%~$$:$" "%~2"
	set $=
)
goto :PlatformName:

:PlatformName:
set $=..\3rdparty\AStyle\build\cb-bcc32c\bin\;..\3rdparty\AStyle\build\cb-mingw\bin\
for %%$ in (
	AStyle.exe
) do if not "%%~$$:$" == "" (
	xcopy /y "%%~$$:$" "%~2"
	set $=
)

REM Copy MSVCRT redistributables for xdoc2txt.exe, from either VS2010 or VS2012,
REM and in case of the latter, rename DLLs to make them usable for xdoc2txt.exe.
if not exist "%~2xdoc2txt" md "%~2xdoc2txt"
set $=%VS100COMNTOOLS%..\..\VC\redist\x86\;%VS110COMNTOOLS%..\..\VC\redist\x86\
for %%$ in (
	Microsoft.VC100.CRT
	Microsoft.VC110.CRT
) do if not "%%~$$:$" == "" (
	xcopy /y "%%~$$:$\msvcr1?0.dll" "%~2xdoc2txt\msvcr100.*"
	xcopy /y "%%~$$:$\msvcp1?0.dll" "%~2xdoc2txt\msvcp100.*"
	set $=
)

REM Copy WinMerge.chm
xcopy /y "..\..\WM2011_Help\Build\Manual\htmlhelp\WinMerge.chm" "%~2Docs\"

REM Create English.pot and MergeLang.rc from Merge.rc
cd ..\Translations\WinMerge
cscript CreateMasterPotFile.vbs

REM Create MergeLang.dll from MergeLang.rc
rc /fo%~1\MergeLang.res /i..\..\Src MergeLang.rc
link /DLL /NOENTRY /MACHINE:IX86 /OUT:"%~2MergeLang.dll" "%~1\MergeLang.res"

exit

:usage
echo ####################################################################
echo # Merge.vcproj post-build script                                   #
echo # Not intended for direct invocation through user interface        #
echo # Post-build command line:                                         #
echo # PostBuild.bat "$(IntDir)" "$(OutDir)" $(PlatformName)            #
echo ####################################################################
pause
