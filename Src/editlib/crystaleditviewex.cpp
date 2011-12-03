// CrystalEditViewEx.cpp: Implementierung der Klasse CCrystalEditViewEx.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "crystalparser.h"
#include "crystaleditviewex.h"


//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////

CCrystalEditViewEx::CCrystalEditViewEx(size_t ZeroInit)
: CCrystalEditView(ZeroInit)
{
}

DWORD CCrystalEditViewEx::ParseLine( DWORD dwCookie, int nLineIndex, 
																		TEXTBLOCK *pBuf, int &nActualItems )
{
	if( m_pSyntaxParser )
	{
		CCrystalTextBlock	*pTextBlock = 
			pBuf? new CCrystalTextBlock( (CCrystalTextBlock::TEXTBLOCK*)pBuf, nActualItems ) : NULL;
		dwCookie = m_pSyntaxParser->ParseLine( dwCookie, nLineIndex, pTextBlock );
		
		if( pTextBlock )
			delete pTextBlock;

		return dwCookie;
	}
	else
		return 0;
}


CCrystalParser *CCrystalEditViewEx::SetSyntaxParser( CCrystalParser *pParser )
{
	CCrystalParser	*pOldParser = m_pSyntaxParser;

	m_pSyntaxParser = pParser;

	if( pParser )
  //BEGIN FP
    pParser->m_pTextView = this;
		/*ORIGINAL
    pParser->m_pEditViewEx = this;
		*/
  //END FP

	return pOldParser;
}
