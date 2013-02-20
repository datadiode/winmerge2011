@echo off

if not exist %1 rem: goto :usage
if not exist %2 rem: goto :usage

set SevenZip=C:\7z920bin
"%SevenZip%\7z.exe" e -so %~1 _7z.dll > %~2\Merge7z\7z.dll
copy "%SevenZip%\History.txt" %~2\Merge7z
copy "%SevenZip%\License.txt" %~2\Merge7z
md %~2\Merge7z\Lang
copy "%SevenZip%\Lang" %~2\Merge7z\Lang

exit

:usage
echo ####################################################################
echo # Merge7z.vcproj post-build script                                 #
echo # Not intended for direct invocation through user interface        #
echo # Post-build command line: PostBuild.bat "<msi_file>" "$(OutDir)"  #
echo ####################################################################
pause
