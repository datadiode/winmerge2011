:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Creates projects/workspaces/solutions for involved 3rd party components.  ::
:: To fix VS90 issues, you may want to get VS90SP1-KB980263-x86.exe from     ::
:: thehotfixshare.net/board/index.php?autocom=downloads&showfile=11873.      ::
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
@echo off
setlocal
set SevenZip=%ProgramFiles%\7-Zip\
cd /d "%~dp0"
REM Extract all archives contained in this folder, but keep any existing files
"%SevenZip%\7z.exe" x -aos "*.zip"
REM Run CMake on tidy source tree
for /d %%x in (%~dp0\tidy-html*) do (
	if not exist "%%x\CMakeLists.txt" goto :invalid-argument
	if not exist "%~dp0tidy" md "%~dp0tidy"
	if not exist "%~dp0tidy\build32" (
		md "%~dp0tidy\build32"
		cd /d "%~dp0tidy\build32"
		cmake -D "CMAKE_USER_MAKE_RULES_OVERRIDE=%~dp0c_flag_overrides.cmake" -D "CMAKE_USER_MAKE_RULES_OVERRIDE_CXX=%~dp0cxx_flag_overrides.cmake" -G "Visual Studio 10 2010" -Tv90 -DSUPPORT_LOCALIZATIONS=0 -DBUILD_SHARED_LIB=OFF "%%x"
	)
	if not exist "%~dp0tidy\build64" (
		md "%~dp0tidy\build64"
		cd /d "%~dp0tidy\build64"
		cmake -D "CMAKE_USER_MAKE_RULES_OVERRIDE=%~dp0c_flag_overrides.cmake" -D "CMAKE_USER_MAKE_RULES_OVERRIDE_CXX=%~dp0cxx_flag_overrides.cmake" -G "Visual Studio 10 2010 Win64" -DSUPPORT_LOCALIZATIONS=0 -DBUILD_SHARED_LIB=OFF "%%x"
	)
)
goto :quit

:invalid-argument
echo Invalid argument
goto :quit

:quit
REM Wait for keystroke if invoked from explorer
for %%x in (%cmdcmdline%) do for %%y in (%%~x) do if %%y == %0 pause
