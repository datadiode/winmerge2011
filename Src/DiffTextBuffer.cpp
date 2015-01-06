/** 
 * @file  DiffTextBuffer.cpp
 *
 * @brief Implementation file for CDiffTextBuffer
 *
 */
#include "StdAfx.h"
#include "UniFile.h"
#include "locality.h"
#include "coretools.h"
#include "Merge.h"
#include "LogFile.h"
#include "OptionsMgr.h"
#include "Environment.h"
#include "FileTransform.h"
#include "FileTextEncoding.h"
#include "DiffTextBuffer.h"

using std::vector;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static bool IsTextFileStylePure(const UniMemFile::txtstats & stats);
static CRLFSTYLE GetTextFileStyle(const UniMemFile::txtstats & stats);

/**
 * @brief Check if file has only one EOL type.
 * @param [in] stats File's text stats.
 * @return true if only one EOL type is found, false otherwise.
 */
static bool IsTextFileStylePure(const UniMemFile::txtstats & stats)
{
	int nType = 0;
	if (stats.ncrlfs > 0)
		nType++;
	if (stats.ncrs > 0)
		nType++;
	if (stats.nlfs > 0)
		nType++;
	return nType <= 1;
}

/**
 * @brief Get file's EOL type.
 * @param [in] stats File's text stats.
 * @return EOL type.
 */
static CRLFSTYLE GetTextFileStyle(const UniMemFile::txtstats & stats)
{
	// Check if file has more than one EOL type.
	if (!IsTextFileStylePure(stats))
		return CRLF_STYLE_MIXED;
	else if (stats.ncrlfs >= stats.nlfs)
	{
		if (stats.ncrlfs >= stats.ncrs)
			return CRLF_STYLE_DOS;
		else
			return CRLF_STYLE_MAC;
	}
	else
	{
		if (stats.nlfs >= stats.ncrs)
			return CRLF_STYLE_UNIX;
		else
			return CRLF_STYLE_MAC;
	}
}

/**
 * @brief Constructor.
 * @param [in] pDoc Owning CMergeDoc.
 * @param [in] pane Pane number this buffer is associated with.
 */
CDiffTextBuffer::CDiffTextBuffer(CChildFrame *pDoc, int pane)
: m_pOwnerDoc(pDoc)
, m_nThisPane(pane)
, m_unpackerSubcode(0)
, m_bMixedEOL(false)
{
	InitNew();
}

CDiffTextBuffer::~CDiffTextBuffer()
{
	FreeAll();
}

UndoRecord &CDiffTextBuffer::AddUndoRecord(BOOL bInsert, const POINT &ptStartPos,
		const POINT &ptEndPos, LPCTSTR pszText, int cchText,
		int nLinesToValidate, int nActionType)
{
	UndoRecord &ur = CGhostTextBuffer::AddUndoRecord(bInsert, ptStartPos, ptEndPos, pszText,
		cchText, nLinesToValidate, nActionType);
	if (m_aUndoBuf[m_nUndoPosition - 1].m_dwFlags & UNDO_BEGINGROUP)
	{
		m_pOwnerDoc->undoTgt.erase(m_pOwnerDoc->curUndo, m_pOwnerDoc->undoTgt.end());
		m_pOwnerDoc->undoTgt.push_back(m_pOwnerDoc->GetView(m_nThisPane));
		m_pOwnerDoc->curUndo = m_pOwnerDoc->undoTgt.end();
	}
	return ur;
}
/**
 * @brief Checks if a flag is set for line.
 * @param [in] line Index (0-based) for line.
 * @param [in] flag Flag to check.
 * @return TRUE if flag is set, FALSE otherwise.
 */
bool CDiffTextBuffer::FlagIsSet(UINT line, DWORD flag) const
{
	return (m_aLines[line].m_dwFlags & flag) == flag;
}

