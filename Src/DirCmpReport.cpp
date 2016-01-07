/** 
 * @file  DirCmpReport.cpp
 *
 * @brief Implementation file for DirCmpReport
 *
 */
#include "StdAfx.h"
#include "resource.h"
#include "markdown.h"
#include "LanguageSelect.h"
#include "WaitStatusCursor.h"
#include "Merge.h"
#include "DirView.h"
#include "DirFrame.h"
#include "DirCmpReport.h"
#include "DirCmpReportDlg.h"
#include "coretools.h"
#include "paths.h"
#include "OptionsMgr.h"

UINT CF_HTML = RegisterClipboardFormat(_T("HTML Format"));

/**
 * @brief Return current time as string.
 */
static locality::TimeString GetCurrentTimeString()
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	return st;
}

/**
 * @brief Permute COLORREF such that printf("#%06x") yields equivalent HTML.
 */
static DWORD MakeHtmlColor(COLORREF cr)
{
	return (cr & 0x0000FF) << 16 | (cr & 0x00FF00) | (cr & 0xFF0000) >> 16;
}

/**
 * @brief Constructor.
 */
DirCmpReport::DirCmpReport(CDirView *pList)
	: m_pList(pList), m_pFile(NULL), m_sSeparator(_T(",")), m_rootPaths(2)
{
	CDirFrame *const pDoc = pList->m_pFrame;
	const CDiffContext *ctxt = pDoc->GetDiffContext();
	m_nColumns = pList->GetHeaderCtrl()->GetItemCount();
	m_rootPaths[0] = ctxt->GetLeftPath();
	m_rootPaths[1] = ctxt->GetRightPath();
	// If inside archive, convert paths
	pDoc->ApplyLeftDisplayRoot(m_rootPaths[0]);
	pDoc->ApplyRightDisplayRoot(m_rootPaths[1]);
	paths_UndoMagic(m_rootPaths[0]);
	paths_UndoMagic(m_rootPaths[1]);
}

/**
 * @brief Generate report and save it to file.
 * @param [out] errStr Empty if succeeded, otherwise contains error message.
 * @return TRUE if report was created, FALSE if user canceled report.
 */
bool DirCmpReport::GenerateReport(String &errStr)
{
	assert(m_pList != NULL);
	assert(m_pFile == NULL);
	bool bRet = false;

	DirCmpReportDlg dlg;
	if (LanguageSelect.DoModal(dlg) == IDOK) try
	{
		WaitStatusCursor waitstatus(IDS_STATUS_CREATEREPORT);
		if (dlg.m_bCopyToClipboard)
		{
			if (!m_pList->OpenClipboard())
				return false;
			if (!EmptyClipboard())
				return false;
			CreateStreamOnHGlobal(NULL, FALSE, &m_pFile);
			GenerateReport(dlg.m_nReportType);
			IStream_Write(m_pFile, "", sizeof ""); // write terminating zero
			HGLOBAL hGlobal = NULL;
			GetHGlobalFromStream(m_pFile, &hGlobal);
			SetClipboardData(CF_TEXT, hGlobal);
			m_pFile->Release();
			m_pFile = NULL;
			// If report type is HTML, render CF_HTML format as well
			if (dlg.m_nReportType == REPORT_TYPE_SIMPLEHTML)
			{
				CreateStreamOnHGlobal(NULL, FALSE, &m_pFile);
				// Write preliminary CF_HTML header with all offsets zero
				static const char header[] =
					"Version:0.9\n"
					"StartHTML:%09d\n"
					"EndHTML:%09d\n"
					"StartFragment:%09d\n"
					"EndFragment:%09d\n";
				static const char start[] = "<html><body>\n<!--StartFragment -->";
				static const char end[] = "\n<!--EndFragment -->\n</body>\n</html>\n";
				char buffer[256];
				int cbHeader = wsprintfA(buffer, header, 0, 0, 0, 0);
				IStream_Write(m_pFile, buffer, cbHeader);
				IStream_Write(m_pFile, start, sizeof start - 1);
				GenerateHTMLHeaderBodyPortion();
				GenerateXmlHtmlContent(false);
				IStream_Write(m_pFile, end, sizeof end); // include terminating zero
				ULARGE_INTEGER size = { 0, 0 };
				IStream_Size(m_pFile, &size);
				IStream_Reset(m_pFile);
				wsprintfA(buffer, header, cbHeader, size.LowPart - 1,
					cbHeader + sizeof start - 1, size.LowPart - sizeof end + 1);
				IStream_Write(m_pFile, buffer, cbHeader);
				HGLOBAL hGlobal = NULL;
				GetHGlobalFromStream(m_pFile, &hGlobal);
				SetClipboardData(CF_HTML, GlobalReAlloc(hGlobal, size.LowPart, 0));
				m_pFile->Release();
				m_pFile = NULL;
			}
			CloseClipboard();
		}
		if (!dlg.m_sReportFile.empty())
		{
			if (!paths_CreateIfNeeded(dlg.m_sReportFile.c_str(), true))
			{
				errStr = LanguageSelect.LoadString(IDS_FOLDER_NOTEXIST);
				return false;
			}
			SHCreateStreamOnFile(dlg.m_sReportFile.c_str(),
				STGM_WRITE | STGM_CREATE | STGM_SHARE_DENY_WRITE, &m_pFile);
			GenerateReport(dlg.m_nReportType);
			m_pFile->Release();
			m_pFile = NULL;
		}
		bRet = true;
	}
	catch (OException *e)
	{
		e->ReportError(m_pList->m_hWnd, MB_ICONSTOP);
		delete e;
	}
	m_pFile = NULL;
	return bRet;
}

