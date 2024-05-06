@echo on

if not exist %1 rem: goto :usage
if not exist %2 rem: goto :usage

REM Enable echo lines below if you need to debug this script
REM echo %0
REM echo $(IntDir) = %1
REM echo $(OutDir) = %2
REM echo $(PlatformName) = %3
REM echo $(ConfigurationName) = %4

for /f tokens^=2^ delims^=^" %%$ in (
  'findstr "\/Version" ..\Setup_%3.bat'
) do set WinMergeTag=%%$

echo WinMergeTag=%WinMergeTag%

call WriteDocFile.bat ..\Docs\Users\CHANGES > "%~2Docs\CHANGES"
call WriteDocFile.bat ..\Docs\Users\ReleaseNotes.html > "%~2Docs\ReleaseNotes.html"

goto :PlatformName$%3

:PlatformName$Win32

REM Copy AStyle
for /R ..\3rdparty %f in (build\vs2015\x86\binstatic\AStyle.exe) do if exist "%f" copy /y "%f" "%~2"

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
	if errorlevel 1 copy /y "..\3rdparty\FreeImage_license-gplv3.txt" "%~2WinIMerge\license-gplv3.txt"
	set $=
)
goto :PlatformName$

:PlatformName$x64

REM Copy AStyle
for /R ..\3rdparty %f in (build\vs2015\x64\binstatic\AStyle.exe) do if exist "%f" copy /y "%f" "%~2"

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
	if errorlevel 1 copy /y "..\3rdparty\FreeImage_license-gplv3.txt" "%~2WinIMerge\license-gplv3.txt"
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
set $=..\..\Frhed\Build\FRHED\%3\Unicode%4;..\3rdparty\Frhed\Build\FRHED\%3\Unicode%4
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

REM Copy ToxyExtract
set $=..\..\toxy\Toxy.Tools\ToxyExtract\bin\%4\net40\;..\3rdparty\toxy\Toxy.Tools\ToxyExtract\bin\%4\
for %%$ in (
	.
) do if not "%%~$$:$" == "" (
	xcopy /y "%%~$$:$\*.dll" "%~2ToxyExtract\bin\"
	xcopy /y "%%~$$:$\*.pdb" "%~2ToxyExtract\bin\"
	xcopy /y "%%~$$:$\ToxyExtract.*" "%~2ToxyExtract\bin\"
	xcopy /y "..\..\toxy\LICENSE" "%~2ToxyExtract\"
	if errorlevel 1 copy /y ..\3rdparty\toxy_LICENSE "%~2ToxyExtract\LICENSE"
	set $=
)

REM Copy B2XTranslator\src\Shell\xls2x
set $=..\..\B2XTranslator\src\Shell\xls2x\bin\%4\net40\;..\3rdparty\B2XTranslator\src\Shell\xls2x\bin\%4\
for %%$ in (
	.
) do if not "%%~$$:$" == "" (
	xcopy /ys "%%~$$:$\*.dll" "%~2B2XTranslator\bin\"
	xcopy /ys "%%~$$:$\*.pdb" "%~2B2XTranslator\bin\"
	xcopy /y "%%~$$:$\*.exe" "%~2B2XTranslator\bin\"
	set $=
)

REM Copy B2XTranslator\src\Shell\doc2x
set $=..\..\B2XTranslator\src\Shell\doc2x\bin\%4\net40\;..\3rdparty\B2XTranslator\src\Shell\doc2x\bin\%4\
for %%$ in (
	.
) do if not "%%~$$:$" == "" (
	xcopy /ys "%%~$$:$\*.dll" "%~2B2XTranslator\bin\"
	xcopy /ys "%%~$$:$\*.pdb" "%~2B2XTranslator\bin\"
	xcopy /y "%%~$$:$\*.exe" "%~2B2XTranslator\bin\"
	set $=
)

REM Copy B2XTranslator\src\Shell\ppt2x
set $=..\..\B2XTranslator\src\Shell\ppt2x\bin\%4\net40\;..\3rdparty\B2XTranslator\src\Shell\ppt2x\bin\%4\
for %%$ in (
	.
) do if not "%%~$$:$" == "" (
	xcopy /ys "%%~$$:$\*.dll" "%~2B2XTranslator\bin\"
	xcopy /ys "%%~$$:$\*.pdb" "%~2B2XTranslator\bin\"
	xcopy /y "%%~$$:$\*.exe" "%~2B2XTranslator\bin\"
	set $=
)

REM Copy B2XTranslator\src\Shell\OdfConverter
set $=..\..\B2XTranslator\src\Shell\OdfConverter\bin\%4\net40\;..\3rdparty\B2XTranslator\src\Shell\OdfConverter\bin\%4\
for %%$ in (
	.
) do if not "%%~$$:$" == "" (
	xcopy /ys "%%~$$:$\*.dll" "%~2B2XTranslator\bin\"
	xcopy /ys "%%~$$:$\*.pdb" "%~2B2XTranslator\bin\"
	xcopy /y "%%~$$:$\*.exe" "%~2B2XTranslator\bin\"
	set $=
)

REM Copy B2XTranslator\LICENSE
xcopy /y "..\..\B2XTranslator\LICENSE" "%~2B2XTranslator\"
if errorlevel 1 copy /y ..\3rdparty\B2XTranslator_LICENSE "%~2B2XTranslator\LICENSE"

REM Copy WinMerge.chm
set $=..\..\winmerge2011_help\Build\Manual\htmlhelp\;..\3rdparty\
for %%$ in (
	WinMerge.chm
) do if not "%%~$$:$" == "" (
	copy /y "%%~$$:$" "%~2Docs\"
	set $=
)

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
