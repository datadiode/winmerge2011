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

using stl::vector;

// Uncomment this to see stringdiff log messages
// We don't use _DEBUG since stringdiff logging is verbose and slows down WinMerge
// #define STRINGDIFF_LOGGING

static const TCHAR BreakCharDefaults[] = _T(",.;:");
static const TCHAR *BreakChars = BreakCharDefaults;

static bool isWordBreak(int breakType, TCHAR ch);
static void wordLevelToByteLevel(vector<wdiff> &diffs,
	const String &str1, const String &str2, bool casitive, int xwhite);

static bool sd_findsyn(const wdiff &diff,
	const String &str1, const String &str2, bool casitive, int xwhite,
	int &begin1, int &begin2, int &end1, int &end2, bool equal,
	enum sd_findsyn_func func, int &s1, int &e1, int &s2, int &e2);

void sd_SetBreakChars(const TCHAR *breakChars)
{
	TRACE("Setting BreakChars to %ls\n", breakChars);
	if (BreakChars != BreakCharDefaults)
		free(const_cast<TCHAR *>(BreakChars));
	BreakChars = breakChars ? _tcsdup(breakChars) : BreakCharDefaults;
}

/**
 * @brief Construct our worker object and tell it to do the work
 */
void sd_ComputeWordDiffs(const String & str1, const String & str2,
	bool case_sensitive, int whitespace, int breakType, bool byte_level,
	vector<wdiff> &diffs)
{
	stringdiffs sdiffs(str1, str2, case_sensitive, whitespace, breakType, diffs);
	// Hash all words in both lines and then compare them word by word
	// storing differences into m_wdiffs
	sdiffs.BuildWordDiffList();
	// Now copy m_wdiffs into caller-supplied m_pDiffs (coalescing adjacents if possible)
	sdiffs.PopulateDiffs();
	// Adjust the range of the word diff down to byte (char) level.
	if (byte_level)
		wordLevelToByteLevel(diffs, str1, str2, case_sensitive, whitespace);
}

/**
 * @brief stringdiffs constructor simply loads all members from arguments
 */
stringdiffs::stringdiffs(const String &str1, const String &str2,
	bool case_sensitive, int whitespace, int breakType, vector<wdiff> &diffs)
: m_str1(str1)
, m_str2(str2)
, m_case_sensitive(case_sensitive)
, m_whitespace(whitespace)
, m_breakType(breakType)
, m_diffs(diffs)
, m_matchblock(true) // Change to false to get word to word compare
{
}

/**
 * @brief Destructor.
 * The destructor frees all diffs added to the vectors.
 */
stringdiffs::~stringdiffs()
{
}

/**
 * @brief Add all different elements between lines to the wdiff list
 */
