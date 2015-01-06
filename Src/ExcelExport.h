class CExcelExport
{
public:
	struct BorderStyle
	{
		enum WORD_BorderStyle
		{
			NONE				= 0x0000,
			THIN				= 0x1111,
			MEDIUM				= 0x2222,
			DASHED				= 0x3333,
			DOTTED				= 0x4444,
			THICK				= 0x5555,
			DOUBLE				= 0x6666,
			HAIR				= 0x7777,
			MEDIUMDASHED		= 0x8888,
			DASHDOT				= 0x9999,
			MEDIUMDASHDOT		= 0xAAAA,
			DASHDOTDOT			= 0xBBBB,
			MEDIUMDASHDOTDOT	= 0xCCCC,
			SLANTDASHDOT		= 0xDDDD
		};
	};
	CExcelExport();
	~CExcelExport();
	operator HRESULT() { return hr; }
	bool Open(LPCWSTR);
	void Close(LPCTSTR lpVerb = NULL);
	void ApplyProfile(LPCTSTR app, LPCTSTR ini, bool fWriteDefaults = false);
	void WriteWorkbook(HListView *, int flags);
	BOOL fPrintGrid : 1;
	BorderStyle::WORD_BorderStyle wBorderStyle : 16;
	COLORREF crBorderColor;
	String sHeader;
	String sFooter;
	String sViewer;
	int nShowViewer;
	std::string sSheetName;
private:
	struct BiffRecord;
	HRESULT hr;
	IStorage *pstg;
	IStream *pstm;
};