/**
 * @brief Generate report of given type.
 * @param [in] nReportType Type of report.
 */
void DirCmpReport::GenerateReport(REPORT_TYPE nReportType)
{
	m_sTitle = LanguageSelect.FormatStrings(IDS_DIRECTORY_REPORT_TITLE,
		m_rootPaths[0].c_str(), m_rootPaths[1].c_str());
	switch (nReportType)
	{
	case REPORT_TYPE_SIMPLEHTML:
		GenerateHTMLHeader();
		GenerateXmlHtmlContent(false);
		GenerateHTMLFooter();
		break;
	case REPORT_TYPE_SIMPLEXML:
		GenerateXmlHeader();
		GenerateXmlHtmlContent(true);
		GenerateXmlFooter();
		break;
	case REPORT_TYPE_COMMALIST:
		m_sSeparator = _T(",");
		GenerateHeader();
		GenerateContent();
		break;
	case REPORT_TYPE_SEMICOLONLIST:
		m_sSeparator = _T(";");
		GenerateHeader();
		GenerateContent();
		break;
	case REPORT_TYPE_TABLIST:
		m_sSeparator = _T("\t");
		GenerateHeader();
		GenerateContent();
		break;
	}
}

/**
 * @brief Write text to report file.
 * @param [in] H Text to write to report file.
 */
void DirCmpReport::WriteString(HString *H, UINT codepage)
{
	const OString strOctets = H->Oct(codepage);
	LPCSTR pchOctets = strOctets.A;
	UINT cchAhead = strOctets.ByteLen();
	while (LPCSTR pchAhead = (LPCSTR)memchr(pchOctets, '\n', cchAhead))
	{
		ULONG cchLine = static_cast<ULONG>(pchAhead - pchOctets);
		IStream_Write(m_pFile, pchOctets, cchLine);
		static const char eol[] = { '\r', '\n' };
		IStream_Write(m_pFile, eol, sizeof eol);
		++cchLine;
		pchOctets += cchLine;
		cchAhead -= cchLine;
	}
	IStream_Write(m_pFile, pchOctets, cchAhead);
}

/**
 * @brief Write text to report file.
 * @param [in] pszText Text to write to report file.
 */
void DirCmpReport::WriteString(LPCTSTR pszText)
{
	WriteString(HString::Uni(pszText), CP_UTF8);
}

/**
 * @brief Write text to report file while turning special chars to entities.
 * @param [in] pszText Text to write to report file.
 */
void DirCmpReport::WriteStringEntityAware(LPCTSTR pszText)
{
	WriteString(CMarkdown::Entitify(pszText), CP_UTF8);
}

/**
 * @brief Generate header-data for report.
 */
