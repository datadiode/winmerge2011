Option Explicit
''
' This script creates the master POT file (English.pot).
'
' Copyright (C) 2007-2009 by Tim Gerundt
' Released under the "GNU General Public License"
'
' ID line follows -- this is updated by SVN
' $Id$

Const ForReading = 1

Const NO_BLOCK = 0
Const MENU_BLOCK = 1
Const DIALOGEX_BLOCK = 2
Const STRINGTABLE_BLOCK = 3
Const VERSIONINFO_BLOCK = 4
Const ACCELERATORS_BLOCK = 5
Const TEXTINCLUDE_BLOCK = 6

Const PATH_ENGLISH_POT = "English.pot"
Const PATH_MERGE_RC = "../../Src/Merge.rc"
Const PATH_MERGELANG_RC = "MergeLang.rc"

Dim oFSO
Set oFSO = CreateObject("Scripting.FileSystemObject")

Dim oShell
Set oShell = Wscript.CreateObject("WScript.Shell")

Dim env
Set env = oShell.environment("Process")

Dim VCInstallDir
VCInstallDir = env("VCINSTALLDIR")

Dim bRunFromCmd
bRunFromCmd = LCase(oFSO.GetFileName(Wscript.FullName)) = "cscript.exe"

Call Main

''
' ...
Sub Main
  Dim oStrings
  Dim StartTime, EndTime, Seconds
  Dim bNecessary, oFile
  
  StartTime = Time
  
  InfoBox "Creating POT file from Merge.rc...", 3
  
  bNecessary = True
  If oFSO.FileExists(PATH_ENGLISH_POT) And oFSO.FileExists(PATH_MERGELANG_RC) Then 'If the POT and RC file exists...
    'bNecessary = GetArchiveBit(PATH_MERGE_RC) Or GetArchiveBit(PATH_ENGLISH_POT) Or GetArchiveBit(PATH_MERGELANG_RC) 'RCs or POT file changed?
  End If
  
  If bNecessary Then 'If update necessary...
    Set oStrings = GetStringsFromRcFile(PATH_MERGE_RC)
    CreateMasterPotFile PATH_ENGLISH_POT, oStrings, "1252"
    SetArchiveBit PATH_MERGE_RC, False
    SetArchiveBit PATH_ENGLISH_POT, False
    SetArchiveBit PATH_MERGELANG_RC, False
    For Each oFile In oFSO.GetFolder(".").Files 'For all files in the current folder...
      If (LCase(oFSO.GetExtensionName(oFile.Name)) = "po") Then 'If a PO file...
        SetArchiveBit oFile.Path, True
      End If
    Next
    
    EndTime = Time
    Seconds = DateDiff("s", StartTime, EndTime)
    
    InfoBox "POT file created, after " & Seconds & " second(s).", 10
  Else 'If update NOT necessary...
    InfoBox "POT file already up-to-date.", 10
  End If
End Sub

''
' ...
Class CString
  Dim Comment, References, Context, Id, Str
End Class

