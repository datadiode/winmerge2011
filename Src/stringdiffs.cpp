/** 
 * @file  stringdiffs.cpp
 *
 * @brief Implementation file for sd_ComputeWordDiffs (q.v.)
 *
 */
#include "StdAfx.h"
#include "string_util.h"
#include "stringdiffs.h"
#include "CompareOptions.h"
#include "stringdiffsi.h"

#ifdef _WIN64
#	define STRINGDIFF_WORD_LIMIT 20480
#else
#	define STRINGDIFF_WORD_LIMIT 2048
#endif

static TCHAR const BreakCharDefaults[] = _T(",.;:");
static TCHAR const *BreakChars = BreakCharDefaults;

static bool isWordBreak(int breakType, TCHAR ch);

void sd_SetBreakChars(LPCTSTR breakChars)
{
	TRACE("Setting BreakChars to %ls\n", breakChars);
	if (BreakChars != BreakCharDefaults)
		free(const_cast<TCHAR *>(BreakChars));
	BreakChars = breakChars ? _tcsdup(breakChars) : BreakCharDefaults;
}

/**
 * @brief Construct our worker object and tell it to do the work
 */
void sd_ComputeWordDiffs(LPCTSTR str1, int len1, LPCTSTR str2, int len2,
	bool case_sensitive, int whitespace, int breakType, bool byte_level,
	vector<wdiff> &diffs)
{
	if (len1 != 0 && len2 != 0)
		stringdiffs(str1, len1, str2, len2, case_sensitive, whitespace, breakType, byte_level, diffs);
	else if (len1 != 0 || len2 != 0)
		diffs.push_back(wdiff(0, len1, 0, len2));
}

/**
 * @brief stringdiffs constructor simply loads all members from arguments
 */
stringdiffs::stringdiffs(LPCTSTR str1, int len1, LPCTSTR str2, int len2,
	bool case_sensitive, int whitespace, int breakType, bool byte_level,
	vector<wdiff> &diffs)
: m_line1(str1, len1)
, m_line2(str2, len2)
, m_case_sensitive(case_sensitive)
, m_whitespace(whitespace)
, m_breakType(breakType)
, m_diffs(diffs)
{
	// Hash all words in both lines and then compare them word by word
	// storing differences into m_diffs

	BuildWordsArray(m_line1.str, m_line1.len, m_line1.words);
	BuildWordsArray(m_line2.str, m_line2.len, m_line2.words);

	// If task seems unaffordable, return single diff spanning whole line
	if (m_line1.words.size() > STRINGDIFF_WORD_LIMIT ||
		m_line2.words.size() > STRINGDIFF_WORD_LIMIT)
	{
		m_diffs.push_back(wdiff(
			m_line1.words.front().start, m_line1.words.back().end,
			m_line2.words.front().start, m_line2.words.back().end));
		return;
	}

	BuildWordDiffList();
	CombineAdjacentDiffs();

	// Adjust the range of the word diff down to byte (char) level
	if (byte_level)
		wordLevelToByteLevel();
}

/**
 * @brief Destructor.
 * The destructor frees all diffs added to the vectors.
 */
stringdiffs::~stringdiffs()
{
}

#ifdef STRINGDIFF_LOGGING
void stringdiffs::debugoutput()
{
	vector<wdiff>::const_iterator p = m_diffs.begin();
	while (p != m_diffs.end())
	{
		wdiff const &d = *p++;
		String a(m_line1.str + d.start[0], min(d.end[0] - d.start[0], 50));
		String b(m_line2.str + d.start[1], min(d.end[1] - d.start[1], 50));
		DbgPrint("left = %ls, %d,%d\nright = %ls, %d,%d\n",
			a.c_str(), d.start[0], d.end[0], b.c_str(), d.start[1], d.end[1]);
	}
}
#endif

/**
 * @brief Break line into constituent words
 */
