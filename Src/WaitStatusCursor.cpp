/////////////////////////////////////////////////////////////////////////////
//    WaitStatusCursur
//    Copyright (C) 2003  Perry Rapp (Author)
//    Copyright (C) 2011  Jochen Neubeck
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version. This program is distributed in
//    the hope that it will be useful, but WITHOUT ANY WARRANTY; without
//    even the implied warranty of MERCHANTABILITY or FITNESS FOR A
//    PARTICULAR PURPOSE.  See the GNU General Public License for more
//    details. You should have received a copy of the GNU General Public
//    License along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
/////////////////////////////////////////////////////////////////////////////
/**
 *  @file WaitStatusCursor.cpp
 *
 *  @brief Implementation of the WaitStatusCursur class.
 */ 
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "Merge.h"

IStatusDisplay *WaitStatusCursor::m_piStatusDisplay = 0;
WaitStatusCursor *WaitStatusCursor::m_head = &waitStatusCursor;

// The waitStatusCursor is always at the bottom of the stack and remains there
// because its m_next member points to itself.
WaitStatusCursor waitStatusCursor(0, NULL);

bool WaitStatusCursor::SetCursor(IStatusDisplay *piStatusDisplay)
{
	m_piStatusDisplay = piStatusDisplay;
	if (m_head->m_cursor == NULL)
		return false;
	m_head->Restore();
	return true;
}

WaitStatusCursor::WaitStatusCursor(UINT msgid, HCURSOR cursor)
: m_next(m_head), m_cursorNext(m_head->m_cursor)
{
	Begin(msgid, cursor);
}

void WaitStatusCursor::Begin(UINT msgid, HCURSOR cursor)
{
	m_msgid = msgid;
	m_cursor = cursor;
	if (m_head == m_next)
	{
		if (cursor != NULL && m_next->m_cursor == NULL)
			m_next->m_cursor = ::GetCursor();
		m_head = this;
		Restore();
	}
}

WaitStatusCursor::~WaitStatusCursor()
{
	End();
}

void WaitStatusCursor::End()
{
	m_msgid = 0;
	m_cursor = NULL;
	if (m_head == this)
	{
		m_head = m_next;
		m_head->Restore();
		m_head->m_cursor = m_cursorNext;
	}
}

void WaitStatusCursor::Restore()
{
	// restore text and cursor
	if (m_cursor)
		::SetCursor(m_cursor);
	if (m_piStatusDisplay)
		m_piStatusDisplay->SetStatus(m_msgid);
}
