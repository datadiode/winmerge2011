/** 
 * @file  stringdiffs.h
 *
 * @brief Interface file declaring sd_ComputeWordDiffs (q.v.)
 *
 */
#pragma once

/** @brief One difference between two strings */
struct wdiff
{
	int start[2]; // 0-based, eg, start[0] is from str1
	int end[2]; // 0-based, eg, end[1] is from str2
	wdiff(int s1 = 0, int e1 = 0, int s2 = 0, int e2 = 0)
	{
		start[0] = s1;
		start[1] = s2;
		end[0] = e1;
		end[1] = e2;
	}
};

void sd_SetBreakChars(const TCHAR *breakChars);

void sd_ComputeWordDiffs(const String &str1, const String &str2,
		bool case_sensitive, int whitespace, int breakType, bool byte_level,
		std::vector<wdiff> &diffs);
bool IsSide0Empty(const std::vector<wdiff> &worddiffs, int nLineLengt);
bool IsSide1Empty(const std::vector<wdiff> &worddiffs, int nLineLengt);
