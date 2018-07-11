/////////////////////////////////////////////////////////////////////////////
//    License (GPLv3+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 3 of the License, or (at
//    your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
/////////////////////////////////////////////////////////////////////////////
/**
 * @file  MovedBlocks.cpp
 *
 * @brief Moved block detection code.
 */
#include "stdafx.h"
#include "diff.h"

/**
 * @brief  Set of equivalent lines
 * This uses diffutils line numbers, which are counted from the prefix
 */
class EqGroup
{
public:
	int m_balance;
	int m_cookie0;
	int m_cookie1;
	EqGroup()
		: m_balance(0)
		, m_cookie0(0)
		, m_cookie1(0)
	{
	}
	void deleted(int lineno)
	{
		m_cookie0 = m_cookie0 == 0 || m_cookie0 == lineno ? lineno + 1 : -1;
		--m_balance;
	}
	void inserted(int lineno)
	{
		m_cookie1 = m_cookie1 == 0 || m_cookie1 == lineno ? lineno + 1 : -1;
		++m_balance;
	}
	bool isPerfectMatch() const { return m_balance == 0 && m_cookie0 > 0 && m_cookie1 > 0; }
};

/*
 WinMerge moved block code
 This is called by diffutils code, by diff_2_files routine (in ANALYZE.C)
 read_files earlier computed the hash chains ("equivs" file variable) and freed them,
 but the equivs numerics are still available in each line

 match1 set by scan from line0 to deleted
 match0 set by scan from line1 to inserted

*/
void moved_block_analysis(struct change *script, struct file_data fd[])
{
	// Hash all altered lines
	std::map<int, EqGroup> map;

	struct change *e;
	for (e = script; e; e = e->link)
	{
		int k;
		for (k = e->line0; k - e->line0 < e->deleted; ++k)
			map[fd[0].equivs[k]].deleted(k);
		for (k = e->line1; k - e->line1 < e->inserted; ++k)
			map[fd[1].equivs[k]].inserted(k);
	}

	// Scan through diff blocks, finding moved sections from left side
	// and splitting them out
	// That is, we actually fragment diff blocks as we find moved sections
	for (e = script; e; e = e->link)
	{
		// scan down block for a match
		for (int k = e->line0; k - e->line0 < e->deleted; ++k)
		{
			EqGroup &group = map[fd[0].equivs[k]];
			if (group.isPerfectMatch())
			{
				// found a match
				int i = group.m_cookie0;
				int j = group.m_cookie1;
				// Ok, now our moved block precedes the line i, j
				// extend moved block downward as far as possible
				int i2 = i;
				int j2 = j;
				while (i2 - e->line0 < e->deleted && fd[0].equivs[i2] == fd[1].equivs[j2])
				{
					++i2;
					++j2;
				}
				// extend moved block upward as far as possible
				int i1;
				int j1;
				do
				{
					i1 = i--;
					j1 = j--;
				} while (i >= e->line0 && fd[0].equivs[i] == fd[1].equivs[j]);
				// Ok, now our moved block is i1->i2, j1->j2

				ASSERT(i2 - i1 > 0);
				ASSERT(i2 - i1 == j2 - j1);

				if (int const prefix = i1 - e->line0)
				{
					// break e (current change) into two pieces
					// first part is the prefix, before the moved part
					// that stays in e
					// second part is the moved part & anything after it
					// that goes in newob
					// leave the right side (e->inserted) on e
					// so no right side on newob
					// newob will be the moved part only, later after we split off any suffix from it
					struct change *const newob = static_cast<struct change *>(zalloc(sizeof(struct change)));

					newob->line0 = i1;
					newob->line1 = e->line1 + e->inserted;
					newob->inserted = 0;
					newob->deleted = e->deleted - prefix;
					newob->link = e->link;
					newob->match0 = -1;
					newob->match1 = -1;

					e->deleted = prefix;
					e->link = newob;

					// now make e point to the moved part (& any suffix)
					e = newob;
				}
				// now e points to a moved diff chunk with no prefix, but maybe a suffix

				e->match1 = j1;

				if (int const suffix = e->deleted - (i2 - e->line0))
				{
					// break off any suffix from e
					// newob will be the suffix, and will get all the right side
					struct change *const newob = static_cast<struct change *>(zalloc(sizeof(struct change)));

					newob->line0 = i2;
					newob->line1 = e->line1;
					newob->inserted = e->inserted;
					newob->deleted = suffix;
					newob->link = e->link;
					newob->match0 = -1;
					newob->match1 = -1;

					e->inserted = 0;
					e->deleted -= suffix;
					e->link = newob;
				}
				break;
			}
		}
	}

	// Scan through diff blocks, finding moved sections from right side
	// and splitting them out
	// That is, we actually fragment diff blocks as we find moved sections
	for (e = script; e; e = e->link)
	{
		// scan down block for a match
		for (int k = e->line1; k - e->line1 < e->inserted; ++k)
		{
			EqGroup &group = map[fd[1].equivs[k]];
			if (group.isPerfectMatch())
			{
				// found a match
				int i = group.m_cookie0;
				int j = group.m_cookie1;
				// Ok, now our moved block precedes the line i, j
				// extend moved block downward as far as possible
				int i2 = i;
				int j2 = j;
				while (j2 - e->line1 < e->inserted && fd[0].equivs[i2] == fd[1].equivs[j2])
				{
					++i2;
					++j2;
				}
				// extend moved block upward as far as possible
				int i1;
				int j1;
				do
				{
					i1 = i--;
					j1 = j--;
				} while (j >= e->line1 && fd[0].equivs[i] == fd[1].equivs[j]);
				// Ok, now our moved block is i1->i2, j1->j2

				ASSERT(i2 - i1 > 0);
				ASSERT(i2 - i1 == j2 - j1);

				if (int const prefix = j1 - e->line1)
				{
					// break e (current change) into two pieces
					// first part is the prefix, before the moved part
					// that stays in e
					// second part is the moved part & anything after it
					// that goes in newob
					// leave the left side (e->deleted) on e
					// so no right side on newob
					// newob will be the moved part only, later after we split off any suffix from it
					struct change *const newob = static_cast<struct change *>(zalloc(sizeof(struct change)));

					newob->line0 = e->line0 + e->deleted;
					newob->line1 = j1;
					newob->inserted = e->inserted - prefix;
					newob->deleted = 0;
					newob->link = e->link;
					newob->match0 = -1;
					newob->match1 = -1;

					e->inserted = prefix;
					e->link = newob;

					// now make e point to the moved part (& any suffix)
					e = newob;
				}
				// now e points to a moved diff chunk with no prefix, but maybe a suffix

				e->match0 = i1;

				if (int const suffix = e->inserted - (j2 - e->line1))
				{
					// break off any suffix from e
					// newob will be the suffix, and will get all the left side
					struct change *const newob = static_cast<struct change *>(zalloc(sizeof(struct change)));

					newob->line0 = e->line0;
					newob->line1 = j2;
					newob->inserted = suffix;
					newob->deleted = e->deleted;
					newob->link = e->link;
					newob->match0 = -1;
					newob->match1 = e->match1;

					e->inserted -= suffix;
					e->deleted = 0;
					e->match1 = -1;
					e->link = newob;
				}
				break;
			}
		}
	}
}
