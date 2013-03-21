#pragma once

/**
 * @brief Container for one filtering rule / compiled expression.
 * This structure holds compiled regular expression and a original expression
 * as a string. We need the original expression string in case we want to
 * know which regular expression did match.
 */
struct regexp_item
{
	HString *filterString; /** Original regular expression string */
	HString *injectString; /** String to inject in place of submatches */
	HString *filenameSpec; /** Optional target filename patterns */
	pcre *pRegExp; /**< Compiled regular expression */
	pcre_extra *pRegExpExtra; /**< Additional information got from regex study */
	int options;
	bool global;
	bool permutive;

	regexp_item()
		: filterString(NULL)
		, injectString(NULL)
		, filenameSpec(NULL)
		, pRegExp(NULL)
		, pRegExpExtra(NULL)
		, options(0)
		, global(false)
		, permutive(false)
	{
	}
	const char *assign(LPCTSTR pch, int cch);
	int execute(const char *subject, int length, int start,
		int options, int *offsets, int offsetcount) const
	{
		return pcre_exec(pRegExp, pRegExpExtra,
			subject, length, start, options, offsets, offsetcount);
	}
	void dispose()
	{
		filterString->Free();
		injectString->Free();
		filenameSpec->Free();
		pcre_free(pRegExp);
		pcre_free(pRegExpExtra);
	}
	static int process(const stl::vector<regexp_item> &,
		char *dst, const char *src, int len, LPCTSTR filename = NULL);
	static bool indifferent(const stl::vector<regexp_item> &, LPCTSTR, LPCTSTR);
};