void stringdiffs::BuildWordsArray(LPCTSTR str, int len, vector<word> &words)
{
	int i = 0, begin = 0;

	// state when we are looking for next word
inspace:
	if (i < len && xisspace(str[i]))
	{
		++i;
		goto inspace;
	}
	if (begin < i)
	{
		// just finished a word
		// i - 1 is first word character (space or at end)
		word wd(begin, i, dlspace, Hash(str, begin, i, 0));
		words.push_back(wd);
	}
	if (i == len)
		return;
	begin = i;
	goto inword;

	// state when we are inside a word
inword:
	int atspace = 0;
	if (i == len || ((atspace = xisspace(str[i])) != 0) || isWordBreak(m_breakType, str[i]))
	{
		if (begin < i)
		{
			// just finished a word
			// i - 1 is first non-word character (space or at end)
			word wd(begin, i, dlword, Hash(str, begin, i, 0));
			words.push_back(wd);
		}
		if (i == len)
		{
			return;
		}
		else if (atspace)
		{
			begin = i;
			goto inspace;
		}
		else
		{
			// start a new word because we hit a non-whitespace word break (eg, a comma)
			// but, we have to put each word break character into its own word
			begin = i + 1;
			word wd(i, begin, dlbreak, Hash(str, i, begin, 0));
			words.push_back(wd);
			i = begin;
			goto inword;
		}
	}
	++i;
	goto inword; // safe even if we're at the end or no longer in a word
}

/**
 * @brief Add all different elements between lines to the wdiff list
 */
void stringdiffs::BuildWordDiffList()
{
	vector<char> edscript;

	onp(edscript);

	int i = 0, j = 0;
	word const w0; // dummy
	vector<char>::const_iterator k;
	for (k = edscript.begin(); k != edscript.end(); ++k)
	{
		switch (*k)
		{
		case '-':
			if (m_whitespace != WHITESPACE_IGNORE_ALL ||
				!IsSpace(m_line1.words[i]))
			{
				word const &w1 = m_line1.words[i];
				word const &w2 = j ? m_line2.words[j - 1] : w0;
				m_diffs.push_back(wdiff(w1.start, w1.end, w2.end, w2.end));
			}
			i++;
			break;
		case '+':
			if (m_whitespace != WHITESPACE_IGNORE_ALL ||
				!IsSpace(m_line2.words[j]))
			{
				word const &w1 = i ? m_line1.words[i - 1] : w0;
				word const &w2 = m_line2.words[j];
				m_diffs.push_back(wdiff(w1.end, w1.end, w2.start, w2.end));
			}
			j++;
			break;
		case '!':
			if (m_whitespace == WHITESPACE_COMPARE_ALL ||
				!IsSpace(m_line1.words[i]) || !IsSpace(m_line2.words[j]))
			{
				word const &w1 = m_line1.words[i];
				word const &w2 = m_line2.words[j];
				m_diffs.push_back(wdiff(w1.start, w1.end, w2.start, w2.end));
			}
			// fall through
		default:
			i++;
			j++;
			break;
		}
	}
#ifdef STRINGDIFF_LOGGING
	debugoutput();
#endif
}

void stringdiffs::CombineAdjacentDiffs()
{
	if (vector<wdiff>::size_type j = m_diffs.size())
	{
		while (vector<wdiff>::size_type i = --j)
		{
			--i;
			if (m_diffs[i].end[0] == m_diffs[j].start[0] &&
				m_diffs[i].end[1] == m_diffs[j].start[1])
			{
				m_diffs[i].end[0] = m_diffs[j].end[0];
				m_diffs[i].end[1] = m_diffs[j].end[1];
				m_diffs.erase(m_diffs.begin() + j);
			}
		}
	}
}

// diffutils hash

/* Rotate a value n bits to the left. */
#define UINT_BIT (sizeof(unsigned) * CHAR_BIT)
#define ROL(v, n) ((v) << (n) | (v) >> (UINT_BIT - (n)))
/* Given a hash value and a new character, return a new hash value. */
#define HASH(h, c) ((c) + ROL(h, 7))

UINT stringdiffs::Hash(LPCTSTR str, int begin, int end, UINT h) const
{
	for (int i = begin; i < end; ++i)
	{
		UINT ch = (UINT)str[i];
		if (m_case_sensitive)
		{
			h += HASH(h, ch);
		}
		else
		{
			ch = (UINT)_totupper(ch);
			h += HASH(h, ch);
		}
	}
	return h;
}

/**
 * @brief Compare two words (by reference to original strings)
 */
