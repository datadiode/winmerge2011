/*
Adaption of pluma's modeline-parser.c for use with WinMerge:
Emacs, Kate and Vim-style modelines support for pluma.
Copyright (C) 2005-2007 - Steve Frécinaux <code@istique.net>
SPDX-License-Identifier: GPL-3.0-or-later
Last change: 2018-07-08 by Jochen Neubeck
*/

#include "StdAfx.h"
#include "ccrystaltextview.h"
#include "ccrystaltextbuffer.h"
#include "modeline-parser.h"
#include <coretools.h>
#include <string_util.h>

/* base dir to lookup configuration files */
static BSTR modelines_mappings_file;

/* Mappings: language name -> Pluma language ID */
static BSTR vim_languages;
static BSTR emacs_languages;
static BSTR kate_languages;

typedef enum
{
	MODELINE_SET_NONE = 0,
	MODELINE_SET_TAB_WIDTH = 1 << 0,
	MODELINE_SET_INDENT_WIDTH = 1 << 1,
	MODELINE_SET_WRAP_MODE = 1 << 2,
	MODELINE_SET_SHOW_RIGHT_MARGIN = 1 << 3,
	MODELINE_SET_RIGHT_MARGIN_POSITION = 1 << 4,
	MODELINE_SET_LANGUAGE = 1 << 5,
	MODELINE_SET_INSERT_SPACES = 1 << 6
} ModelineSet;

typedef struct _ModelineOptions
{
	const TCHAR	*language_id;

	/* these options are similar to the GtkSourceView properties of the
	 * same names.
	 */
	bool		insert_spaces;
	UINT		tab_width;
	UINT		indent_width;
	bool		wrap_mode;
	bool		display_right_margin;
	UINT		right_margin_position;
	
	int			set; /* combined from ModelineSet bits */
} ModelineOptions;

#define MODELINE_OPTIONS_DATA_KEY "ModelineOptionsDataKey"

static BOOL
has_option (ModelineOptions *options,
            ModelineSet      set)
{
	return options->set & set;
}

void
modeline_parser_init (const TCHAR *mappings_file)
{
	if (modelines_mappings_file == NULL)
		modelines_mappings_file = SysAllocString (mappings_file);
}

void
modeline_parser_shutdown ()
{
	if (vim_languages != NULL)
		SysFreeString (vim_languages);

	if (emacs_languages != NULL)
		SysFreeString (emacs_languages);

	if (kate_languages != NULL)
		SysFreeString (kate_languages);
	
	vim_languages = NULL;
	emacs_languages = NULL;
	kate_languages = NULL;

	SysFreeString (modelines_mappings_file);
	modelines_mappings_file = NULL;
}

static BSTR
load_language_mappings_group (const TCHAR *key_file, const TCHAR *section)
{
	TCHAR buffer[0x8000];
	return SysAllocStringLen(buffer, GetPrivateProfileSection(section, buffer, _countof(buffer), key_file));
}

/* lazy loading of language mappings */
static void
load_language_mappings (void)
{
	vim_languages = load_language_mappings_group (modelines_mappings_file, _T("vim"));
	emacs_languages	= load_language_mappings_group (modelines_mappings_file, _T("emacs"));
	kate_languages = load_language_mappings_group (modelines_mappings_file, _T("kate"));
}

static const TCHAR *
get_language_id (const TCHAR *language_name, const TCHAR *mapping)
{
	while (size_t len = _tcslen(mapping))
	{
		if (const TCHAR *p = EatPrefix(mapping, language_name))
			if (*p == _T('='))
				return p + 1;
		mapping += len + 1;
	}
	/* by default assume that the gtksourcevuew id is the same */
	return language_name;
}

static const TCHAR *
get_language_id_vim (const TCHAR *language_name)
{
	if (vim_languages == NULL)
		load_language_mappings ();

	return get_language_id (language_name, vim_languages);
}

static const TCHAR *
get_language_id_emacs (const TCHAR *language_name)
{
	if (emacs_languages == NULL)
		load_language_mappings ();

	return get_language_id (language_name, emacs_languages);
}

static const TCHAR *
get_language_id_kate (const TCHAR *language_name)
{
	if (kate_languages == NULL)
		load_language_mappings ();

	return get_language_id (language_name, kate_languages);
}

