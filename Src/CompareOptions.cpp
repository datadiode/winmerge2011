/** 
 * @file  CompareOptions.cpp
 *
 * @brief Compare options implementation.
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "CompareOptions.h"

/**
 * @brief Default constructor.
 */
CompareOptions::CompareOptions()
: m_ignoreWhitespace(WHITESPACE_COMPARE_ALL)
, m_bIgnoreBlankLines(false)
, m_bIgnoreCase(false)
, m_bIgnoreEOLDifference(false)
, m_bApplyLineFilters(false)
{
}

/**
 * @brief Sets options from DIFFOPTIONS structure.
 * @param [in] options Diffutils options.
 */
void CompareOptions::SetFromDiffOptions(const DIFFOPTIONS &options)
{
	m_ignoreWhitespace = options.nIgnoreWhitespace;
	m_bIgnoreBlankLines = options.bIgnoreBlankLines;
	m_bIgnoreCase = options.bIgnoreCase;
	m_bIgnoreEOLDifference = options.bIgnoreEol;
	m_bApplyLineFilters = options.bApplyLineFilters;
}

/**
 * @brief Default constructor.
 */
QuickCompareOptions::QuickCompareOptions()
: m_bStopAfterFirstDiff(false)
{
}

/**
 * @brief Default constructor.
 */
DiffutilsOptions::DiffutilsOptions()
: m_filterCommentsLines(false)
{
}

/**
 * @brief Destructor.
 */
DiffutilsOptions::~DiffutilsOptions()
{
}

/**
 * @brief Sets options from DIFFOPTIONS structure.
 * @param [in] options Diffutils options.
 */
void DiffutilsOptions::SetFromDiffOptions(const DIFFOPTIONS & options)
{
	CompareOptions::SetFromDiffOptions(options);
	m_filterCommentsLines = options.bFilterCommentsLines;
	FilterList::RemoveAllFilters();
	if (m_bApplyLineFilters)
	{
		FilterList::AddFrom(globalLineFilters);
	}
}

/**
 * @brief Gets options to DIFFOPTIONS structure.
 * @param [out] options Diffutils options.
 */
void DiffutilsOptions::GetAsDiffOptions(DIFFOPTIONS &options)
{
	options.bFilterCommentsLines = m_filterCommentsLines;
	options.bIgnoreBlankLines = m_bIgnoreBlankLines;
	options.bIgnoreCase = m_bIgnoreCase;
	options.bIgnoreEol = m_bIgnoreEOLDifference;
	options.bApplyLineFilters = m_bApplyLineFilters;
	options.nIgnoreWhitespace = m_ignoreWhitespace;
}