void stringdiffs::BuildWordDiffList()
{
	BuildWordsArray(m_str1, m_words1);
	BuildWordsArray(m_str2, m_words2);

	int start = 0;
	int end = 0;
	// If we have to ignore all whitespace change,
	// just remove leading and ending if one is there
	if (m_whitespace == WHITESPACE_IGNORE_ALL && m_matchblock)
	{
		// Remove a leading whitespace
		if (m_words1.size() > 0 && IsSpace(m_words1[0]))
		{
			m_words1.erase(m_words1.begin());
		}
		// Remove a ending whitespace
		if (m_words1.size() > 0 && IsSpace(m_words1[m_words1.size() - 1]))
		{
			m_words1.pop_back();
		}
		// Remove a leading whitespace
		if (m_words2.size() > 0 && IsSpace(m_words2[0]))
		{
			m_words2.erase(m_words2.begin());
		}
		// Remove a ending whitespace
		if (m_words2.size() > 0 && IsSpace(m_words2[m_words2.size() - 1]))
		{
			m_words2.pop_back();
		}
	}
	// Look for a match of word2 in word1
	// not found put an empty record at beginn of word1
	// if distance to far, crosscheck match word1 in word2
	// whatever is shorter, it's result.
	if (m_matchblock)
	{
		int bw1 = 0; // start position in m_words1
		int bw2 = 0; // start position in m_words2
		while (bw2 < (int)m_words2.size())
		{
			if (bw1 >= (int)m_words1.size())
				break;
			if (m_whitespace == WHITESPACE_IGNORE_ALL)
			{
				bool lbreak = false; // repeat
				if (IsSpace(m_words1[bw1]))
				{
					m_words1.erase(m_words1.begin() + bw1);
					lbreak = true;
				}
				if (IsSpace(m_words2[bw2]))
				{
					m_words2.erase(m_words2.begin() + bw2);
					lbreak = true;
				}
				if (lbreak)
					continue;
			}
			// Are we looking for a spacebreak, so just look for word->bBreak
			// position found in array
			int w1 = IsSpace(m_words2[bw2]) ?
				FindNextSpaceInWords(m_words1, bw1) :
				FindNextMatchInWords(m_words1, m_words2[bw2], bw1,1);
			// Found at same position, so go on with next word 
			if (w1 == bw1)
			{
				++bw1;
				++bw2;
				continue;
			}
			int w2 = w1;
			// Not found, not same, check whitch distance is shorter
			if (w1 == -1 || (w1 - bw1) > 0)
			{
				// Are we looking for a spacebreak, so just look for word->bBreak
				if (IsSpace(m_words1[bw1]))
					w2 = FindNextSpaceInWords(m_words2, bw2);
				else
					w2 = FindNextMatchInWords(m_words2, m_words1[bw1], bw2, 2);
				// Execption both are not found in other string
				// so all between keep as a differ
				if (w1 == -1 && w2 == -1)
				{
					// check if it would be same if we remove the spaceblock
					// we must only ckeck on the side with the shortes word
					if ((m_words1[bw1].end - m_words1[bw1].start) > (m_words2[bw2].end - m_words2[bw2].start))
					{
						// first we check for size word2 and next word as space
						if ((bw2 < (int)m_words2.size() - 2) && IsSpace(m_words2[bw2 + 1]))
						{
							// Are the contents same
							if (m_words1[bw1].hash ==
								Hash(m_str2, m_words2[bw2 + 2].start, m_words2[bw2 + 2].end, m_words2[bw2].hash))
							{
								m_words2[bw2].end = m_words2[bw2 + 2].end;
								m_words2[bw1].hash = m_words1[bw1].hash;
								// Now remove the detected blocks on side2.
								m_words2.erase(m_words2.begin() + bw2 + 1, m_words2.begin() + bw2 + 3);
							}
						}
					}
					else if ((m_words1[bw1].end - m_words1[bw1].start) < (m_words2[bw2].end - m_words2[bw2].start))
					{
						// first we check for size  word1 and next word as space
						if ((bw1 < (int)m_words1.size() - 2) && IsSpace(m_words1[bw1 + 1]))
						{
							// Are the contents same
							if (m_words2[bw2].hash ==
								Hash(m_str1, m_words1[bw1 + 2].start, m_words1[bw1 + 2].end, m_words1[bw1].hash))
							{
								m_words1[bw1].end = m_words1[bw1 + 2].end;
								m_words1[bw1].hash = m_words2[bw1].hash;
								// Now remove the detected blocks on side2.
								m_words1.erase(m_words1.begin() + bw1 + 1, m_words1.begin() + bw1 + 3);
							}
						}
					}
					// Otherwise keep as diff
					++bw1;
					++bw2;
					continue;
				}
				// Not found on one side, so check if we are synchron again on
				// next words (try not to check breaks)
				if ((w1 == -1) || (w2 == -1))
				{
					// In relation to words.size()
					// Check if in distance 2 and (3 or 4) is equal again
					if (bw1 + 4 < (int)m_words1.size() && (bw2 + 4 < (int)m_words2.size()))
					{
						if (AreWordsSame(m_words1[bw1 + 2], m_words2[bw2 + 2]))
						{
							if (AreWordsSame(m_words1[bw1 + 3], m_words2[bw2 + 3]) || AreWordsSame(m_words1[bw1 + 4], m_words2[bw2 + 4]))
							{
								// Ok than keep it as a differ
								bw1 += 2;
								bw2 += 2;
								continue;
							}
						}
					}
					// Check if in distance 2 is equal again
					else if (bw1 + 2 < (int)m_words1.size() && (bw2 + 2 < (int)m_words2.size()))
					{
						if (AreWordsSame(m_words1[bw1 + 1], m_words2[bw2 + 1]))
						{
							// Ok than keep it as a differ
							bw1 += 2;
							bw2 += 2;
							continue;
						}
					}
				}
			}
			// distance from word1 to word2 is too far away on both side
			// or too far away and not found on the other side
			const int maxDistance = 6;
			const bool ok = (w2 - bw2 > maxDistance);
			if (((w1 - bw1 > maxDistance) && (w2 - bw2 > maxDistance))
				|| ((w1 - bw1 > maxDistance) && (w2 == -1))
				|| ((w2 - bw2 > maxDistance) && (w1 == -1)))
			{
				// keep as diff
				bw1++;
				bw2++;
				continue;
			}
			else if ((w2 > bw2) && ((w1 == -1 || (w2 - bw2) < (w1 - bw1))))
			{
				// Numbers of inserts in word1
				int k = w2 - bw2;
				for (int l = 0; l < k; l++)
				{
					// Remember last start end
					if (bw1)
					{
						end = m_words1[bw1 - 1].end;
					}
					else
					{
						end = -1;
					}
					start = end + 1;
					vector<word>::iterator iter = m_words1.begin() + bw1;
					m_words1.insert(iter, word(start, end, dlinsert, 0));
				}
				bw1 += k + 1;
				bw2 += k + 1; // Next record
				continue;
			}
			else if (w1 > bw2)
			{
				// Numbers of inserts in word2			
				int k = w1 - bw2;
				for (int l = 0; l < k; l++)
				{
					// Remember last start end
					if (bw2)
					{
						end = m_words2[bw2 - 1].end;
					}
					else
					{
						end = -1;
					}
					start = end + 1;
					vector<word>::iterator iter = m_words2.begin() + bw2;
					m_words2.insert(iter, word(start, end, dlinsert, 0));
				}
				bw1 += k + 1;
				bw2 += k + 1; // Next record
				continue;
			}
			else if (w1 == bw2)
			{
				++bw1;
				++bw2;
			}
		}
	}

	// Make both array to same length
	if (m_words1.size() > 0 || m_words2.size() > 0)
	{
		if (m_words1.size() != m_words2.size())
		{
			int length1 = (int)m_words1.size() - 1;
			int length2 = (int)m_words2.size() - 1;

			if (length1 < length2)
			{
				// Numbers of inserts in word1			
				int k = length2 - length1;
				// Remember last start end
				if (length1 == -1)
				{
					end = -1;
				}
				else
				{
					end = m_words1[length1].end;
				}
				start = end + 1;
				for (int l = 0; l < k; l++)
				{
					m_words1.push_back(word(start, end, dlinsert, 0));
				}
			}
			else if (length1 > length2)
			{
				// Numbers of inserts in word2			
				int k = length1 - length2;
				// Remember last start end
				if (length2 == -1)
				{
					end = -1;
				}
				else
				{
					end = m_words2[length2].end;
				}
				start = end + 1;
				for (int l = 0; l < k; l++)
				{
					m_words2.push_back(word(start, end, dlinsert, 0));
				}
			}
		}

		if (m_matchblock)
		{
			// Look for a match of word2 in word1
			// We do it from back to get more acurated matches
			int bw1 = (int)m_words1.size(); // start position in m_words1
			int bw2 = (int)m_words2.size(); // start position in m_words2
			while (--bw1 > 1 && --bw2 > 1)
			{
				if (AreWordsSame(m_words1[bw1], m_words2[bw2]) ||
					IsSpace(m_words1[bw1]) && IsSpace(m_words2[bw2]))
				{
					continue;
				}
				int w1 = -2, w2 = -2; // position found in array

				// Normaly we synchronise with a *word2 to a match in word1
				// If it is an Insert in word2 so look for a *word1 in word2
				if (IsInsert(m_words2[bw2]))
					w2 = FindPreMatchInWords(m_words2, m_words1[bw1], bw2, 2);
				else
					w1 = FindPreMatchInWords(m_words1, m_words2[bw2], bw1, 1);
				// check all between are inserts
				// if so, so exchange the position
				if (w1 >= 0)
				{
					// Numbers of inserts in word1			
					const int k = bw1 - w1;
					int l = 0;
					for (l = 0; l < k; l++)
					{
						if (!IsInsert(m_words1[bw1 - l]))
							break;
					}
					// all are inserts so k==0
					if ((k - l) == 0)
					{
						MoveInWordsUp(m_words1, w1, bw1);
						continue;
					}
					else if (((k - l) == 1) && !AreWordsSame(m_words1[bw1-l], m_words2[bw2-l]))
					{
						MoveInWordsUp(m_words1, bw2 - l, bw1);
						MoveInWordsUp(m_words1, w1, bw1 - 1);
						//insert a record before in words1 , after in words2
						InsertInWords(m_words2, bw2);
						InsertInWords(m_words1, w1);
						continue;
					}
				}
				if (w2 >= 0)
				{
					// Numbers of inserts in word2			
					const int k = bw2 - w2;
					int l = 0;
					for (l = 0; l < k; l++)
					{
						if (!IsInsert(m_words2[bw2 - l]))
							break;
					}
					// all are inserts so k==0
					if ((k - l) == 0)
					{
						MoveInWordsUp(m_words2, w2, bw2);
						continue;
					}
					else if (((k - l) == 1) && !AreWordsSame(m_words1[bw1-l], m_words2[bw2-l]))
					{
						MoveInWordsUp(m_words2, bw1 - l, bw2);
						MoveInWordsUp(m_words2, w2, bw2 - 1);
						//insert a record before in words2 , after in words1
						InsertInWords(m_words1, bw1);
						InsertInWords(m_words2, w2);
						continue;
					}
				}
				// otherwise go on
				// check for an insert on both side
				// if so move next preblock to this position
				if (IsInsert(m_words1[bw1]))
				{
					int k = FindPreNoInsertInWords(m_words1, bw1);
					if (k == 1)
					{
						continue;
					}
					if (k >= 0 && !AreWordsSame(m_words1[k], m_words2[k]) ||
						k > 0 && !AreWordsSame(m_words1[k - 1], m_words2[k - 1]))
					{
						MoveInWordsUp(m_words1, k, bw1);
					}
				}
				if (IsInsert(m_words2[bw2]))
				{
					int k = FindPreNoInsertInWords(m_words2, bw2);
					if (k == 1)
					{
						continue;
					}
					if (k >= 0 && !AreWordsSame(m_words1[k], m_words2[k]) ||
						k > 0 && !AreWordsSame(m_words1[k - 1], m_words2[k - 1]))
					{
						MoveInWordsUp(m_words2, k, bw2);
					}
				}
				continue; // TODO: Why does this affect release build code size?
			}
		}
// I care about consistency and I think most apps will highlight the space
//  after the word so that would be my preference.
// to get this we need a thirt run, only look for inserts now!
		int w1, w2; // position found in array
		int bw1 = 0; // start position in m_words1
		int bw2 = 0; // start position in m_words2
		do
		{
			w1 = FindNextInsertInWords(m_words1, bw1);
			w2 = FindNextInsertInWords(m_words2, bw2);
			if (w1 == w2)
			{
				bw1++;
				bw2++;
			}
			// word1 is first
			else if (w1 >= 0 && (w1 < w2 || w2 == -1))
			{
				bw1 = FindNextNoInsertInWords(m_words1, w1);
				if (bw1 >=0 && !AreWordsSame(m_words1[bw1], m_words2[bw1]))
				{
					// Move block to actual position
					MoveInWordsDown(m_words1, bw1, w1);
				}
				bw1 = ++w1;
				bw2 = bw1;
			}
			else if(w2 >= 0 && (w2 < w1 || w1 == -1))
			{
				bw2 = FindNextNoInsertInWords(m_words2,w2);
				if (bw2 >=0 && !AreWordsSame(m_words1[bw2], m_words2[bw2]))
				{
					// Move block to actual position
					MoveInWordsDown(m_words2, bw2, w2);
				}
				bw1 = ++w2;
				bw2 = bw1;
			}		
		} while (w1 >= 0 || w2 >= 0);

		// Remove empty records on both side
#ifdef STRINGDIFF_LOGGING
		DbgPrint("remove empty records on both side\n");
#endif
		int i = 0; 
		while ((i < (int)m_words1.size()) && (i < (int)m_words2.size()))
		{
#ifdef STRINGDIFF_LOGGING
			DbgPrint("left=%d, op=%d, right=%d, op=%d\n",
				m_words1[i].hash, m_words1[i].bBreak, m_words2[i].hash, m_words2[i].bBreak);
#endif

			if (IsInsert(m_words1[i]) && AreWordsSame(m_words1[i], m_words2[i]))
			{
				m_words1.erase(m_words1.begin() + i);
				m_words2.erase(m_words2.begin() + i);
				continue;
			}
			i++;
		}

		// Remove empty records on both side
		// in case on the otherside +1 is also an empty one
		i = 0;  
		while ((i < ((int)m_words1.size() - 2)) && (i < ((int)m_words2.size() - 2)))
		{
			if (IsInsert(m_words1[i]) && IsInsert(m_words2[i + 1]))
			{
				m_words1.erase(m_words1.begin() + i);
				m_words2.erase(m_words2.begin() + i + 1);
				continue;
			}
			if (IsInsert(m_words1[i + 1]) && IsInsert(m_words2[i]))
			{
				m_words1.erase(m_words1.begin() + i + 1);
				m_words2.erase(m_words2.begin() + i);
				continue;
			}
			i++;
		}	

		// remove equal records on both side
		// even diff is close together
		i = 0; 
		while ((i < (int) m_words1.size() - 1) && (i < (int) m_words2.size() - 1))
		{
			if (AreWordsSame(m_words1[i], m_words2[i]))
			{
				m_words1.erase(m_words1.begin() + i);
				m_words2.erase(m_words2.begin() + i);
				continue;
			}
			++i;
		}

		// ignore all diff in whitespace
		// so remove same records
		if (m_whitespace != WHITESPACE_COMPARE_ALL)
		{
			i = 0; 
			while ((i < (int)m_words1.size()) && (i < (int)m_words2.size()))
			{
				if (IsSpace(m_words1[i]) && IsSpace(m_words2[i]))
				{
					m_words1.erase(m_words1.begin() + i);
					m_words2.erase(m_words2.begin() + i);
					continue;
				}
				++i;
			}
		}

		// Now lets connect inserts to one if same words 
		// are also words at a block
		// doit for word1
		i = 0;
		while ((int)m_words1.size() > i + 1)
		{
			if (IsInsert(m_words1[i]) && IsInsert(m_words1[i + 1]))
			{
				if ((m_words2[i].end +1 ) == (m_words2[i + 1].start))
				{
					m_words2[i].end = m_words2[i + 1].end;
					m_words1.erase(m_words1.begin() + i + 1);
					m_words2.erase(m_words2.begin() + i + 1);
					continue;
				}
			}
			i++;
		}
		// Doit for word2
		i = 0;
		while ((int)m_words2.size() > i + 1)
		{
			if (IsInsert(m_words2[i]) && IsInsert(m_words2[i + 1]))
			{
				if ((m_words1[i].end + 1) == (m_words1[i + 1].start))
				{
					m_words1[i].end = m_words1[i + 1].end;
					m_words1.erase(m_words1.begin() + i + 1);
					m_words2.erase(m_words2.begin() + i + 1);
					continue;
				}
			}
			i++;
		}
		// Check last insert to be at string.length
		// protect to put last insert behind string for editor, otherwise
		// it shows last char double
		i = (int)m_words1.size() - 1;
		if (i >= 1)
		{
			if (IsInsert(m_words1[i]) &&
				((m_words1[i].start == m_str1.length() - 1) || (m_words1[i].start == m_words1[i - 1].end)))
			{
				m_words1[i].start = m_str1.length();
				m_words1[i].end = m_words1[i].start - 1;
			}
			else
			{
				if (IsInsert(m_words2[i]) &&
					((m_words2[i].start == m_str2.length() - 1) || (m_words2[i].start == m_words2[i - 1].end)))
				{
					m_words2[i].start = m_str2.length();
					m_words2[i].end = m_words2[i].start - 1;
				}
			}
		}
		// Final run create diff
#ifdef STRINGDIFF_LOGGING
		DbgPrint("final run create diff\n");
		DbgPrint("left=  %d,   right=  %d\n",
			m_str1.length(), m_str2.length());
#endif
		// Be aware, do not create more wdiffs as shortest line has chars!
		int imaxcount = min(m_str1.length(), m_str2.length());
		i = 0;
		const int iSize1 = (int)m_words1.size();
		const int iSize2 = (int)m_words2.size();
		while ((i < iSize1) && (i < iSize2) && ((int)m_wdiffs.size() <= imaxcount))
		{
			if (!AreWordsSame(m_words1[i], m_words2[i]))
			{
				int s1 =  m_words1[i].start;
				int e1 =  m_words1[i].end;
				int s2 =  m_words2[i].start;
				int e2 =  m_words2[i].end ;

#ifdef STRINGDIFF_LOGGING
				int len1 = e1 - s1 + 1;
				int len2 = e2 - s2 + 1;
				String str1;
				String str2;
				if (!IsInsert(m_words1[i]))
				{
					if (len1 < 50)
						str1 = m_str1.substr(s1 ,e1 - s1 + 1);
					else
						str1 = m_str1.substr(s1, 50);
				}
				if (!IsInsert(m_words2[i]))
				{
					if (len2 < 50)
						str2 = m_str2.substr(s2, e2- s2 + 1);
					else
						str2 = m_str2.substr(s2, 50);
				}
				DbgPrint("left=  %ls,   %d,%d\n, right=  %ls,   %d,%d\n",
					str1.c_str(), s1, e1, str2.c_str(), s2, e2);
				DbgPrint("left=%d , op=%d, right=%d, op=%d\n",
					m_words1[i].hash, m_words1[i].bBreak, m_words2[i].hash, m_words2[i].bBreak);
#endif
				m_wdiffs.push_back(wdiff(s1, e1, s2, e2));
			}
			i++;
		}
	}
}

