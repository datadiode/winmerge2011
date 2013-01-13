class CExcelExport
{
public:
	CExcelExport();
	~CExcelExport();
	operator HRESULT() { return hr; }
	bool Open(LPCWSTR);
	void Close(int nShow = SW_HIDE);
	void ApplyProfile(LPCTSTR app, LPCTSTR ini, bool fWriteDefaults = false);
	void WriteWorkbook(HListView *);
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
