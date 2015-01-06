/** 
 * @file  DiffTextBuffer.h
 *
 * @brief Declaration of CDiffTextBuffer class
 */
#pragma once

#include "GhostTextBuffer.h"
#include "FileTextEncoding.h"

class PackingInfo;

/**
 * @brief File-operation return-statuses
 * Note that FileLoadResult class has no instance data or methods.
 * It is only a namespace for static methods & constants.
 * Everything is public.
 */
class FileLoadResult
{
public:
	/** @brief Return values for functions. */
	enum FILES_RESULT
	{
		/**
		 * Mask for the main return values.
		 * This mask defines bits used for main return values, separated from
		 * modifier flags.
		 */
		FRESULT_MAIN_MASK = 0xF,

		/**
		 * Error.
		 * This defines general error return value.
		 */
		FRESULT_ERROR = 0x0,
		/**
		 * Success.
		 * This defines general success return value.
		 */
		FRESULT_OK = 0x1,
		/**
		 * Success, but impure file.
		 * The file operation was successful. But the files was detected
		 * to be impure file. Impure file is a file with two or three
		 * different EOL styles.
		 */
		FRESULT_OK_IMPURE = 0x2,
		/**
		 * Binary file.
		 * The file was loaded OK, and was detected to be a binary file.
		 */
		FRESULT_BINARY = 0x3,

		/**
		 * Lossy conversions done.
		 * Unicode / codepage conversions caused lossy conversions.
		 */
		FRESULT_LOSSY = 0x10000,
	};
// Checking results
	/**
	 * Is result an error?
	 * @param [in] flr Return value to test.
	 * @return true if return value is an error value.
	 */
	static bool IsError(FILES_RESULT flr)
	{
		return Main(flr) == FRESULT_ERROR;
	}
	/**
	 * Is result a success?
	 * @param [in] flr Return value to test.
	 * @return true if return value is an success value.
	 */
	static bool IsOk(FILES_RESULT flr)
	{
		return Main(flr) == FRESULT_OK;
	}
	/**
	 * Is result OK but file is impure?
	 * @param [in] flr Return value to test.
	 * @return true if return value is success and file is impure.
	 */
	static bool IsOkImpure(FILES_RESULT flr)
	{
		return Main(flr) == FRESULT_OK_IMPURE;
	}
	/**
	 * Is result binary file?
	 * @param [in] flr Return value to test.
	 * @return true if return value determines binary file.
	 */
	static bool IsBinary(FILES_RESULT flr)
	{
		return Main(flr) == FRESULT_BINARY;
	}
	/**
	 * Is result unpack error?
	 * @param [in] flr Return value to test.
	 * @return true if return value determines unpacking error.
	 */
	static bool IsLossy(FILES_RESULT flr)
	{
		return (flr & FRESULT_LOSSY) != 0;
	}

// Assigning results
	// main results
	static void SetMainOk(FILES_RESULT & flr)
	{
		SetMain(flr, FRESULT_OK);
	}
	// modifiers
	static void AddModifier(FILES_RESULT & flr, FILES_RESULT modifier)
	{
		flr = static_cast<FILES_RESULT>(flr | modifier);
	}

	// bit manipulations
	static void SetMain(FILES_RESULT & flr, FILES_RESULT newmain)
	{
		flr = static_cast<FILES_RESULT>(flr & ~FRESULT_MAIN_MASK | newmain);
	}
	static FILES_RESULT Main(FILES_RESULT flr)
	{
		return static_cast<FILES_RESULT>(flr & FRESULT_MAIN_MASK);
	}
};

/**
 * @brief Specialized buffer to save file data
 */
class CDiffTextBuffer : public CGhostTextBuffer
{
	friend class CChildFrame;
	friend class DiffList;

private:
	using CGhostTextBuffer::InitNew;
	using CGhostTextBuffer::FreeAll;
	CChildFrame *const m_pOwnerDoc; /**< Merge document owning this buffer. */
	int m_nThisPane; /**< Left/Right side */
	int m_unpackerSubcode; /**< Plugin information. */
	bool m_bMixedEOL; /**< EOL style of this buffer is mixed? */

	/** 
	 * @brief Unicode encoding from UNICODESET.
	 *
	 * @note m_unicoding and m_codepage are indications of how the buffer is
	 * supposed to be saved on disk. In memory, it is invariant, depending on
	 * build:
	 * - ANSI: in memory it is CP_ACP/CP_THREAD_ACP 8-bit characters
	 * - Unicode: in memory it is wchars
	 */
	FileTextEncoding m_encoding;

	bool FlagIsSet(UINT line, DWORD flag) const;

public:
	CDiffTextBuffer(CChildFrame *pDoc, int pane);
	~CDiffTextBuffer();

	virtual UndoRecord &AddUndoRecord(BOOL bInsert, const POINT &ptStartPos,
		const POINT &ptEndPos, LPCTSTR pszText, int cchText,
		int nLinesToValidate, int nActionType = CE_ACTION_UNKNOWN);

	FileLoadResult::FILES_RESULT LoadFromFile(LPCTSTR pszFileName,
		PackingInfo *infoUnpacker, bool &readOnly, CRLFSTYLE nCrlfStyle,
		FileTextEncoding &encoding, String &sError);
	int SaveToFile(LPCTSTR pszFileName, ISequentialStream *pTempStream, String &sError,
		PackingInfo *infoUnpacker = NULL, CRLFSTYLE nCrlfStyle = CRLF_STYLE_AUTOMATIC,
		int nStartLine = 0, int nLines = -1);
	UNICODESET getUnicoding() const { return m_encoding.m_unicoding; }
	void setUnicoding(UNICODESET value) { m_encoding.m_unicoding = value; }
	int getCodepage() const { return m_encoding.m_codepage; }
	void setCodepage(int value) { m_encoding.m_codepage = value; }
	const FileTextEncoding & getEncoding() const { return m_encoding; }
	bool IsMixedEOL() const { return m_bMixedEOL; }
	void SetMixedEOL(bool bMixed) { m_bMixedEOL = bMixed; }

	void prepareForRescan();
	virtual void OnNotifyLineHasBeenEdited(int nLine);
	bool IsSaveable() const;
};