bool stringdiffs::AreWordsSame(line const &line1, int u, line const &line2, int v) const
{
	word const &word1 = line1.words[u];
	word const &word2 = line2.words[v];
	if (m_whitespace != WHITESPACE_COMPARE_ALL)
		if (IsSpace(word1) && IsSpace(word2))
			return true;
	if (word1.hash != word2.hash)
		return false;
	int i = word1.end - word1.start;
	if (i != word2.end - word2.start)
		return false;
	LPCTSTR str1 = line1.str + word1.start;
	LPCTSTR str2 = line2.str + word2.start;
	while (i != 0)
	{
		--i;
		if (!caseMatch(str1[i], str2[i]))
			return false;
	}
	return true;
}

/**
 * @brief Is this block a word one?
 */
bool stringdiffs::IsWord(word const &word1)
{
	return word1.kind == dlword;
}

/**
 * @brief Is this block an space or whitespace one?
 */
bool stringdiffs::IsSpace(word const &word1)
{
	return word1.kind == dlspace;
}
/**
 * @brief Is this block a break?
 */
bool stringdiffs::IsBreak(word const &word1)
{
	return word1.kind == dlbreak || word1.kind == dlspace;
}

/**
 * @brief Is this block an empty (insert) one?
 */
bool stringdiffs::IsInsert(word const &word1)
{
	return word1.kind == dlinsert;
}

/**
 * @brief Return true if characters match
 */
bool stringdiffs::caseMatch(TCHAR ch1, TCHAR ch2) const
{
	return (ch1 == ch2) || (!m_case_sensitive && _totupper(ch1) == _totupper(ch2));
}

/**
 * @brief An O(NP) Sequence Comparison Algorithm. Sun Wu, Udi Manber, Gene Myers
 */
void stringdiffs::onp(vector<char> &edscript)
{
	bool const exchanged = m_line1.words.size() > m_line2.words.size();

	line const &line1 = exchanged ? m_line2 : m_line1;
	line const &line2 = exchanged ? m_line1 : m_line2;

	int const M = static_cast<int>(line1.words.size());
	int const N = static_cast<int>(line2.words.size());
	
	vector<int> fp_owner(M + 1 + N, -1);
	vector<vector<char> > es_owner(M + 1 + N);
	int *const fp = fp_owner.data() + M;
	vector<char> *const es = es_owner.data() + M;

	int const DELTA = N - M;
	int p = 0;
	do
	{
		int k;
		for (k = -p; k < DELTA; ++k)
		{
			int const z = fp[k + 1];
			int const y = max(fp[k - 1] + 1, z);
			fp[k] = snake(k, y, line1, line2);
			es[k] = y > z ? es[k - 1] : es[k + 1];
			if (int const n = static_cast<int>(es[k].size()))
			{
				es[k].resize(n + 1 + fp[k] - y, '=');
				es[k][n] = y > z ? '+' : '-';
			}
			else
			{
				es[k].resize(fp[k] - y, '=');
			}
		}
		for (k = DELTA + p; k >= DELTA; --k)
		{
			int const z = fp[k + 1];
			int const y = max(fp[k - 1] + 1, z);
			fp[k] = snake(k, y, line1, line2);
			es[k] = y > z ? es[k - 1] : es[k + 1];
			if (int const n = static_cast<int>(es[k].size()))
			{
				es[k].resize(n + 1 + fp[k] - y, '=');
				es[k][n] = y > z ? '+' : '-';
			}
			else
			{
				es[k].resize(fp[k] - y, '=');
			}
		}
		++p;
	} while (fp[DELTA] < N);

	vector<char> &ses = es[DELTA]; // Shortest edit script
	vector<char>::iterator w = ses.begin();
	vector<char>::iterator r = ses.begin();
	vector<char>::iterator const e = ses.end();
	while (r != e)
	{
		char c = *r++;
		switch (c)
		{
		case '+':
			if (r != e && *r == '-')
			{
				c = '!';
				++r;
			}
			else if (exchanged)
			{
				c = '-';
			}
			break;
		case '-':
			if (r != e && *r == '+')
			{
				c = '!';
				++r;
			}
			else if (exchanged)
			{
				c = '+';
			}
			break;
		}
		*w++ = c;
	}

	ses.erase(w, e);
	ses.swap(edscript);
}

