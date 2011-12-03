class CWindowPlacement : public WINDOWPLACEMENT
{
public:
	CWindowPlacement()
	{
		length = sizeof(WINDOWPLACEMENT);
		flags = 0;
	}
	bool RegQuery(HKEY, LPCTSTR);
	void RegWrite(HKEY, LPCTSTR);
};