static BOOL
skip_whitespaces (const TCHAR **s)
{
	while (**s != '\0' && xisspace (**s))
		(*s)++;
	return **s != '\0';
}

/* Parse vi(m) modelines.
 * Vi(m) modelines looks like this: 
 *   - first form:   [text]{white}{vi:|vim:|ex:}[white]{options}
 *   - second form:  [text]{white}{vi:|vim:|ex:}[white]se[t] {options}:[text]
 * They can happen on the three first or last lines.
 */
static const TCHAR *
parse_vim_modeline (const TCHAR *s, ModelineOptions *options)
{
	BOOL in_set = FALSE;
	BOOL neg;
	UINT intval;
	String key, value;

	while (*s != '\0' && !(in_set && *s == ':'))
	{
		while (*s != '\0' && (*s == ':' || xisspace (*s)))
			s++;

		if (*s == '\0')
			break;

		if (_tcsncmp (s, _T("set "), 4) == 0 ||
		    _tcsncmp (s, _T("se "), 3) == 0)
		{
			s = _tcschr(s, ' ') + 1;
			in_set = TRUE;
		}

		neg = FALSE;
		if (_tcsncmp (s, _T("no"), 2) == 0)
		{
			neg = TRUE;
			s += 2;
		}

		key.resize (0);
		value.resize (0);

		while (*s != '\0' && *s != ':' && *s != '=' && !xisspace (*s))
		{
			key.push_back (*s);
			s++;
		}

		if (*s == '=')
		{
			s++;
			while (*s != '\0' && *s != ':' && !xisspace (*s))
			{
				value.push_back (*s);
				s++;
			}
		}

		if (_tcscmp (key.c_str(), _T("ft")) == 0 ||
		    _tcscmp (key.c_str(), _T("filetype")) == 0)
		{
			options->language_id = get_language_id_vim (value.c_str());
			options->set |= MODELINE_SET_LANGUAGE;
		}
		else if (_tcscmp (key.c_str(), _T("et")) == 0 ||
		    _tcscmp (key.c_str(), _T("expandtab")) == 0)
		{
			options->insert_spaces = !neg;
			options->set |= MODELINE_SET_INSERT_SPACES;
		}
		else if (_tcscmp (key.c_str(), _T("ts")) == 0 ||
			 _tcscmp (key.c_str(), _T("tabstop")) == 0)
		{
			intval = _ttoi (value.c_str());
			if (intval)
			{
				options->tab_width = intval;
				options->set |= MODELINE_SET_TAB_WIDTH;
			}
		}
		else if (_tcscmp (key.c_str(), _T("sw")) == 0 ||
			 _tcscmp (key.c_str(), _T("shiftwidth")) == 0)
		{
			intval = _ttoi (value.c_str());
			if (intval)
			{
				options->indent_width = intval;
				options->set |= MODELINE_SET_INDENT_WIDTH;
			}
		}
		else if (_tcscmp (key.c_str(), _T("wrap")) == 0)
		{
			options->wrap_mode = neg ? FALSE : TRUE;
			options->set |= MODELINE_SET_WRAP_MODE;
		}
		else if (_tcscmp (key.c_str(), _T("textwidth")) == 0)
		{
			intval = _ttoi (value.c_str());
			if (intval)
			{
				options->right_margin_position = intval;
				options->display_right_margin = TRUE;
				options->set |= MODELINE_SET_SHOW_RIGHT_MARGIN |
				                MODELINE_SET_RIGHT_MARGIN_POSITION;
				
			}
		}
	}
	return s;
}

/* Parse emacs modelines.
 * Emacs modelines looks like this: "-*- key1: value1; key2: value2 -*-"
 * They can happen on the first line, or on the second one if the first line is
 * a shebang (#!)
 * See http://www.delorie.com/gnu/docs/emacs/emacs_486.html
 */
