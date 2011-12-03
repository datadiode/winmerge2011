/**
 *  @file SortHeaderCtrl.h
 *
 *  @brief Declaration of CSortHeaderCtrl
 */ 
// RCS ID line follows -- this is updated by CVS
// $Id$
//////////////////////////////////////////////////////////////////////


#ifndef __SORTHEADERCTRL_H__
#define __SORTHEADERCTRL_H__


/////////////////////////////////////////////////////////////////////////////
// CSortHeaderCtrl

class CSortHeaderCtrl : public OHeaderCtrl
{
// Construction
public:
	CSortHeaderCtrl();
// Attributes
protected:
	int 	m_nSortCol;
	BOOL	m_bSortAsc;
// Operations
public:
	void DrawItem(LPDRAWITEMSTRUCT);
	int SetSortImage(int nCol, BOOL bAsc);
};

#endif
// __SORTHEADERCTRL_H__
