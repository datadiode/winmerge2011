setlocal

FOR /F "tokens=*" %%A IN ('"git describe --tags --long --abbrev=40"') DO SET GIT_DESCRIBE=%%A
echo GIT_DESCRIBE=%GIT_DESCRIBE%

set FrhedTag=0.10909.2021
set WinIMergeTag=1.0.25
set SQLiteCompareTag=3.1.0.0-datadiode-008
set ReoGridTag=3.0.0-datadiode-66-g4616e16
set B2XTranslatorTag=1.0.0-datadiode-19-gaf4be09
set ToxyTag=1.6.0-datadiode-49-gc904104
set AStyleTag=3.1
set HtmlTidyTag=5.7.28
set ProtoDecTag=0.6.3
set jqTag=1.6

call "%VS140COMNTOOLS%\vsdevcmd.bat"

REM Locate 7z.exe
set SevenZip=%ProgramFiles%\7-Zip\7z.exe

cd /d "%~dp0"

for %%p in (
  https://7-zip.org/a/7z1900.msi!7z1900.msi
  https://7-zip.org/a/7z1900-x64.msi!7z1900-x64.msi
  https://github.com/kornelski/7z/archive/a6e2c7e401a3e5976e8522de518a169b0d8a7fac.zip!7z1900-src.zip
  https://github.com/htacg/tidy-html5/archive/%HtmlTidyTag%.zip!tidy-html5.zip
  https://fossies.org/windows/misc/AStyle_%AStyleTag%_windows.zip!AStyle_windows.zip
  https://github.com/schdub/protodec/releases/download/v%ProtoDecTag%-win/protodec.exe!protodec\protodec.exe
  https://raw.githubusercontent.com/stedolan/jq/master/COPYING!jq\COPYING
  https://github.com/stedolan/jq/releases/download/jq-%jqTag%/jq-win32.exe!jq\jq-win32.exe
  https://github.com/stedolan/jq/releases/download/jq-%jqTag%/jq-win64.exe!jq\jq-win64.exe
  https://github.com/datadiode/frhed/releases/download/v%FrhedTag%/Frhed_%FrhedTag%-Win32-UnicodeDebug.zip!Frhed_Win32-UnicodeDebug
  https://github.com/datadiode/frhed/releases/download/v%FrhedTag%/Frhed_%FrhedTag%-Win32-UnicodeRelease.zip!Frhed_Win32-UnicodeRelease
  https://github.com/datadiode/frhed/releases/download/v%FrhedTag%/Frhed_%FrhedTag%-x64-UnicodeDebug.zip!Frhed_x64-UnicodeDebug
  https://github.com/datadiode/frhed/releases/download/v%FrhedTag%/Frhed_%FrhedTag%-x64-UnicodeRelease.zip!Frhed_x64-UnicodeRelease
  https://raw.githubusercontent.com/datadiode/SQLiteCompare/master/LICENSE!SQLiteCompare_LICENSE
  https://github.com/datadiode/SQLiteCompare/releases/download/%SQLiteCompareTag%/SQLiteCompare_%SQLiteCompareTag%-Debug.zip!SQLiteCompare_Debug
  https://github.com/datadiode/SQLiteCompare/releases/download/%SQLiteCompareTag%/SQLiteCompare_%SQLiteCompareTag%-Release.zip!SQLiteCompare_Release
  https://raw.githubusercontent.com/datadiode/ReoGrid/master/LICENSE!ReoGrid_LICENSE
  https://github.com/datadiode/ReoGrid/releases/download/%ReoGridTag%/ReoGridCompare_%ReoGridTag%-Debug.zip!ReoGridCompare_Debug
  https://github.com/datadiode/ReoGrid/releases/download/%ReoGridTag%/ReoGridCompare_%ReoGridTag%-Release.zip!ReoGridCompare_Release
  https://raw.githubusercontent.com/datadiode/toxy/master/LICENSE!toxy_LICENSE
  https://github.com/datadiode/toxy/releases/download/%ToxyTag%/ToxyExtract_%ToxyTag%-Debug.zip!ToxyExtract_Debug
  https://github.com/datadiode/toxy/releases/download/%ToxyTag%/ToxyExtract_%ToxyTag%-Release.zip!ToxyExtract_Release
  https://raw.githubusercontent.com/datadiode/B2XTranslator/master/LICENSE!B2XTranslator_LICENSE
  https://github.com/datadiode/B2XTranslator/releases/download/%B2XTranslatorTag%/doc2x_%B2XTranslatorTag%-Debug.zip!doc2x_Debug
  https://github.com/datadiode/B2XTranslator/releases/download/%B2XTranslatorTag%/doc2x_%B2XTranslatorTag%-Release.zip!doc2x_Release
  https://github.com/datadiode/B2XTranslator/releases/download/%B2XTranslatorTag%/xls2x_%B2XTranslatorTag%-Debug.zip!xls2x_Debug
  https://github.com/datadiode/B2XTranslator/releases/download/%B2XTranslatorTag%/xls2x_%B2XTranslatorTag%-Release.zip!xls2x_Release
  https://github.com/datadiode/B2XTranslator/releases/download/%B2XTranslatorTag%/OdfConverter_%B2XTranslatorTag%-Debug.zip!OdfConverter_Debug
  https://github.com/datadiode/B2XTranslator/releases/download/%B2XTranslatorTag%/OdfConverter_%B2XTranslatorTag%-Release.zip!OdfConverter_Release
  https://raw.githubusercontent.com/WinMerge/freeimage/master/license-gplv3.txt!FreeImage_license-gplv3.txt
  https://github.com/WinMerge/winimerge/releases/download/v%WinIMergeTag%/winimerge-%WinIMergeTag:.=-%-0-exe.zip!winimerge-exe.zip
  https://github.com/datadiode/winmerge2011/releases/download/0.2011.210.381/WinMerge_0.2011.210.381_setup.cpl!WinMerge_0.2011.210.381_setup.cpl
  https://github.com/datadiode/winmerge2011_help/releases/download/0.2011.008.226/WinMerge.chm!WinMerge.chm
) do (
  for /F "tokens=1,2 delims=!" %%u in ("%%p") do (
    if not exist 3rdparty\%%v (
      powershell -command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest %%u -Outfile 3rdparty\%%v"
    )
  )
)