static const TCHAR *
parse_emacs_modeline (const TCHAR *s, ModelineOptions *options)
{
	String key, value;

	while (*s != '\0')
	{
		while (*s != '\0' && (*s == ';' || xisspace (*s)))
			s++;
		if (*s == '\0' || _tcsncmp (s, _T("-*-"), 3) == 0)
			break;

		key.resize (0);
		value.resize (0);

		while (*s != '\0' && *s != ':' && *s != ';' && !xisspace (*s))
		{
			key.push_back (*s);
			s++;
		}

		if (!skip_whitespaces (&s))
			break;

		if (*s != ':')
			continue;
		s++;

		if (!skip_whitespaces (&s))
			break;

		while (*s != '\0' && *s != ';' && !xisspace (*s))
		{
			value.push_back (*s);
			s++;
		}

		/* "Mode" key is case insenstive */
		if (_tcsicmp (key.c_str(), _T("Mode")) == 0)
		{
			options->language_id = get_language_id_emacs (value.c_str());
			options->set |= MODELINE_SET_LANGUAGE;
		}
		else if (_tcscmp (key.c_str(), _T("tab-width")) == 0)
		{
			UINT intval = _ttoi (value.c_str());
			if (intval)
			{
				options->tab_width = intval;
				options->set |= MODELINE_SET_TAB_WIDTH;
			}
		}
		else if (_tcscmp (key.c_str(), _T("indent-offset")) == 0)
		{
			UINT intval = _ttoi (value.c_str());
			if (intval)
			{
				options->indent_width = intval;
				options->set |= MODELINE_SET_INDENT_WIDTH;
			}
		}
		else if (_tcscmp (key.c_str(), _T("indent-tabs-mode")) == 0)
		{
			bool intval = _tcscmp (value.c_str(), _T("nil")) == 0;
			options->insert_spaces = intval;
			options->set |= MODELINE_SET_INSERT_SPACES;
		}
		else if (_tcscmp (key.c_str(), _T("autowrap")) == 0)
		{
			bool intval = _tcscmp (value.c_str(), _T("nil")) != 0;
			options->wrap_mode = intval;
			options->set |= MODELINE_SET_WRAP_MODE;
		}
	}

	return *s == '\0' ? s : s + 2;
}

/*
 * Parse kate modelines.
 * Kate modelines are of the form "kate: key1 value1; key2 value2;"
 * These can happen on the 10 first or 10 last lines of the buffer.
 * See http://wiki.kate-editor.org/index.php/Modelines
 */
static const TCHAR *
parse_kate_modeline (const TCHAR *s, ModelineOptions *options)
{
	String key, value;

	while (*s != '\0')
	{
		while (*s != '\0' && (*s == ';' || xisspace (*s)))
			s++;
		if (*s == '\0')
			break;

		key.resize (0);
		value.resize (0);

		while (*s != '\0' && *s != ';' && !xisspace (*s))
		{
			key.push_back (*s);
			s++;
		}

		if (!skip_whitespaces (&s))
			break;
		if (*s == ';')
			continue;

		while (*s != '\0' && *s != ';' && !xisspace (*s))
		{
			value.push_back (*s);
			s++;
		}

		if (_tcscmp (key.c_str(), _T("hl")) == 0 ||
		    _tcscmp (key.c_str(), _T("syntax")) == 0)
		{
			options->language_id = get_language_id_kate (value.c_str());
			options->set |= MODELINE_SET_LANGUAGE;
		}
		else if (_tcscmp (key.c_str(), _T("tab-width")) == 0)
		{
			UINT intval = _ttoi (value.c_str());
			if (intval)
			{
				options->tab_width = intval;
				options->set |= MODELINE_SET_TAB_WIDTH;
			}
		}
		else if (_tcscmp (key.c_str(), _T("indent-width")) == 0)
		{
			UINT intval = _ttoi (value.c_str());
			if (intval)
				options->indent_width = intval;
		}
		else if (_tcscmp (key.c_str(), _T("space-indent")) == 0)
		{
			bool intval = _tcscmp (value.c_str(), _T("on")) == 0 ||
			              _tcscmp (value.c_str(), _T("true")) == 0 ||
			              _tcscmp (value.c_str(), _T("1")) == 0;
			options->insert_spaces = intval;
			options->set |= MODELINE_SET_INSERT_SPACES;
		}
		else if (_tcscmp (key.c_str(), _T("word-wrap")) == 0)
		{
			bool intval = _tcscmp (value.c_str(), _T("on")) == 0 ||
			              _tcscmp (value.c_str(), _T("true")) == 0 ||
			              _tcscmp (value.c_str(), _T("1")) == 0;
			options->wrap_mode = intval;
			options->set |= MODELINE_SET_WRAP_MODE;			
		}
		else if (_tcscmp (key.c_str(), _T("word-wrap-column")) == 0)
		{
			UINT intval = _ttoi (value.c_str());
			if (intval)
			{
				options->right_margin_position = intval;
				options->display_right_margin = TRUE;
				options->set |= MODELINE_SET_RIGHT_MARGIN_POSITION |
				                MODELINE_SET_SHOW_RIGHT_MARGIN;
			}
		}
	}

	return s;
}

