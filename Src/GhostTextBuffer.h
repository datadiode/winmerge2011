/**
 * @file  GhostTextBuffer.h
 *
 * @brief Declaration of CGhostTextBuffer (subclasses CCrystalTextBuffer to handle ghost lines)
 */
// ID line follows -- this is updated by SVN
// $Id$

#ifndef __GHOSTTEXTBUFFER_H__
#define __GHOSTTEXTBUFFER_H__

#include "ccrystaltextbuffer.h"
#include "GhostUndoRecord.h"

/////////////////////////////////////////////////////////////////////////////

/**
 * The Crystal Editor keeps a DWORD of flags for each line.
 * It does not use all of the available bits.
 * WinMerge uses some of the high bits to keep WinMerge-specific
 * information; here are the list of WinMerge flags.
 * So, these constants are used with the SetLineFlags(2) calls.
 *
 * LF_GHOST must be cleared and set in GhostTextBuffer.cpp 
 * and MergeDoc.cpp (Rescan) only.
 *
 * GetLineColors (in MergeEditView) reads it to choose the line color.
 */
enum GHOST_LINEFLAGS
{
	LF_GHOST = 0x00400000L, /**< Ghost line. */
	LF_DIFF = 0x00200000L,
	LF_TRIVIAL = 0x00800000L,
	LF_MOVED = 0x01000000L,
	LF_SKIPPED = 0x02000000L, /**< Skipped line. */
};

// WINMERGE_FLAGS is MERGE_LINEFLAGS | GHOST_LINEFLAGS | LF_TRIVIAL | LF_MOVED
#define LF_WINMERGE_FLAGS    0x03E00000

C_ASSERT(LF_WINMERGE_FLAGS == (LF_TRIVIAL | LF_MOVED | LF_DIFF | LF_SKIPPED | LF_GHOST));

// Flags for non-ignored difference
// Note that we must include ghost flag to include ghost lines
#define LF_NONTRIVIAL_DIFF ((LF_DIFF | LF_GHOST) & (~LF_TRIVIAL))

/////////////////////////////////////////////////////////////////////////////
// CCrystalTextBuffer command target

/**
 * @brief A class handling ghost lines.
 * Features offered with this class : 
 * <ul>
 *  <li> apparent/real line conversion 
 *  <li> insertText/deleteText working with ghost lines 
 *  <li> AddUndoRecord/Undo/Redo working with ghost lines 
 *  <li> insertGhostLine function 
 * </ul>
 */
class EDITPADC_CLASS CGhostTextBuffer : public CCrystalTextBuffer
{
protected:
	//  Nested class declarations
	enum
	{
		UNDO_INSERT = 0x0001,
		UNDO_BEGINGROUP = 0x0100
	};

protected:
	/** 
	We need another array with our richer structure.

	We share the positions with the CCrystalTextBuffer object. 
	We share m_bUndoGroup, its utility is to check we opened the UndoBeginGroup.
	We share m_nUndoBufSize which is the max buffer size.
	*/
	stl::vector<GhostUndoRecord> m_aUndoBuf;
	virtual const UndoRecord &GetUndoRecord(stl_size_t i) const
	{
		return m_aUndoBuf[i];
	}
	virtual stl_size_t GetUndoRecordCount() const
	{
		return m_aUndoBuf.size();
	}

	// [JRT] Support For Descriptions On Undo/Redo Actions
	virtual UndoRecord &AddUndoRecord(BOOL bInsert, const POINT &ptStartPos, const POINT &ptEndPos,
                              LPCTSTR pszText, int cchText, int nRealLinesChanged, int nActionType = CE_ACTION_UNKNOWN);

private:
	/**
	 * @brief A struct mapping real lines and apparent (screen) lines.
	 * This struct maps lines between real lines and apparent (screen) lines.
	 * The mapping records for each text block an apparent line and matching
	 * real line.
	 */
	struct RealityBlock
	{
		int nStartReal; /**< Start line of real block. */
		int nStartApparent; /**< Start line of apparent block. */
		int nCount; /**< Lines in the block. */
	};
	stl::vector<RealityBlock> m_RealityBlocks; /**< Mapping of real and apparent lines. */

	// Operations
private:
	BOOL InternalInsertGhostLine(CCrystalTextView *pSource, int nLine);
	BOOL InternalDeleteGhostLine(CCrystalTextView *pSource, int nLine, int nCount);
public:
	// Construction/destruction code
	CGhostTextBuffer();
	BOOL InitNew(CRLFSTYLE nCrlfStyle = CRLF_STYLE_DOS);
	void FreeAll();

	// Text modification functions
	virtual POINT InsertText(CCrystalTextView *pSource, int nLine, int nPos,
		LPCTSTR pszText, int cchText,
		int nAction = CE_ACTION_UNKNOWN, BOOL bHistory = TRUE);
	virtual void DeleteText(CCrystalTextView *pSource, int nStartLine,
		int nStartPos, int nEndLine, int nEndPos,
		int nAction = CE_ACTION_UNKNOWN, BOOL bHistory = TRUE);
	BOOL InsertGhostLine(CCrystalTextView *pSource, int nLine);

	// Undo/Redo
	virtual bool Undo(CCrystalTextView *pSource, POINT &ptCursorPos);
	virtual bool Redo(CCrystalTextView *pSource, POINT &ptCursorPos);

public:
	//@{
	/**
	 * @name Real/apparent line number conversion functions.
	 * These functions convert line numbers between file line numbers
	 * (real line numbers) and screen line numbers (apparent line numbers).
	 *
	 * This mapping is needed to handle ghost lines (ones with no text or
	 * EOL chars) which WinMerge uses for left-only or right-only lines.
	*/
	int ApparentLastRealLine() const;
	int ComputeRealLine(int nApparentLine) const;
	int ComputeApparentLine(int nRealLine) const;
	/** richer position information   yApparent = apparent(yReal) - yGhost */
	int ComputeRealLineAndGhostAdjustment(int nApparentLine, int& decToReal) const;
	/** richer position information   yApparent = apparent(yReal) - yGhost */
	int ComputeApparentLine(int nRealLine, int decToReal) const;
	//@}

	/** for loading file */
	void FinishLoading();
	/** for saving file */ 
	void RemoveAllGhostLines(DWORD dwMask = LF_GHOST);


private:
	void RecomputeRealityMapping();
	/** 
	Code to set EOL, if the status ghost/real of the line changes 

	We should call a CCrystalTextBuffer function to add the correct EOL
	(if CCrystalTextBuffer keeps the default EOL for the file)
	*/
	void RecomputeEOL(CCrystalTextView *pSource, int nStartLine, int nEndLine);

#ifdef _DEBUG
	/** For debugging purpose */
	void checkFlagsFromReality() const;
#endif

protected:
	virtual void OnNotifyLineHasBeenEdited(int nLine) = 0;
};

/////////////////////////////////////////////////////////////////////////////

#endif //__GHOSTTEXTBUFFER_H__
