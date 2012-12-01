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
 * @param Pointer to item to remove
 */
void DiffItemList::RemoveDiff(DIFFITEM *p)
{
	p->RemoveSelf();
	delete p;
}

/**
 * @brief Empty structured DIFFITEM tree
 */
void DiffItemList::RemoveAll()
{
	while (m_root.IsSibling(m_root.Flink))
		RemoveDiff(static_cast<DIFFITEM *>(m_root.Flink));
}

/**
 * @brief Get pointer to first child item in structured DIFFITEM tree
 * @param  parent [in] Pointer to parent item 
 * @return Pointer to first child item
 */
DIFFITEM *DiffItemList::GetFirstChildDiff(const DIFFITEM *parent) const
{
	if (parent)
		return static_cast<DIFFITEM *>(parent->children.IsSibling(parent->children.Flink));
	else
		return static_cast<DIFFITEM *>(m_root.IsSibling(m_root.Flink));
}

/**
 * @brief Get pointer to next item in structured DIFFITEM tree
 * @param Pointer to current item
 * @return Pointer to next item
 */
DIFFITEM *DiffItemList::GetNextDiff(const DIFFITEM *p) const
{
	DIFFITEM *q = GetFirstChildDiff(p);
	while (q == NULL && p != NULL)
	{
		q = GetNextSiblingDiff(p);
		p = p->parent;
	}
	return q;
}

/**
 * @brief Get pointer to next sibling item in structured DIFFITEM tree
 * @param Pointer to current item
 * @return Pointer to next sibling item
 */
DIFFITEM *DiffItemList::GetNextSiblingDiff(const DIFFITEM *p) const
{
	const ListEntry &root = p->parent ? p->parent->children : m_root;
	return static_cast<DIFFITEM *>(root.IsSibling(p->Flink));
}
