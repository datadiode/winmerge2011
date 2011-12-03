@echo off
@rem  This file was contributed by Ralf Junker, and touched up by
@rem  Daniel Richard G. Tests 10-12 added by Philip H.
@rem  Philip H also changed test 3 to use "wintest" files.
@rem
@rem  Updated by Tom Fortmann to support explicit test numbers on the command line.
@rem  Added argument validation and added error reporting.
@rem
@rem  MS Windows batch file to run pcretest on testfiles with the correct
@rem  options.
@rem
@rem  Output is written to a newly created subfolder named testout.
@rem

setlocal

if [%srcdir%]==[]   set srcdir=.
if [%pcretest%]==[] set pcretest=pcretest

if not exist testout md testout

set do1=no
set do2=no
set do3=no
set do4=no
set do5=no
set do6=no
set do7=no
set do8=no
set do9=no
set do10=no
set do11=no
set do12=no
set all=yes

setlocal enabledelayedexpansion
for %%a in (%*) do (
  set valid=no
  for %%v in (1 2 3 4 5 6 7 8 9 10 11 12) do if %%v == %%a set valid=yes
  if "!valid!" == "yes" (
    set do%%a=yes
    set all=no
  ) else (
    echo Invalid test number - %%a!
        echo Usage: %0 [ test_number ] ...
        echo Where test_number is one or more optional test numbers 1 through 12, default is all tests.
        exit /b 1
  )
)

if "%all%" == "yes" (
  set do1=yes
  set do2=yes
  set do3=yes
  set do4=yes
  set do5=yes
  set do6=yes
  set do7=yes
  set do8=yes
  set do9=yes
  set do10=yes
  set do11=yes
  set do12=yes
)

if "%do1%" == "yes" (
  call :runtest 1 "Main functionality - Compatible with Perl 5.8 and above" -q
  if errorlevel 1 exit /b 1
)

if "%do2%" == "yes" (
  call :runtest 2 "API, errors, internals, and non-Perl stuff" -q
  if errorlevel 1 exit /b 1
)

if "%do3%" == "yes" (
  call :runtest 3 "Locale-specific features" -q
  if errorlevel 1 exit /b 1
)

if "%do4%" == "yes" (
  call :runtest 4 "UTF-8 support - Compatible with Perl 5.8 and above" -q
  if errorlevel 1 exit /b 1
)

if "%do5%" == "yes" (
  call :runtest 5 "API, internals, and non-Perl stuff for UTF-8 support" -q
  if errorlevel 1 exit /b 1
)

if "%do6%" == "yes" (
  call :runtest 6 "Unicode property support - Compatible with Perl 5.10 and above" -q
  if errorlevel 1 exit /b 1
)

if "%do7%" == "yes" (
  call :runtest 7 "DFA matching" -q -dfa
  if errorlevel 1 exit /b 1
)

if "%do8%" == "yes" (
  call :runtest 8 "DFA matching with UTF-8" -q -dfa
  if errorlevel 1 exit /b 1
)

if "%do9%" == "yes" (
  call :runtest 9 "DFA matching with Unicode properties" -q -dfa
  if errorlevel 1 exit /b 1
)

if "%do10%" == "yes" (
  call :runtest 10 "Internal offsets and code size tests" -q
  if errorlevel 1 exit /b 1
)

if "%do11%" == "yes" (
  call :runtest 11 "Features from Perl 5.10 and above" -q
  if errorlevel 1 exit /b 1
)

if "%do12%" == "yes" (
  call :runtest 12 "API, internals, and non-Perl stuff for Unicode property support" -q
  if errorlevel 1 exit /b 1
)

exit /b 0


@rem Function to execute pcretest and compare the output
@rem Arguments are as follows:
@rem
@rem       1 = test number
@rem       2 = test name (use double quotes)
@rem   3 - 9 = pcretest options

:runtest

if [%1] == [] (
  echo Missing test number argument!
  exit /b 1
)

if [%2] == [] (
  echo Missing test name argument!
  exit /b 1
)

set testinput=testinput%1
set testoutput=testoutput%1
if exist %srcdir%\testdata\win%testinput% (
  set testinput=wintestinput%1
  set testoutput=wintestoutput%1
)

echo.
echo Test %1: %2

%pcretest% %3 %4 %5 %6 %7 %8 %9 %srcdir%\testdata\%testinput% > testout\%testoutput%
if errorlevel 1 (
  echo Test %1: %pcretest% failed!
  exit /b 1
)

fc /n %srcdir%\testdata\%testoutput% testout\%testoutput%
if errorlevel 1 (
  echo Test %1: file compare failed!
  exit /b 1
)

echo Test %1: Passed.
echo.
exit /b 0

