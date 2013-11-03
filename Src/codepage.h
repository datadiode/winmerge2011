/**
 * @file  codepage.h
 */
// RCS ID line follows -- this is updated by SVN
// $Id$

#ifndef __CODEPAGE_H__
#define __CODEPAGE_H__

const stl::map<int, int> &codepage_status();

void updateDefaultCodepage(int cpDefaultMode, int customCodepage);
int getDefaultCodepage();

bool isCodepageInstalled(int codepage);
int isCodepageDBCS(int codepage);

#endif //__CODEPAGE_H__
