/** 
 * @file  MergeDocDiffSync.cpp
 *
 * @brief Code to layout diff blocks, to find matching lines, and insert ghost lines
 *
 */
#include "StdAfx.h"
#include "Merge.h"
#include "OptionsMgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class CChildFrame::DiffMap
{
public:
	enum
	{
		BAD_MAP_ENTRY = -999999999,
		GHOST_MAP_ENTRY = 888888888
	};
	DiffMap(CChildFrame *, int begin0, int begin1, int lines0, int lines1);
	int operator[](int i) { return map[i]; }
private:
	std::vector<int> map;
	std::vector<int> cost;
	void AdjustDiffBlock(int lo0, int hi0, const int lo1, const int hi1);
};

CChildFrame::DiffMap::DiffMap(CChildFrame *pDoc,
	int begin0, int begin1, int lines0, int lines1
) :	map(lines0, BAD_MAP_ENTRY), cost(lines0 * lines1)
{
	// Map & lo & hi numbers are all relative to begin0 & begin1
	CDiffTextBuffer *const tbuf0 = pDoc->m_ptBuf[0];
	CDiffTextBuffer *const tbuf1 = pDoc->m_ptBuf[1];
	for (int j = 0 ; j < lines1 ; ++j)
	{
		LineInfo const &li1 = tbuf1->GetLineInfo(begin1 + j);
		int const k = j * map.size();
		for (int i = 0 ; i < lines0 ; ++i)
		{
			LineInfo const &li0 = tbuf0->GetLineInfo(begin0 + i);
			cost[k + i] = pDoc->GetMatchCost(li0, li1);
		}
	}
	AdjustDiffBlock(0, lines0 - 1, 0, lines1 - 1);
}

/**
 * @brief Map lines from left to right for specified range in diff block, as best we can
 * Map left side range [lo0;hi0] to right side range [lo1;hi1]
 * (ranges include ends)
 *
 * Algorithm:
 * Find best match, and use that to split problem into two parts (above & below match)
 * and call ourselves recursively to solve each smaller problem
 */
void CChildFrame::DiffMap::AdjustDiffBlock(int lo0, int hi0, const int lo1, const int hi1)
{
	// shortcut special case
	if ((lo0 == hi0) && (lo1 == hi1))
	{
		map[lo0] = hi1;
		return;
	}
	// Find best fit
	int ibest = -1;
	int isavings = INT_MAX;
	int itarget = -1;
	for (int j = lo1 ; j <= hi1 ; ++j)
	{
		const int k = j * map.size();
		for (int i = lo0 ; i <= hi0 ; ++i)
		{
			const int savings = cost[k + i];
			// TODO
			// Need to penalize assignments that push us outside the box
			// further than is required
			if (savings < isavings)
			{
				ibest = i;
				itarget = j;
				isavings = savings;
			}
		}
	}
	// I think we have to find at least one
	// because cost (Levenshtein distance) is less than or equal to maxlen
	// so cost is always nonnegative
	ASSERT(ibest >= 0);

	ASSERT(map[ibest] == BAD_MAP_ENTRY);

	map[ibest] = itarget;

	// Half of the remaining problem is below our match
	while (lo0 < ibest)
	{
		if (lo1 < itarget)
		{
			AdjustDiffBlock(lo0, ibest - 1, lo1, itarget - 1);
			break;
		}
		// No target space for the ones below our match
		map[lo0++] = GHOST_MAP_ENTRY;
	}

	// Half of the remaining problem is above our match
	while (hi0 > ibest)
	{
		if (hi1 > itarget)
		{
			AdjustDiffBlock(ibest + 1, hi0, itarget + 1, hi1);
			break;
		}
		// No target space for the ones above our match
		map[hi0--] = GHOST_MAP_ENTRY;
	}
}

/**
 * @brief Divide diff blocks to match lines in diff blocks.
 */
