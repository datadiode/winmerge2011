/**
 *  @file DiffItem.h
 *
 *  @brief Declaration of DIFFITEM
 */
// ID line follows -- this is updated by SVN
// $Id$

#ifndef _DIFF_ITEM_H_
#define _DIFF_ITEM_H_

#include "ListEntry.h"
#include "DiffFileInfo.h"

/**
 * @brief Status of one item comparison, stored as bitfields
 *
 * Bitmask can be seen as a 4 dimensional space; that is, there are four
 * different attributes, and each entry picks one of each attribute
 * independently.
 *
 * One dimension is how the compare went: same or different or
 * skipped or error.
 *
 * One dimension is file mode: text or binary (text is only if
 * both sides were text)
 *
 * One dimension is existence: both sides, left only, or right only
 *
 * One dimension is type: directory, or file
 */
struct DIFFCODE
{
	/**
	 * @brief values for DIFFITEM.diffcode
	 */
	enum
	{
		// We use extra bits so that no valid values are 0
		// and each set of flags is in a different hex digit
		// to make debugging easier
		// These can always be packed down in the future
		TEXTFLAGS=0x7, TEXT=0x4, BINSIDE1=0x1, BINSIDE2=0x2, BIN=0x3,
		TYPEFLAGS=0x30, FILE=0x10, DIR=0x20,
		SIDEFLAGS=0x300, LEFT=0x100, RIGHT=0x200, BOTH=0x300,
		COMPAREFLAGS=0x7000, NOCMP=0x0000, SAME=0x1000, DIFF=0x2000, CMPERR=0x3000, CMPABORT=0x4000,
		FILTERFLAGS=0x30000, INCLUDED=0x10000, SKIPPED=0x20000,
		SCANFLAGS=0x100000, NEEDSCAN=0x100000,
	};

	UINT diffcode;

	DIFFCODE(UINT diffcode = 0) : diffcode(diffcode) { }

	// file/directory
	bool isDirectory() const { return (diffcode & TYPEFLAGS) == DIR; }
	// left/right
	bool isSideLeftOnly() const { return (diffcode & SIDEFLAGS) == LEFT; }
	bool isSideLeftOrBoth() const { return (diffcode & LEFT) != 0; }
	bool isSideRightOnly() const { return (diffcode & SIDEFLAGS) == RIGHT; }
	bool isSideRightOrBoth() const { return (diffcode & RIGHT) != 0; }
	bool isSideBoth() const { return (diffcode & SIDEFLAGS) == BOTH; }
	// compare result
	bool isResultSame() const { return (diffcode & COMPAREFLAGS) == SAME; }
	bool isResultDiff() const { return (diffcode & COMPAREFLAGS) == DIFF; }
	bool isResultError() const { return (diffcode & COMPAREFLAGS) == CMPERR; }
	bool isResultAbort() const { return (diffcode & COMPAREFLAGS) == CMPABORT; }
	// filter status
	bool isResultFiltered() const { return (diffcode & FILTERFLAGS) == SKIPPED; }
	// type
	bool isText() const { return (diffcode & TEXT) != 0; }
	bool isBin() const { return (diffcode & BIN) != 0; }
	// rescan
	bool isScanNeeded() const { return (diffcode & SCANFLAGS) == NEEDSCAN; }
};

/**
 * @brief information about one file/folder item.
 * This class holds information about one compared item in the folder compare.
 * The item can be a file item or folder item. The item can have data from
 * both compare sides (file/folder exists in both sides) or just from one
 * side (file/folder exists only in other side).
 *
 * This class is for backend differences processing, presenting physical
 * files and folders. This class is not for GUI data like selection or
 * visibility statuses. So do not include any GUI-dependent data here. 
 */
struct DIFFITEM : ListEntry
{
	DIFFITEM *const parent; /**< Parent of current item */
	ListEntry children; /**< Head of doubly linked list for children */

	DiffFileInfo left; /**< Fileinfo for left file */
	DiffFileInfo right; /**< Fileinfo for right file */
	int	nsdiffs; /**< Amount of non-ignored differences */
	int nidiffs; /**< Amount of ignored differences */
	UINT customFlags1; /**< Custom flags set 1 */
	DIFFCODE diffcode; /**< Compare result */

	static DIFFITEM emptyitem; /**< singleton to represent a diffitem that doesn't have any data */

	DIFFITEM(DIFFITEM *parent)
		: parent(parent), nidiffs(-1), nsdiffs(-1), customFlags1(0) { }
	~DIFFITEM();

	String GetLeftFilepath(const String &sLeftRoot) const;
	String GetRightFilepath(const String &sRightRoot) const;
	int GetDepth() const;
	bool IsAncestor(const DIFFITEM *pdi) const;
	/** @brief Return whether the current item has children */
	bool HasChildren() const { return !children.IsSolitary(); }
};

#endif // _DIFF_ITEM_H_
