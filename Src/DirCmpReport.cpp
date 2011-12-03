/** 
 * @file  DirCmpReport.cpp
 *
 * @brief Implementation file for DirCmpReport
 *
 */
// ID line follows -- this is updated by SVN
// $Id$
//

#include <StdAfx.h>
#include "resource.h"
#include "LanguageSelect.h"
#include "WaitStatusCursor.h"
#include "DirCmpReport.h"
#include "DirCmpReportDlg.h"
#include "coretools.h"
#include "paths.h"

UINT CF_HTML = RegisterClipboardFormat(_T("HTML Format"));

/**
 * @brief Return current time as string.
 */
static locality::TimeString GetCurrentTimeString()
{
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	return ft;
}

/**
 * @brief Format string as beginning tag.
 * @param [in] elName String to format as beginning tag.
 * @return String formatted as beginning tag.
 */
static String BeginEl(LPCTSTR elName)
{
	return string_format(_T("<%s>"), elName);
}

/**
 * @brief Format string as ending tag.
 * @param [in] elName String to format as ending tag.
 * @return String formatted as ending tag.
 */
static String EndEl(LPCTSTR elName)
{
	return string_format(_T("</%s>"), elName);
}

/**
 * @brief Constructor.
 */
DirCmpReport::DirCmpReport(const stl::vector<String> & colRegKeys)
: m_pList(NULL)
, m_pFile(NULL)
, m_nColumns(0)
, m_colRegKeys(colRegKeys)
, m_sSeparator(_T(","))
, m_rootPaths(2)
{
}

/**
 * @brief Set UI-list pointer.
 */
void DirCmpReport::SetList(HListView *pList)
{
	m_pList = pList;
}

/**
 * @brief Set root-paths of current compare so we can add them to report.
 */
void DirCmpReport::SetRootPaths(LPCTSTR left, LPCTSTR right)
{
	m_rootPaths[0] = left;
	m_rootPaths[1] = right;
	m_sTitle = LanguageSelect.FormatMessage(IDS_DIRECTORY_REPORT_TITLE, left, right);
}

/**
 * @brief Set column-count.
 */
void DirCmpReport::SetColumns(int columns)
{
	m_nColumns = columns;
}

/**
 * @brief Generate report and save it to file.
 * @param [out] errStr Empty if succeeded, otherwise contains error message.
 * @return TRUE if report was created, FALSE if user canceled report.
 */
BOOL DirCmpReport::GenerateReport(String &errStr)
{
	assert(m_pList != NULL);
	assert(m_pFile == NULL);
	BOOL bRet = FALSE;

	DirCmpReportDlg dlg;
	if (LanguageSelect.DoModal(dlg) == IDOK) try
	{
		WaitStatusCursor waitstatus(IDS_STATUS_CREATEREPORT);
		if (dlg.m_bCopyToClipboard)
		{
			if (!m_pList->OpenClipboard())
				return FALSE;
			if (!EmptyClipboard())
				return FALSE;
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
			String path;
			SplitFilename(dlg.m_sReportFile.c_str(), &path, NULL, NULL);
			if (!paths_CreateIfNeeded(path.c_str()))
			{
				errStr = LanguageSelect.LoadString(IDS_FOLDER_NOTEXIST);
				return FALSE;
			}
			SHCreateStreamOnFile(dlg.m_sReportFile.c_str(),
				STGM_WRITE | STGM_CREATE | STGM_SHARE_DENY_WRITE, &m_pFile);
			GenerateReport(dlg.m_nReportType);
			m_pFile->Release();
			m_pFile = NULL;
		}
		bRet = TRUE;
	}
	catch (OException *e)
	{
		e->ReportError(m_pList->m_hWnd, MB_ICONSTOP);
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
	case REPORT_TYPE_TABLIST:
		m_sSeparator = _T("\t");
		GenerateHeader();
		GenerateContent();
		break;
	}
}

/**
 * @brief Write text to report file.
 * @param [in] pszText Text to write to report file.
 */
