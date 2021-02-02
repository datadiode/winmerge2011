@echo on

if not exist %1 rem: goto :usage
if not exist %2 rem: goto :usage

REM Enable echo lines below if you need to debug this script
REM echo %0
REM echo $(IntDir) = %1
REM echo $(OutDir) = %2
REM echo $(PlatformName) = %3
REM echo $(ConfigurationName) = %4

goto :PlatformName$%3

:PlatformName$Win32

REM Copy AStyle
set $=..\3rdparty\AStyle\build\vs2010\binstatic\
for %%$ in (
	AStyle.exe
) do if not "%%~$$:$" == "" (
	xcopy /y "%%~$$:$" "%~2"
	set $=
)

REM Copy tidy
set $=..\3rdparty\tidy\build32\MinSizeRel\
for %%$ in (
	tidy.exe
) do if not "%%~$$:$" == "" (
	xcopy /y "%%~$$:$" "%~2"
	set $=
)

REM Copy jq
set $=..\3rdparty\jq\
for %%$ in (
	.
) do if not "%%~$$:$" == "" (
	xcopy /yt "%%~$$:$" "%~2jq\"
	copy /y "%$%COPYING" "%~2jq\"
	copy /y "%$%jq-win32.exe" "%~2jq\jq.exe"
	set $=
)

REM Copy WinIMergeLib
set $=..\..\winimerge\Build\%4\;..\3rdparty\WinIMerge\bin\
for %%$ in (
	.
) do if not "%%~$$:$" == "" (
	xcopy /y "%%~$$:$\WinIMergeLib.dll" "%~2WinIMerge\"
	xcopy /y "%%~$$:$\WinIMergeLib.pdb" "%~2WinIMerge\"
	xcopy /y "..\..\freeimage\license-gplv3.txt" "%~2WinIMerge\"
	if errorlevel 1 xcopy /y "..\3rdparty\freeimage-master\license-gplv3.txt" "%~2WinIMerge\"
	set $=
)
goto :PlatformName$

:PlatformName$x64

REM Copy AStyle
set $=..\3rdparty\AStyle\build\vs2010\x64\binstatic\
for %%$ in (
	AStyle.exe
) do if not "%%~$$:$" == "" (
	xcopy /y "%%~$$:$" "%~2"
	set $=
)

REM Copy tidy
set $=..\3rdparty\tidy\build64\MinSizeRel\
for %%$ in (
	tidy.exe
) do if not "%%~$$:$" == "" (
	xcopy /y "%%~$$:$" "%~2"
	set $=
)

REM Copy jq
set $=..\3rdparty\jq\
for %%$ in (
	.
) do if not "%%~$$:$" == "" (
	xcopy /yt "%%~$$:$" "%~2jq\"
	copy /y "%$%COPYING" "%~2jq\"
	copy /y "%$%jq-win64.exe" "%~2jq\jq.exe"
	set $=
)

REM Copy WinIMergeLib
set $=..\..\winimerge\Build\x64\%4\;..\3rdparty\WinIMerge\bin64\
for %%$ in (
	.
) do if not "%%~$$:$" == "" (
	xcopy /y "%%~$$:$\WinIMergeLib.dll" "%~2WinIMerge\"
	xcopy /y "%%~$$:$\WinIMergeLib.pdb" "%~2WinIMerge\"
	xcopy /y "..\..\freeimage\license-gplv3.txt" "%~2WinIMerge\"
	if errorlevel 1 xcopy /y "..\3rdparty\freeimage-master\license-gplv3.txt" "%~2WinIMerge\"
	set $=
)
goto :PlatformName$

:PlatformName$

REM Copy protodec
set $=..\3rdparty\protodec\
for %%$ in (
	protodec.exe
) do if not "%%~$$:$" == "" (
	xcopy /y "%%~$$:$" "%~2"
	set $=
)

REM Copy Frhed
set $=..\..\Frhed\Build\FRHED\%3\UnicodeRelease
for %%$ in (
	.
) do if not "%%~$$:$" == "" (
	xcopy /ys "%%~$$:$" "%~2Frhed\"
	set $=
)

REM Copy SQLiteCompare
set $=..\..\SQLiteCompare\SQLiteTurbo\bin\%4\;..\3rdparty\SQLiteCompare\SQLiteTurbo\bin\%4\
for %%$ in (
	.
) do if not "%%~$$:$" == "" (
	xcopy /ys "%%~$$:$\*.dll" "%~2SQLiteCompare\bin\"
	xcopy /ys "%%~$$:$\*.pdb" "%~2SQLiteCompare\bin\"
	xcopy /y "%%~$$:$\SQLiteCompare.*" "%~2SQLiteCompare\bin\"
	xcopy /y "%%~$$:$\..\..\..\LICENSE" "%~2SQLiteCompare\"
	if errorlevel 1 copy /y ..\3rdparty\SQLiteCompare_LICENSE "%~2SQLiteCompare\LICENSE"
	set $=
)

REM Copy ReoGridCompare
set $=..\..\ReoGrid\Compare\bin\%4\;..\3rdparty\ReoGrid\Compare\bin\%4\
for %%$ in (
	.
) do if not "%%~$$:$" == "" (
	xcopy /ys "%%~$$:$\*.dll" "%~2ReoGridCompare\bin\"
	xcopy /ys "%%~$$:$\*.pdb" "%~2ReoGridCompare\bin\"
	xcopy /y "%%~$$:$\ReoGridCompare.*" "%~2ReoGridCompare\bin\"
	xcopy /y "..\..\ReoGrid\LICENSE" "%~2ReoGridCompare\"
	if errorlevel 1 copy /y ..\3rdparty\ReoGrid_LICENSE "%~2ReoGridCompare\LICENSE"
	set $=
)

REM Copy B2XTranslator
set $=..\..\B2XTranslator\src\Shell\xls2x\bin\%4\;..\3rdparty\B2XTranslator\src\Shell\xls2x\bin\%4\
for %%$ in (
	.
) do if not "%%~$$:$" == "" (
	xcopy /y "%%~$$:$\*.dll" "%~2B2XTranslator\bin\"
	xcopy /y "%%~$$:$\*.pdb" "%~2B2XTranslator\bin\"
	xcopy /y "%%~$$:$\*.exe" "%~2B2XTranslator\bin\"
	xcopy /y "..\..\B2XTranslator\LICENSE" "%~2B2XTranslator\"
	if errorlevel 1 copy /y ..\3rdparty\B2XTranslator_LICENSE "%~2B2XTranslator\LICENSE"
	set $=
)

REM Copy WinMerge.chm
xcopy /y "..\..\winmerge2011_help\Build\Manual\htmlhelp\WinMerge.chm" "%~2Docs\"

REM Create English.pot and MergeLang.rc from Merge.rc
cd ..\Translations\WinMerge
cscript CreateMasterPotFile.vbs

REM Create MergeLang.dll from MergeLang.rc
rc /fo%~1\MergeLang.res /i..\..\Src MergeLang.rc
link /DLL /NOENTRY /MACHINE:IX86 /OUT:"%~2MergeLang.dll" "%~1\MergeLang.res"

exit

:usage
echo ##############################################################################
echo # Merge.vcproj post-build script                                             #
echo # Not intended for direct invocation through user interface                  #
echo # Post-build command line:                                                   #
echo # PostBuild.bat "$(IntDir)" "$(OutDir)" $(PlatformName) $(ConfigurationName) #
echo ##############################################################################
pause
