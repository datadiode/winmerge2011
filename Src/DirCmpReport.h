/** 
 * @file  DirCmpReport.h
 *
 * @brief Declaration file for DirCmpReport.
 *
 */
#pragma once

#include "DirReportTypes.h"

/**
 * @brief This class creates directory compare reports.
 *
 * This class creates a directory compare report. To make things easier
 * we read data from view's listview. Reading from listview avoids
 * re-formatting data and complex GUI for selecting what info to show in
 * reports. Downside is we only have data that is visible in GUI.
 *
 * @todo We should read DIFFITEMs from CDirDoc and format data to better
 * fit for reporting. Duplicating formatting and sorting code should be
 * avoided.
 */
class DirCmpReport
{
public:

	DirCmpReport(CDirView *);
	bool GenerateReport(String &errStr);

protected:
	void GenerateReport(REPORT_TYPE nReportType);
	void WriteString(HString *, UINT = CP_THREAD_ACP);
	void WriteString(LPCTSTR);
	template<class any>
	void WriteString(any fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		String s;
		s.append_sprintf_va_list(fmt, args);
		WriteString(s.c_str());
		va_end(args);
	}
	void WriteStringEntityAware(LPCTSTR);
	void GenerateHeader();
	void GenerateContent();
	void GenerateHTMLHeader();
	void GenerateHTMLHeaderBodyPortion();
	void GenerateXmlHeader();
	void GenerateXmlHtmlContent(bool xml);
	void GenerateHTMLFooter();
	void GenerateXmlFooter();

private:
	CDirView *const m_pList; /**< Pointer to UI-list */
	std::vector<String> m_rootPaths; /**< Root paths, printed to report */
	String m_sTitle; /**< Report title, built from root paths */
	int m_nColumns; /**< Columns in UI */
	String m_sSeparator; /**< Column separator for report */
	IStream *m_pFile; /**< File to write report to */
};
