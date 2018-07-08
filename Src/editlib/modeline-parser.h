/*
Adaption of pluma's modeline-parser.h for use with WinMerge:
Emacs, Kate and Vim-style modelines support for pluma.
Copyright (C) 2005-2007 - Steve Frécinaux <code@istique.net>
SPDX-License-Identifier: GPL-3.0-or-later
Last change: 2018-07-08 by Jochen Neubeck
*/

#ifndef __MODELINE_PARSER_H__
#define __MODELINE_PARSER_H__

void modeline_parser_init(const TCHAR *data_dir);
void modeline_parser_shutdown(void);
void modeline_parser_apply_modeline(CCrystalTextView *view);

#endif /* __MODELINE_PARSER_H__ */
