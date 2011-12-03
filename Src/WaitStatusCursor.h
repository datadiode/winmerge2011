/**
 *  @file WaitStatusCursor.h
 *
 *  @brief Declaration WaitStatusCursor classes.
 */ 
// ID line follows -- this is updated by SVN
// $Id$

#ifndef WaitStatusCursor_included_h
#define WaitStatusCursor_included_h

class IStatusDisplay;

/**
 * @brief WaitStatusCursor enhanced cursor with GUI status feedback.
 * 
 * A WaitStatusCursor does what a CWaitCursor does, plus the caller selects
 * the wait cursor and sets the status message. 
 * Clients create WaitStatusCursors instead of CWaitCursors (don't mix them), and are isolated from
 * status message implementation.
 */
extern class WaitStatusCursor
{
public:
	WaitStatusCursor(UINT msgid = 0, HCURSOR cursor = ::LoadCursor(NULL, IDC_WAIT));
	void Begin(UINT msgid = 0, HCURSOR cursor = ::LoadCursor(NULL, IDC_WAIT));
	void End();
	~WaitStatusCursor();
	UINT GetMsgId() const { return m_msgid; }
private:
	WaitStatusCursor *m_next;
	static WaitStatusCursor *m_head;
	UINT m_msgid;
	HCURSOR m_cursor;
	HCURSOR m_cursorNext;
// factor/class interface
	void Restore();
public:
	static bool SetCursor(IStatusDisplay *);
private:
	static IStatusDisplay *m_piStatusDisplay;
} waitStatusCursor;

// Something that implements status messages for WaitStatusCursors
class IStatusDisplay
{
public:
	virtual void SetStatus(UINT msgid) = 0;
};

#endif // WaitStatusCursor_included_h