/**
 * @brief Insert a new block in words)
 */
void stringdiffs::InsertInWords(vector<word> &words, int bw)
{
	// Remember last start end
	int end, start;
	if (bw)
	{
		end = words[bw - 1].end;
	}
	else
	{
		end = -1;
	}
	start = end + 1;
	vector<word>::iterator iter = words.begin() + bw;
	words.insert(iter, word(start, end, dlinsert, 0));
}

/**
 * @brief Find pre word in m_words2 (starting at bw2) that matches needword1 (in m_words1)
 */
int stringdiffs::FindPreMatchInWords(const vector<word> &words, const word & needword, int bw, int side) const
{
	while (bw >= 0)
	{
		if (side == 1)
		{
			if (AreWordsSame(words[bw], needword))
				return bw;
		}
		else
		{
			if (AreWordsSame(needword, words[bw]))
				return bw;
		}		--bw;
	}
	return -1;
}

/**
 * @brief Find next word in words (starting at bw) that matches needword1 (in m_words1)
 */
int stringdiffs::FindNextMatchInWords(const vector<word> &words, const word & needword, int bw, int side) const
{
	const int iSize = (int) words.size();
	while (bw < iSize)
	{
		if (side == 1)
		{
			if (AreWordsSame(words[bw], needword))
				return bw;
		}
		else
		{
			if (AreWordsSame(needword, words[bw]))
				return bw;
		}
		++bw;
	}
	return -1;
}

