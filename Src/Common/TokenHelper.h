template<DWORD count>
class CAdjustProcessToken
{
public:
	CAdjustProcessToken()
		: hToken(NULL), prev_size(0)
	{
		const DWORD DesiredAccess = TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES;
		OpenProcessToken(GetCurrentProcess(), DesiredAccess, &hToken);
	};
	void Release()
	{
		if (prev_size != 0)
		{
			AdjustTokenPrivileges(hToken, FALSE, &tp_prev, prev_size, NULL, NULL);
		}
	};
	~CAdjustProcessToken()
	{
		if (hToken)
		{
			Release();
			CloseHandle(hToken);
		}
	};
	void Enable(LPCTSTR lpName)
	{
		if (LookupPrivilegeValue(NULL, lpName,
			&tp.Privileges[tp.PrivilegeCount].Luid))
		{
			tp.Privileges[tp.PrivilegeCount].Attributes = SE_PRIVILEGE_ENABLED;
			tp.PrivilegeCount++;
		}
	}
	BOOL Acquire()
	{
		BOOL ret = FALSE;
		if (tp.PrivilegeCount == count)
		{
			prev_size = sizeof tp_prev;
			ret = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof tp, &tp_prev, &prev_size);
		}
		return ret;
	}

protected:
	HANDLE hToken;
	DWORD prev_size;
	class TokenPrivileges : public TOKEN_PRIVILEGES
	{
	public:
		TokenPrivileges()
		{
			PrivilegeCount = 0;
		}
	private:
		LUID_AND_ATTRIBUTES ExtraPrivileges[count]; // waste one item for simplicity
	} tp, tp_prev;
};
