/////////////////////////////////////////////////////////////////////////////
//    License (GPLv2+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
/////////////////////////////////////////////////////////////////////////////
/** 
 * @file  DiffList.h
 *
 * @brief Declaration file for DiffList class
 */
#ifndef _DIFFLIST_H_
#define _DIFFLIST_H_

class CDiffTextBuffer;

/**
 * @brief Operations in diffranges.
 * DIFFRANGE structs op-member can have these values
 */
enum OP_TYPE
{
	OP_NONE,
	OP_LEFTONLY,
	OP_DIFF,
	OP_RIGHTONLY,
	OP_TRIVIAL
};

/**
 * @brief One difference defined by linenumbers.
 *
 * This struct defines one set of different lines "diff".
 * @p begin0, @p end0, @p begin1 & @p end1 are linenumbers
 * in original files. Other struct members point to linenumbers
 * calculated by WinMerge after adding empty lines to make diffs
 * be in line in screen.
 *
 * @note @p blank0 & @p blank1 are -1 if there are no blank lines
 */
struct DIFFRANGE
{
	UINT begin0;	/**< First diff line in original file1 */
	UINT end0;		/**< Last diff line in original file1 */
	UINT begin1;	/**< First diff line in original file2 */
	UINT end1;		/**< Last diff line in original file2 */
	UINT dbegin0;	/**< Synchronised (ghost lines added) first diff line in file1 */
	UINT dend0;		/**< Synchronised (ghost lines added) last diff line in file1 */
	UINT dbegin1;	/**< Synchronised (ghost lines added) first diff line in file2 */
	UINT dend1;		/**< Synchronised (ghost lines added) last diff line in file2 */
	UINT blank0;	/**< Number of blank lines in file1 */
	UINT blank1;	/**< Number of blank lines in file2 */
	OP_TYPE op;		/**< Operation done with this diff */
	DIFFRANGE() { memset(this, 0, sizeof *this); }
	void swap_sides();
};

/**
 * @brief DIFFRANGE with links for chain of non-trivial entries
 *
 * Next and prev are array indices used by the owner (DiffList)
 */
struct DiffRangeInfo
{
	DIFFRANGE diffrange;
	int next; /**< link (array index) for doubly-linked chain of non-trivial DIFFRANGEs */
	int prev; /**< link (array index) for doubly-linked chain of non-trivial DIFFRANGEs */
	DiffRangeInfo(const DIFFRANGE & di) : diffrange(di), next(-1), prev(-1) { }
};

/**
 * @brief Class for storing differences in files (difflist).
 *
 * This class stores diffs in list and also offers diff-related
 * functions to e.g. check if linenumber is inside diff.
 *
 * There are two kinds of diffs:
 * - significant diffs are 'normal' diffs we want to merge and browse
 * - non-significant diffs are diffs ignored by linefilters
 * 
 * The code assumes diff lists don't grow bigger than 32-bit int type's
 * range. And what a trouble we'd have if we have so many diffs...
 */
class DiffList
{
public:
	DiffList();
	void Clear();
	int GetSize() const;
	int GetSignificantDiffs() const;
	void AddDiff(const DIFFRANGE & di);
	bool IsDiffSignificant(int nDiff) const;
	int GetSignificantIndex(int nDiff) const;
	bool LineInDiff(UINT nLine, int nDiff) const;
	int LineToDiff(UINT nLine) const;
	int PrevSignificantDiffFromLine(UINT nLine) const;
	int NextSignificantDiffFromLine(UINT nLine) const;
	int FirstSignificantDiff() const;
	int NextSignificantDiff(int nDiff) const;
	int PrevSignificantDiff(int nDiff) const;
	int LastSignificantDiff() const;

	DIFFRANGE *DiffRangeAt(int nDiff);

	void ConstructSignificantChain(); // must be called after diff list is entirely populated
	void SwapSides();
	void AddExtraLinesCounts(UINT &nLeftLines, UINT &nRightLines,
		CDiffTextBuffer **ptBuf = NULL, UINT nContextLines = UINT_MAX);
	int FinishSyncPoint(int nDiff, int nRealStartLine[]);
	void swap(DiffList &);

private:
	stl::vector<DiffRangeInfo> m_diffs; /**< Difference list. */
	int m_firstSignificant; /**< Index of first significant diff in m_diffs */
	int m_lastSignificant; /**< Index of last significant diff in m_diffs */
};

#endif // _DIFFLIST_H_