/**
 * @brief Find next space in m_words (starting at bw)
 */
int stringdiffs::FindNextSpaceInWords(const vector<word> &words, int bw)
{
	const int iSize = (int) words.size();
	while (bw < iSize)
	{
		if (IsSpace(words[bw]))
			return bw;
		++bw;
	}
	return -1;
}

/**
 * @brief Find next pre noinsert in words (starting at bw)
 */
int stringdiffs::FindPreNoInsertInWords(const vector<word> &words, int bw)
{
	while (bw >= 0)
	{
		if (!IsInsert(words[bw]))
			return bw;
		--bw;
	}
	return -1;
}

/**
 * @brief Find next insert in m_words (starting at bw)
 */
int stringdiffs::FindNextInsertInWords(const vector<word> &words, int bw)
{
	const int iSize = (int) words.size();
	while (bw < iSize)
	{
		if (IsInsert(words[bw]))
			return bw;
		++bw;
	}
	return -1;
}

/**
 * @brief Find next noinsert in m_words (starting at bw)
 */
int stringdiffs::FindNextNoInsertInWords(const vector<word> &words, int bw)
{
	const int iSize = (int) words.size();
	while (bw < iSize)
	{
		if (!IsInsert(words[bw]))
			return bw;
		++bw;
	}
	return -1;
}

