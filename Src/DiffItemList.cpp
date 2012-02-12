/**
 *  @file DiffItemList.cpp
 *
 *  @brief Implementation of DiffItemList
 */ 
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "DiffItemList.h"

/**
 * @brief Constructor
 */
DiffItemList::DiffItemList()
{
}

/**
 * @brief Destructor
 */
DiffItemList::~DiffItemList()
{
	RemoveAll();
}

/**
 * @brief Add new diffitem to structured DIFFITEM tree.
 * @param [in] parent Parent item, or NULL if no parent.
 * @return Pointer to the added item.
 */
DIFFITEM *DiffItemList::AddDiff(DIFFITEM *parent)
{
	DIFFITEM *p = new DIFFITEM(parent);
	if (parent)
		parent->children.Append(p);
	else
		m_root.Append(p);
	return p;
}

/**
 * @brief Remove diffitem from structured DIFFITEM tree
 * @param diffpos position of item to remove
 */
void DiffItemList::RemoveDiff(UINT_PTR diffpos)
{
	DIFFITEM *p = (DIFFITEM *)diffpos;
	p->RemoveSelf();
	delete p;
}

/**
 * @brief Empty structured DIFFITEM tree
 */
void DiffItemList::RemoveAll()
{
	while (m_root.IsSibling(m_root.Flink))
		RemoveDiff((UINT_PTR)m_root.Flink);
}

/**
 * @brief Get position of first item in structured DIFFITEM tree
 */
UINT_PTR DiffItemList::GetFirstDiffPosition() const
{
	return (UINT_PTR)m_root.IsSibling(m_root.Flink);
}

/**
 * @brief Get position of first child item in structured DIFFITEM tree
 * @param  parentdiffpos [in] Position of parent diff item 
 * @return Position of first child item
 */
UINT_PTR DiffItemList::GetFirstChildDiffPosition(UINT_PTR parentdiffpos) const
{
	DIFFITEM *parent = (DIFFITEM *)parentdiffpos;
	if (parent)
		return (UINT_PTR)parent->children.IsSibling(parent->children.Flink);
	else
		return (UINT_PTR)m_root.IsSibling(m_root.Flink);
}

/**
 * @brief Get position of next item in structured DIFFITEM tree
 * @param diffpos position of current item, updated to next item position
 * @return Diff Item in current position
 */
DIFFITEM &DiffItemList::GetNextDiffPosition(UINT_PTR & diffpos) const
{
	DIFFITEM *p = (DIFFITEM *)diffpos;
	if (p->HasChildren())
	{
		diffpos = GetFirstChildDiffPosition(diffpos);
	}
	else
	{
		DIFFITEM *cur = p;
		do
		{
			if (cur->parent)
				diffpos = (UINT_PTR)cur->parent->children.IsSibling(cur->Flink);
			else
				diffpos = (UINT_PTR)m_root.IsSibling(cur->Flink);
			cur = cur->parent;
		} while (!diffpos && cur);
	}
	return *p;
}

/**
 * @brief Get position of next sibling item in structured DIFFITEM tree
 * @param diffpos position of current item, updated to next sibling item position
 * @return Diff Item in current position
 */
DIFFITEM &DiffItemList::GetNextSiblingDiffPosition(UINT_PTR & diffpos) const
{
	DIFFITEM *p = (DIFFITEM *)diffpos;
	if (p->parent)
		diffpos = (UINT_PTR)p->parent->children.IsSibling(p->Flink);
	else
		diffpos = (UINT_PTR)m_root.IsSibling(p->Flink);
	return *p;
}
