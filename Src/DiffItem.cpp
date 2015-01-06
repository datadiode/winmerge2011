/**
 *  @file DiffItem.cpp
 *
 *  @brief Implementation of DIFFITEM
 */ 
#include "StdAfx.h"
#include "DiffItem.h"
#include "paths.h"

/** @brief DIFFITEM's destructor */
DIFFITEM::~DIFFITEM()
{
	while (ListEntry *p = children.IsSibling(children.Flink))
	{
		p->RemoveSelf();
		delete static_cast<DIFFITEM *>(p);
	}
}

/** @brief Return path to left file, including all but file name */
String DIFFITEM::GetLeftFilepath(const String &sLeftRoot) const
{
	String sPath;
	if (!isSideRightOnly())
	{
		sPath = paths_ConcatPath(sLeftRoot, left.path);
	}
	return sPath;
}

/** @brief Return path to right file, including all but file name */
String DIFFITEM::GetRightFilepath(const String &sRightRoot) const
{
	String sPath;
	if (!isSideLeftOnly())
	{
		sPath = paths_ConcatPath(sRightRoot, right.path);
	}
	return sPath;
}

/** @brief Return depth of path */
int DIFFITEM::GetDepth() const
{
	int depth = 0;
	const DIFFITEM *cur = this;
	while ((cur = cur->parent) != NULL)
	{
		++depth;
	}
	return depth;
}

/**
 * @brief Return whether the specified item is an ancestor of the current item
 */
bool DIFFITEM::IsAncestor(const DIFFITEM *pdi) const
{
	const DIFFITEM *cur = this;
	while (const DIFFITEM *parent = cur->parent)
	{
		if (parent == pdi)
			return true;
		cur = parent;
	}
	return false;
}