/**
 * @brief Move word to new position (starting at bw)
 */
void stringdiffs::MoveInWordsUp(vector<word> &words, int source, int target)
{
	word wd = words[source];
	words.erase(words.begin() + source);
	vector<word>::iterator iter = words.begin() + target;
	words.insert(iter, wd);
	// Correct the start-end pointer
	const int istart = words[target].start;
	const int iend =istart - 1;
	for (; source < target ; source++)
	{
		words[source].start = istart;
		words[source].end = iend;	
	}
}

/**
 * @brief Move word to new position (starting at bw)
 */
void stringdiffs::MoveInWordsDown(vector<word> &words, int source, int target)
{
	word wd = words[source];
	words.erase(words.begin() + source);
	vector<word>::iterator iter = words.begin() + target;
	words.insert(iter, wd);
	// Correct the start-end pointer
	const int istart = words[target].end + 1;
	const int iend =istart - 1;
	target++;
	for (; target < source + 1; target++)
	{
		words[target].start = istart;
		words[target].end = iend;	
	}
}

/**
 * @brief Break line into constituent words
 */
void stringdiffs::BuildWordsArray(const String & str, vector<word> &words)
{
	int i=0, begin=0;

	// state when we are looking for next word
inspace:
	if (xisspace(str[i])) 
	{
		++i;
		goto inspace;
	}
	if (begin < i)
	{
		// just finished a word
		// e is first word character (space or at end)
		int e = i - 1;

		word wd(begin, e, dlspace, Hash(str, begin, e, 0));
		words.push_back(wd);
	}
	if (i == str.length())
		return;
	begin = i;
	goto inword;

	// state when we are inside a word
inword:
	int atspace = 0;
	if (i == str.length() || ((atspace = xisspace(str[i])) != 0) || isWordBreak(m_breakType, str[i]))
	{
		if (begin<i)
		{
			// just finished a word
			// e is first non-word character (space or at end)
			int e = i-1;
			
			word wd(begin, e, dlword, Hash(str, begin, e, 0));
			words.push_back(wd);
		}
		if (i == str.length())
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
			word wd(i, i, dlbreak, Hash(str, i, i, 0));
			words.push_back(wd);
			++i;
			begin = i;
			goto inword;
		}
	}
	++i;
	goto inword; // safe even if we're at the end or no longer in a word
}

