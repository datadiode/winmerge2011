class CSplitState
{
public:
	//Construction
	CSplitState(const LONG *SplitScript = 0): SplitScript(SplitScript) { }
	//Data
	const LONG *SplitScript;
	//Methods
	BOOL Split(HWND);
};
