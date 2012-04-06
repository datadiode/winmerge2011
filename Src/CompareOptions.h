/** 
 * @file  CompareOptions.h
 *
 * @brief Compare options classes and types.
 */
// ID line follows -- this is updated by SVN
// $Id$

#ifndef CompareOptions_h_included
#define CompareOptions_h_included

#include "FilterList.h"
#include "LineFiltersList.h"

/**
 * @brief Whether to ignore whitespace (or to ignore changes in whitespace)
 *
 * Examples:
 * "abc def" is only equivalent to "abcdef" under WHITESPACE_IGNORE_ALL
 *
 * but "abc def" is equivalent to "abc    def" under both 
 *   WHITESPACE_IGNORE_CHANGE and WHITESPACE_IGNORE_ALL
 *
 * Also, trailing and leading whitespace is ignored for both
 *   WHITESPACE_IGNORE_CHANGE and WHITESPACE_IGNORE_ALL
 */
enum
{
	WHITESPACE_COMPARE_ALL,        /**< no special handling of whitespace */
	WHITESPACE_IGNORE_CHANGE,      /**< ignore changes in whitespace */
	WHITESPACE_IGNORE_ALL,         /**< ignore whitespace altogether */
};

/**
 * @brief Diffutils options.
 */
struct DIFFOPTIONS
{
	int nIgnoreWhitespace; /**< Ignore whitespace -option. */
	bool bIgnoreCase; /**< Ignore case -option. */
	bool bIgnoreBlankLines; /**< Ignore blank lines -option. */
	bool bIgnoreEol; /**< Ignore EOL differences -option. */
	bool bFilterCommentsLines; /**< Ignore Multiline comments differences -option. */
	bool bApplyLineFilters; /**< Apply line filters? */
};

/**
 * @brief General compare options.
 * This class has general compare options we expect every compare engine and
 * routine to implement.
 */
class CompareOptions
{
protected:
	CompareOptions();
	void SetFromDiffOptions(const DIFFOPTIONS & options);
public:
	int m_ignoreWhitespace; /**< Ignore whitespace characters */
	bool m_bIgnoreBlankLines; /**< Ignore blank lines (both sides) */
	bool m_bIgnoreCase; /**< Ignore case differences? */
	bool m_bIgnoreEOLDifference; /**< Ignore EOL style differences? */
	bool m_bApplyLineFilters; /**< Apply line filters? */
};

/**
 * @brief Compare options used with diffutils.
 * This class adds some diffutils-specific compare options to general compare
 * options class. And also methods for easy setting options and forwarding
 * options to diffutils.
 */
class DiffutilsOptions
	: public CompareOptions
	, public FilterList /**< Filter list for line filters. */
{
protected:
	DiffutilsOptions();
	~DiffutilsOptions();
public:
	void GetAsDiffOptions(DIFFOPTIONS &options);
	void SetFromDiffOptions(const DIFFOPTIONS & options);

	bool m_filterCommentsLines;/**< Ignore Multiline comments differences.*/
};

/**
 * @brief Compare options used with Quick compare -method.
 * This class has some Quick Compare specifics in addition to general compare
 * options.
 */
class QuickCompareOptions : public CompareOptions
{
protected:
	QuickCompareOptions();
public:
	bool m_bStopAfterFirstDiff; /**< Optimize compare by stopping after first difference? */
};

#endif // CompareOptions_h_included
