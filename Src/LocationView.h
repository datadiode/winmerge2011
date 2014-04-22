//////////////////////////////////////////////////////////////////////
/** 
 * @file  LocationView.h
 *
 * @brief Declaration of CLocationView class
 */
//
//////////////////////////////////////////////////////////////////////

// ID line follows -- this is updated by SVN
// $Id$

#ifndef __LOCATIONVIEW_H__
#define __LOCATIONVIEW_H__

class CMergeEditView;

/**
 * @brief Status for display moved block
 */
enum DISPLAY_MOVED_BLOCKS
{
	DISPLAY_MOVED_NONE,
	DISPLAY_MOVED_ALL,
	DISPLAY_MOVED_FOLLOW_DIFF,
};

/**
 * @brief Endpoints of line connecting moved blocks
 */
struct MovedLine
{
	POINT ptLeft;
	POINT ptRight;
};

typedef stl::list<MovedLine> MOVEDLINE_LIST;

/**
 * @brief A struct mapping difference lines to pixels in location pane.
 * This structure maps one difference's line numbers to pixel locations in
 * the location pane. The line numbers are "fixed" i.e. they are converted to
 * word-wrapped absolute line numbers if needed.
 */
struct DiffBlock
{
	unsigned top_line; /**< First line of the difference. */
	unsigned bottom_line; /**< Last line of the difference. */
	unsigned top_coord; /**< X-coord of diff block begin. */
	unsigned bottom_coord; /**< X-coord of diff block end. */
	unsigned diff_index; /**< Index of difference in the original diff list. */
};

/** 
 * @brief Class showing map of files.
 * The location is a view showing two vertical bars. Each bar depicts one file
 * in the file compare. The bars show a scaled view of the files. The
 * difference areas are drawn with the same colors than in actual file compare.
 * Also visible area of files is drawn as "shaded".
 *
 * These visualizations allow user to easily see a overall picture of the files
 * in comparison. Using mouse it allows easy and fast moving in files.
 */
class CLocationView
	: public OWindow
	, public IDropTarget
{
public:
	CLocationView(CChildFrame *);
	~CLocationView();
	void SetConnectMovedBlocks(int displayMovedBlocks);
	void UpdateVisiblePos(int nTopLine, int nBottomLine);
	void ForceRecalculate();

	void SubclassWindow(HWindow *pWnd)
	{
		OWindow::SubclassWindow(pWnd);
		RegisterDragDrop(m_hWnd, this);
	}

	STDMETHOD(QueryInterface)(REFIID, void **);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();
	STDMETHOD(DragEnter)(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
	STDMETHOD(DragLeave)();
	STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
	STDMETHOD(Drop)(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

protected:
	enum LOCBAR_TYPE;
	CChildFrame *const m_pMergeDoc;
	void DrawRect(HSurface *, const RECT& r, COLORREF cr, BOOL bSelected = FALSE);
	void GotoLocation(const POINT &point);
	int GetLineFromYPos(int nYCoord, CMergeEditView *);
	LOCBAR_TYPE IsInsideBar(const POINT &pt);
	void DrawVisibleAreaRect(HSurface *, int nTopLine = -1, int nBottomLine = -1);
	void DrawConnectLines(HSurface *);
	void DrawDiffMarker(HSurface *, int yCoord);
	void CalculateBars();
	void CalculateBlocks();
	void CalculateBlocksPixel(int nBlockStart, int nBlockEnd, int nBlockLength,
			int &nBeginY, int &nEndY);
	void DrawBackground(HSurface *);

private:
	int m_displayMovedBlocks; //*< Setting for displaying moved blocks */
	double m_pixInLines; //*< How many pixels is one line in bars */
	double m_lineInPix; //*< How many lines is one pixel?
	SIZE m_size;
	RECT m_leftBar; //*< Left-side file's bar.
	RECT m_rightBar; //*< Right-side file's bar.
	int m_visibleTop; //*< Top visible line for visible area indicator */
	int m_visibleBottom; //*< Bottom visible line for visible area indicator */
	MOVEDLINE_LIST m_movedLines; //*< List of moved block connecting lines */
	HBitmap *m_pSavedBackgroundBitmap; //*< Saved background */
	stl::vector<DiffBlock> m_diffBlocks; //*< List of pre-calculated diff blocks.
	bool m_bRecalculateBlocks; //*< Recalculate diff blocks in next repaint.

	// Generated message map functions
protected:
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	void OnDown(LPARAM);
	void OnDrag(LPARAM);
	void OnContextMenu(LPARAM);
	void OnDraw(HSurface *);
};
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.


#endif //__LOCATIONVIEW_H__