/**
Remove blank lines and clear winmerge flags
(2003-06-21, Perry: I don't understand why this is necessary, but if this isn't 
done, more and more gray lines appear in the file)
(2003-07-31, Laoran I don't understand either why it is necessary, but it works
fine, so let's go on with it)
*/
void CDiffTextBuffer::prepareForRescan()
{
	RemoveAllGhostLines();
	int i = GetLineCount();
	while (i)
	{
		--i;
		m_aLines[i].m_dwFlags &= ~(LF_DIFF | LF_TRIVIAL | LF_MOVED);
	}
}

/** 
 * @brief Called when line has been edited.
 * After editing a line, we don't know if there is a diff or not.
 * So we clear the LF_DIFF flag (and it is more easy to read during edition).
 * Rescan will set the proper color.
 * @param [in] nLine Line that has been edited.
 */

void CDiffTextBuffer::OnNotifyLineHasBeenEdited(int nLine)
{
	m_aLines[nLine].m_dwFlags &= ~(LF_DIFF | LF_TRIVIAL | LF_MOVED);
}

bool CDiffTextBuffer::IsSaveable() const
{
	return IsModified() && !GetReadOnly();
}

/**
 * @brief Load file from disk into buffer
 *
 * @param [in] pszFileNameInit File to load
 * @param [in] infoUnpacker Unpacker plugin
 * @param [out] readOnly Loading was lossy so file should be read-only
 * @param [in] nCrlfStyle EOL style used
 * @param [in] encoding Encoding used
 * @param [out] sError Error message returned
 * @return FRESULT_OK when loading succeed or (list in files.h):
 * - FRESULT_OK_IMPURE : load OK, but the EOL are of different types
 * - FRESULT_ERROR_UNPACK : plugin failed to unpack
 * - FRESULT_ERROR : loading failed, sError contains error message
 * - FRESULT_BINARY : file is binary file
 * @note If this method fails, it calls InitNew so the CDiffTextBuffer is in a valid state
 */
