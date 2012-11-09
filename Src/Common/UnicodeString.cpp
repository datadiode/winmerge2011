/////////////////////////////////////////////////////////////////////////////
//    License (GPLv2+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
/////////////////////////////////////////////////////////////////////////////

/** 
 * @file  UnicodeString.cpp
 *
 * @brief String utilities.
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "UnicodeString.h"

/**
 * @brief Convert a string to lower case string.
 * @param [in] str String to convert to lower case.
 */
void string_makelower(String &str)
{
	String::size_type i;
	for (i = 0; i < str.length(); i++)
		str[i] = _totlower(str[i]);
}

/**
 * @brief Convert a string to lower case string.
 * @param [in] str String to convert to lower case.
 */
void string_makeupper(String &str)
{
	String::size_type i;
	for (i = 0; i < str.length(); i++)
		str[i] = _totupper(str[i]);
}

/**
 * @brief Replace a string inside a string with another string.
 * This function searches for a string inside another string an if found,
 * replaces it with another string. Function can replace several instances
 * of the string inside one string.
 * @param [in] target A string containing another string to replace.
 * @param [in] find A string to search and replace with another (@p replace).
 * @param [in] replace A string used to replace original (@p find).
 */
void string_replace(String &target, LPCTSTR find, LPCTSTR replace)
{
	const String::size_type find_len = _tcslen(find);
	const String::size_type replace_len = _tcslen(replace);
	String::size_type pos = 0;
	while ((pos = target.find(find, pos)) != String::npos)
	{
		target.replace(pos, find_len, replace, replace_len);
		pos += replace_len;
	}
}

/**
 * @brief Trims whitespace chars from begin and end of the string.
 */
void string_trim_ws(String &result)
{
	string_trim_ws_end(result);
	string_trim_ws_begin(result);
}

/**
 * @brief Trims whitespace chars from begin of the string.
 */
void string_trim_ws_begin(String &result)
{
	String::iterator it = result.begin();
	while (it != result.end() && _istspace(*it))
	{
		++it;
	}
	result.erase(result.begin(), it);
}

/**
 * @brief Trims whitespace chars from end of the string.
 */
void string_trim_ws_end(String &result)
{
	String::iterator it = result.end();
	String::iterator pos;
	do
	{
		pos = it;
	} while (it != result.begin() && _istspace(*--it));
	result.erase(pos, result.end());
}

/**
 * @brief printf()-style formatting for STL string.
 * Use this function to format String:s in printf() style.
 */
string_format::string_format(const TCHAR *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	append_sprintf_va_list(fmt, args);
	va_end(args);
}
