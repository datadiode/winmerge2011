/**
 * @file  codepage.h
 */
#pragma once

const std::map<int, int> &codepage_status();

void updateDefaultCodepage(int cpDefaultMode, int customCodepage);
int getDefaultCodepage();

bool isCodepageInstalled(int codepage);
int isCodepageDBCS(int codepage);
