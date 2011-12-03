/** 
 * @file  Environment.h
 *
 * @brief Declaration file for Environment-related routines.
 */
// ID line follows -- this is updated by SVN
// $Id$

#ifndef _ENVIRONMENT_H_
#define _ENVIRONMENT_H_

LPCTSTR env_GetTempPath();
String env_GetTempFileName(LPCTSTR lpPathName, LPCTSTR lpPrefixString);

String env_GetWindowsDirectory();
String env_GetMyDocuments();

String env_ExpandVariables(LPCTSTR text);

LPCTSTR env_ResolveMoniker(String &);

#endif // _ENVIRONMENT_H_