void DirCmpReport::GenerateHeader()
{
	static const BYTE bom[] = { 0xEF, 0xBB, 0xBF };
	IStream_Write(m_pFile, bom, sizeof bom);
	WriteString(m_sTitle.c_str());
	WriteString(_T("\n"));
	WriteString(GetCurrentTimeString().c_str());
	WriteString(_T("\n"));
	for (int currCol = 0; currCol < m_nColumns; currCol++)
	{
		TCHAR columnName[160]; // Assuming max col header will never be > 160
		LVCOLUMN lvc;
		lvc.mask = LVCF_TEXT;
		lvc.pszText = columnName;
		lvc.cchTextMax = _countof(columnName);
		if (m_pList->GetColumn(currCol, &lvc))
			WriteString(lvc.pszText);
		// Add col-separator, but not after last column
		if (currCol < m_nColumns - 1)
			WriteString(m_sSeparator.c_str());
	}
}

/**
 * @brief Generate report content (compared items).
 */
void DirCmpReport::GenerateContent()
{
	int nRows = m_pList->GetItemCount();

	// Report:Detail. All currently displayed columns will be added
	for (int currRow = 0; currRow < nRows; currRow++)
	{
		WriteString(_T("\n"));
		for (int currCol = 0; currCol < m_nColumns; currCol++)
		{
			String value = m_pList->GetItemText(currRow, currCol);
			if (value.find(m_sSeparator) != String::npos)
			{
				WriteString(_T("\""));
				WriteString(value.c_str());
				WriteString(_T("\""));
			}
			else
			{
				WriteString(value.c_str());
			}
			// Add col-separator, but not after last column
			if (currCol < m_nColumns - 1)
				WriteString(m_sSeparator.c_str());
		}
	}
}

/**
 * @brief Generate simple html report header.
 */