/**
 * @brief Populate m_pDiffs from m_wdiffs (combining adjacent diffs)
 *
 * Doing the combining of adjacent diffs here keeps some complexity out of BuildWordsArray.
 */
void stringdiffs::PopulateDiffs()
{
	for (int i=0; i< (int)m_wdiffs.size(); ++i)
	{
		bool skipIt = false;
		// combine it with next ?
		if (i+1< (int)m_wdiffs.size())
		{
			if (m_wdiffs[i].end[0] == m_wdiffs[i+1].start[0]
				&& m_wdiffs[i].end[1] == m_wdiffs[i+1].start[1])
			{
				// diff[i] and diff[i+1] are contiguous
				// so combine them into diff[i+1] and ignore diff[i]
				m_wdiffs[i+1].start[0] = m_wdiffs[i].start[0];
				m_wdiffs[i+1].start[1] = m_wdiffs[i].start[1];
				skipIt = true;
			}
		}
		if (!skipIt)
		{
			// Should never have a pair where both are missing
			assert(m_wdiffs[i].start[0] >= 0 || m_wdiffs[i].start[1] >= 0);

			// Store the diff[i] in the caller list (m_pDiffs)
			m_diffs.push_back(m_wdiffs[i]);
		}
	}
}

// diffutils hash

/* Rotate a value n bits to the left. */
#define UINT_BIT (sizeof(unsigned) * CHAR_BIT)
#define ROL(v, n) ((v) << (n) | (v) >> (UINT_BIT - (n)))
/* Given a hash value and a new character, return a new hash value. */
#define HASH(h, c) ((c) + ROL(h, 7))

