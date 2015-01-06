#pragma once

class FileTime : public FILETIME
{
private:
	struct null;
public:
	static const UINT TicksPerSecond = 10000000;
	FileTime(null * = 0)
	{
		dwLowDateTime = 0;
		dwHighDateTime = 0;
	}
	FileTime(const FILETIME &other)
		: FILETIME(other)
	{
	}
	const FileTime &operator = (null *)
	{
		dwLowDateTime = 0;
		dwHighDateTime = 0;
		return *this;
	}
	const FileTime &operator = (const FILETIME &other)
	{
		dwLowDateTime = other.dwLowDateTime;
		dwHighDateTime = other.dwHighDateTime;
		return *this;
	}
	template<class SCALAR>
	const SCALAR &castTo() const
	{
		C_ASSERT(sizeof(SCALAR) == sizeof(*this));
		C_ASSERT((SCALAR)0.5 == 0);
		return reinterpret_cast<const SCALAR &>(*this);
	}
	bool operator != (null *) const
	{
		return (dwLowDateTime | dwHighDateTime) != 0;
	}
	bool operator == (null *) const
	{
		return (dwLowDateTime | dwHighDateTime) == 0;
	}
	bool operator == (const FileTime &other) const
	{
		return castTo<UINT64>() == other.castTo<UINT64>();
	}
	bool operator != (const FileTime &other) const
	{
		return castTo<UINT64>() != other.castTo<UINT64>();
	}
	bool operator < (const FileTime &other) const
	{
		return castTo<UINT64>() < other.castTo<UINT64>();
	}
	bool operator <= (const FileTime &other) const
	{
		return castTo<UINT64>() <= other.castTo<UINT64>();
	}
	bool operator > (const FileTime &other) const
	{
		return castTo<UINT64>() > other.castTo<UINT64>();
	}
	bool operator >= (const FileTime &other) const
	{
		return castTo<UINT64>() >= other.castTo<UINT64>();
	}
	UINT64 secondsSinceEpoch() const
	{
		return castTo<UINT64>() / TicksPerSecond;
	}
	INT64 secondsSince(const FileTime &other) const
	{
		return secondsSinceEpoch() - other.secondsSinceEpoch();
	}
	UINT64 ticksSinceBC0001() const
	{
		return castTo<UINT64>() + 505227456000000000UI64;
	}
	BOOL Parse(LPCSTR p)
	{
		SYSTEMTIME st;
		int fields = sscanf(p, "%04hu-%02hu-%02hu %02hu:%02hu:%02hu",
			&st.wYear, &st.wMonth, &st.wDay, &st.wHour, &st.wMinute, &st.wSecond);
		if (fields != 6)
			return FALSE;
		st.wDayOfWeek = 0;
		st.wMilliseconds = 0;
		return SystemTimeToFileTime(&st, this);
	}
	BOOL Parse(LPCOLESTR strIn, DWORD dwFlags)
	{
		DATE date;
		if (FAILED(VarDateFromStr(strIn, LOCALE_USER_DEFAULT, dwFlags, &date)))
			return FALSE;
		SYSTEMTIME st;
		if (!VariantTimeToSystemTime(date, &st))
			return FALSE;
		return SystemTimeToFileTime(&st, this);
	}
};

class LogFont : public LOGFONT
{
private:
	struct null;
	template<class T>
	static LPCTSTR Parse(LPCTSTR p, T &value)
	{
		if (p)
		{
			LPTSTR q = NULL;
			value = static_cast<T>(_tcstol(p, &q, 10));
			p = *q == _T(',') ? q + 1 : NULL;
		}
		return p;
	}
public:
	LogFont(null * = 0)
	{
		memset(this, 0, sizeof *this);
	}
	LogFont(const LOGFONT &other)
		: LOGFONT(other)
	{
	}
	const LogFont &operator = (null *)
	{
		memset(this, 0, sizeof *this);
		return *this;
	}
	const LogFont &operator = (const LOGFONT &other)
	{
		memcpy(this, &other, sizeof *this);
		return *this;
	}
	bool operator == (const LOGFONT &other) const
	{
		return memcmp(this, &other, sizeof *this) == 0;
	}
	void Parse(LPCTSTR buf)
	{
		buf = Parse(buf, lfHeight);
		buf = Parse(buf, lfWidth);
		buf = Parse(buf, lfEscapement);
		buf = Parse(buf, lfOrientation);
		buf = Parse(buf, lfWeight);
		buf = Parse(buf, lfItalic);
		buf = Parse(buf, lfUnderline);
		buf = Parse(buf, lfStrikeOut);
		buf = Parse(buf, lfCharSet);
		buf = Parse(buf, lfOutPrecision);
		buf = Parse(buf, lfClipPrecision);
		buf = Parse(buf, lfQuality);
		buf = Parse(buf, lfPitchAndFamily);
		// lstrcpyn() protects itself against NULL arguments
		lstrcpyn(lfFaceName, buf, LF_FACESIZE);
	}
	LPCTSTR Print(LPTSTR buf = locality::string<128>().c_str())
	{
		wsprintf(buf, _T("%ld,%ld,%ld,%ld,%ld,%u,%u,%u,%u,%u,%u,%u,%u,%s"),
			lfHeight, lfWidth, lfEscapement, lfOrientation, lfWeight,
			lfItalic, lfUnderline, lfStrikeOut, lfCharSet, lfOutPrecision,
			lfClipPrecision, lfQuality, lfPitchAndFamily, lfFaceName);
		return buf;
	}
};