FileLoadResult::FILES_RESULT CDiffTextBuffer::LoadFromFile(LPCTSTR pszFileName,
		PackingInfo *infoUnpacker, bool &readOnly,
		CRLFSTYLE nCrlfStyle, FileTextEncoding &encoding, String &sError)
{
	ASSERT(!m_bInit);
	ASSERT(m_aLines.size() == 0);

	FileLoadResult::FILES_RESULT nRetVal = FileLoadResult::FRESULT_OK;

	UniFile *pufile = NULL;

	try
	{
		pufile = infoUnpacker->pfnCreateUniFile(infoUnpacker);
		if (infoUnpacker->readOnly)
			readOnly = TRUE;
		// Now we only use the UniFile interface
		// which is something we could implement for HTTP and/or FTP files

		if (pufile->OpenReadOnly(pszFileName))
		{
			pufile->ReadBom();
			// If the file is not unicode file, use the codepage we were given to
			// interpret the 8-bit characters.
			if (UNICODESET unicoding = pufile->GetUnicoding())
				encoding.SetUnicoding(unicoding);
			else
				pufile->SetCodepage(encoding.m_codepage);
			UINT lineno = 0;
			String eol, preveol;
			String sline;
			bool done = false;

			// Manually grow line array exponentially
			UINT arraysize = 500;
			m_aLines.resize(arraysize);
			
			// preveol must be initialized for empty files
			preveol = _T("\n");
			
			do
			{
				bool lossy = false;
				done = !pufile->ReadString(sline, eol, &lossy);

				// if last line had no eol, we can quit
				if (done && preveol.empty())
					break;
				// but if last line had eol, we add an extra (empty) line to buffer

				// Grow line array
				if (lineno == arraysize)
				{
					// For smaller sizes use exponential growth, but for larger
					// sizes grow by constant ratio. Unlimited exponential growth
					// easily runs out of memory.
					if (arraysize < 100 * 1024)
						arraysize *= 2;
					else
						arraysize += 100 * 1024;
					m_aLines.resize(arraysize);
				}

				sline += eol;
				if (lossy)
				{
					// TODO: Should record lossy status of line
				}
				AppendLine(lineno, sline.c_str(), sline.length());
				++lineno;
				preveol = eol;
			} while (!done);


			// fix array size (due to our manual exponential growth
			m_aLines.resize(lineno);
			
			//Try to determine current CRLF mode (most frequent)
			if (nCrlfStyle == CRLF_STYLE_AUTOMATIC)
			{
				nCrlfStyle = GetTextFileStyle(pufile->GetTxtStats());
			}
			ASSERT(nCrlfStyle >= 0 && nCrlfStyle <= 3);
			SetCRLFMode(nCrlfStyle);
			
			//  At least one empty line must present
			// (view does not work for empty buffers)
			ASSERT(m_aLines.size() > 0);
			
			m_bInit = true;
			m_bModified = false;
			m_bUndoGroup = m_bUndoBeginGroup = FALSE;
			m_nSyncPosition = m_nUndoPosition = 0;
			ASSERT(m_aUndoBuf.size() == 0);
			m_dwCurrentRevisionNumber = 0;
			m_dwRevisionNumberOnSave = 0;
			m_ptLastChange.x = m_ptLastChange.y = -1;
			
			FinishLoading();
			// flags don't need initialization because 0 is the default value

			// Set the return value : OK + info if the file is impure
			// A pure file is a file where EOL are consistent (all DOS, or all UNIX, or all MAC)
			// An impure file is a file with several EOL types
			// WinMerge may display impure files, but the default option is to unify the EOL
			// We return this info to the caller, so it may display a confirmation box
			if (IsTextFileStylePure(pufile->GetTxtStats()))
				nRetVal = FileLoadResult::FRESULT_OK;
			else
				nRetVal = FileLoadResult::FRESULT_OK_IMPURE;

			// stash original encoding away
			m_encoding.m_unicoding = pufile->GetUnicoding();
			m_encoding.m_bom = pufile->HasBom();
			m_encoding.m_codepage = pufile->GetCodepage();

			if (pufile->GetTxtStats().nlosses)
			{
				FileLoadResult::AddModifier(nRetVal, FileLoadResult::FRESULT_LOSSY);
				readOnly = TRUE;
			}
		}
		else
		{
			nRetVal = FileLoadResult::FRESULT_ERROR;
			UniFile::UniError uniErr = pufile->GetLastUniError();
			if (uniErr.HasError())
			{
				sError = uniErr.GetError();
			}
			InitNew(); // leave crystal editor in valid, empty state
		}
		
		// close the file now to free the handle
		pufile->Close();
	}
	catch (OException *e)
	{
		nRetVal = FileLoadResult::FRESULT_ERROR;
		sError = e->msg;
		delete e;
	}
	delete pufile;

	return nRetVal;
}

/**
 * @brief Saves file from buffer to disk
 *
 * @param pTempStream : NULL if we are saving user files and
 * NOT NULL if we are saving workin-temp-files for diff-engine
 *
 * @return SAVE_DONE or an error code (list in MergeDoc.h)
 */