int stringdiffs::snake(int k, int y, line const &line1, line const &line2)
{
	int const M = static_cast<int>(line1.words.size());
	int const N = static_cast<int>(line2.words.size());
	int x = y - k;
	while (x < M && y < N && AreWordsSame(line1, x, line2, y))
	{
		++x;
		++y;
	}
	return y;
}

/**
 * @brief Return true if chars match
 *
 * Caller must not call this for lead bytes
 */
static bool matchchar(TCHAR ch1, TCHAR ch2, bool casitive)
{
	return (ch1 == ch2) || (!casitive && _totupper(ch1) == _totupper(ch2));
}

/**
 * @brief Is it a non-whitespace wordbreak character (ie, punctuation)?
 */
static bool isWordBreak(int breakType, TCHAR ch)
{
	// breakType == 0 means whitespace only
	if (breakType == 0)
		return false;
	// breakType == 1 means break also on punctuation
	return _tcschr(BreakChars, ch) != 0;
}

/**
 * @brief advance current pointer over whitespace, until not whitespace or beyond end
 * @param pcurrent [in,out] current location (to be advanced)
 * @param end [in] last valid position (only go one beyond this)
 */
static LPCTSTR AdvanceOverWhitespace(LPCTSTR pcurrent, LPCTSTR end)
{
	// advance over whitespace
	while (pcurrent <= end && xisspace(*pcurrent))
		++pcurrent;
	return pcurrent;
}

/**
 * @brief back current pointer over whitespace, until not whitespace or at start
 * @param pcurrent [in,out] current location (to be backed up)
 * @param start [in] first valid position (do not go before this)
 *
 * NB: Unlike AdvanceOverWhitespace, this will not go over the start
 * This because WinMerge doesn't need to
 */
static LPCTSTR RetreatOverWhitespace(LPCTSTR pcurrent, LPCTSTR start)
{
	// back over whitespace
	while (pcurrent > start && xisspace(*pcurrent))
		--pcurrent;
	return pcurrent;
}

/**
 * @brief Compute begin1,begin2,end1,end2 to display byte difference between strings str1 & str2
 * @param [in] casitive true for case-sensitive, false for case-insensitive
 * @param [in] xwhite This governs whether we handle whitespace specially
 * (see WHITESPACE_COMPARE_ALL, WHITESPACE_IGNORE_CHANGE, WHITESPACE_IGNORE_ALL)
 * @param [out] begin1 return -1 if not found or pos of equal
 * @param [out] begin2 return -1 if not found or pos of equal
 * @param [out] end1 return -1 if not found or pos of equal valid if begin1 >=0
 * @param [out] end2 return -1 if not found or pos of equal valid if begin2 >=0
 * @param [in] equal false surch for a diff, true surch for equal
 *
 * Assumes whitespace is never leadbyte or trailbyte!
 */
