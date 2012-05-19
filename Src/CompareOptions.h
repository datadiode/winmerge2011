/** 
 * @file  CompareOptions.h
 *
 * @brief Compare options classes and types.
 */
// ID line follows -- this is updated by SVN
// $Id$

#ifndef CompareOptions_h_included
#define CompareOptions_h_included

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
	bool bIgnoreCase; /**< Ignore case -option. */
	bool bIgnoreBlankLines; /**< Ignore blank lines -option. */
	bool bIgnoreEol; /**< Ignore EOL differences -option. */
	bool bFilterCommentsLines; /**< Ignore Multiline comments differences -option. */
	bool bApplyLineFilters; /**< Apply line filters? */
	DIFFOPTIONS(null *)
	{
		memset(this, 0, sizeof *this);
	}
};

#endif // CompareOptions_h_included
