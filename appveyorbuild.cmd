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
) do (
  for /F "tokens=1,2 delims=!" %%u in ("%%p") do (
    if not exist appveyor-downloads\%%v (
      powershell -command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest %%u -Outfile appveyor-downloads\%%v"
    )
  )
)

"%SevenZip%" x appveyor-downloads\*.zip -aoa -oappveyor-downloads\*

set PathTo7zSrc=%~dp0appveyor-downloads\7z1900-src\7z-a6e2c7e401a3e5976e8522de518a169b0d8a7fac
set PathTo7zMsi=%~dp0appveyor-downloads

for %%p in (%*) do (
  for /F "tokens=1,2 delims=!" %%u in ("%%p") do (
    MSBuild WinMerge.sln /t:Rebuild /p:Platform="%%u" /p:Configuration="%%v" || pause
  )
)