void stringdiffs::ComputeByteDiff(wdiff const &diff,
	int &begin1, int &begin2, int &end1, int &end2, bool equal) const
{
	// Set to sane values
	// Also this way can distinguish if we set begin1 to -1 for no diff in line
	begin1 = end1 = begin2 = end2 = 0;

	int const max1 = diff.end[0] - diff.start[0] - 1;
	int const max2 = diff.end[1] - diff.start[1] - 1;

	if (max1 == -1 || max2 == -1)
	{
		if (max1 == max2)
		{
			begin1 = -1;
			begin2 = -1;
		}
		end1 = max1;
		end2 = max2;
		return;
	}

	LPCTSTR const pbeg1 = m_line1.str + diff.start[0];
	LPCTSTR const pbeg2 = m_line2.str + diff.start[1];

	// cursors from front, which we advance to beginning of difference
	LPCTSTR py1 = pbeg1;
	LPCTSTR py2 = pbeg2;

	// pen1,pen2 point to the last valid character
	LPCTSTR pen1 = py1 + max1;
	LPCTSTR pen2 = py2 + max2;

	if (m_whitespace != WHITESPACE_COMPARE_ALL)
	{
		// Ignore leading and trailing whitespace
		// by advancing py1 and py2
		// and retreating pen1 and pen2
		while (py1 < pen1 && xisspace(*py1))
			++py1;
		while (py2 < pen2 && xisspace(*py2))
			++py2;
		while (pen1 > py1 && xisspace(*pen1))
			--pen1;
		while (pen2 > py2 && xisspace(*pen2))
			--pen2;
	}
	//check for excaption of empty string on one side
	//In that case display all as a diff
	if (!equal && (((py1 == pen1) && xisspace(*pen1)) ||
		((py2 == pen2) && xisspace(*pen2))))
	{
		begin1 = 0;
		begin2 = 0;
		end1 = max1;
		end2 = max2;
		return;
	}
	bool alldone = false;
	bool found = false;

	// Advance over matching beginnings of lines
	// Advance py1 & py2 from beginning until find difference or end
	while (1)
	{
		// Potential difference extends from py1 to pen1 and py2 to pen2

		// Check if either side finished
		if (py1 > pen1 || py2 > pen2)
		{
			if (py1 > pen1 && py2 > pen2)
				begin1 = end1 = begin2 = end2 = -1;
			alldone = true;
			break;
		}

		// handle all the whitespace logic (due to WinMerge whitespace settings)
		if (m_whitespace != WHITESPACE_COMPARE_ALL && xisspace(*py1))
		{
			if (m_whitespace == WHITESPACE_IGNORE_CHANGE && !xisspace(*py2))
			{
				// py1 is white but py2 is not
				// in WHITESPACE_IGNORE_CHANGE mode,
				// this doesn't qualify as skippable whitespace
				break; // done with forward search
			}
			// gobble up all whitespace in current area
			py1 = AdvanceOverWhitespace(py1, pen1); // will go beyond end
			py2 = AdvanceOverWhitespace(py2, pen2); // will go beyond end
			continue;

		}
		if (m_whitespace != WHITESPACE_COMPARE_ALL && xisspace(*py2))
		{
			if (m_whitespace == WHITESPACE_IGNORE_CHANGE && !xisspace(*py1))
			{
				// py2 is white but py1 is not
				// in WHITESPACE_IGNORE_CHANGE mode,
				// this doesn't qualify as skippable whitespace
				break; // done with forward search
			}
			// gobble up all whitespace in current area
			py1 = AdvanceOverWhitespace(py1, pen1); // will go beyond end
			py2 = AdvanceOverWhitespace(py2, pen2); // will go beyond end
			continue;
		}

		// Now do real character match
		if (!equal)
		{
			if (!matchchar(py1[0], py2[0], m_case_sensitive))
				break; // done with forward search
		}
		else 
		{
			if (matchchar(py1[0], py2[0], m_case_sensitive))
			{
				// check at least two chars are identical
				if (!found)
				{
					found = true;
				}
				else
				{
					if (py1 > pbeg1)
						--py1;
					if (py2 > pbeg2)
						--py2;
					break; // done with forward search
				}
			}
			else
			{
				found = false;
			}
		}
		++py1;
		++py2;
	}

	// Potential difference extends from py1 to pen1 and py2 to pen2

	// Store results of advance into return variables (begin1 & begin2)
	// -1 in a begin variable means no visible diff area
	begin1 = (py1 > pen1) ? -1 : static_cast<int>(py1 - pbeg1);
	begin2 = (py2 > pen2) ? -1 : static_cast<int>(py2 - pbeg2);

	if (alldone)
	{
		end1 = static_cast<int>(pen1 - pbeg1);
		end2 = static_cast<int>(pen2 - pbeg2);
	}
	else
	{
		// cursors from front, which we advance to ending of difference
		LPCTSTR pz1 = pen1;
		LPCTSTR pz2 = pen2;

		// Retreat over matching ends of lines
		// Retreat pz1 & pz2 from end until find difference or beginning
		found = false;
		while (1)
		{
			// Potential difference extends from py1 to pz1 and from py2 to pz2

			// Check if either side finished
			if (pz1 < py1 && pz2 < py2)
			{
				begin1 = end1 = begin2 = end2 = -1;
				break;
			}
			if (pz1 < py1 || pz2 < py2)
			{
				found = true;
				break;
			}

			// handle all the whitespace logic (due to WinMerge whitespace settings)
			if (m_whitespace != WHITESPACE_COMPARE_ALL && xisspace(*pz1))
			{
				if (m_whitespace == WHITESPACE_IGNORE_CHANGE && !xisspace(*pz2))
				{
					// pz1 is white but pz2 is not
					// in WHITESPACE_IGNORE_CHANGE mode,
					// this doesn't qualify as skippable whitespace
					break; // done with reverse search
				}
				// gobble up all whitespace in current area
				pz1 = RetreatOverWhitespace(pz1, py1); // will not go over beginning
				pz2 = RetreatOverWhitespace(pz2, py2); // will not go over beginning
				if (((pz1 == py1) && xisspace(*pz1)) || ((pz2 == py2) && xisspace(*pz2)))
				{
					break;
				}
				continue;
			}
			if (m_whitespace != WHITESPACE_COMPARE_ALL && xisspace(*pz2))
			{
				if (m_whitespace == WHITESPACE_IGNORE_CHANGE && !xisspace(*pz1))
				{
					// pz2 is white but pz1 is not
					// in WHITESPACE_IGNORE_CHANGE mode,
					// this doesn't qualify as skippable whitespace
					break; // done with reverse search
				}
				// gobble up all whitespace in current area
				pz1 = RetreatOverWhitespace(pz1, py1); // will not go over beginning
				pz2 = RetreatOverWhitespace(pz2, py2); // will not go over beginning
				if (((pz1 == py1) && xisspace(*pz1)) || ((pz2 == py2) && xisspace(*pz2)))
				{
					break;
				}
				continue;
			}

			// Now do real character match
			if (!equal)
			{
				if (!matchchar(pz1[0], pz2[0], m_case_sensitive))
				{
					found = true;
					break; // done with forward search
				}
			}
			else 
			{
				if (matchchar(pz1[0], pz2[0], m_case_sensitive))
				{
					// check at least two chars are identical
					if (!found)
					{
						found = true;
					}
					else
					{
						pz1++;
						pz2++;
						break; // done with forward search
					}
				}
				else
				{
					found = false;
				}
			}
			// decrement pz1 and pz2
			--pz1; // earlier than pbeg1 signifies no difference
			--pz2; // earlier than pbeg1 signifies no difference
		}

		// Store results of advance into return variables (end1 & end2)
		// return -1 for Not found, otherwise distance from begin
		if (found && pz1 == pbeg1 ) 
		{
			// Found on begin
			end1 = 0;
		}
		else if (pz1 <= pbeg1 || pz1 < py1) 
		{
			// No visible diff in line 1
			end1 = -1; 
		}
		else
		{
			// Found on distance line 1
			end1 = static_cast<int>(pz1 - pbeg1);
		}
		if (found && pz2 == pbeg2 ) 
		{
			// Found on begin
			end2 = 0;
		}
		else if (pz2 <= pbeg2 || pz2 < py2)
		{
			// No visible diff in line 2
			end2 = -1; 
		}
		else
		{
			// Found on distance line 2
			end2 = static_cast<int>(pz2 - pbeg2);
		}
	}
}

