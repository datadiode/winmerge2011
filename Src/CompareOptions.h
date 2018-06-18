/** 
 * @file  CompareOptions.h
 *
 * @brief Compare options classes and types.
 */
#pragma once

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
private:
	struct null;
public:
	int nIgnoreWhitespace; /**< Ignore whitespace -option. */
	int nTabSize; /**< Distance between adjacent tab stops. */
	bool bIgnoreCase; /**< Ignore case -option. */
	bool bIgnoreBlankLines; /**< Ignore blank lines -option. */
	bool bIgnoreEol; /**< Ignore EOL differences -option. */
	bool bMinimal; /**< Minimize the number of affected lines. */
	bool bSpeedLargeFiles; /**< Speed up the processing of large files. */
	bool bApplyHistoricCostLimit; /**< Control costs as strictly as Diffutils 2.5 did. */
	bool bFilterCommentsLines; /**< Ignore Multiline comments differences -option. */
	bool bApplyLineFilters; /**< Apply line filters? */
	DIFFOPTIONS(null *)
	{
		memset(this, 0, sizeof *this);
		nTabSize = 8;
	}
};
