/** 
 * @file  Environment.h
 *
 * @brief Declaration file for Environment-related routines.
 */
#pragma once

LPCTSTR env_GetTempPath();
String env_GetTempFileName(LPCTSTR lpPathName, LPCTSTR lpPrefixString);

String env_GetWindowsDirectory();
String env_GetMyDocuments();

String env_ExpandVariables(LPCTSTR text);

LPCTSTR env_ResolveMoniker(String &);