''
' ...
Function GetStringsFromRcFile(ByVal sRcFilePath)
  Dim oBlacklist, oStrings, oString, oRcFile, sLine, iLine, iDepth
  Dim sRcFileName, iBlockType, sReference, sString, sComment, sContext, oMatch, sTemp, sKey
  Dim oLcFile, fContinuation

  Set oBlacklist = GetStringBlacklist("StringBlacklist.txt")
  
  Set oStrings = CreateObject("Scripting.Dictionary")
  Set oLcFile = oFSO.CreateTextFile("MergeLang.rc", True)
  
  'If oFSO.FileExists(sRcFilePath) Then 'If the RC file exists...
  For Each sRcFilePath In Array(sRcFilePath, VCInstallDir & "\atlmfc\include\afxres.rc", VCInstallDir & "\atlmfc\include\afxprint.rc")
    sRcFileName = oFSO.GetFileName(sRcFilePath)
    iDepth = 0
    iLine = 1
    iBlockType = NO_BLOCK
    fContinuation = False
    Set oRcFile = oFSO.OpenTextFile(sRcFilePath, ForReading)
    Do Until oRcFile.AtEndOfStream 'For all lines...
      sLine = oRcFile.ReadLine
      If InStr(sLine, "BEGIN") = 1 Or InStr(sLine, "{") = 1 Then 'BEGIN...
        iDepth = iDepth + 1
      ElseIf InStr(sLine, "END") = 1  Or InStr(sLine, "}") = 1 Then 'END...
        iDepth = iDepth - 1
        If iDepth = 0 Then iBlockType = NO_BLOCK
      ElseIf iBlockType = NO_BLOCK Then
        If InStr(sLine, " MENU") > 0 Then 'MENU...
          iBlockType = MENU_BLOCK
        ElseIf InStr(sLine, " DIALOG ") > 0 Then 'DIALOG...
          iBlockType = DIALOGEX_BLOCK
        ElseIf InStr(sLine, " DIALOGEX ") > 0 Then 'DIALOGEX...
          iBlockType = DIALOGEX_BLOCK
        ElseIf InStr(sLine, "STRINGTABLE") = 1 Then 'STRINGTABLE...
          iBlockType = STRINGTABLE_BLOCK
        ElseIf InStr(sLine, " VERSIONINFO") > 0 Then 'VERSIONINFO...
          iBlockType = VERSIONINFO_BLOCK
        ElseIf InStr(sLine, " ACCELERATORS") > 0 Then 'ACCELERATORS...
          iBlockType = ACCELERATORS_BLOCK
        ElseIf InStr(sLine, " TEXTINCLUDE") > 0 Then 'TEXTINCLUDE...
          iBlockType = TEXTINCLUDE_BLOCK
        End If
      Else
        sReference = sRcFileName & ":" & iLine
        sString = ""
        sComment = ""
        sContext = ""
        Select Case iBlockType
        Case MENU_BLOCK, DIALOGEX_BLOCK, STRINGTABLE_BLOCK:
          If InStr(sLine, """") > 0 And Not fContinuation Then 'If quote found (for speedup)...
            '--------------------------------------------------------------------------------
            ' Replace 1st string literal only - 2nd string literal specifies control class!
            '--------------------------------------------------------------------------------
            If FoundRegExpMatch(sLine, """((?:""""|[^""])*)""", oMatch) Then 'String...
              sTemp = oMatch.SubMatches(0)
              If (sTemp <> "") And (oBlacklist.Exists(sTemp) = False) Then 'If NOT blacklisted...
                sLine = Replace(sLine, """" & sTemp & """", """" & sReference & """", 1, 1)
                sString = Replace(sTemp, """""", "\""")
                If (FoundRegExpMatch(sLine, "//#\. (.*?)$", oMatch) = True) Then 'If found a comment for the translators...
                  sComment = Trim(oMatch.SubMatches(0))
                ElseIf (FoundRegExpMatch(sLine, "//msgctxt (.*?)$", oMatch) = True) Then 'If found a context for the translation...
                  sContext = Trim(oMatch.SubMatches(0))
                  sComment = sContext
                End If
              End If
            End If
          End If
          
        Case VERSIONINFO_BLOCK:
          If (FoundRegExpMatch(sLine, "BLOCK ""([0-9A-F]+)""", oMatch) = True) Then 'StringFileInfo.Block...
            sString = oMatch.SubMatches(0)
            sComment = "StringFileInfo.Block"
          ElseIf (FoundRegExpMatch(sLine, "VALUE ""Comments"", ""(.*?)\\?0?""", oMatch) = True) Then 'StringFileInfo.Comments...
            sString = oMatch.SubMatches(0)
            sComment = "You should use a string like ""Translated by "" followed by the translator names for this string. It is ONLY VISIBLE in the StringFileInfo.Comments property from the final resource file!"
          ElseIf (FoundRegExpMatch(sLine, "VALUE ""Translation"", (.*?)$", oMatch) = True) Then 'VarFileInfo.Translation...
            sString = oMatch.SubMatches(0)
            sComment = "VarFileInfo.Translation"
          End If
        End Select
      
        If sString <> "" Then
          sKey = sContext & "\-" & sString
          If oStrings.Exists(sKey) Then 'If the key is already used...
            Set oString = oStrings(sKey)
          Else
            Set oString = New CString
          End If
          If sComment <> "" Then
            oString.Comment = sComment
          End If
          If oString.References <> "" Then
            oString.References = oString.References & vbTab & sReference
          Else
            oString.References = sReference
          End If
          oString.Context = sContext
          oString.Id = sString
          oString.Str = ""
          If oStrings.Exists(sKey) Then 'If the key is already used...
            Set oStrings(sKey) = oString
          Else 'If the key is NOT already used...
            oStrings.Add sKey, oString
          End If
          fContinuation = InStr(",|", Right(sLine, 1)) <> 0
        Else
          fContinuation = False
        End If
      End If
      If sLine = "#ifndef APSTUDIO_INVOKED" Then Exit Do
      oLcFile.WriteLine sLine
      iLine = iLine + 1
    Loop
    oRcFile.Close
    ' The Exit below causes exclusion of afxres.rc and afxprint.rc.
    ' These are only needed for linking statically with MFC, which does not
	' apply to current WinMerge2011, but has proven to work in earlier code.
	' Current WinMerge2011 does not link with MFC at all.
    Exit For
  Next
  oLcFile.WriteLine "MERGEPOT RCDATA ""English.pot"""
  oLcFile.WriteLine "#include ""version.rc"""
  oLcFile.Close
  Set GetStringsFromRcFile = oStrings
End Function

''
' ...
Function GetStringBlacklist(ByVal sTxtFilePath)
  Dim oBlacklist, oTxtFile, sLine
  
  Set oBlacklist = CreateObject("Scripting.Dictionary")
  
  If (oFSO.FileExists(sTxtFilePath) = True) Then 'If the blacklist file exists...
    Set oTxtFile = oFSO.OpenTextFile(sTxtFilePath, ForReading)
    Do Until oTxtFile.AtEndOfStream = True 'For all lines...
      sLine = Trim(oTxtFile.ReadLine)
      
      If (sLine <> "") Then
        If (oBlacklist.Exists(sLine) = False) Then 'If the key is NOT already used...
          oBlacklist.Add sLine, True
        End If
      End If
    Loop
    oTxtFile.Close
  End If
  Set GetStringBlacklist = oBlacklist
End Function

''
' ...
Sub CreateMasterPotFile(ByVal sPotPath, ByVal oStrings, ByVal sCodePage)
  Dim oPotFile, sKey, oString, aReferences, i
  
  Set oPotFile = oFSO.CreateTextFile(sPotPath, True)
  
  oPotFile.WriteLine "# This file is part from WinMerge <http://winmerge.org/>"
  oPotFile.WriteLine "# Released under the ""GNU General Public License"""
  oPotFile.WriteLine "#"
  oPotFile.WriteLine "# ID line follows -- this is updated by SVN"
  oPotFile.WriteLine "# $" & "Id: " & "$"
  oPotFile.WriteLine "#"
  oPotFile.WriteLine "msgid """""
  oPotFile.WriteLine "msgstr """""
  oPotFile.WriteLine """Project-Id-Version: WinMerge\n"""
  oPotFile.WriteLine """Report-Msgid-Bugs-To: http://bugs.winmerge.org/\n"""
  oPotFile.WriteLine """POT-Creation-Date: " & GetPotCreationDate() & "\n"""
  oPotFile.WriteLine """PO-Revision-Date: \n"""
  oPotFile.WriteLine """Last-Translator: \n"""
  oPotFile.WriteLine """Language-Team: English <winmerge-translate@lists.sourceforge.net>\n"""
  oPotFile.WriteLine """MIME-Version: 1.0\n"""
  oPotFile.WriteLine """Content-Type: text/plain; charset=CP" & sCodePage & "\n"""
  oPotFile.WriteLine """Content-Transfer-Encoding: 8bit\n"""
  oPotFile.WriteLine """X-Poedit-Language: English\n"""
  oPotFile.WriteLine """X-Poedit-SourceCharset: CP" & sCodePage & "\n"""
  oPotFile.WriteLine """X-Poedit-Basepath: ../../Src/\n"""
  'oPotFile.WriteLine """X-Generator: CreateMasterPotFile.vbs\n"""
  oPotFile.WriteLine
  
  oPotFile.WriteLine "#. LANGUAGE, SUBLANGUAGE"
  oPotFile.WriteLine "#, c-format"
  oPotFile.WriteLine "msgid ""LANG_ENGLISH, SUBLANG_ENGLISH_US"""
  oPotFile.WriteLine "msgstr """""
  oPotFile.WriteLine

  oPotFile.WriteLine "#. Codepage"
  oPotFile.WriteLine "#, c-format"
  oPotFile.WriteLine "msgid ""1252"""
  oPotFile.WriteLine "msgstr """""
  oPotFile.WriteLine
  
  For Each sKey In oStrings.Keys 'For all strings...
    Set oString = oStrings(sKey)
    If (oString.Comment <> "") Then 'If comment exists...
      oPotFile.WriteLine "#. " & oString.Comment
    End If
    aReferences = SplitByTab(oString.References)
    For i = LBound(aReferences) To UBound(aReferences) 'For all references...
      oPotFile.WriteLine "#: " & aReferences(i)
    Next
    oPotFile.WriteLine "#, c-format"
    If (oString.Context <> "") Then 'If context exists...
      oPotFile.WriteLine "msgctxt """ & oString.Context & """"
    End If
    oPotFile.WriteLine "msgid """ & oString.Id & """"
    oPotFile.WriteLine "msgstr """""
    oPotFile.WriteLine
  Next
  oPotFile.Close
End Sub

''
' ...
Function FoundRegExpMatch(ByVal sString, ByVal sPattern, ByRef oMatchReturn)
  Dim oRegExp, oMatches
  
  Set oRegExp = New RegExp
  oRegExp.Pattern = sPattern
  oRegExp.IgnoreCase = True
  
  oMatchReturn = Null
  FoundRegExpMatch = False
  If (oRegExp.Test(sString) = True) Then
    Set oMatches = oRegExp.Execute(sString)
    Set oMatchReturn = oMatches(0)
    FoundRegExpMatch = True
  End If
End Function

''
' ...
Function SplitByTab(ByVal sString)
  SplitByTab = Array()
  If (InStr(sString, vbTab) > 0) Then
    SplitByTab = Split(sString, vbTab, -1)
  Else
    SplitByTab = Array(sString)
  End If
End Function

''
' ...
Function GetPotCreationDate()
  Dim oNow, sYear, sMonth, sDay, sHour, sMinute
  
  oNow = Now()
  sYear = Year(oNow)
  sMonth = Month(oNow)
  If (sMonth < 10) Then sMonth = "0" & sMonth
  sDay = Day(oNow)
  If (sDay < 10) Then sDay = "0" & sDay
  sHour = Hour(oNow)
  If (sHour < 10) Then sHour = "0" & sHour
  sMinute = Minute(oNow)
  If (sMinute < 10) Then sMinute = "0" & sMinute
  
  GetPotCreationDate = sYear & "-" & sMonth & "-" & sDay & " " & sHour & ":" & sMinute & "+0000"
End Function

''
' ...
Function InfoBox(ByVal sText, ByVal iSecondsToWait)
  If bRunFromCmd Then 'If run from command line...
    Wscript.Echo sText
  Else 'If NOT run from command line...
    InfoBox = oShell.Popup(sText, iSecondsToWait, Wscript.ScriptName, 64)
  End If
End Function

''
' ...
Function GetArchiveBit(ByVal sFilePath)
  Dim oFile
  
  GetArchiveBit = False
  If (oFSO.FileExists(sFilePath) = True) Then 'If the file exists...
    Set oFile = oFSO.GetFile(sFilePath)
    If (oFile.Attributes AND 32) Then 'If archive bit set...
      GetArchiveBit = True
    End If
  End If
End Function

''
' ...
Sub SetArchiveBit(ByVal sFilePath, ByVal bValue)
  Dim oFile
  
  If (oFSO.FileExists(sFilePath) = True) Then 'If the file exists...
    Set oFile = oFSO.GetFile(sFilePath)
    If (oFile.Attributes AND 32) Then 'If archive bit set...
      If (bValue = False) Then
        oFile.Attributes = oFile.Attributes - 32
      End If
    Else 'If archive bit NOT set...
      If (bValue = True) Then
        oFile.Attributes = oFile.Attributes + 32
      End If
    End If
  End If
End Sub
