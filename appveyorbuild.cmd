setlocal

call "%VS140COMNTOOLS%\vsdevcmd.bat"

REM Locate 7z.exe
set $=%ProgramFiles%\7-zip;%ProgramFiles(x86)%\7-zip
for %%$ in (7z.exe) do if not "%%~$$:$" == "" set SevenZip=%%~$$:$

cd /d "%~dp0"

mkdir appveyor-downloads

for %%p in (
  https://7-zip.org/a/7z1900.msi!7z1900.msi
  https://7-zip.org/a/7z1900-x64.msi!7z1900-x64.msi
  https://github.com/kornelski/7z/archive/a6e2c7e401a3e5976e8522de518a169b0d8a7fac.zip!7z1900-src.zip
  https://github.com/htacg/tidy-html5/archive/5.7.28.zip!tidy-html5.zip
  https://github.com/datadiode/winmerge2011/releases/download/0.2011.210.381/WinMerge_0.2011.210.381_setup.cpl!WinMerge_0.2011.210.381_setup.cpl
) do (
  for /F "tokens=1,2 delims=!" %%u in ("%%p") do (
    if not exist appveyor-downloads\%%v (
      powershell -command "Invoke-WebRequest %%u -Outfile appveyor-downloads\%%v"
    )
  )
)

"%SevenZip%" x appveyor-downloads\*.zip -aoa -oappveyor-downloads\*

set PathTo7zSrc=%~dp0appveyor-downloads\7z1900-src\7z-a6e2c7e401a3e5976e8522de518a169b0d8a7fac
set PathTo7zMsi=%~dp0appveyor-downloads

copy appveyor-downloads\tidy-html5.zip 3rdparty\
call 3rdparty\configure.bat

MSBuild 3rdparty\tidy\build32\tidy.sln /t:Rebuild /p:Configuration="MinSizeRel"
MSBuild 3rdparty\tidy\build64\tidy.sln /t:Rebuild /p:Configuration="MinSizeRel"

"%SevenZip%" x -oappveyor-downloads -t# "%~dp0appveyor-downloads\WinMerge_0.2011.210.381_setup.cpl" 1.7zSfxHtm.exe
"%SevenZip%" x -oappveyor-downloads "%~dp0appveyor-downloads\1.7zSfxHtm.exe" DOWNLOADER
copy /Y appveyor-downloads\DOWNLOADER Setup\Support\Downloader.exe

"%ProgramFiles%\Microsoft SDKs\Windows\v7.1\Setup\WindowsSdkVer.exe" -q -version:v7.1

for %%p in (%*) do (
  for /F "tokens=1,2 delims=!" %%u in ("%%p") do (
    MSBuild WinMerge.sln /t:Rebuild /p:Platform="%%u" /p:Configuration="%%v" || goto :eof
  )
)