void DirCmpReport::WriteString(LPCTSTR pszText)
{
	const OString strOctets = HString::Uni(pszText)->Oct(CP_THREAD_ACP);
	LPCSTR pchOctets = strOctets.A;
	size_t cchAhead = strOctets.ByteLen();
	while (LPCSTR pchAhead = (LPCSTR)memchr(pchOctets, '\n', cchAhead))
	{
		int cchLine = pchAhead - pchOctets;
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
 * @brief Generate header-data for report.
 */
void DirCmpReport::GenerateHeader()
{
	WriteString(m_sTitle.c_str());
	WriteString(_T("\n"));
	WriteString(GetCurrentTimeString().c_str());
	WriteString(_T("\n"));
	for (int currCol = 0; currCol < m_nColumns; currCol++)
	{
		TCHAR columnName[160]; // Assuming max col header will never be > 160
		LVCOLUMN lvc;
		lvc.mask = LVCF_TEXT;
		lvc.pszText = &columnName[0];
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
			if (value.find(m_sSeparator) != String::npos) {
				WriteString(_T("\""));
				WriteString(value.c_str());
				WriteString(_T("\""));
			}
			else
				WriteString(value.c_str());

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
		_T("<html>\n<head>\n\t<title>"));
	WriteString(m_sTitle.c_str());
	WriteString(_T("</title>\n"));
	WriteString(_T("\t<style type=\"text/css\">\n\t<!--\n"));
	WriteString(_T("\t\tbody {\n"));
	WriteString(_T("\t\t\tfont-family: sans-serif;\n"));
	WriteString(_T("\t\t\tfont-size: smaller;\n"));
	WriteString(_T("\t\t}\n"));
	WriteString(_T("\t\ttable {\n"));
	WriteString(_T("\t\t\tborder-collapse: collapse;\n"));
	WriteString(_T("\t\t\tborder: 1px solid gray;\n"));
	WriteString(_T("\t\t}\n"));
	WriteString(_T("\t\tth,td {\n"));
	WriteString(_T("\t\t\tpadding: 3px;\n"));
	WriteString(_T("\t\t\ttext-align: left;\n"));
	WriteString(_T("\t\t\tvertical-align: top;\n"));
	WriteString(_T("\t\t\tborder: 1px solid gray;\n"));
	WriteString(_T("\t\t}\n"));
	WriteString(_T("\t\tth {\n"));
	WriteString(_T("\t\t\tcolor: black;\n"));
	WriteString(_T("\t\t\tbackground: silver;\n"));
	WriteString(_T("\t\t}\n"));
	WriteString(_T("\t-->\n\t</style>\n"));
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
	WriteString(_T("</p>\n"));
	WriteString(_T("<table border=\"1\">\n<tr>\n"));

	for (int currCol = 0; currCol < m_nColumns; currCol++)
	{
		TCHAR columnName[160]; // Assuming max col header will never be > 160
		LVCOLUMN lvc;
		lvc.mask = LVCF_TEXT;
		lvc.pszText = &columnName[0];
		lvc.cchTextMax = _countof(columnName);
		if (m_pList->GetColumn(currCol, &lvc))
		{
			WriteString(_T("<th>"));
			WriteString(lvc.pszText);
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
	WriteString(_T("")); // @todo xml declaration
	WriteString(_T("<WinMergeDiffReport version=\"1\">\n"));
	WriteString(Fmt(_T("<left>%s</left>\n"), m_rootPaths[0].c_str()).c_str());
	WriteString(Fmt(_T("<right>%s</right>\n"), m_rootPaths[1].c_str()).c_str());
	WriteString(Fmt(_T("<time>%s</time>\n"), GetCurrentTimeString().c_str()).c_str());

	// Add column headers
	static const TCHAR rowEl[] = _T("column_name");
	WriteString(BeginEl(rowEl).c_str());
	for (int currCol = 0; currCol < m_nColumns; currCol++)
	{
		TCHAR columnName[160]; // Assuming max col header will never be > 160
		LVCOLUMN lvc;
		lvc.mask = LVCF_TEXT;
		lvc.pszText = &columnName[0];
		lvc.cchTextMax = _countof(columnName);
		
		LPCTSTR colEl = m_colRegKeys[currCol].c_str();
		if (m_pList->GetColumn(currCol, &lvc))
		{
			WriteString(BeginEl(colEl).c_str());
			WriteString(lvc.pszText);
			WriteString(EndEl(colEl).c_str());
		}
	}
	WriteString(EndEl(rowEl).c_str());
	WriteString(_T("\n"));
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
		LPCTSTR const rowEl = xml ? _T("filediff") : _T("tr");
		WriteString(BeginEl(rowEl).c_str());
		for (int currCol = 0; currCol < m_nColumns; currCol++)
		{
			LPCTSTR const colEl = xml ? m_colRegKeys[currCol].c_str() : _T("td");
			WriteString(BeginEl(colEl).c_str());
			WriteString(m_pList->GetItemText(currRow, currCol).c_str());
			WriteString(EndEl(colEl).c_str());
		}
		WriteString(EndEl(rowEl).c_str());
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
