setlocal

call "%VS140COMNTOOLS%\vsdevcmd.bat"

REM Locate 7z.exe
set SevenZip=%ProgramFiles%\7-Zip\7z.exe

cd /d "%~dp0"

for %%p in (
  https://7-zip.org/a/7z1900.msi!7z1900.msi
  https://7-zip.org/a/7z1900-x64.msi!7z1900-x64.msi
  https://github.com/kornelski/7z/archive/a6e2c7e401a3e5976e8522de518a169b0d8a7fac.zip!7z1900-src.zip
  https://github.com/htacg/tidy-html5/archive/5.7.28.zip!tidy-html5.zip
  https://github.com/datadiode/winmerge2011/releases/download/0.2011.210.381/WinMerge_0.2011.210.381_setup.cpl!WinMerge_0.2011.210.381_setup.cpl
) do (
  for /F "tokens=1,2 delims=!" %%u in ("%%p") do (
    if not exist 3rdparty\%%v (
      powershell -command "Invoke-WebRequest %%u -Outfile 3rdparty\%%v"
    )
  )
)

set PathTo7zSrc=%~dp03rdparty\7z1900-src\7z-a6e2c7e401a3e5976e8522de518a169b0d8a7fac
set PathTo7zMsi=%~dp03rdparty

call 3rdparty\configure.bat

MSBuild 3rdparty\tidy\build32\tidy.sln /t:Rebuild /p:Platform="Win32" /p:Configuration="MinSizeRel"
MSBuild 3rdparty\tidy\build64\tidy.sln /t:Rebuild /p:Platform="x64" /p:Configuration="MinSizeRel"

"%SevenZip%" x -o3rdparty -t# "%~dp03rdparty\WinMerge_0.2011.210.381_setup.cpl" 1.7zSfxHtm.exe
"%SevenZip%" x -o3rdparty "%~dp03rdparty\1.7zSfxHtm.exe" DOWNLOADER
copy /Y 3rdparty\DOWNLOADER Setup\Support\Downloader.exe

"%ProgramFiles%\Microsoft SDKs\Windows\v7.1\Setup\WindowsSdkVer.exe" -q -version:v7.1

for %%p in (%*) do (
  for /F "tokens=1,2 delims=!" %%u in ("%%p") do (
    MSBuild WinMerge.sln /t:Rebuild /p:Platform="%%u" /p:Configuration="%%v" || goto :eof
  )
)
