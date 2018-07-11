/**
 *  @file SortHeaderCtrl.h
 *
 *  @brief Declaration of CSortHeaderCtrl
 */
#pragma once

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
	LRESULT DrawItem(LPDRAWITEMSTRUCT);
	int SetSortImage(int nCol, BOOL bAsc);
};
