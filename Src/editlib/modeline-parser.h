/*
Adaption of pluma's modeline-parser.h for use with WinMerge:
Emacs, Kate and Vim-style modelines support for pluma.
Copyright (C) 2005-2007 - Steve Frécinaux <code@istique.net>
SPDX-License-Identifier: GPL-3.0-or-later
Last change: 2018-07-08 by Jochen Neubeck
*/

#ifndef __MODELINE_PARSER_H__
#define __MODELINE_PARSER_H__

enum ModelineSet
{
	MODELINE_SET_NONE = 0,
	MODELINE_SET_TAB_WIDTH = 1 << 0,
	MODELINE_SET_INDENT_WIDTH = 1 << 1,
	MODELINE_SET_WRAP_MODE = 1 << 2,
	MODELINE_SET_SHOW_RIGHT_MARGIN = 1 << 3,
	MODELINE_SET_RIGHT_MARGIN_POSITION = 1 << 4,
	MODELINE_SET_LANGUAGE = 1 << 5,
	MODELINE_SET_INSERT_SPACES = 1 << 6
};

void modeline_parser_init(const TCHAR *data_dir);
void modeline_parser_shutdown(void);
int modeline_parser_apply_modeline(CCrystalTextBuffer *buffer);

#endif /* __MODELINE_PARSER_H__ */