UINT stringdiffs::Hash(const String &str, int begin, int end, UINT h) const
{
	for (int i = begin; i <= end; ++i)
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
bool stringdiffs::AreWordsSame(const word &word1, const word &word2) const
{
	if (m_whitespace == WHITESPACE_IGNORE_ALL)
		if (word1.hash == word2.hash)
			return true;
	if (word1.hash != word2.hash)
		return false;
	if (word1.length() != word2.length())
		return false;
	for (int i = 0; i < word1.length(); ++i)
	{
		if (!caseMatch(m_str1[word1.start + i], m_str2[word2.start + i]))
			return false;
	}
	return true;
}

/**
 * @brief Is this block a word one?
 */
bool stringdiffs::IsWord(const word &word1)
{
	return word1.kind == dlword;
}

/**
 * @brief Is this block an space or whitespace one?
 */
bool stringdiffs::IsSpace(const word &word1)
{
	return word1.kind == dlspace;
}
/**
 * @brief Is this block a break?
 */
bool stringdiffs::IsBreak(const word &word1)
{
	return word1.kind == dlbreak || word1.kind == dlspace;
}

/**
 * @brief Is this block an empty (insert) one?
 */
bool stringdiffs::IsInsert(const word &word1)
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
void sd_ComputeByteDiff(const String &str1, const String &str2, bool casitive,
	int xwhite, int &begin1, int &begin2, int &end1, int &end2, bool equal)
{
	// Set to sane values
	// Also this way can distinguish if we set begin1 to -1 for no diff in line
	begin1 = end1 = begin2 = end2 = 0;

	const int len1 = str1.length();
	const int len2 = str2.length();

	if (len1 == 0 || len2 == 0)
	{
		if (len1 == len2)
		{
			begin1 = -1;
			begin2 = -1;
		}
		end1 = len1 - 1;
		end2 = len2 - 1;
		return;
	}

	LPCTSTR pbeg1 = str1.c_str();
	LPCTSTR pbeg2 = str2.c_str();

	// cursors from front, which we advance to beginning of difference
	LPCTSTR py1 = pbeg1;
	LPCTSTR py2 = pbeg2;

	// pen1,pen2 point to the last valid character
	LPCTSTR pen1 = py1 + len1 - 1;
	LPCTSTR pen2 = py2 + len2 - 1;

	if (xwhite != WHITESPACE_COMPARE_ALL)
	{
		// Ignore leading and trailing whitespace
		// by advancing py1 and py2
		// and retreating pen1 and pen2
		while (py1 < pen1 && xisspace(*py1))
			++py1; // DBCS safe because of xisspace above
		while (py2 < pen2 && xisspace(*py2))
			++py2; // DBCS safe because of xisspace above
		if ((pen1 < pbeg1 + len1 - 1 || pen2 < pbeg2 + len2 -1)
			&& (!len1 || !len2 || pbeg1[len1] != pbeg2[len2]))
		{
			// mismatched broken multibyte ends
		}
		else
		{
			while (pen1 > py1 && xisspace(*pen1))
				--pen1;
			while (pen2 > py2 && xisspace(*pen2))
				--pen2;
		}
	}
	//check for excaption of empty string on one side
	//In that case display all as a diff
	if (!equal && (((py1 == pen1) && xisspace(*pen1)) ||
		((py2 == pen2) && xisspace(*pen2))))
	{
		begin1 = 0;
		begin2 = 0;
		end1 = len1 - 1;
		end2 = len2 - 1;
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
		if (xwhite != WHITESPACE_COMPARE_ALL && xisspace(*py1))
		{
			if (xwhite == WHITESPACE_IGNORE_CHANGE && !xisspace(*py2))
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
		if (xwhite != WHITESPACE_COMPARE_ALL && xisspace(*py2))
		{
			if (xwhite == WHITESPACE_IGNORE_CHANGE && !xisspace(*py1))
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
			if (!matchchar(py1[0], py2[0], casitive))
				break; // done with forward search
		}
		else 
		{
			if (matchchar(py1[0], py2[0], casitive))
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
		++py1; // DBCS safe b/c we checked above
		++py2; // DBCS safe b/c we checked above
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
			if (xwhite!=WHITESPACE_COMPARE_ALL && xisspace(*pz1))
			{
				if (xwhite==WHITESPACE_IGNORE_CHANGE && !xisspace(*pz2))
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
			if (xwhite!=WHITESPACE_COMPARE_ALL && xisspace(*pz2))
			{
				if (xwhite==WHITESPACE_IGNORE_CHANGE && !xisspace(*pz1))
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
				if (!matchchar(pz1[0], pz2[0], casitive))
				{
					found = true;
					break; // done with forward search
				}
			}
			else 
			{
				if (matchchar(pz1[0], pz2[0], casitive))
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
 * @brief adjust the range of the specified word diffs down to byte(char) level.
 * @param str1, str2 [in] line to be compared
 * @param casitive [in] true for case-sensitive, false for case-insensitive
 * @param xwhite [in] This governs whether we handle whitespace specially
 *  (see WHITESPACE_COMPARE_ALL, WHITESPACE_IGNORE_CHANGE, WHITESPACE_IGNORE_ALL)
 */
static void wordLevelToByteLevel(vector<wdiff> &diffs,
	const String &str1, const String &str2, bool casitive, int xwhite)
{
	for (stl_size_t i = 0; i < diffs.size(); i++)
	{
		wdiff &diff = diffs[i];
		bool bRepeat = true;

		// Something to differ?
		if (diff.start[0] > diff.end[0] || diff.start[1] > diff.end[1])
			continue;
#ifdef STRINGDIFF_LOGGING
		DbgPrint("actual\n left=  %d,%d\n right=  %d,%d\n",
			diff.start[0], diff.end[0], diff.start[1], diff.end[1]);
#endif	

		// Check for first and last difference in word
		String str1_2 = str1.substr(diff.start[0], diff.end[0] - diff.start[0] + 1);
		String str2_2 = str2.substr(diff.start[1], diff.end[1] - diff.start[1] + 1);
		int begin1, begin2, end1, end2;
		sd_ComputeByteDiff(str1_2, str2_2, casitive, xwhite, begin1, begin2, end1, end2, false);
		if (begin1 == -1)
		{
			// no visible diff on side1
			diff.end[0] = diff.start[0] - 1;
			bRepeat = false;
		}
		else
		{
			if (end1 == -1)
			{
				diff.start[0] += begin1;
				diff.end[0] = diff.start[0] + end1;
			}
			else
			{
				diff.end[0] = diff.start[0] + end1;
				diff.start[0] += begin1;
			}
		}
		if (begin2 == -1)
		{
			// no visible diff on side2
			diff.end[1] = diff.start[1] - 1;
			bRepeat = false;
		}
		else
		{
			if (end2 == -1)
			{
				diff.start[1] += begin2;
				diff.end[1] = diff.start[1] + end2;
			}
			else
			{
				diff.end[1] = diff.start[1] + end2;
				diff.start[1] += begin2;
			}
		}
#ifdef STRINGDIFF_LOGGING
		DbgPrint("changed\n left=  %d,%d\n right=  %d,%d\n",
			diff.start[0], diff.end[0], diff.start[1], diff.end[1]);
#endif	

		// Nothing to display, remove item
		if (diff.start[0] > diff.end[0] && diff.start[1] > diff.end[1])
		{
			diffs.erase(diffs.begin() + i);
			i--;
			continue;
		}
		// Nothing more todo
		if (((diff.end[0] - diff.start[0]) < 3) ||
			((diff.end[1] - diff.start[1]) < 3))
		{
			continue;
		}
		// Now check if possiblity to get more details
		// We have to check if there is again a synchronisation inside the pDiff
		if (bRepeat && diff.start[0] < diff.end[0] && diff.start[1] < diff.end[1])
		{
			// define offset to zero
			int s1 = 0, e1 = 0, s2 = 0, e2 = 0; 
			// Try to synchron side1 from begin
			bool bsynchron = sd_findsyn(diff, str1, str2, casitive, xwhite,
				begin1, begin2, end1, end2,
				true, synbegin1, s1, e1, s2, e2);

			if (!bsynchron)
			{
				// Try to synchron side2 from begin
				s1 = 0;
				bsynchron = sd_findsyn(diff, str1, str2, casitive, xwhite,
					begin1, begin2, end1, end2,
					true, synbegin2, s1, e1, s2, e2);
			}
			if (bsynchron)
			{
				// Try to synchron side1 from end
				bsynchron = sd_findsyn(diff, str1, str2, casitive, xwhite,
					begin1, begin2, end1, end2,
					true, synend1, s1, e1, s2, e2);
				if (!bsynchron)
				{
					// Try to synchron side1 from end
					e1 = 0;
					bsynchron = sd_findsyn(diff, str1, str2, casitive, xwhite,
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
				DbgPrint("org\n left=  %d,%d\n right=  %d,%d\n",
					diff.start[0], diff.end[0], diff.start[1], diff.end[1]);
				DbgPrint("insert\n left=  %d,%d\n right=  %d,%d\n",
					wdf.start[0], wdf.end[0], wdf.start[1], wdf.end[1]);
#endif
				// change end of actual diff
				diff.end[0] = diff.start[0] + s1 + begin1 - 1;
				diff.end[1] = diff.start[1] + s2 + begin2 - 1;
#ifdef STRINGDIFF_LOGGING
				DbgPrint("changed\n left=  %d,%d\n right=  %d,%d\n",
					diff.start[0], diff.end[0], diff.start[1], diff.end[1]);
#endif			
				// visible sync on side1 and side2
				// new in middle with diff
				wdiff wdfm(diff.end[0]  + 1, wdf.start[0] - 1,
					diff.end[1] + 1, wdf.start[1] - 1);
#ifdef STRINGDIFF_LOGGING
				DbgPrint("insert\n left=  %d,%d\n right=  %d,%d\n",
					wdf.start[0], wdf.end[0], wdf.start[1], wdf.end[1]);
#endif
				diffs.insert(diffs.begin() + i + 1, wdf);
				diffs.insert(diffs.begin() + i + 1, wdfm);
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
				DbgPrint("org\n left=  %d,%d\n right=  %d,%d\n",
					diff.start[0], diff.end[0],diff.start[1], diff.end[1]);
				DbgPrint("insert\n left=  %d,%d\n right=  %d,%d\n",
					wdf.start[0], wdf.end[0], wdf.start[1], wdf.end[1]);
#endif
				// change end of actual diff
				diff.end[0] = diff.start[0] + s1 + begin1;
				diff.end[1] = diff.start[1] + s2 + begin2;
#ifdef STRINGDIFF_LOGGING
				DbgPrint("changed\n left=  %d,%d\n right=  %d,%d\n",
					diff.start[0], diff.end[0],diff.start[1], diff.end[1]);
#endif	
				diffs.insert(diffs.begin() + i + 1, wdf);
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
				DbgPrint("org\n left=  %d,%d\n right=  %d,%d\n",
					diff.start[0], diff.end[0],diff.start[1], diff.end[1]);
				DbgPrint("insert\n left=  %d,%d\n right=  %d,%d\n",
					wdf.start[0], wdf.end[0], wdf.start[1], wdf.end[1]);
#endif	
				// change end of actual diff
				diff.end[0] = diff.start[0] + s1 + begin1;
				diff.end[1] = diff.start[1] + s2 + begin2;
#ifdef STRINGDIFF_LOGGING
				DbgPrint("changed\n left=  %d,%d\n right=  %d,%d\n",
					diff.start[0], diff.end[0],diff.start[1], diff.end[1]);
#endif	
				diffs.insert(diffs.begin() + i + 1, wdf);
			}
		}
	}
}

/**
 * @brief Compute begin1,begin2,end1,end2 to display byte difference between strings str1 & str2
 *
 * @param [in] casitive true for case-sensitive, false for case-insensitive
 * @param [in] xwhite This governs whether we handle whitespace specially
 * (see WHITESPACE_COMPARE_ALL, WHITESPACE_IGNORE_CHANGE, WHITESPACE_IGNORE_ALL)
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
bool sd_findsyn(const wdiff &diff,
	const String &str1, const String &str2, bool casitive, int xwhite,
	int &begin1, int &begin2, int &end1, int &end2, bool equal,
	enum sd_findsyn_func func, int &s1, int &e1, int &s2, int &e2)
{
	const int max1 = diff.end[0] - diff.start[0];
	const int max2 = diff.end[1] - diff.start[1];
	while ((s1 + e1) < max1 && (s2 + e2) < max2)
	{
		// Get substrings for both sides
		String str1_2 = str1.substr(diff.start[0] + s1,
			diff.end[0] - e1 - (diff.start[0] + s1) + 1);
		String str2_2 = str2.substr(diff.start[1] + s2,
			diff.end[1] - e2 - (diff.start[1] + s2) + 1);
		sd_ComputeByteDiff(str1_2, str2_2, casitive, xwhite, begin1, begin2, end1, end2, equal);

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

/**
 * @brief Check if first side is identical.
 * @param [in] worddiffs Diffs from sd_ComputeWordDiffs.
 * @param [in] nLineLength Length of the string.
 * @return true if first side is identical.
 */
bool IsSide0Empty(const vector<wdiff> &worddiffs, int nLineLength)
{
	return (worddiffs[0].end[0] == -1 && worddiffs[0].start[1] == 0 &&
			worddiffs[0].end[1] + 1 == nLineLength);
}
/**
 * @brief Check if second side is identical.
 * @param [in] worddiffs Diffs from sd_ComputeWordDiffs.
 * @param [in] nLineLength Length of the string.
 * @return true if second side is identical.
 */
bool IsSide1Empty(const vector<wdiff> &worddiffs, int nLineLength)
{
	return (worddiffs[0].end[1] == -1  && worddiffs[0].start[0] == 0 &&
			worddiffs[0].end[0] + 1 == nLineLength);
}