/* Scan a line for vi(m)/emacs/kate modelines.
 * Line numbers are counted starting at one.
 */
static void
parse_modeline (LineInfo const &li, int line_number, int line_count, ModelineOptions *options)
{
	TCHAR prev;
	TCHAR const *s = li.GetLine();
	TCHAR const *const e = s + li.Length();
	/* look for the beginning of a modeline */
	for (prev = ' '; s < e; prev = *s++)
	{
		if (!xisspace (prev))
			continue;

		if ((line_number <= 3 || line_number > line_count - 3) &&
		    (_tcsncmp (s, _T("ex:"), 3) == 0 ||
		     _tcsncmp (s, _T("vi:"), 3) == 0 ||
		     _tcsncmp (s, _T("vim:"), 4) == 0))
		{
	    	while (*s != ':') s++;
	    	s = parse_vim_modeline (s + 1, options);
		}
		else if (line_number <= 2 && _tcsncmp (s, _T("-*-"), 3) == 0)
		{
			s = parse_emacs_modeline (s + 3, options);
		}
		else if ((line_number <= 10 || line_number > line_count - 10) &&
			 _tcsncmp (s, _T("kate:"), 5) == 0)
		{
			s = parse_kate_modeline (s + 5, options);
		}
	}
}

void
modeline_parser_apply_modeline (CCrystalTextView *view)
{
	ModelineOptions options;
	CCrystalTextBuffer *buffer = view->GetTextBuffer();
	int iter = 0;
	int line_count = buffer->GetLineCount();
	
	options.language_id = NULL;
	options.set = MODELINE_SET_NONE;

	/* Parse the modelines on the 10 first lines... */
	while (iter < 10 && iter < line_count)
	{
		LineInfo const &li = buffer->GetLineInfo(iter);
		parse_modeline (li, 1 + iter, line_count, &options);
		++iter;
	}

	/* ...and on the 10 last ones (modelines are not allowed in between) */
	int remaining_lines = line_count - iter;
	if (remaining_lines > 10)
	{
		remaining_lines = 10;
	}
	iter = line_count;

	while (iter > line_count - remaining_lines)
	{
		--iter;
		LineInfo const &li = buffer->GetLineInfo(iter);
		parse_modeline (li, 1 + iter, line_count, &options);
	}

	/* Try to set language */
	if (has_option (&options, MODELINE_SET_LANGUAGE) && options.language_id)
	{
		buffer->SetTextType(options.language_id);
	}

	/* Apply the options we got from modelines */
	if (has_option (&options, MODELINE_SET_INSERT_SPACES))
	{
		buffer->SetInsertTabs(!options.insert_spaces);
	}
	
	if (has_option (&options, MODELINE_SET_TAB_WIDTH))
	{
		buffer->SetTabSize(min(options.tab_width, 64U), buffer->GetSeparateCombinedChars());
	}
	
	/*if (has_option (&options, MODELINE_SET_INDENT_WIDTH))
	{
		// Unavailable in CrystalEdit
		buffer->SetIndentWidth(options.indent_width);
	}*/
	
	/*if (has_option (&options, MODELINE_SET_WRAP_MODE))
	{
		// Ignored for WinMerge
		view->SetWordWrapping(options.wrap_mode);
	}*/
	
	/*if (has_option (&options, MODELINE_SET_RIGHT_MARGIN_POSITION))
	{
		// Unavailable in CrystalEdit
		view->SetRightMarginPosition(options.right_margin_position);
	}*/
	
	/*if (has_option (&options, MODELINE_SET_SHOW_RIGHT_MARGIN))
	{
		// Unavailable in CrystalEdit
		view->ShowRightMargin(options.display_right_margin);
	}*/
}

/* vi:ts=8 */