/**
 * @brief adjust the range of the contained word diffs down to byte(char) level.
 */
void stringdiffs::wordLevelToByteLevel() const
{
	for (vector<wdiff>::size_type i = 0; i < m_diffs.size(); ++i)
	{
		wdiff &diff = m_diffs[i];
		bool bRepeat = true;
		// Something to differ?
		if (diff.IsInsert())
			continue;
#ifdef STRINGDIFF_LOGGING
		DbgPrint("actual\n left = %d,%d\n right = %d,%d\n",
			diff.start[0], diff.end[0], diff.start[1], diff.end[1]);
#endif	
		// Check for first and last difference in word
		int begin1, begin2, end1, end2;
		ComputeByteDiff(diff, begin1, begin2, end1, end2, false);
		if (begin1 == -1)
		{
			// no visible diff on side1
			diff.start[0] = diff.end[0];
			bRepeat = false;
		}
		else if (end1 == -1)
		{
			diff.start[0] += begin1;
			diff.end[0] = diff.start[0];
		}
		else
		{
			diff.end[0] = diff.start[0] + end1 + 1;
			diff.start[0] += begin1;
		}
		if (begin2 == -1)
		{
			// no visible diff on side2
			diff.start[1] = diff.end[1];
			bRepeat = false;
		}
		else if (end2 == -1)
		{
			diff.start[1] += begin2;
			diff.end[1] = diff.start[1];
		}
		else
		{
			diff.end[1] = diff.start[1] + end2 + 1;
			diff.start[1] += begin2;
		}
#ifdef STRINGDIFF_LOGGING
		DbgPrint("changed\n left = %d,%d\n right = %d,%d\n",
			diff.start[0], diff.end[0], diff.start[1], diff.end[1]);
#endif	
		// Nothing to display, remove item
		if (diff.start[0] == diff.end[0] && diff.start[1] == diff.end[1])
		{
			m_diffs.erase(m_diffs.begin() + i);
			i--;
			continue;
		}
		int const len1 = diff.end[0] - diff.start[0];
		int const len2 = diff.end[1] - diff.start[1];
		// Nothing more todo
		if (len1 <= 3 || len2 <= 3)
		{
			continue;
		}
		// Now check if possiblity to get more details
		// We have to check if there is again a synchronisation inside the pDiff
		if (bRepeat && len1 > 1 && len2 > 1)
		{
			// define offset to zero
			int s1 = 0, e1 = 0, s2 = 0, e2 = 0; 
			// Try to synchron side1 from begin
			bool bsynchron = findsyn(diff,
				begin1, begin2, end1, end2,
				true, synbegin1, s1, e1, s2, e2);

			if (!bsynchron)
			{
				// Try to synchron side2 from begin
				s1 = 0;
				bsynchron = findsyn(diff,
					begin1, begin2, end1, end2,
					true, synbegin2, s1, e1, s2, e2);
			}
			if (bsynchron)
			{
				// Try to synchron side1 from end
				bsynchron = findsyn(diff,
					begin1, begin2, end1, end2,
					true, synend1, s1, e1, s2, e2);
				if (!bsynchron)
				{
					// Try to synchron side1 from end
					e1 = 0;
					bsynchron = findsyn(diff,
						begin1, begin2, end1, end2,
						true, synend2, s1, e1, s2, e2);
				}
			}

			// Do we have more details?
			if ((begin1 == -1) && (begin2 == -1) || (!bsynchron))
			{
				// No visible synchcron on side1 and side2
				bRepeat = false;
			}
			else if ((begin1 >= 0) && (begin2 >= 0) && (end1 >= 0) && (end2 >= 0))
			{
				// Visible sync on side1 and side2
				// Now split in two diff
				// New behind actual diff
				// wdf->start diff->start + offset + end + 1 (zerobased)
				wdiff wdf(diff.start[0] + s1 + end1 + 1, diff.end[0],
					diff.start[1] + s2 + end2 + 1, diff.end[1]);
#ifdef STRINGDIFF_LOGGING
				DbgPrint("org\n left = %d,%d\n right = %d,%d\n",
					diff.start[0], diff.end[0], diff.start[1], diff.end[1]);
				DbgPrint("insert\n left = %d,%d\n right = %d,%d\n",
					wdf.start[0], wdf.end[0], wdf.start[1], wdf.end[1]);
#endif
				// change end of actual diff
				diff.end[0] = diff.start[0] + s1 + begin1;
				diff.end[1] = diff.start[1] + s2 + begin2;
#ifdef STRINGDIFF_LOGGING
				DbgPrint("changed\n left = %d,%d\n right = %d,%d\n",
					diff.start[0], diff.end[0], diff.start[1], diff.end[1]);
#endif			
				// visible sync on side1 and side2
				// new in middle with diff
				wdiff wdfm(diff.end[0], wdf.start[0],
					diff.end[1], wdf.start[1]);
#ifdef STRINGDIFF_LOGGING
				DbgPrint("insert\n left = %d,%d\n right = %d,%d\n",
					wdf.start[0], wdf.end[0], wdf.start[1], wdf.end[1]);
#endif
				m_diffs.insert(m_diffs.begin() + i + 1, wdf);
				m_diffs.insert(m_diffs.begin() + i + 1, wdfm);
			}
			else if ((begin1 >= 0) && (begin1 < end1))
			{
				// insert side2
				// Visible sync on side1 and side2
				// Now split in two diff
				// New behind actual diff
				wdiff wdf(diff.start[0] + s1 + e1 + end1, diff.end[0],
					diff.start[1] + s2 + e2 , diff.end[1]);
#ifdef STRINGDIFF_LOGGING
				DbgPrint("org\n left = %d,%d\n right = %d,%d\n",
					diff.start[0], diff.end[0],diff.start[1], diff.end[1]);
				DbgPrint("insert\n left = %d,%d\n right = %d,%d\n",
					wdf.start[0], wdf.end[0], wdf.start[1], wdf.end[1]);
#endif
				// change end of actual diff
				diff.end[0] = diff.start[0] + s1 + begin1 + 1;
				diff.end[1] = diff.start[1] + s2 + begin2 + 1;
#ifdef STRINGDIFF_LOGGING
				DbgPrint("changed\n left = %d,%d\n right = %d,%d\n",
					diff.start[0], diff.end[0],diff.start[1], diff.end[1]);
#endif	
				m_diffs.insert(m_diffs.begin() + i + 1, wdf);
			}
			else if ((begin2 >= 0) && (begin2 < end2))
			{
				// insert side1
				// Visible sync on side1 and side2
				// Now split in two diff
				// New behind actual diff
				wdiff wdf(diff.start[0] + s1 + e1, diff.end[0],
					diff.start[1] + s2 + e2 + end2, diff.end[1]);
#ifdef STRINGDIFF_LOGGING
				DbgPrint("org\n left = %d,%d\n right = %d,%d\n",
					diff.start[0], diff.end[0],diff.start[1], diff.end[1]);
				DbgPrint("insert\n left = %d,%d\n right = %d,%d\n",
					wdf.start[0], wdf.end[0], wdf.start[1], wdf.end[1]);
#endif	
				// change end of actual diff
				diff.end[0] = diff.start[0] + s1 + begin1;
				diff.end[1] = diff.start[1] + s2 + begin2;
#ifdef STRINGDIFF_LOGGING
				DbgPrint("changed\n left = %d,%d\n right = %d,%d\n",
					diff.start[0], diff.end[0],diff.start[1], diff.end[1]);
#endif	
				m_diffs.insert(m_diffs.begin() + i + 1, wdf);
			}
		}
	}
}

