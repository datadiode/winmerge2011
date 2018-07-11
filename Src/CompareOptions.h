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
	/* Radio options */
	WHITESPACE_COMPARE_ALL,        /**< no special handling of whitespace */
	WHITESPACE_IGNORE_CHANGE,      /**< ignore changes in whitespace */
	WHITESPACE_IGNORE_ALL,         /**< ignore whitespace altogether */
	/* Check options */
	WHITESPACE_IGNORE_TAB_EXPANSION = 0x04,
	WHITESPACE_IGNORE_TRAILING_SPACE = 0x08,
	/* Masks to separate between radio & check options */
	WHITESPACE_CHECK_OPTIONS_MASK = 0x10 - WHITESPACE_IGNORE_TAB_EXPANSION,
	WHITESPACE_RADIO_OPTIONS_MASK = WHITESPACE_IGNORE_TAB_EXPANSION - 0x01,
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