set PathTo7zSrc=%~dp03rdparty\7z-a6e2c7e401a3e5976e8522de518a169b0d8a7fac
set PathTo7zMsi=%~dp03rdparty

call 3rdparty\configure.bat

MSBuild 3rdparty\tidy\build32\tidy.sln /t:Rebuild /p:Platform="Win32" /p:Configuration="MinSizeRel"
MSBuild 3rdparty\tidy\build64\tidy.sln /t:Rebuild /p:Platform="x64" /p:Configuration="MinSizeRel"

MSBuild 3rdparty\AStyle\build\vs2010\AStyle.sln /t:Rebuild /p:Platform="Win32" /p:Configuration="Static"
MSBuild 3rdparty\AStyle\build\vs2010\AStyle.sln /t:Rebuild /p:Platform="x64" /p:Configuration="Static"

"%SevenZip%" x -o3rdparty\Frhed\Build\FRHED\Win32\UnicodeDebug\ "%~dp03rdparty\Frhed_Win32-UnicodeDebug"
"%SevenZip%" x -o3rdparty\Frhed\Build\FRHED\Win32\UnicodeRelease\ "%~dp03rdparty\Frhed_Win32-UnicodeRelease"
"%SevenZip%" x -o3rdparty\Frhed\Build\FRHED\x64\UnicodeDebug\ "%~dp03rdparty\Frhed_x64-UnicodeDebug"
"%SevenZip%" x -o3rdparty\Frhed\Build\FRHED\x64\UnicodeRelease\ "%~dp03rdparty\Frhed_x64-UnicodeRelease"

"%SevenZip%" x -o3rdparty\SQLiteCompare\SQLiteTurbo\bin\Debug\ "%~dp03rdparty\SQLiteCompare_Debug"
"%SevenZip%" x -o3rdparty\SQLiteCompare\SQLiteTurbo\bin\Release\ "%~dp03rdparty\SQLiteCompare_Release"

"%SevenZip%" x -o3rdparty\ReoGrid\Compare\bin\Debug\ "%~dp03rdparty\ReoGridCompare_Debug"
"%SevenZip%" x -o3rdparty\ReoGrid\Compare\bin\Release\ "%~dp03rdparty\ReoGridCompare_Release"

"%SevenZip%" x -o3rdparty\toxy\Toxy.Tools\ToxyExtract\bin\Debug\ "%~dp03rdparty\ToxyExtract_Debug"
"%SevenZip%" x -o3rdparty\toxy\Toxy.Tools\ToxyExtract\bin\Release\ "%~dp03rdparty\ToxyExtract_Release"

"%SevenZip%" x -o3rdparty\B2XTranslator\src\Shell\xls2x\bin\Debug\ "%~dp03rdparty\xls2x_Debug"
"%SevenZip%" x -o3rdparty\B2XTranslator\src\Shell\xls2x\bin\Release\ "%~dp03rdparty\xls2x_Release"
"%SevenZip%" x -o3rdparty\B2XTranslator\src\Shell\doc2x\bin\Debug\ "%~dp03rdparty\doc2x_Debug"
"%SevenZip%" x -o3rdparty\B2XTranslator\src\Shell\doc2x\bin\Release\ "%~dp03rdparty\doc2x_Release"
"%SevenZip%" x -o3rdparty\B2XTranslator\src\Shell\OdfConverter\bin\Debug\ "%~dp03rdparty\OdfConverter_Debug"
"%SevenZip%" x -o3rdparty\B2XTranslator\src\Shell\OdfConverter\bin\Release\ "%~dp03rdparty\OdfConverter_Release"

"%SevenZip%" x -o3rdparty -t# "%~dp03rdparty\WinMerge_0.2011.210.381_setup.cpl" 1.7zSfxHtm.exe
"%SevenZip%" x -o3rdparty "%~dp03rdparty\1.7zSfxHtm.exe" DOWNLOADER
copy /Y 3rdparty\DOWNLOADER Setup\Support\Downloader.exe

"%ProgramFiles%\Microsoft SDKs\Windows\v7.1\Setup\WindowsSdkVer.exe" -q -version:v7.1

for %%p in (%*) do (
  for /F "tokens=1,2 delims=!" %%u in ("%%p") do (
    MSBuild WinMerge.sln /t:Rebuild /p:Platform="%%u" /p:Configuration="%%v" || goto :eof
  )
)

start /wait "%SystemRoot%\system32\mshta.exe" "%~dp0Setup\setup.hta" "silent:hidden:version" "%~dp0src\version.rh" "Win32" "%GIT_DESCRIBE%"
start /wait "%SystemRoot%\system32\mshta.exe" "%~dp0Setup\setup.hta" "silent:hidden:version" "%~dp0src\version.rh" "x64" "%GIT_DESCRIBE%"

type "%~dp0Setup\setup.err"
type "%~dp0src\version.rh"
dir *.bat

for %%f in (Setup_*.bat) do call %%f
dir *.7z
dir *.cpl
