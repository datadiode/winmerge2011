: << :sh
@echo off
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: This shell script grabs Windows Script 5.6 from download.microsoft.com   ::
:: and installs it to Wine. CAUTION: EXISTING FILES UNDER %WINSYSDIR% ARE   ::
:: REPLACED. THIS WILL TAKE AWAY FROM YOU THE AUTHORITY TO FILE BUG REPORTS ::
:: AT WINEHQ, AS WELL AS TO REDISTRIBUTE COPIES OF YOUR INSTALLATION.       ::
:: Distributed under the GNU General Public License, version 2.1            ::
:: Copyright (C) 2013 Jochen Neubeck                                        ::
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if not exist "%winsysdir%\winecfg.exe" goto :nowinecfg

del %winsysdir%\cscript.exe
del %winsysdir%\dispex.dll
del %winsysdir%\jscript.dll
del %winsysdir%\jsde.dll
del %winsysdir%\scode.dll
del %winsysdir%\scrobj.dll
del %winsysdir%\scrrnde.dll
del %winsysdir%\scrrun.dll
del %winsysdir%\vbscript.dll
del %winsysdir%\vbsde.dll
del %winsysdir%\wscript.exe
del %winsysdir%\wshcon.dll
del %winsysdir%\wshde.dll
del %winsysdir%\wshext.dll
del %winsysdir%\wshom.ocx
C:\scriptde.exe
if errorlevel 1 pause
goto :eof

:nowinecfg
echo This does not appear to be a Wine installation
goto :eof

:sh
cd ~/.wine/drive_c/
xterm -e wget -nc http://download.microsoft.com/download/winscript56/Install/5.6/W98NT42KMe/DE/scriptde.exe
wine start cmd.exe /c $0
