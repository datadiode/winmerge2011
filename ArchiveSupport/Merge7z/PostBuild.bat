@echo off

if not exist %1 rem: goto :usage
if not exist %2 rem: goto :usage

set SevenZip=\7z1602bin
"%SevenZip%\7z.exe" e -so %~1 _7z.dll > %~2\7z.dll
copy "%SevenZip%\History.txt" %~2
copy "%SevenZip%\License.txt" %~2
md %~2\Lang
copy "%SevenZip%\Lang" %~2\Lang

exit

:usage
echo ####################################################################
echo # Merge7z.vcproj post-build script                                 #
echo # Not intended for direct invocation through user interface        #
echo # Post-build command line: PostBuild.bat "<msi_file>" "$(OutDir)"  #
echo ####################################################################
pause
