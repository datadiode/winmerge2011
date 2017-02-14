/** 
 * @file  stringdiffsi.h
 *
 * @brief Declaration file for class stringdiffs
 *
 */

// Uncomment this to see stringdiff log messages
// We don't use _DEBUG since stringdiff logging is verbose and slows down WinMerge
// #define STRINGDIFF_LOGGING

using std::vector;
using std::max;

/**
 * @brief Class to hold together data needed to implement sd_ComputeWordDiffs
 */
class stringdiffs
{
public:
	stringdiffs(LPCTSTR str1, int len1, LPCTSTR str2, int len2,
		bool case_sensitive, int whitespace, int breakType,
		bool byte_level, vector<wdiff> &);

	~stringdiffs();

// Implementation types
private:
	/**
	 * @brief kind of diff blocks.
	 */
	enum sd_kind
	{
		dlword,
		dlspace,
		dlbreak, 
		dlinsert,
	};
	/**
	 * @brief kind of synchronaction
	 */
	enum sd_findsyn_func
	{
		synbegin1,
		synbegin2,
		synend1, 
		synend2 
	};

	struct word
	{
		int start; // index of first character of word in original string
		int end;   // index of last character of word in original string
		int hash;
		sd_kind kind; // Is it a isWordBreak 0 = word -1= whitespace -2 = empty 1 = breakWord (Hä?)
		word(int s = 0, int e = 0, sd_kind k = dlword, int h = 0) : start(s), end(e), kind(k), hash(h) { }
	};

	struct line
	{
		LPCTSTR const str;
		int const len;
		vector<word> words;
		line(LPCTSTR str, int len) : str(str), len(len) { }
	};

#ifdef STRINGDIFF_LOGGING
	void debugoutput();
#endif

// Implementation methods
	void BuildWordsArray(LPCTSTR str, int len, vector<word> &words);
	void BuildWordDiffList();
	void CombineAdjacentDiffs();
	UINT Hash(LPCTSTR str, int begin, int end, UINT h) const;
	bool AreWordsSame(line const &, int, line const &, int) const;
	static bool IsWord(word const &);
	static bool IsSpace(word const &);
	static bool IsBreak(word const &);
	static bool IsInsert(word const &);
	bool caseMatch(TCHAR, TCHAR) const;
	void onp(vector<char> &edscript);
	int snake(int k, int y, line const &, line const &);
	void wordLevelToByteLevel() const;
	void ComputeByteDiff(wdiff const &,
		int &begin1, int &begin2, int &end1, int &end2, bool equal) const;
	bool findsyn(wdiff const &,
		int &begin1, int &begin2, int &end1, int &end2, bool equal,
		sd_findsyn_func func, int &s1, int &e1, int &s2, int &e2) const;

// Implementation data
	line m_line1, m_line2;
	bool const m_case_sensitive;
	int const m_whitespace;
	int const m_breakType;
	vector<wdiff> &m_diffs;
};