/**
 * @brief Compute begin1,begin2,end1,end2 to display byte difference between strings str1 & str2
 *
 * @param [out] begin1 return -1 if not found or pos of equal
 * @param [out] begin2 return -1 if not found or pos of equal
 * @param [out] end1 return -1 if not found or pos of equal valid if begin1 >=0
 * @param [out] end2 return -1 if not found or pos of equal valid if begin2 >=0
 * @param [in] equal false surch for a diff, true surch for equal
 * @param [in] func 0-3 how to synchron,0,2 str1, 1,3 str2
 * @param [in,out] s1 offset str1 begin
 * @param [in,out] e1 offset str1 end
 * @param [in,out] s2 offset str2 begin
 * @param [in,out] e2 offset str2 end
 */
bool stringdiffs::findsyn(wdiff const &diff,
	int &begin1, int &begin2, int &end1, int &end2, bool equal,
	enum sd_findsyn_func func, int &s1, int &e1, int &s2, int &e2) const
{
	int const max1 = diff.end[0] - diff.start[0] - 1;
	int const max2 = diff.end[1] - diff.start[1] - 1;
	while ((s1 + e1) < max1 && (s2 + e2) < max2)
	{
		// Get substrings for both sides
		wdiff const diff_2(
			diff.start[0] + s1, diff.end[0] - e1,
			diff.start[1] + s2, diff.end[1] - e2);
		ComputeByteDiff(diff_2, begin1, begin2, end1, end2, equal);

		switch (func)
		{
		case synbegin1:
			if ((begin1 != -1) && (begin2 != -1))
				return true; // Found a synchronisation
			++s1;
			break;
		case synbegin2:
			if ((begin1 != -1) && (begin2 != -1))
				return true; // Found a synchronisation
			++s2;
			break;
		case synend1:
			if ((begin1 == -1) || (begin2 == -1))
				return false; // Found no synchronisation
			if ((end1 != -1) && (end2 != -1))
				return true; // Found a synchronisation
			++e1;
			break;
		case synend2:
			if ((begin1 == -1) || (begin2 == -1))
				return false; // Found no synchronisation
			if ((end1 != -1) && (end2 != -1))
				return true; // Found a synchronisation
			++e2;
			if ((begin1 > -1) && (begin2 > -1) && (0 <= end1) && (0 <= end2))
				return true; // Found a synchronisation
			break;
		}
	}
	// No synchronisation found
	return false;
}