void CChildFrame::AdjustDiffBlocks()
{
	int nMatchSimilarLinesMax = COptionsMgr::Get(OPT_CMP_MATCH_SIMILAR_LINES_MAX);
	// Go through and do our best to line up lines within each diff block
	// between left side and right side
	DiffList newDiffList;
	const int nDiffCount = m_diffList.GetSize();
	for (int nDiff = 0; nDiff < nDiffCount; ++nDiff)
	{
		const DIFFRANGE *pdr = m_diffList.DiffRangeAt(nDiff);
		// size map correctly (it will hold one entry for each left-side line
		int nlines0 = pdr->end0 - pdr->begin0 + 1;
		int nlines1 = pdr->end1 - pdr->begin1 + 1;
		if (nlines0 > 0 && nlines1 > 0 &&		// Only if both sides nonempty
			nlines0 <= nMatchSimilarLinesMax &&
			nlines1 <= nMatchSimilarLinesMax)	// Only if range isn't large
		{
			// Map lines from file1 to file2
			//stl::vector<int> diffmap(nlines0, BAD_MAP_ENTRY);
			DiffMap diffmap(this, pdr->begin0, pdr->begin1, nlines0, nlines1);
			//AdjustDiffBlock(diffmap, pdr->begin0, pdr->begin1, 0, nlines0 - 1, 0, nlines1 - 1);
#ifdef _DEBUG
			for (int i = 0 ; i < nlines0 ; ++i)
			{
				TRACE("begin0=%d begin1=%d diffmap[%d]=%d\n",
					pdr->begin0, pdr->begin1, i, diffmap[i]);
			}
#endif
			// divide diff blocks
			DIFFRANGE dr;
			dr.op = pdr->op;
			int line0 = 0;
			int line1 = 0;
			while (line0 < nlines0)
			{
				int map_line0 = diffmap[line0];
				if (map_line0 == DiffMap::GHOST_MAP_ENTRY ||
					map_line0 == DiffMap::BAD_MAP_ENTRY)
				{
					// insert ghostlines on right side
					dr.begin0  = pdr->begin0 + line0;
					dr.begin1  = pdr->begin1 + line1;
					while ((map_line0 == DiffMap::GHOST_MAP_ENTRY ||
						map_line0 == DiffMap::BAD_MAP_ENTRY) &&
						(++line0 < nlines0))
					{
						map_line0 = diffmap[line0];
					}
					dr.end0    = pdr->begin0 + line0 - 1;
					dr.end1    = dr.begin1 - 1;
					newDiffList.AddDiff(dr);
				}
				else
				{
					// insert ghostlines on left side
					dr.begin0  = pdr->begin0 + line0;
					if (line1 < map_line0)
					{
						dr.begin1  = pdr->begin1 + line1;
						dr.end0    = dr.begin0 - 1;
						dr.end1    = pdr->begin1 + map_line0 - 1;
						newDiffList.AddDiff(dr);
						line1 = map_line0;
					} 
					dr.begin1  = pdr->begin1 + line1;
					do
					{
						line1 = diffmap[line0++] + 1;
					} while ((map_line0 == line1) && (line0 < nlines0));
					dr.end0    = pdr->begin0 + line0 - 1;
					dr.end1    = pdr->begin1 + line1 - 1;
					newDiffList.AddDiff(dr);
				}
			}
			if (line1 < nlines1)
			{
				dr.begin0  = pdr->begin0 + line0;
				dr.begin1  = pdr->begin1 + line1;
				dr.end0    = dr.begin0 - 1;
				dr.end1    = pdr->begin1 + nlines1 - 1;
				newDiffList.AddDiff(dr);
			}
		}
		else
		{
			newDiffList.AddDiff(*pdr);
		}
	}

	// Swap into m_diffList
	m_diffList.swap(newDiffList);
}

/**
 * @brief Return cost of making strings equal
 *
 * The cost of making them equal is the measure of their dissimilarity,
 * which at an early stage of implementation was represented by their
 * Levenshtein distance, but now is represented by the summed up lengths
 * of the word diffs produced by sd_ComputeWordDiffs().
 */
int CChildFrame::GetMatchCost(LineInfo const &li0, LineInfo const &li1)
{
	// Options that affect comparison
	bool const casitive = !m_diffWrapper.bIgnoreCase;
	int const xwhite = m_diffWrapper.nIgnoreWhitespace;
	int const breakType = COptionsMgr::Get(OPT_BREAK_TYPE); // whitespace only or include punctuation
	bool const byteColoring = COptionsMgr::Get(OPT_CHAR_LEVEL);

	int const len1 = li0.Length();
	LPCTSTR const str1 = li0.GetLine();
	int const len2 = li1.Length();
	LPCTSTR const str2 = li1.GetLine();

	std::vector<wdiff> worddiffs;
	sd_ComputeWordDiffs(str1, len1, str2, len2, casitive, xwhite, breakType, byteColoring, worddiffs);

	int cost = 0;
	std::vector<wdiff>::const_iterator it = worddiffs.begin();
	while (it != worddiffs.end())
	{
		wdiff const &wd = *it++;
		cost += max(wd.end[0] - wd.start[0], wd.end[1] - wd.start[1]);
	}
	return cost;
}
