setlocal

for /f "tokens=1,* delims=:" %%A in ('findstr /n $ %1') do (
  set line=%%B
  setlocal EnableDelayedExpansion
  if not "!line!"=="" (
    set line=!line:[WinMergeTag]=%WinMergeTag%!
    set line=!line:[WinMergeCommitHash]=%GIT_DESCRIBE:~-40%!
    set line=!line:[FrhedTag]=%FrhedTag%!
    set line=!line:[WinIMergeTag]=%WinIMergeTag%!
    set line=!line:[SQLiteCompareTag]=%SQLiteCompareTag%!
    set line=!line:[ReoGridTag]=%ReoGridTag%!
    set line=!line:[B2XTranslatorTag]=%B2XTranslatorTag%!
    set line=!line:[ToxyTag]=%ToxyTag%!
    set line=!line:[AStyleTag]=%AStyleTag%!
    set line=!line:[HtmlTidyTag]=%HtmlTidyTag%!
    set line=!line:[ProtoDecTag]=%ProtoDecTag%!
    set line=!line:[jqTag]=%jqTag%!
  )
  echo.!line!
  endlocal
)
