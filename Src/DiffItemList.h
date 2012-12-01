/**
 *  @file DiffItemList.h
 *
 *  @brief Declaration of DiffItemList
 */
// ID line follows -- this is updated by SVN
// $Id$

#ifndef _DIFF_ITEM_LIST_H_
#define _DIFF_ITEM_LIST_H_

#include "DiffItem.h"

/**
 * @brief List of DIFFITEMs in folder compare.
 * This class holds a list of items we have in the folder compare. Basically
 * we have a linked list of DIFFITEMs. But there is a structure that follows
 * the actual folder structure. Each DIFFITEM can have a parent folder and
 * another list of child items. Parent DIFFITEM is always a folder item.
 */
class DiffItemList
{
public:
	DiffItemList();
	~DiffItemList();
	// add & remove differences
	DIFFITEM *AddDiff(DIFFITEM *parent);
	void RemoveDiff(DIFFITEM *);
	void RemoveAll();

	// to iterate over all differences on list
	DIFFITEM *GetFirstChildDiff(const DIFFITEM *) const;
	DIFFITEM *GetNextDiff(const DIFFITEM *) const;
	DIFFITEM *GetNextSiblingDiff(const DIFFITEM *) const;

protected:
	ListEntry m_root; /**< Root of list of diffitems */
};

#endif // _DIFF_ITEM_LIST_H_