void DirCmpReport::GenerateHTMLHeader()
{
	WriteString(_T("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\"\n")
				_T("\t\"http://www.w3.org/TR/html4/loose.dtd\">\n")
				_T("<html>\n")
				_T("<head>\n")
				_T("\t<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">\n")
				_T("\t<title>"));
	WriteStringEntityAware(m_sTitle.c_str());
	WriteString(_T("</title>\n")
				_T("\t<style type=\"text/css\">\n\t<!--\n")
				_T("\t\tbody {\n")
				_T("\t\t\tfont-family: sans-serif;\n")
				_T("\t\t\tfont-size: smaller;\n")
				_T("\t\t}\n")
				_T("\t\ttable {\n")
				_T("\t\t\tborder-collapse: collapse;\n")
				_T("\t\t\tborder: 1px solid gray;\n")
				_T("\t\t}\n")
				_T("\t\tth,td {\n")
				_T("\t\t\tpadding: 3px;\n")
				_T("\t\t\ttext-align: left;\n")
				_T("\t\t\tvertical-align: top;\n")
				_T("\t\t\tborder: 1px solid gray;\n")
				_T("\t\t}\n")
				_T("\t\tth {\n")
				_T("\t\t\tcolor: black;\n")
				_T("\t\t\tbackground: silver;\n")
				_T("\t\t}\n")
				_T("\t-->\n\t</style>\n"));

	WriteString(_T("\t<style%s type=\"text/css\">\n\t<!--\n")
				_T("\t\ttr.leftonly {\n")
				_T("\t\t\tbackground-color: #%06x;\n")
				_T("\t\t}\n")
				_T("\t\ttr.rightonly {\n")
				_T("\t\t\tbackground-color: #%06x;\n")
				_T("\t\t}\n")
				_T("\t\ttr.suspicious {\n")
				_T("\t\t\tbackground-color: #%06x;\n")
				_T("\t\t}\n")
				_T("\t-->\n\t</style>\n"),
				&_T("\0 disabled")[COptionsMgr::Get(OPT_CLR_DEFAULT_LIST_COLORING)],
				MakeHtmlColor(COptionsMgr::Get(OPT_LIST_LEFTONLY_BKGD_COLOR)),
				MakeHtmlColor(COptionsMgr::Get(OPT_LIST_RIGHTONLY_BKGD_COLOR)),
				MakeHtmlColor(COptionsMgr::Get(OPT_LIST_SUSPICIOUS_BKGD_COLOR)));

	WriteString(_T("</head>\n<body>\n"));
	GenerateHTMLHeaderBodyPortion();
}

/**
 * @brief Generate body portion of simple html report header (w/o body tag).
 */
void DirCmpReport::GenerateHTMLHeaderBodyPortion()
{
	WriteString(_T("<h2>"));
	WriteString(m_sTitle.c_str());
	WriteString(_T("</h2>\n<p>"));
	WriteString(GetCurrentTimeString().c_str());
	WriteString(_T("</p>\n")
				_T("<table border=\"1\">\n<tr>\n"));

	for (int currCol = 0; currCol < m_nColumns; currCol++)
	{
		TCHAR columnName[160]; // Assuming max col header will never be > 160
		LVCOLUMN lvc;
		lvc.mask = LVCF_TEXT;
		lvc.pszText = columnName;
		lvc.cchTextMax = _countof(columnName);
		if (m_pList->GetColumn(currCol, &lvc))
		{
			WriteString(_T("<th>"));
			WriteStringEntityAware(lvc.pszText);
			WriteString(_T("</th>"));
		}
	}
	WriteString(_T("</tr>\n"));
}

/**
 * @brief Generate simple xml report header.
 */
void DirCmpReport::GenerateXmlHeader()
{
	WriteString(_T("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
				_T("<WinMergeDiffReport version=\"2\">\n")
				_T("<left>"));
	WriteStringEntityAware(m_rootPaths[0].c_str());
	WriteString(_T("</left>\n")
				_T("<right>"));
	WriteStringEntityAware(m_rootPaths[1].c_str());
	WriteString(_T("</right>\n")
				_T("<time>"));
	WriteStringEntityAware(GetCurrentTimeString().c_str());
	WriteString(_T("</time>\n"));

	// Add column headers
	WriteString(_T("<column_name>"));
	for (int currCol = 0; currCol < m_nColumns; currCol++)
	{
		TCHAR columnName[160]; // Assuming max col header will never be > 160
		LVCOLUMN lvc;
		lvc.mask = LVCF_TEXT;
		lvc.pszText = columnName;
		lvc.cchTextMax = _countof(columnName);
		int logcol = m_pList->ColPhysToLog(currCol);
		LPCTSTR colEl = CDirView::f_cols[logcol].regName;
		if (m_pList->GetColumn(currCol, &lvc))
		{
			WriteString(_T("<%s>"), colEl);
			WriteStringEntityAware(lvc.pszText);
			WriteString(_T("</%s>"), colEl);
		}
	}
	WriteString(_T("</column_name>\n"));
}

/**
 * @brief Generate simple html or xml report content.
 */
void DirCmpReport::GenerateXmlHtmlContent(bool xml)
{
	int nRows = m_pList->GetItemCount();
	// Report:Detail. All currently displayed columns will be added
	for (int currRow = 0; currRow < nRows; currRow++)
	{
		LPCTSTR tr = _T("<tr>");
		if (DIFFITEM *di = m_pList->GetDiffItem(currRow))
		{
			switch (di->diffcode & (DIFFCODE::SIDEFLAGS | DIFFCODE::COMPAREFLAGS))
			{
			case DIFFCODE::BOTH | DIFFCODE::NOCMP:
			case DIFFCODE::BOTH | DIFFCODE::SAME:
				// either identical or irrelevant
				break;
			case DIFFCODE::LEFT:
				// left-only
				tr = _T("<tr class='leftonly'>");
				break;
			case DIFFCODE::RIGHT:
				// right-only
				tr = _T("<tr class='rightonly'>");
				break;
			default:
				// otherwise suspicious
				tr = _T("<tr class='suspicious'>");
				break;
			}
		}
		WriteString(xml ? _T("<filediff>") : tr);
		for (int currCol = 0; currCol < m_nColumns; currCol++)
		{
			int logcol = m_pList->ColPhysToLog(currCol);
			LPCTSTR const colEl = xml ? CDirView::f_cols[logcol].regName : _T("td");
			WriteString(_T("<%s>"), colEl);
			WriteStringEntityAware(m_pList->GetItemText(currRow, currCol).c_str());
			WriteString(_T("</%s>"), colEl);
		}
		WriteString(xml ? _T("</filediff>") : _T("</tr>"));
		WriteString(_T("\n"));
	}
	if (!xml)
		WriteString(_T("</table>\n"));
}

/**
 * @brief Generate simple html report footer.
 */
void DirCmpReport::GenerateHTMLFooter()
{
	WriteString(_T("</body>\n</html>\n"));
}

/**
 * @brief Generate simple xml report header.
 */
void DirCmpReport::GenerateXmlFooter()
{
	WriteString(_T("</WinMergeDiffReport>\n"));
}
