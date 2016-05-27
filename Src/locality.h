/** 
 * @file  locality.h
 *
 * @brief Declaration of helper functions involving locale
 */
#pragma once

namespace locality
{
	template<size_t n>
	class string
	{
	public:
		LPTSTR c_str() { return out; }
		operator String() { return out; }
		String::size_type length()
		{
			return static_cast<String::size_type>(_tcslen(out));
		}
	protected:
		TCHAR out[n];
	};
	class NumToLocaleStr : public string<48>
	{
	public:
		NumToLocaleStr(INT n, INT r)
		{
			_ltot(n, out, r);
		}
		NumToLocaleStr(INT n)
		{
			TCHAR numbuff[24];
			getLocaleStr(_ltot(n, numbuff, 10));
		}
		NumToLocaleStr(INT64 n, INT r)
		{
			_i64tot(n, out, r);
		}
		NumToLocaleStr(INT64 n)
		{
			TCHAR numbuff[24];
			getLocaleStr(_i64tot(n, numbuff, 10));
		}
		NumToLocaleStr(UINT n, INT r)
		{
			_ultot(n, out, r);
		}
		NumToLocaleStr(UINT n)
		{
			TCHAR numbuff[24];
			getLocaleStr(_ultot(n, numbuff, 10));
		}
		NumToLocaleStr(UINT64 n, INT r)
		{
			_ui64tot(n, out, r);
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
		void Format(const SYSTEMTIME &);
		void FormatAsUTime(const FILETIME &);
		void FormatAsLTime(const FILETIME &);
	public:
		enum UTime { UTime };
		enum LTime { LTime };
		TimeString(const SYSTEMTIME &st) { Format(st); } 
		TimeString(const FILETIME &ft, enum UTime) { FormatAsUTime(ft); } 
		TimeString(const FILETIME &ft, enum LTime) { FormatAsLTime(ft); } 
	};
};

typedef locality::NumToLocaleStr NumToStr;