int CDiffTextBuffer::SaveToFile(LPCTSTR pszFileName,
		ISequentialStream *pTempStream, String & sError, PackingInfo * infoUnpacker /*= NULL*/,
		CRLFSTYLE nCrlfStyle /*= CRLF_STYLE_AUTOMATIC*/,
		int nStartLine /*= 0*/, int nLines /*= -1*/)
{
	ASSERT(m_bInit);

	if (nLines == -1)
		nLines = m_aLines.size() - nStartLine;

	if ((!pszFileName || pszFileName[0] == _T('\0')) && !pTempStream)
		return SAVE_FAILED;	// No filename, cannot save...

	if (nCrlfStyle == CRLF_STYLE_AUTOMATIC &&
		!COptionsMgr::Get(OPT_ALLOW_MIXED_EOL) ||
		infoUnpacker && infoUnpacker->disallowMixedEOL)
	{
			// get the default nCrlfStyle of the CDiffTextBuffer
		nCrlfStyle = GetCRLFMode();
		ASSERT(nCrlfStyle >= 0 && nCrlfStyle <= 3);
	}

	UniStdioFile file;

	String sIntermediateFilename; // used when !pTempStream

	if (pTempStream)
	{
		file.Attach(pTempStream);
		// DiffUtils want it octet-encoded without BOM
		file.SetUnicoding(m_encoding.m_unicoding ? UTF8 : NONE);
		file.SetCodepage(m_encoding.m_codepage);
	}
	else
	{
		file.SetBom(m_encoding.m_bom);
		file.SetUnicoding(m_encoding.m_unicoding);
		file.SetCodepage(m_encoding.m_codepage);
		sIntermediateFilename = env_GetTempFileName(env_GetTempPath(), _T("MRG_"));
		if (sIntermediateFilename.empty())
			return SAVE_FAILED;  //Nothing to do if even tempfile name fails
		if (!file.OpenCreate(sIntermediateFilename.c_str()))
		{	
			UniFile::UniError uniErr = file.GetLastUniError();
			if (uniErr.HasError())
			{
				sError = uniErr.GetError();
				LogFile.Write(LogFile.LERROR,
					_T("Opening file %s failed: %s"),
					sIntermediateFilename.c_str(), sError.c_str());
			}
			return SAVE_FAILED;
		}
	}

	file.WriteBom();

	// line loop : get each real line and write it in the file
	LPCTSTR sEol = GetStringEol(nCrlfStyle);
	const stl_size_t nEndLine = nStartLine + nLines;
	for (stl_size_t line = nStartLine; line < nEndLine ; ++line)
	{
		const LineInfo &li = GetLineInfo(line);
		if (li.m_dwFlags & LF_GHOST)
			continue;

		// write the characters of the line (excluding EOL)
		file.WriteString(li.GetLine(), li.Length());

		LPCTSTR sOriginalEol = li.GetEol();
		// last real line is never EOL terminated
		if (sOriginalEol == NULL)
			break;

		// normal real line : append an EOL
		// either the EOL of the line (when preserve original EOL chars is on)
		// or the default EOL for this file
		if (nCrlfStyle == CRLF_STYLE_AUTOMATIC || nCrlfStyle == CRLF_STYLE_MIXED)
			sEol = sOriginalEol;
		file.WriteString(sEol, static_cast<String::size_type>(_tcslen(sEol)));
	}

	// If we are saving temp files, we are done
	if (pTempStream)
	{
		return SAVE_DONE;
	}

	file.Close();
	// Write tempfile over original file
	if (!::CopyFile(sIntermediateFilename.c_str(), pszFileName, FALSE))
	{
		sError = GetSysError(GetLastError());
		LogFile.Write(LogFile.LERROR, _T("CopyFile(%s, %s) failed: %s"),
			sIntermediateFilename.c_str(), pszFileName, sError.c_str());
		return SAVE_FAILED;
	}
	// Now that original file is up to date, delete the tempfile
	if (!::DeleteFile(sIntermediateFilename.c_str()))
	{
		LogFile.Write(LogFile.LERROR, _T("DeleteFile(%s) failed: %s"),
			sIntermediateFilename.c_str(), GetSysError(GetLastError()).c_str());
	}
	// Updating original file implies that we must clear the "modified" flag,
	// which implies that there is no need for an extra bClearModifiedFlag.
	SetModified(FALSE);
	m_nSyncPosition = m_nUndoPosition;
	// remember revision number on save
	m_dwRevisionNumberOnSave = m_dwCurrentRevisionNumber;
	// redraw line revision marks
	UpdateViews(NULL, NULL, UPDATE_FLAGSONLY);	
	return SAVE_DONE;
}
