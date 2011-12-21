/**
 * @file GhostUndoRecord.h
 *
 * @brief Declaration for GhostUndoRecord structure.
 *
 */
// ID line follows -- this is updated by SVN
// $Id$

#ifndef _GHOST_UNDO_RECORD_H_
#define _GHOST_UNDO_RECORD_H_

/** 
 * @brief Support For Descriptions On Undo/Redo Actions.
 *
 * We need a structure to remember richer information position
 * and the number of real lines inserted/deleted (to set ghost lines during undo)
 *
 * This flags are parameters of AddUndoRecord ; so AddUndoRecord
 * is not the virtual version of CCrystalTextBuffer::AddUndoRecord
 *
 * The text is duplicated (already in CCrystalTextBuffer::SUndoRecord),
 * and it is not useful. If someone finds a clean way to correct this...
 */
class GhostUndoRecord : public UndoRecord
{
public:
	// Undo records store file line numbers, not screen line numbers
	// File line numbers do not count ghost lines
	// (ghost lines are lines with no text and no EOL chars, which are
	// used by WinMerge as left-only or right-only placeholders)
	// All the stored line number needed are real !

	int m_ptStartPos_nGhost, m_ptEndPos_nGhost;

	// Redo records store file line numbers, not screen line numbers
	// they store the file number of the previous real line
	// and (apparentLine - ComputeApparentLine(previousRealLine))

	POINT m_redo_ptStartPos, m_redo_ptEndPos;  // Block of text participating
	int m_redo_ptStartPos_nGhost, m_redo_ptEndPos_nGhost;

	int m_nRealLinesCreated;         //  number of lines created during insertion 
		                                //  (= total of real lines after - total before)
	int m_nRealLinesInDeletedBlock;  //  number of real lines in the deleted block 
		                                // (<> total of real lines after - total before  
		                                //  as first/end line may be just truncated, not removed)

public:
	GhostUndoRecord() // default constructor
		: m_redo_ptStartPos_nGhost(0)
		, m_redo_ptEndPos_nGhost(0)
		, m_nRealLinesCreated(0)
		, m_nRealLinesInDeletedBlock(0)
	{
	}
};

#endif // _GHOST_UNDO_RECORD_H_
