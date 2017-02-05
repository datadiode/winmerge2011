/** 
 * @file  stringdiffsi.h
 *
 * @brief Declaration file for class stringdiffs
 *
 */

/**
 * @brief Class to hold together data needed to implement sd_ComputeWordDiffs
 */
class stringdiffs
{
public:
	stringdiffs(String const &str1, String const &str2,
		bool case_sensitive, int whitespace, int breakType,
		bool byte_level, std::vector<wdiff> &);

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
		int length() const { return end + 1 - start; }
	};

// Implementation methods
private:
	void ResizeWordArraysToSameLength();
	void BuildWordsArray(String const &str, std::vector<word> &words);
	static void InsertInWords(std::vector<word> &words, int bw);
	int FindPreMatchInWords(std::vector<word> const &words, word const &needword, int bw, int side) const;
	int FindNextMatchInWords(std::vector<word> const &words, word const &needword, int bw, int side) const;
	static int FindNextSpaceInWords(std::vector<word> const &words, int bw);
	static int FindPreNoInsertInWords(std::vector<word> const &words, int bw);
	static int FindNextInsertInWords(std::vector<word> const &words, int bw);
	static int FindNextNoInsertInWords(std::vector<word> const &words, int bw);
	static void MoveInWordsUp(std::vector<word> &words, int source, int target);
	static void MoveInWordsDown(std::vector<word> &words, int source, int target);
	UINT Hash(String const &str, int begin, int end, UINT h) const;
	bool AreWordsSame(word const &word1, word const &word2) const;
	static bool IsWord(word const &);
	static bool IsSpace(word const &);
	static bool IsBreak(word const &);
	static bool IsInsert(word const &);
	bool caseMatch(TCHAR ch1, TCHAR ch2) const;
	void wordLevelToByteLevel() const;
	void ComputeByteDiff(wdiff const &,
		int &begin1, int &begin2, int &end1, int &end2, bool equal) const;
	bool findsyn(wdiff const &,
		int &begin1, int &begin2, int &end1, int &end2, bool equal,
		sd_findsyn_func func, int &s1, int &e1, int &s2, int &e2) const;

// Implementation data
private:
	const String &m_str1;
	const String &m_str2;
	bool m_case_sensitive;
	int m_whitespace;
	int m_breakType;
	bool m_matchblock;
	std::vector<wdiff> &m_diffs;
	std::vector<word> m_words1;
	std::vector<word> m_words2;
};
