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
 * @file UnicodeString.h
 *
 * @brief Unicode string based on std::wstring.
 *
 */
#pragma once

using std::min;
using std::max;

#define std_tchar(type) std::w##type

typedef std_tchar(string) String;

String::size_type string_replace(String &, LPCTSTR find, LPCTSTR replace);
String::size_type string_replace(String &, TCHAR find, TCHAR replace);

// Trimming
void string_trim_ws(String &);
void string_trim_ws_begin(String &);
void string_trim_ws_end(String &);

// Formatting
class string_format : public String
{
public:
	string_format(const TCHAR *fmt, ...);
};

typedef string_format Fmt;
