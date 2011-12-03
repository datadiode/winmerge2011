/** 
 * @file  locality.h
 *
 * @brief Declaration of helper functions involving locale
 */
// RCS ID line follows -- this is updated by CVS
// $Id$

#ifndef locality_h_included
#define locality_h_included

namespace locality
{
	template<size_t n>
	class string
	{
	public:
		LPTSTR c_str() { return out; }
		operator String() { return out; }
	protected:
		TCHAR out[n];
	};
	class NumToLocaleStr : public string<48>
	{
	public:
		NumToLocaleStr(INT n)
		{
			TCHAR numbuff[24];
			getLocaleStr(_ltot(n, numbuff, 10));
		}
		NumToLocaleStr(INT64 n)
		{
			TCHAR numbuff[24];
			getLocaleStr(_i64tot(n, numbuff, 10));
		}
		NumToLocaleStr(UINT n)
		{
			TCHAR numbuff[24];
			getLocaleStr(_ultot(n, numbuff, 10));
		}
		NumToLocaleStr(UINT64 n)
		{
			TCHAR numbuff[24];
			getLocaleStr(_ui64tot(n, numbuff, 10));
		}
	private:
		void getLocaleStr(LPCTSTR);
	};
	class TimeString : public string<128>
	{
	public:
		TimeString(const FILETIME &);
	};
};

typedef locality::NumToLocaleStr NumToStr;

#endif // locality_h_included
