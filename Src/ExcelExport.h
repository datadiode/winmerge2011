class CExcelExport
{
public:
	CExcelExport();
	~CExcelExport();
	operator HRESULT() { return hr; }
	bool Open(LPCWSTR);
	void Close(LPCTSTR lpVerb = NULL);
	void ApplyProfile(LPCTSTR app, LPCTSTR ini, bool fWriteDefaults = false);
	void WriteWorkbook(HListView *, int flags);
	BOOL fPrintGrid : 1;
	String sHeader;
	String sFooter;
	String sViewer;
	int nShowViewer;
	stl::string sSheetName;
private:
	struct BiffRecord;
	HRESULT hr;
	IStorage *pstg;
	IStream *pstm;
};
